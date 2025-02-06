#ifndef GEMMATHREAD_H
#define GEMMATHREAD_H

#include <ixwebsocket/IXSocket.h>
#include <ixwebsocket/IXSocketFactory.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "gemma.h"  // Gemma
#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"
#include "util/app.h"
#include "util/threading.h"

using namespace gcpp;

class GemmaThread : public std::thread {
 public:
  using callback =
      std::function<void(int, int, std::string, std::string, bool, void*)>;

 public:
  explicit GemmaThread(const std::string& file);
  virtual ~GemmaThread();

  void stop();

  bool running() { return m_running; }

 private:
  void threadFunction();

  bool checkFile(const std::string& path);
  void signal();

  bool config();

  void setPrompt(std::string session, std::string prompt, callback cb,
                 void* user);
  void cleanPrompt();

  bool webSocketStart(int port);
  void webSocketStop();

  void OnWebSocketOpen(std::shared_ptr<ix::ConnectionState> connectionState,
                       ix::WebSocket& webSocket,
                       const ix::WebSocketMessagePtr& msg);
  void OnWebSocketMessage(std::shared_ptr<ix::ConnectionState> connectionState,
                          ix::WebSocket& webSocket,
                          const ix::WebSocketMessagePtr& msg);
  void sendBack(int progress, int max, std::string session, std::string text,
                bool eos, void* user);

  bool PictureToPPM(Image& image);
  std::string GetPrompt(std::istream& input, int verbosity,
                        std::string_view eot_line);
  void ReplGemma(Gemma& model, KVCache& kv_cache, const AppArgs& app,
                 const InferenceArgs& args, const AcceptFunc& accept_token,
                 std::string& eot_line);

 private:
  class Session {
   public:
    Session(gcpp::Gemma* model, size_t size)
        : m_kv_cache(gcpp::KVCache::Create(model->GetModelConfig(), size)) {}
    gcpp::KVCache m_kv_cache;
    std::vector<char> m_binary;
  };

  Session* getSession(std::string session = "");

  std::string m_session;
  std::map<std::string, std::shared_ptr<Session>> m_sessions;

 private:
  bool m_break;
  bool m_running;

  std::string m_config_file;
  gcpp::RuntimeConfig m_config;

  std::shared_ptr<std::thread> m_thread;
  std::shared_ptr<gcpp::Gemma> m_model;
  std::shared_ptr<gcpp::NestedPools> m_pool;

  gcpp::Model m_modelType;
  enum gcpp::ModelTraining m_modelTraining;
  int m_port;
  size_t m_prefillTBatchSize;

  std::queue<std::string> m_prompts;

  callback m_callback;
  void* m_user;

  std::condition_variable m_cv;
  std::mutex m_mtx;
  bool m_ready;

  std::shared_ptr<ix::WebSocketServer> m_server;

  std::shared_ptr<gcpp::LoaderArgs> m_loader;
  std::shared_ptr<gcpp::InferenceArgs> m_inference;
  std::shared_ptr<gcpp::AppArgs> m_app;
};

#endif  // GEMMATHREAD_H
