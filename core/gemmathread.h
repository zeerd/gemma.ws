#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <queue>
#include <map>
#include <thread>

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

// For c++, define a custom empty thread class using std::thread and do nothing in the run function.

class MainWindow;
class GemmaThread {

public:
    explicit GemmaThread();
    ~GemmaThread();
    void setPrompt(std::string session, std::string prompt);
    void appendPrompt(std::string session, std::string prompt);
    void cleanPrompt();

    void gemmaInit();
    void gemmaUninit();

    void setCallback(std::function<void(int, int, std::string, bool)> cb) {
        m_callback = cb;
    }

    void start();
    bool isRunning() { return m_running; }
    void terminate() { m_running = false; }

private:
    bool checkFile(const std::string &path);

protected:
    void run();

public:
    bool m_break;
    std::shared_ptr<std::thread> m_thread;

    std::string m_fileWeight;
    std::string m_fileTokenizer;
    std::string m_model_type;
    gcpp::Model model_type;
    enum gcpp::ModelTraining model_training;

    gcpp::RuntimeConfig m_config;

    std::shared_ptr<gcpp::Gemma> m_model;
    std::shared_ptr<hwy::ThreadPool> m_pool;

private:
    size_t m_num_threads;
    std::string m_prompt;
    bool m_running;

    std::queue<std::string> m_prompts;
    std::map<std::string, std::shared_ptr<Session>> m_sessions;
    std::string m_session;

    std::function<void(int, int, std::string, bool)> m_callback;
};

#endif // GEMMATHREAD_H
