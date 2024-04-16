#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <iostream>
#include <memory>

#include <ixwebsocket/IXSocket.h>
#include <ixwebsocket/IXSocketFactory.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

class GemmaThread;
class WebSocketServer {

public:
    explicit WebSocketServer(std::shared_ptr<GemmaThread> gemma);
    ~WebSocketServer();

private:
    std::shared_ptr<ix::WebSocketServer> m_server;
    std::shared_ptr<GemmaThread> m_gemma;
};

#endif /* WEBSOCKET_H */
