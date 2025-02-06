#include "gemmathread.h"

#include <Magick++.h>
#include <sys/prctl.h>

#include <fstream>
#include <iostream>

#include "logger.h"
#include "nlohmann/json.hpp"
#include "setting.h"
#include "util/args.h"  // Path
using json = nlohmann::json;

GemmaThread::GemmaThread(const std::string& file)
    : m_config_file(file),
      std::thread(&GemmaThread::threadFunction, this),
      m_break(false),
      m_model(NULL),
      m_pool(NULL),
      m_ready(false) {
  logger(logger::TI).os << __FUNCTION__;
  logger(logger::TO).os << __FUNCTION__;
}

GemmaThread::~GemmaThread() {
  logger(logger::TI).os << __FUNCTION__;
  m_break = true;
  webSocketStop();
  logger(logger::TO).os << __FUNCTION__;
}

void GemmaThread::stop() {
  if (m_model != NULL) {
    m_break = true;
    m_running = false;
    signal();
  }
}

void GemmaThread::signal() {
  std::unique_lock<std::mutex> lock(m_mtx);
  m_ready = true;
  m_break = true;
  m_cv.notify_one();

  logger(logger::D).os << "signal() triggered." << std::endl;
}

GemmaThread::Session* GemmaThread::getSession(std::string session) {
  std::shared_ptr<Session> p = NULL;
  std::string s = session.empty() ? m_session : session;
  if (m_model != NULL) {
    if (m_sessions.find(s) != m_sessions.end()) {
      p = m_sessions[s];
    } else {
      p = std::make_shared<Session>(m_model.get(), m_prefillTBatchSize);
      m_sessions[s] = p;
    }
  }
  logger(logger::D).os << "getSession() : " << p.get() << std::endl;
  return p.get();
}

void GemmaThread::threadFunction() {
  prctl(PR_SET_NAME, "GemmaThread", 0, 0, 0);
  logger(logger::TI).os << __FUNCTION__;
  if (config()) {
    ParseModelTypeAndTraining(m_loader->model_type_str.c_str(), m_modelType,
                              m_modelTraining);
    gcpp::ModelInfo info = {.model = m_modelType, .training = m_modelTraining};
    ParseType("sfp", info.weight);
    m_pool = std::make_shared<gcpp::NestedPools>(
        0, gcpp::Tristate::kDefault, gcpp::BoundedSlice(0, 0),
        gcpp::BoundedSlice(0, 0), gcpp::BoundedSlice(0, 0));
    m_model = std::make_shared<gcpp::Gemma>(m_loader->tokenizer,
                                            m_loader->weights, info, *m_pool);

    if (webSocketStart(m_port)) {
      m_running = true;
      while (m_running) {
        std::unique_lock<std::mutex> lock(m_mtx);
        logger(logger::D).os << "wait signal" << std::endl;
        m_ready = false;
        m_cv.wait(lock, [this] { return m_ready; });
        logger(logger::D).os << "Caught signal" << std::endl;

        if (m_running) {
          ReplGemma(*m_model, getSession()->m_kv_cache, *m_app, *m_inference,
                    AcceptFunc(), m_app->eot_line);
        }
      }
    }
  }
  logger(logger::TO).os << __FUNCTION__;
}

void GemmaThread::setPrompt(std::string session, std::string prompt,
                            callback cb, void* user) {
  m_session = session;
  m_prompts.push(prompt);
  m_callback = cb;
  m_user = user;

  signal();
}

bool GemmaThread::checkFile(const std::string& path) {
  // check if the given file exists.
  bool ret = false;
  std::ifstream fin(path);
  if (fin) {
    ret = true;
  }
  return ret;
}

