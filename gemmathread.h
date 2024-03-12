#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <QString>
#include <QThread>

#include "gemma.h"  // Gemma
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
    void setPrompt(std::string prompt);

    void gemmaInit();
    void gemmaUninit();

protected:
    void run() override;

public:
    bool m_break;

    QString m_fileWeight;
    QString m_fileTokenizer;

    gcpp::Gemma *m_model;
    gcpp::InferenceArgs *m_inference;
    hwy::ThreadPool *m_inner_pool;
    hwy::ThreadPool *m_pool;
    int m_abs_pos;
    int m_current_pos; // token index within the current turn

private:
    MainWindow *m_mainWindow;
    std::string m_prompt;
    bool m_running;
};

#endif // GEMMATHREAD_H
