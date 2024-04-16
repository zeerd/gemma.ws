#include "gemmathread.h"

#include <iostream>
#include <fstream>

#include "util/args.h"  // Path


GemmaThread::GemmaThread()
        : m_break(false)
        , m_model_type("2b-it")
        , m_model(NULL)
        , m_pool(NULL)
        , m_running(false)
{
    m_num_threads = static_cast<size_t>(std::clamp(
            static_cast<int>(std::thread::hardware_concurrency()) - 2, 1, 18));
}

GemmaThread::~GemmaThread()
{
}

void GemmaThread::setPrompt(std::string session, std::string prompt)
{
    m_session = session;
    m_prompts.push(prompt);
}

void GemmaThread::appendPrompt(std::string session, std::string prompt)
{
    m_session = session;
    m_prompts.push(prompt);
}

bool GemmaThread::checkFile(const std::string &path)
{
    // using c++, check if the given file exists.
    bool ret = false;
    std::ifstream fin(path);
    if (fin) {
        ret = true;
    }
    return ret;
}

void GemmaThread::gemmaInit()
{
    if(checkFile(m_fileWeight) && checkFile(m_fileTokenizer)
    && (m_model_type != "") && (m_model == NULL)) {
        gcpp::Path tokenizer;
        gcpp::Path compressed_weights;

        tokenizer = m_fileTokenizer.c_str();
        compressed_weights = m_fileWeight.c_str();

        ParseModelTypeAndTraining(m_model_type, model_type, model_training);

        m_pool = std::make_shared<hwy::ThreadPool>(m_num_threads);
        m_model = std::make_shared<gcpp::Gemma>(tokenizer, compressed_weights,
                            model_type, *m_pool);

        std::string welcome = "**Start**\n";
        if(checkFile(m_fileWeight)) {
            welcome += "- " + m_fileWeight + " loaded.\n";
        }
        else {
            m_fileWeight = "";
            welcome += "- Weight file missing.\n";
        }
        if(checkFile(m_fileTokenizer)) {
            welcome += "- " + m_fileTokenizer + " loaded.\n";
        }
        else {
            m_fileTokenizer = "";
            welcome += "- Tokenizer file missing.\n";
        }
        if(m_model_type.length() == 0) {
            welcome += "- Model type not set.\n";
        }
        welcome += "\n";
        m_callback(0, 1, welcome, false);

        const char* instructions = "\n"
            "**Usage**\n\n"
            "- Enter an instruction and press enter.\n\n"
            "**Examples**\n\n"
            "- Write an email to grandma thanking her for the cookies.\n"
            "- What are some historical attractions to visit around "
            "Massachusetts?\n"
            "- Compute the nth fibonacci number in javascript.\n"
            "- Write a standup comedy bit about GPU programming.\n"
            "\n\n---\n";
        m_callback(0, 1, instructions, false);
    }
}

void GemmaThread::gemmaUninit()
{
    if(m_model != NULL) {
        if(isRunning()) {
            terminate();
        }
    }
}

void GemmaThread::cleanPrompt()
{
    while(!m_prompts.empty()) {
        m_prompts.pop();
    }
    m_sessions[m_session]->m_abs_pos = 0;
}

void GemmaThread::start()
{
    m_thread = std::make_shared<std::thread>(&GemmaThread::run, this);
    m_thread->detach();
}

void GemmaThread::run()
{
    std::cout << "GemmaThread::run()";

    m_break = false;

    gemmaInit();

    int prompt_size{};

    std::mt19937 gen;
    std::random_device rd;
    gen.seed(rd());

    std::shared_ptr<Session> session = NULL;
    if(m_model != NULL) {
        if(m_sessions.find(m_session) != m_sessions.end()) {
            session = m_sessions[m_session];
        }
        else {
            session = std::make_shared<Session>(model_type);
            m_sessions[m_session] = session;
        }
    }

    std::string prompt_text = "";
    while (!m_prompts.empty() && !m_break && m_model != NULL) {
        prompt_text = m_prompts.front();
        m_prompts.pop();

        int current_pos = 0; // token index within the current turn
        auto stream_token = [this, &gen, &prompt_size, &session, &current_pos,
                             tokenizer = m_model->Tokenizer()
                             ](int token, float) {
            ++(session->m_abs_pos);
            ++current_pos;

            if (current_pos < prompt_size) {
                m_callback(1 + current_pos, prompt_size, "", false);
            }
            else if (token == gcpp::EOS_ID) {
                // GenerateGemma() may be finished by many reasons.
                // EOS is only one of them.
            }
            else {
                std::string token_text;
                HWY_ASSERT(tokenizer->Decode(std::vector<int>{token}, &token_text));
                // +1 since position is incremented above
                if (current_pos == prompt_size + 1) {
                  // first token of response
                  token_text.erase(0, token_text.find_first_not_of(" \t\n"));
                }
                std::cout << token_text;
                fflush(stdout);
                m_callback(0, 0, token_text, false);
            }
            return !m_break;
        };

        std::vector<int> prompt;
        if (model_training == gcpp::ModelTraining::GEMMA_IT) {
            // For instruction-tuned models: add control tokens.
            prompt_text = "<start_of_turn>user\n" + prompt_text +
                          "<end_of_turn>\n<start_of_turn>model\n";
            if (session->m_abs_pos > 0) {
                // Prepend "<end_of_turn>" token if this is a multi-turn dialogue
                // continuation.
                prompt_text = "<end_of_turn>\n" + prompt_text;
            }
        }

        // qDebug() << tr(prompt_text.c_str());
        HWY_ASSERT(m_model->Tokenizer()->Encode(prompt_text, &prompt));

        // For both pre-trained and instruction-tuned models: prepend "<bos>" token
        // if needed.
        if (session->m_abs_pos == 0) {
            prompt.insert(prompt.begin(), 2);
        }

        prompt_size = prompt.size();

        std::string markdown_prompt = ("\n\n**Question:**\n\n```\n");
        markdown_prompt += prompt_text.c_str();
        markdown_prompt += "\n```\n\n**Answer:**\n\n";
        m_callback(1, prompt_size, markdown_prompt, false);

        /*bool succ = */gcpp::GenerateGemma(*m_model, m_config,
                prompt, session->m_abs_pos, session->m_kv_cache, *m_pool,
                stream_token, gen);
        // if(!succ) {
        //     break;
        // }
    }

    if(m_callback != nullptr) {
        m_callback(0, 1, "", true);
    }
    std::cout << "GemmaThread::run() : finished";
}