bool GemmaThread::config() {
  bool ret = false;

  do {
    if (!checkFile(m_config_file)) {
      logger(logger::E).os << "Failed to load Setting file(" + m_config_file +
                                  ").\n";
      break;
    }

    Setting settings(m_config_file);

    std::string fileWeight = settings.getStringValue("Weight");
    if (checkFile(fileWeight)) {
      logger(logger::I).os << fileWeight + " loaded.\n";
    } else {
      logger(logger::I).os << "Weight file(" + fileWeight + ") missing.\n";
      fileWeight = "";
      break;
    }

    std::string fileTokenizer = settings.getStringValue("Tokenizer");
    if (checkFile(fileTokenizer)) {
      logger(logger::I).os << fileTokenizer + " loaded.\n";
    } else {
      logger(logger::I).os << "Tokenizer file(" + fileTokenizer +
                                  ") missing.\n";
      fileTokenizer = "";
      break;
    }

    std::string modelTypeName = settings.getStringValue("ModelType", "2b-it");
    if (modelTypeName.length() == 0) {
      logger(logger::I).os << " Model type not set.\n";
      break;
    }

    m_config.max_generated_tokens =
        settings.getIntValue("MaxGeneratedTokens", 2048);
    m_config.temperature = settings.getFloatValue("Temperature", 1.0);
    m_config.verbosity = settings.getIntValue("Verbosity", 2);

    m_port = settings.getIntValue("WebSocketPort", 9999);

    m_prefillTBatchSize = settings.getIntValue("PrefillTbatchSize", 32);

    m_loader = std::make_shared<gcpp::LoaderArgs>(fileTokenizer, fileWeight,
                                                  modelTypeName);
    m_loader->Validate();
    m_inference = std::make_shared<gcpp::InferenceArgs>();
    m_inference->Validate();
    m_app = std::make_shared<gcpp::AppArgs>();
    m_app->verbosity = m_config.verbosity;

    ret = true;
  } while (0);
  return ret;
}

void GemmaThread::cleanPrompt() {
  while (!m_prompts.empty()) {
    m_prompts.pop();
  }
}

void GemmaThread::OnWebSocketOpen(
    std::shared_ptr<ix::ConnectionState> connectionState,
    ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
  connectionState->computeId();

  logger(logger::D).os << "remote ip: "
                       << connectionState->getRemoteIp().c_str() << std::endl;
  logger(logger::D).os << "id: " << connectionState->getId().c_str()
                       << std::endl;
  logger(logger::D).os << "Uri: " << msg->openInfo.uri.c_str() << std::endl;
  logger(logger::D).os << "Headers:" << std::endl;
  for (auto it : msg->openInfo.headers) {
    logger(logger::D).os << it.first.c_str() << ": " << it.second.c_str()
                         << std::endl;
  }
}

void GemmaThread::sendBack(int progress, int max, std::string session,
                           std::string text, bool eos, void* user) {
  json data;
  data["id"] = session;
  data["object"] = "chat.completion";
  Type type = m_loader->Info().weight;
  logger(logger::D).os << "type: " << (int)type << std::endl;
  data["model"] = std::string("gemma-") + StringFromType(type) + "-" +
                  TypeName<EmbedderInputT>();
  data["choices"]["messages"]["role"] = "assistant";
  data["choices"]["messages"]["content"] = text;
  data["usage"]["prompt_tokens"] = max - progress;
  data["usage"]["completion_tokens"] = progress;
  data["usage"]["total_tokens"] = max;

  logger(logger::D).os << "server client: " << m_server->getClients().size()
                       << std::endl;
  for (auto& client : m_server->getClients()) {
    logger(logger::D).os << "client:" << client.get() << std::endl;
    logger(logger::D).os << "user:" << user << std::endl;
    if (client.get() == user) {
      logger(logger::V).os << data.dump() << std::endl;
      client->send(data.dump(), false);
    }
  }
}

