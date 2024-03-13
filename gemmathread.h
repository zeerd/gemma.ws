#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <queue>

#include <QString>
#include <QThread>

#include "gemma.h"  // Gemma
#include "util/app.h"  // Gemma
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"

class MainWindow;
class GemmaThread : public QThread {
    Q_OBJECT

public:
    explicit GemmaThread(QObject *parent = nullptr);
    ~GemmaThread();
    void setPrompt(std::string prompt);
    void appendPrompt(std::string, std::string);

    void gemmaInit();
    void gemmaUninit();

protected:
    void run() override;

public:
    bool m_break;

    QString m_fileWeight;
    QString m_fileTokenizer;
    QString m_model_type;

    gcpp::Gemma *m_model;
    gcpp::KVCache m_kv_cache;
    hwy::ThreadPool *m_pool;
    int m_abs_pos;
    int m_current_pos; // token index within the current turn

private:
    size_t m_num_threads;
    MainWindow *m_mainWindow;
    std::string m_prompt;
    bool m_running;

    std::queue<std::pair<std::string, std::string>> m_prompts;
};

#endif // GEMMATHREAD_H
