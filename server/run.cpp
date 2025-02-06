// This file contains portions of code copied from the open-source project
// gemma.cpp.
// Original file path: gemma.cpp/gemma/run.cc
// License of the original project: Apache 2.0
// The copyright statement is as follows:
//
// Copyright 2024 Google LLC
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "gemmathread.h"

#include <Magick++.h>

#include <fstream>
#include <iostream>

#include "logger.h"
#include "nlohmann/json.hpp"
#include "setting.h"
#include "util/args.h"  // Path

using json = nlohmann::json;

static constexpr bool kVerboseLogTokens = false;

static void InitGenerator(const InferenceArgs& inference, std::mt19937& gen) {
  if (inference.deterministic) {
    // Nothing up my sleeve number, at least some upper bits set.
    gen.seed(0x12345678);
  } else {
    // Depending on the library implementation, this may still be deterministic.
    std::random_device rd;
    gen.seed(rd());
  }
}

std::string GemmaThread::GetPrompt(std::istream& input, int verbosity,
                                   std::string_view eot_line) {
  PROFILER_ZONE("Gen.input");
  std::string prompt_text = "";
  if (!m_prompts.empty()) {
    prompt_text = m_prompts.front();
    m_prompts.pop();
  }
  logger(logger::D).os << "prompt_text: " << prompt_text << std::endl;
  return prompt_text;
}

void GemmaThread::ReplGemma(Gemma& model, KVCache& kv_cache, const AppArgs& app,
                            const InferenceArgs& args,
                            const AcceptFunc& accept_token,
                            std::string& eot_line) {
  PROFILER_ZONE("Gen.misc");
  size_t abs_pos = 0;                     // across turns
  size_t tokens_generated_this_turn = 0;  // differentiates prefill from reply
  size_t prompt_size = 0;

  logger(logger::D).os << "GemmaThread::ReplGemma() : start" << std::endl;

  std::mt19937 gen;
  InitGenerator(args, gen);

  const bool have_image = getSession()->m_binary.size() > 0;
  Image image;
  ImageTokens image_tokens;
  if (have_image) {
    image_tokens = ImageTokens(Extents2D(model.GetModelConfig().vit_seq_len,
                                         model.GetModelConfig().model_dim));
    HWY_ASSERT(model.Info().training == ModelTraining::PALIGEMMA);
    HWY_ASSERT(PictureToPPM(image));
    image.Resize();
    RuntimeConfig runtime_config = {
        .gen = &gen, .verbosity = app.verbosity, .use_spinning = app.spin};
    double image_tokens_start = hwy::platform::Now();
    model.GenerateImageTokens(runtime_config, image, image_tokens);
    if (app.verbosity >= 1) {
      double image_tokens_duration = hwy::platform::Now() - image_tokens_start;
      logger(logger::D).os << "Image token generation took: "
                           << static_cast<int>(image_tokens_duration * 1000)
                           << " ms" << std::endl;
    }
    // m_binary.clear();
  }

  // callback function invoked for each generated token.
  auto stream_token = [&](int token, float) {
    ++abs_pos;
    ++tokens_generated_this_turn;
    // <= since position is incremented before
    if (tokens_generated_this_turn <= prompt_size) {
      //   std::cerr << "." << std::flush;
      m_callback(tokens_generated_this_turn, prompt_size, m_session, "", false,
                 m_user);
    } else if (token == EOS_ID) {
      if (!args.multiturn) {
        abs_pos = 0;
        InitGenerator(args, gen);
      }
      if (app.verbosity >= 2) {
        // std::cout << "\n[ End ]\n";
      }
    } else {
      std::string token_text;
      HWY_ASSERT(
          model.Tokenizer().Decode(std::vector<int>{token}, &token_text));
      // +1 since position is incremented above
      if (tokens_generated_this_turn == prompt_size + 1) {
        // first token of response
        token_text.erase(0, token_text.find_first_not_of(" \t\n"));
        if (app.verbosity >= 1) {
          //   std::cout << "\n\n";
        }
      }
      //   std::cout << token_text << std::flush;
      m_callback(0, 0, m_session, token_text, false, m_user);
    }
    return true;
  };

  while (!m_prompts.empty()) {  // Loop until user quits.
    tokens_generated_this_turn = 0;
    std::string prompt_string = GetPrompt(std::cin, app.verbosity, eot_line);
    if (prompt_string == "") break;
    // If !eot_line.empty(), we append \n, so only look at the first 2 chars.
    if (prompt_string.size() >= 2 && prompt_string[0] == '%') {
      if (prompt_string[1] == 'q' || prompt_string[1] == 'Q') return;
      if (prompt_string[1] == 'c' || prompt_string[1] == 'C') {
        abs_pos = 0;
        continue;
      }
    }

    if (have_image && abs_pos != 0) {
      // This occurs when we have hit max_generated.
      abs_pos = 0;
    }

    std::vector<int> prompt = WrapAndTokenize(model.Tokenizer(), model.Info(),
                                              abs_pos, prompt_string);
    prompt_size = prompt.size();
    // std::cerr << "\n"
    //           << "[ Reading prompt ] " << std::flush;
    if constexpr (kVerboseLogTokens) {
      for (int i = 0; i < prompt_size; ++i) {
        fprintf(stderr, "DDD TOKEN %3d: %6d\n", i, prompt[i]);
      }
    }

    TimingInfo timing_info = {.verbosity = app.verbosity};
    RuntimeConfig runtime_config = {
        .max_generated_tokens =m_config.max_generated_tokens,
        .temperature = m_config.temperature,
        .gen = &gen,
        .verbosity = app.verbosity,
        .stream_token = stream_token,
        .accept_token = accept_token,
        .use_spinning = app.spin
        };
    args.CopyTo(runtime_config);
    size_t prefix_end = 0;
    if (have_image) {
      runtime_config.image_tokens = &image_tokens;
      prompt.insert(prompt.begin(), image_tokens.BatchSize(), 0);
      prompt_size = prompt.size();
      // The end of the prefix for prefix-LM style attention in Paligemma.
      // See Figure 2 of https://arxiv.org/abs/2407.07726.
      prefix_end = prompt_size;
      // We need to look at all the tokens for the prefix.
      runtime_config.prefill_tbatch_size = prompt_size;
    }
    model.Generate(runtime_config, prompt, abs_pos, prefix_end, kv_cache,
                   timing_info);
    // std::cout << "\n\n";
    logger(logger::D).os << "GemmaThread::ReplGemma() : Generated" << std::endl;
  }
  if (m_callback != nullptr) {
    m_callback(0, 0, m_session, "", true, m_user);
  }
  logger(logger::D).os << "GemmaThread::ReplGemma() : finished" << std::endl;
}
