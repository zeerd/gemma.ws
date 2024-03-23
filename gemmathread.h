#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <queue>

#include <QString>
#include <QThread>

#include "gemma.h"  // Gemma
#include "util/app.h"
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"

class Session {
public:
    Session(gcpp::Model type)
        : m_abs_pos(0)
        , m_kv_cache(CreateKVCache(type)){
    }
    int m_abs_pos;
    gcpp::KVCache m_kv_cache;
};

class MainWindow;
class GemmaThread : public QThread {
    Q_OBJECT

public:
    explicit GemmaThread(QObject *parent = nullptr);
    ~GemmaThread();
    void setPrompt(std::string prompt);
    void appendPrompt(std::string prompt);
    void cleanPrompt();

    void gemmaInit();
    void gemmaUninit();

protected:
    void run() override;

private:
    gcpp::Model ModelType(std::string model_type) {
        if (model_type == "2b-pt" || model_type == "2b-it") {
            return gcpp::Model::GEMMA_2B;
        } else {
            return gcpp::Model::GEMMA_7B;
        }
    }

public:
    bool m_break;

    QString m_fileWeight;
    QString m_fileTokenizer;
    QString m_model_type;

    gcpp::RuntimeConfig m_config;

    std::shared_ptr<gcpp::Gemma> m_model;
    std::shared_ptr<hwy::ThreadPool> m_pool;

private:
    size_t m_num_threads;
    MainWindow *m_mainWindow;
    std::string m_prompt;
    bool m_running;

    std::queue<std::string> m_prompts;
    std::map<QString, std::shared_ptr<Session>> m_sessions;
};

#endif // GEMMATHREAD_H
