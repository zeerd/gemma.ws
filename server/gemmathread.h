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

class GemmaThread : public std::thread {

public:
    using callback = std::function<void(int, int, std::string, std::string, bool, void*)>;

public:
    explicit GemmaThread(const std::string &file);
    virtual ~GemmaThread();

    void stop();

    bool running() { return m_running; }

private:
    void threadFunction();

    bool checkFile(const std::string &path);
    void signal();

    bool config();
    void run();

    void setPrompt(std::string session, std::string prompt, callback cb, void *user);
    void cleanPrompt();

    bool webSocketStart(int port);
    void webSocketStop();

    void OnWebSocketOpen(std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg);
    void OnWebSocketMessage(std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg);

private:
    class Session {
    public:
        Session(gcpp::Model type)
            : m_abs_pos(0)
            , m_kv_cache(CreateKVCache(type)){
        }
        int m_abs_pos;
        gcpp::KVCache m_kv_cache;
    };

    std::string m_session;
    std::map<std::string, std::shared_ptr<Session>> m_sessions;

private:
    bool m_break;
    bool m_running;

    std::string m_config_file;
    gcpp::RuntimeConfig m_config;

    std::shared_ptr<std::thread> m_thread;
    std::shared_ptr<gcpp::Gemma> m_model;
    std::shared_ptr<hwy::ThreadPool> m_pool;

    std::string m_fileWeight;
    std::string m_fileTokenizer;
    std::string m_modelTypeName;
    gcpp::Model m_modelType;
    enum gcpp::ModelTraining m_modelTraining;
    int m_port;

    std::queue<std::string> m_prompts;

    callback m_callback;
    void *m_user;

    std::condition_variable m_cv;
    std::mutex m_mtx;
    bool m_ready;

    std::shared_ptr<ix::WebSocketServer> m_server;
};

#endif // GEMMATHREAD_H
