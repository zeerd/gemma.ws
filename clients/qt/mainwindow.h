#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "document.h"
#include "ui_mainwindow.h"
#include "websocketclient.h"

#include <QMainWindow>
#include <QString>
#include <QWebChannel>
#include <QThread>
#include <QFile>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class GemmaThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool loadFile(const QString &path);
    void prepare();

private slots:
    void onSetting();
    void onParseFile();
    void onParseFunction();
    void onSaveAs();
    void onAbout();
    void onTimerSave();

    void on_doGemma(QString text);
    void on_doGemmaFinished();

    void on_send_clicked();
    void on_reset_clicked();
    void on_newSession_clicked();

private:
    QString readConfig();

public:
    Ui::MainWindow *ui;
    Document m_content;

    std::shared_ptr<GemmaThread> m_gemma;
    std::shared_ptr<WebSocketClient> m_ws;

    QString m_ctags;
    int m_timer_ms;

    int m_port;

private:
    QWebChannel *m_channel;
    std::shared_ptr<QWebEnginePage> m_page;
    std::shared_ptr<QTimer> m_timer;
    std::shared_ptr<QProcess> m_server;
    bool m_processing;
};

#endif // MAINWINDOW_H
