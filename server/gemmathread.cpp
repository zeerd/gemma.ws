#include "gemmathread.h"
#include "setting.h"

#include <iostream>
#include <fstream>

#include "util/args.h"  // Path
#include "logger.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

GemmaThread::GemmaThread(const std::string &file)
        :  std::thread(&GemmaThread::threadFunction, this, file)
        , m_break(false)
        , m_model(NULL)
        , m_pool(NULL)
{
    logger(logger::TI).os << __FUNCTION__;
    m_num_threads = static_cast<size_t>(std::clamp(
            static_cast<int>(std::thread::hardware_concurrency()) - 2, 1, 18));
    logger(logger::TO).os << __FUNCTION__;
}

GemmaThread::~GemmaThread()
{
    logger(logger::TI).os << __FUNCTION__;
    webSocketStop();
    logger(logger::TO).os << __FUNCTION__;
}

void GemmaThread::stop()
{
    if(m_model != NULL) {
        m_break = true;
        m_running = false;
        signal();
    }
    join();
}

void GemmaThread::signal()
{
    std::unique_lock<std::mutex> lock(mtx);
    ready = true;
    m_break = true;
    cv.notify_one();

    logger(logger::D).os << "signal() triggered." << std::endl;
}

void GemmaThread::threadFunction(const std::string &path)
{
    logger(logger::TI).os << __FUNCTION__;
    if(init(path)) {
        m_running = true;
        while(m_running) {
            std::unique_lock<std::mutex> lock(mtx);
            logger(logger::D).os << "wait signal" << std::endl;
            ready = false;
            cv.wait(lock, [this]{ return ready; });
            logger(logger::D).os << "Caught signal" << std::endl;

            if(m_running) {
                run();
            }
        }
    }
    logger(logger::TO).os << __FUNCTION__;
}

void GemmaThread::setPrompt(std::string session, std::string prompt,
                            callback cb, void *user)
{
    m_session = session;
    m_prompts.push(prompt);
    m_callback = cb;
    m_user = user;

    signal();
}

bool GemmaThread::checkFile(const std::string &path)
{
    // check if the given file exists.
    bool ret = false;
    std::ifstream fin(path);
    if (fin) {
        ret = true;
    }
    return ret;
}

bool GemmaThread::init(const std::string &path)
{
    bool ret = true;
    std::string welcome = "\n";
    Setting settings(path);

    m_fileWeight = settings.getStringValue("Weight");
    if(checkFile(m_fileWeight)) {
        welcome += "- " + m_fileWeight + " loaded.\n";
    }
    else {
        welcome += "- Weight file("+m_fileWeight+") missing.\n";
        m_fileWeight = "";
        ret = false;
    }

    m_fileTokenizer = settings.getStringValue("Tokenizer");
    if(checkFile(m_fileTokenizer)) {
        welcome += "- " + m_fileTokenizer + " loaded.\n";
    }
    else {
        welcome += "- Tokenizer file("+m_fileTokenizer+") missing.\n";
        m_fileTokenizer = "";
        ret = false;
    }

    m_model_type = settings.getStringValue("ModelType", "2b-it");
    if(m_model_type.length() == 0) {
        welcome += "- Model type not set.\n";
        ret = false;
    }
    welcome += "\n";
    logger(logger::I).os << welcome;

    m_config.max_tokens = settings.getIntValue("MaxTokens", 3072);
    m_config.max_generated_tokens = settings.getIntValue("MaxGeneratedTokens", 2048);
    m_config.temperature = settings.getFloatValue("Temperature", 1.0);
    m_config.verbosity = settings.getIntValue("Verbosity", 2);

    if(ret && (m_model == NULL)) {
        gcpp::Path tokenizer(m_fileTokenizer.c_str());
        gcpp::Path compressed_weights(m_fileWeight.c_str());

        ParseModelTypeAndTraining(m_model_type, model_type, model_training);

        m_pool = std::make_shared<hwy::ThreadPool>(m_num_threads);
        m_model = std::make_shared<gcpp::Gemma>(tokenizer, compressed_weights,
                            model_type, *m_pool);

        ret = webSocketStart(settings.getIntValue("WebSocketPort", 9999));
    }
    return ret;
}

void GemmaThread::cleanPrompt()
{
    while(!m_prompts.empty()) {
        m_prompts.pop();
    }
    m_sessions[m_session]->m_abs_pos = 0;
}

