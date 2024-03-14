#include "mainwindow.h"
#include "gemmathread.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QFile>

#include "util/app.h"

GemmaThread::GemmaThread(QObject *parent)
        : m_break(false)
        , m_model_type("2b-it")
        , m_model(NULL)
        , m_mainWindow((MainWindow*)parent)
        , m_running(false)
{
    m_num_threads = static_cast<size_t>(std::clamp(
            static_cast<int>(std::thread::hardware_concurrency()) - 2, 1, 18));
}

GemmaThread::~GemmaThread()
{
    delete m_model;
    delete m_pool;
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

        gcpp::LoaderArgs loader(0, NULL);

        loader.tokenizer = m_fileTokenizer.toStdString().c_str();
        loader.model_type = m_model_type.toStdString();
        loader.compressed_weights = m_fileWeight.toStdString().c_str();

        m_pool = new hwy::ThreadPool(m_num_threads);
        gcpp::PinThreadToCore(m_num_threads - 1);  // Main thread

        m_pool->Run(0, m_pool->NumThreads(),
                 [](uint64_t /*task*/, size_t thread) { gcpp::PinThreadToCore(thread); });

        m_kv_cache = CreateKVCache(loader.ModelType());

        m_abs_pos = 0;
        m_current_pos = 0;
        m_model = new gcpp::Gemma(loader.tokenizer, loader.compressed_weights,
                                  loader.ModelType(), *m_pool);

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

void GemmaThread::run()
{
    m_break = false;

    gemmaInit();

    int prompt_size{};

    std::mt19937 gen;
    std::random_device rd;
    gen.seed(rd());

    std::string prompt_text = "";
    while (!m_prompts.empty() && !m_break) {
        prompt_text = m_prompts.front();
        m_prompts.pop();

        QString markdown_prompt = QString("\n\n**Question:**\n\n```\n");
        markdown_prompt += prompt_text.c_str();
        markdown_prompt += "```\n\n**Answer:**\n\n";
        QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemma",
                    Q_ARG(QString, markdown_prompt));
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                    "setValue", Q_ARG(int, 1));

        m_current_pos = 0;
        auto stream_token = [this, &gen, &prompt_size,
                             tokenizer = m_model->Tokenizer()
                             ](int token, float) {
            ++(this->m_abs_pos);
            ++(this->m_current_pos);

            if (this->m_current_pos < prompt_size) {
                QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                    "setValue", Q_ARG(int, 1 + this->m_current_pos));
            }
            else if (token == gcpp::EOS_ID) {
                // GenerateGemma() may be finished by many reasons.
                // EOS is only one of them.
            }
            else {
                std::string token_text;
                HWY_ASSERT(tokenizer->Decode(std::vector<int>{token}, &token_text).ok());
                // +1 since position is incremented above
                if (this->m_current_pos == prompt_size + 1) {
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
            if (m_abs_pos > 0) {
                // Prepend "<end_of_turn>" token if this is a multi-turn dialogue
                // continuation.
                prompt_text = "<end_of_turn>\n" + prompt_text;
            }
        }

        // qDebug() << tr(prompt_text.c_str());
        HWY_ASSERT(m_model->Tokenizer()->Encode(prompt_text, &prompt).ok());

        // For both pre-trained and instruction-tuned models: prepend "<bos>" token
        // if needed.
        if (m_abs_pos == 0) {
            prompt.insert(prompt.begin(), 2);
        }

        prompt_size = prompt.size();
        QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                            "setRange", Q_ARG(int, 0), Q_ARG(int, prompt_size));

        gcpp::GenerateGemma(*m_model, m_config,
                prompt, m_abs_pos/*KV cache position = */, m_kv_cache, *m_pool,
                stream_token, gen);
    }

    QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemmaFinished");
}
