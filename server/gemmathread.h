#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <queue>
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <memory>

#include "gemma.h"  // Gemma
#include "util/app.h"
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"

#include <ixwebsocket/IXSocket.h>
#include <ixwebsocket/IXSocketFactory.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

class Session {
public:
    Session(gcpp::Model type)
        : m_abs_pos(0)
        , m_kv_cache(CreateKVCache(type)){
    }
    int m_abs_pos;
    gcpp::KVCache m_kv_cache;
};

class GemmaThread : public std::thread {

public:
    using callback = std::function<void(int, int, std::string, std::string, bool, void*)>;

public:
    explicit GemmaThread(const std::string &file);
    virtual ~GemmaThread();
    void setPrompt(std::string session, std::string prompt, callback cb, void *user);
    void cleanPrompt();

    bool init(const std::string &path);
    void stop();

private:
    bool checkFile(const std::string &path);
    void signal();
    void threadFunction(const std::string &path);

    bool webSocketStart(int port);
    void webSocketStop();

    void OnWebSocketOpen(std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg);

    void OnWebSocketMessage(std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg);

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
    std::string m_setting_file;

    size_t m_num_threads;
    std::string m_prompt;
    void *m_user;
    bool m_running;

    std::queue<std::string> m_prompts;
    std::map<std::string, std::shared_ptr<Session>> m_sessions;
    std::string m_session;

    callback m_callback;

private:
    std::shared_ptr<ix::WebSocketServer> m_server;
    std::condition_variable cv;
    std::mutex mtx;
    bool ready = false;
};

#endif // GEMMATHREAD_H
