#include "mainwindow.h"
#include "gemmathread.h"

#include <iostream>
#include "util/args.h"  // Path


GemmaThread::GemmaThread(QObject *parent)
        : m_break(false)
        , m_model_type("2b-it")
        , m_model(NULL)
        , m_pool(NULL)
        , m_mainWindow((MainWindow*)parent)
        , m_running(false)
{
    m_num_threads = static_cast<size_t>(std::clamp(
            static_cast<int>(std::thread::hardware_concurrency()) - 2, 1, 18));
}

GemmaThread::~GemmaThread()
{
}

void GemmaThread::setPrompt(std::string prompt)
{
    m_prompts.push(prompt);
}

void GemmaThread::appendPrompt(std::string prompt)
{
    m_prompts.push(prompt);
}

void GemmaThread::gemmaInit()
{
    if(QFile::exists(m_fileWeight) && QFile::exists(m_fileTokenizer)
    && (m_model_type != "") && (m_model == NULL)) {
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                            "setRange", Q_ARG(int, 0), Q_ARG(int, 0));

        gcpp::Path tokenizer;
        gcpp::Path compressed_weights;

        tokenizer = m_fileTokenizer.toStdString().c_str();
        compressed_weights = m_fileWeight.toStdString().c_str();

        m_pool = std::make_shared<hwy::ThreadPool>(m_num_threads);
        m_model = std::make_shared<gcpp::Gemma>(tokenizer, compressed_weights,
                            ModelType(m_model_type.toStdString()), *m_pool);

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
        QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemma",
                    Q_ARG(QString, instructions));
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                            "setRange", Q_ARG(int, 0), Q_ARG(int, 1));
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
    m_sessions[this->m_mainWindow->m_session_name]->m_abs_pos = 0;
}

void GemmaThread::run()
{
    m_break = false;

    gemmaInit();

    int prompt_size{};

    std::mt19937 gen;
    std::random_device rd;
    gen.seed(rd());

    std::shared_ptr<Session> session = NULL;
    if(m_model != NULL) {
        if(m_sessions.find(this->m_mainWindow->m_session_name) != m_sessions.end()) {
            session = m_sessions[this->m_mainWindow->m_session_name];
        }
        else {
            session = std::make_shared<Session>(ModelType(m_model_type.toStdString()));
            m_sessions[this->m_mainWindow->m_session_name] = session;
        }
    }

    std::string prompt_text = "";
    while (!m_prompts.empty() && !m_break && m_model != NULL) {
        prompt_text = m_prompts.front();
        m_prompts.pop();

        QString markdown_prompt = QString("\n\n**Question:**\n\n```\n");
        markdown_prompt += prompt_text.c_str();
        markdown_prompt += "```\n\n**Answer:**\n\n";
        QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemma",
                    Q_ARG(QString, markdown_prompt));
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                    "setValue", Q_ARG(int, 1));

        int current_pos = 0; // token index within the current turn
        auto stream_token = [this, &gen, &prompt_size, &session, &current_pos,
                             tokenizer = m_model->Tokenizer()
                             ](int token, float) {
            ++(session->m_abs_pos);
            ++current_pos;

            if (current_pos < prompt_size) {
                QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                    "setValue", Q_ARG(int, 1 + current_pos));
            }
            else if (token == gcpp::EOS_ID) {
                // GenerateGemma() may be finished by many reasons.
                // EOS is only one of them.
            }
            else {
                std::string token_text;
                HWY_ASSERT(tokenizer->Decode(std::vector<int>{token}, &token_text).ok());
                // +1 since position is incremented above
                if (current_pos == prompt_size + 1) {
                  // first token of response
                  token_text.erase(0, token_text.find_first_not_of(" \t\n"));
                }
                std::cout << token_text;
                fflush(stdout);
                QMetaObject::invokeMethod(this->m_mainWindow,
                            "on_doGemma", Q_ARG(QString, token_text.c_str()));
            }
            return !m_break;
        };

        std::vector<int> prompt;
        if (m_model->model_training == gcpp::ModelTraining::GEMMA_IT) {
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
        HWY_ASSERT(m_model->Tokenizer()->Encode(prompt_text, &prompt).ok());

        // For both pre-trained and instruction-tuned models: prepend "<bos>" token
        // if needed.
        if (session->m_abs_pos == 0) {
            prompt.insert(prompt.begin(), 2);
        }

        prompt_size = prompt.size();
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                            "setRange", Q_ARG(int, 0), Q_ARG(int, prompt_size));

        /*bool succ = */gcpp::GenerateGemma(*m_model, m_config,
                prompt, session->m_abs_pos, session->m_kv_cache, *m_pool,
                stream_token, gen);
        // if(!succ) {
        //     break;
        // }
    }

    QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemmaFinished");
}