bool GemmaThread::PictureToPPM(Image& image) {
  bool ret = false;
  const char* imageData = NULL;
  size_t imageSize = 0;
  Magick::Blob outputBlob;

  Session* session = getSession();
  try {
    Magick::Blob blob(session->m_binary.data(), session->m_binary.size());
    Magick::Image image2;
    image2.read(blob);
    image2.resize(Magick::Geometry(224, 224));
    image2.write(&outputBlob, "PPM");

    imageData = static_cast<const char*>(outputBlob.data());
    imageSize = outputBlob.length();
    ret = image.ReadPPM(hwy::Span<const char>(imageData, imageSize));
  } catch (Magick::Exception& error_) {
    logger(logger::E).os << "Failed to convert image : " << error_.what()
                         << std::endl;
  }
  logger(logger::V).os << "imageSize: " << imageSize << std::endl;
  return ret;
}

void GemmaThread::OnWebSocketMessage(
    std::shared_ptr<ix::ConnectionState> connectionState,
    ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
  // logger(logger::V).os << msg->str.c_str() << std::endl;

  json data = json::parse(msg->str);
  if (data.contains("stop")) {
    cleanPrompt();
    m_break = true;
  } else {
    std::string session = "";
    std::string content = "";

    try {
      session = data["id"];
    } catch (json::exception& e) {
      session = connectionState->getRemoteIp() + "-" + connectionState->getId();
    }
    try {
      content = data["messages"]["content"];
    } catch (json::exception& e) {
      logger(logger::W).os << "Failed to parse " << msg->str << " : "
                           << e.what() << std::endl;
    }
    try {
      if (data.contains("uploads")) {
        std::string filename = data["uploads"]["filename"];
        logger(logger::V).os << "filename: " << filename << std::endl;
        getSession(session)->m_binary =
            data.at("uploads").at("binary").get<std::vector<char>>();
        logger(logger::V).os << "bin.size(): " << getSession()->m_binary.size()
                             << std::endl;
      }
    } catch (json::exception& e) {
      logger(logger::W).os << "Failed to parse  : " << e.what() << std::endl;
    }

    if (content == "") {
      sendBack(0, 0, session, "<|fim_middle|>", true, &webSocket);
      sendBack(0, 0, session, "Caught exception", true, &webSocket);
      sendBack(0, 0, session, "<|file_separator|>", true, &webSocket);
    } else {
      logger(logger::V).os << "prompt: " << content << std::endl;
      setPrompt(
          session, content,
          [&](int progress, int max, std::string session, std::string text,
              bool eos, void* user) {
            logger(logger::V).os << progress << "/" << max << " "
                                 << "session: " << session << " "
                                 << "token: " << text << " "
                                 << "eos: " << eos << " "
                                 << "user: " << user << std::endl;
            sendBack(progress, max, session, text, eos, user);
            if (eos) {
              logger(logger::D).os << "EOS" << std::endl;
            }
          },
          &webSocket);
    }
  }
}

bool GemmaThread::webSocketStart(int port) {
  Magick::InitializeMagick(nullptr);

  m_server = std::make_shared<ix::WebSocketServer>(port);
  m_server->setOnClientMessageCallback(
      [this](std::shared_ptr<ix::ConnectionState> connectionState,
             ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
          logger(logger::I).os << "New connection" << std::endl;
          OnWebSocketOpen(connectionState, webSocket, msg);
        } else if (msg->type == ix::WebSocketMessageType::Close) {
          logger(logger::I).os << "Closed connection" << std::endl;
        } else if (msg->type == ix::WebSocketMessageType::Message) {
          OnWebSocketMessage(connectionState, webSocket, msg);
        }
      });

  auto res = m_server->listen();
  if (!res.first) {
    logger(logger::E).os << "failed : " << res.second << std::endl;
    return false;
  }

  m_server->start();
  logger(logger::I).os << "Websocket Started on " << port << std::endl;
  return true;
}

void GemmaThread::webSocketStop() {
  logger(logger::TI).os << __FUNCTION__;
  if (m_server != NULL) {
    m_server->stop();
  }
  logger(logger::TO).os << __FUNCTION__;
}