void GemmaThread::run()
{
    m_break = false;
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
                m_callback(1 + current_pos, prompt_size, m_session, "", false, m_user);
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
                logger(logger::D).os << token_text << std::endl;
                fflush(stdout);
                m_callback(0, 0, m_session, token_text, false, m_user);
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
        m_callback(1, prompt_size, m_session, markdown_prompt, false, m_user);

        /*bool succ = */gcpp::GenerateGemma(*m_model, m_config,
                prompt, session->m_abs_pos, session->m_kv_cache, *m_pool,
                stream_token, gen);
        // if(!succ) {
        //     break;
        // }
    }

    if(m_callback != nullptr) {
        m_callback(0, 1, m_session, "", true, m_user);
    }
    logger(logger::D).os << "GemmaThread::run() : finished" << std::endl;
}

void GemmaThread::OnWebSocketOpen(
    std::shared_ptr<ix::ConnectionState> connectionState,
    ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
{
    connectionState->computeId();
    logger(logger::D).os << "remote ip: " << connectionState->getRemoteIp().c_str() << std::endl;
    logger(logger::D).os << "id: " << connectionState->getId().c_str() << std::endl;
    logger(logger::D).os << "Uri: " << msg->openInfo.uri.c_str() << std::endl;
    logger(logger::D).os << "Headers:" << std::endl;
    for (auto it : msg->openInfo.headers)
    {
        logger(logger::D).os << it.first.c_str() << ": " << it.second.c_str() << std::endl;
    }
}

void GemmaThread::OnWebSocketMessage(
    std::shared_ptr<ix::ConnectionState> connectionState,
    ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
{
    logger(logger::V).os << msg->str.c_str() << std::endl;

    json data = json::parse(msg->str);
    if(data.contains("stop")) {
        cleanPrompt();
        m_break = true;
    }
    else {
        std::string session = connectionState->getRemoteIp() + connectionState->getId();

        auto sendBack = [&](int progress, int max,
                    std::string session, std::string text,
                    bool eos, void *user) {
            json data;
            data["id"] = session;
            data["object"] = "chat.completion";
            data["model"] = std::string("gemma-")
                          + gcpp::TypeName(gcpp::GemmaWeightT())
                          + "-"
                          + gcpp::TypeName(gcpp::EmbedderInputT());
            data["choices"]["messages"]["role"] = "assistant";
            data["choices"]["messages"]["content"] = text;
            data["usage"]["prompt_tokens"] = max - progress;
            data["usage"]["completion_tokens"] = progress;
            data["usage"]["total_tokens"] = max;
            for (auto&& client : this->m_server->getClients()) {
                if (client.get() == user) {
                    logger(logger::V).os << data.dump() << std::endl;
                    client->send(data.dump(), false);
                }
            }
        };

        std::string content = "";
        try {
            content = data["messages"]["content"];
        }
        catch (json::exception& e) {
            logger(logger::W).os << "Failed to parse " << msg->str
                        << " : " << e.what() << std::endl;
        }
        if(content == "") {
            sendBack(0, 0, session, "<|fim_middle|>", true, &webSocket);
            sendBack(0, 0, session, "Caught exception", true, &webSocket);
            sendBack(0, 0, session, "<|file_separator|>", true, &webSocket);
        }
        else {
            logger(logger::V).os << "prompt: " << content << std::endl;
            setPrompt(session, content,
                [&](int progress, int max,
                        std::string session, std::string text,
                        bool eos, void *user) {
                    logger(logger::V).os << "token: " << text << std::endl;
                    sendBack(progress, max, session, text, eos, user);
                    if(eos) {
                        logger(logger::D).os << "EOS" << std::endl;
                    }
                }, &webSocket);
        }
    }
}

bool GemmaThread::webSocketStart(int port)
{
    m_server = std::make_shared<ix::WebSocketServer>(port);
    std::string connectionId;
    auto factory = []() -> std::shared_ptr<ix::ConnectionState> {
        return std::make_shared<ix::ConnectionState>();
    };
    m_server->setConnectionStateFactory(factory);

    m_server->setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                logger(logger::I).os << "New connection" << std::endl;
                OnWebSocketOpen(connectionState, webSocket, msg);
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                logger(logger::I).os << "Closed connection" << std::endl;
            }
            else if (msg->type == ix::WebSocketMessageType::Message) {
                OnWebSocketMessage(connectionState, webSocket, msg);
            }
        });

    auto res = m_server->listen();
    if (!res.first) {
        logger(logger::E).os << "failed : " << res.second.c_str() << std::endl;
        return false;
    }

    m_server->start();
    logger(logger::I).os << "Websocket Started" << std::endl;
    return true;
}

void GemmaThread::webSocketStop()
{
    logger(logger::TI).os << __FUNCTION__;
    m_server->stop();
    logger(logger::TO).os << __FUNCTION__;
}
