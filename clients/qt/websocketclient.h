#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>

class MainWindow;
class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    virtual ~WebSocketClient();

    QString session() { return m_session_name; }
    void setSession(QString session) { m_session_name = session; }
    void sendMessage(QString message);
    void sendStop();
    bool isValid() { return m_webSocket.isValid(); }

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(QString message);
    void onTimer();

private:
    MainWindow *m_mainWindow;
    QWebSocket m_webSocket;
    std::shared_ptr<QTimer> m_timer;
    QString m_session_name;
};

#endif /* WEBSOCKETCLIENT_H */
