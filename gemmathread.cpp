#include "mainwindow.h"
#include "gemmathread.h"
#include "ui_mainwindow.h"

#include <QFile>

#include "util/app.h"

GemmaThread::GemmaThread(QObject *parent)
        : m_break(false)
        , m_model(NULL)
        , m_mainWindow((MainWindow*)parent)
        , m_prompt("")
        , m_running(false)
{
}

void GemmaThread::setPrompt(std::string prompt) { m_prompt = prompt; }

void GemmaThread::gemmaInit()
{
    if(QFile::exists(m_fileWeight) && QFile::exists(m_fileTokenizer)
    && (m_model == NULL)) {
        gcpp::LoaderArgs loader(0, NULL);
        m_inference = new gcpp::InferenceArgs(0, NULL);
        gcpp::AppArgs app(0, NULL);

        loader.tokenizer = m_fileTokenizer.toStdString().c_str();
        loader.model_type = "2b-it";
        loader.cache = m_fileWeight.toStdString().c_str();

        m_mainWindow->saveConfig();

        m_inner_pool = new hwy::ThreadPool(0);
        m_pool = new hwy::ThreadPool(app.num_threads);
        gcpp::PinThreadToCore(app.num_threads - 1);  // Main thread

        m_pool->Run(0, m_pool->NumThreads(),
                 [](uint64_t /*task*/, size_t thread) { gcpp::PinThreadToCore(thread); });

        m_abs_pos = 0;
        m_current_pos = 0;
        m_model = new gcpp::Gemma(loader, *m_pool);

        const char* instructions = "\n"
            "**Usage**\n\n"
            "- Enter an instruction and press enter.\n\n"
            "**Examples**\n\n"
            "- Write an email to grandma thanking her for the cookies.\n"
            "- What are some historical attractions to visit around "
            "Massachusetts?\n"
            "- Compute the nth fibonacci number in javascript.\n"
            "- Write a standup comedy bit about GPU programming.\n";
        m_mainWindow->m_content.appendText(instructions);
        m_mainWindow->m_content.appendText("\n\n---\n");
    }
}

void GemmaThread::gemmaUninit()
{
    if(m_model != NULL) {
        delete m_pool;
        delete m_inner_pool;
        delete m_inference;
        delete m_model;
    }
}

void GemmaThread::run()
{
    m_break = false;

    int prompt_size{};
    m_current_pos = 0;

    std::mt19937 gen;
    std::random_device rd;
    gen.seed(rd());

    if (m_prompt.length() == 0) {
        m_prompt = m_mainWindow->ui->prompt->text().toStdString();
    }
    auto stream_token = [this, &gen, &prompt_size,
                         tokenizer = &m_model->Tokenizer()
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
            QMetaObject::invokeMethod(this->m_mainWindow,
                        "on_doGemma", Q_ARG(QString, token_text.c_str()));
        }
        return !m_break;
    };

    std::vector<int> prompt;
    if (m_model->model_training == gcpp::ModelTraining::GEMMA_IT) {
        // For instruction-tuned models: add control tokens.
        m_prompt = "<start_of_turn>user\n" + m_prompt +
                      "<end_of_turn>\n<start_of_turn>model\n";
        if (m_abs_pos > 0) {
            // Prepend "<end_of_turn>" token if this is a multi-turn dialogue
            // continuation.
            m_prompt = "<end_of_turn>\n" + m_prompt;
        }
    }

    HWY_ASSERT(m_model->Tokenizer().Encode(m_prompt, &prompt).ok());

    // For both pre-trained and instruction-tuned models: prepend "<bos>" token
    // if needed.
    if (m_abs_pos == 0) {
        prompt.insert(prompt.begin(), 2);
    }

    prompt_size = prompt.size();
    QMetaObject::invokeMethod(this->m_mainWindow->ui->progress,
                        "setRange", Q_ARG(int, 0), Q_ARG(int, prompt_size));

    gcpp::GenerateGemma(*m_model, *m_inference,
        prompt, m_abs_pos,
        *m_pool, *m_inner_pool, stream_token,
                  /*accept_token=*/[](int) { return true; }, gen, 2);

    m_prompt = "";

    if(m_break) {
        QMetaObject::invokeMethod(this->m_mainWindow,
                        "on_doGemma", Q_ARG(QString, "\n<User break>"));
    }
    QMetaObject::invokeMethod(this->m_mainWindow, "on_doGemmaFinished");
}
