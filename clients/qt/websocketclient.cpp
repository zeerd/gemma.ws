#include "mainwindow.h"
#include "websocketclient.h"
#include "logger.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_mainWindow((MainWindow*)parent)
{
    logger(logger::TI).os << __FUNCTION__;
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);

    QUrl url("ws://localhost:9999");
    m_webSocket.open(url);

    m_timer = std::make_shared<QTimer>(this);
    connect(m_timer.get(), &QTimer::timeout, this, &WebSocketClient::onTimer);
    logger(logger::TO).os << __FUNCTION__;
}

WebSocketClient::~WebSocketClient()
{
    logger(logger::TI).os << __FUNCTION__;
    m_webSocket.close();
    logger(logger::TO).os << __FUNCTION__;
}

void WebSocketClient::onConnected()
{
    // qDebug() << "Connected";
    m_timer->stop();
}

void WebSocketClient::onDisconnected()
{
    // qDebug() << "Disconnected";
    m_timer->start(100);
}

void WebSocketClient::onTimer()
{
    QUrl url("ws://localhost:9999");
    m_webSocket.open(url);
}

void WebSocketClient::sendMessage(QString message)
{
    qDebug() << "Send " << message;
    json data;
    data["id"] = m_session_name.toStdString();
    data["messages"]["role"] = "user";
    data["messages"]["content"] = message.toStdString();
    m_webSocket.sendTextMessage(data.dump().c_str());

    QString markdown_prompt = ("\n\n**Question:**\n\n```\n");
    markdown_prompt += message;
    markdown_prompt += "\n```\n\n**Answer:**\n\n";
    m_mainWindow->m_content.appendText(markdown_prompt);
}

void WebSocketClient::sendStop()
{
    qDebug() << "sendStop";
    json data;
    data["id"] = m_session_name.toStdString();
    data["stop"] = "stop"; // FIXME: I'd not understood the value
    m_webSocket.sendTextMessage(data.dump().c_str());
}

void WebSocketClient::onTextMessageReceived(QString message)
{
    qDebug() << "Received " << message;
    json data = json::parse(message.toStdString());
    // session = data["id"].c_str();
    std::string token = data["choices"]["messages"]["content"];
    int progress = data["usage"]["completion_tokens"];
    int max = data["usage"]["total_tokens"];

    QMetaObject::invokeMethod(m_mainWindow->ui->progress,
                                "setValue", Q_ARG(int, progress));
    QMetaObject::invokeMethod(m_mainWindow->ui->progress,
                                "setRange", Q_ARG(int, 0), Q_ARG(int, max));
    if("<|file_separator|>" == token) {
        QMetaObject::invokeMethod(m_mainWindow, "on_doGemmaFinished");
    }
    else {
        QMetaObject::invokeMethod(m_mainWindow, "on_doGemma",
                    Q_ARG(QString, QString(token.c_str())));
    }
}
