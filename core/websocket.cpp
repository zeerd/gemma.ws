#include "websocket.h"
#include "gemmathread.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

WebSocketServer::WebSocketServer(std::shared_ptr<GemmaThread> gemma)
        : m_gemma(gemma)
{
    m_server = std::make_shared<ix::WebSocketServer>(9999);
    std::string connectionId;
    auto factory = []() -> std::shared_ptr<ix::ConnectionState> {
        return std::make_shared<ix::ConnectionState>();
    };
    m_server->setConnectionStateFactory(factory);

    m_server->setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> connectionState,
                                 ix::WebSocket& webSocket,
                                 const ix::WebSocketMessagePtr& msg) {
            auto remoteIp = connectionState->getRemoteIp();

            if (msg->type == ix::WebSocketMessageType::Open)
            {
                std::cout << "New connection";
                connectionState->computeId();
                std::cout << "remote ip: " << remoteIp.c_str();
                std::cout << "id: " << connectionState->getId().c_str();
                std::cout << "Uri: " << msg->openInfo.uri.c_str();
                std::cout << "Headers:";
                for (auto it : msg->openInfo.headers)
                {
                    std::cout << it.first.c_str() << ": " << it.second.c_str();
                }

                std::string connectionId = connectionState->getId();
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                std::cout << "Closed connection";
            }
            else if (msg->type == ix::WebSocketMessageType::Message)
            {
                std::cout << msg->str.c_str();

                json data = json::parse(msg->str);
                std::string prompt = data["prompt"];

                m_gemma->setPrompt(remoteIp, prompt);
                if(m_gemma->isRunning()) {
                    m_gemma->terminate();
                }
                m_gemma->setCallback([&](int progress, int max, std::string text, bool eos) {
                    json data;
                    data["id"] = 0;
                    data["token"] = text;
                    for (auto&& client : this->m_server->getClients())
                    {
                        // if (client.get() != &webSocket)
                        {
                            client->send(data.dump(), false);
                        }
                    }
                    if(eos) {
                    }
                });
                m_gemma->start();
            }
        });

    auto res = m_server->listen();
    if (!res.first)
    {
        std::cout << res.second.c_str();
        return;// false;
    }

    m_server->start();
    std::cout << "started";
}

WebSocketServer::~WebSocketServer()
{
    m_server->stop();
}
