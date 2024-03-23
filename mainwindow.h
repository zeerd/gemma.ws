#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "document.h"
#include "ui_mainwindow.h"

#include <QMainWindow>
#include <QString>
#include <QWebChannel>
#include <QThread>
#include <QFile>

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
    void startThread();
    void readConfig();

public:
    Ui::MainWindow *ui;
    Document m_content;

    std::shared_ptr<GemmaThread> m_gemma;
    QString m_session_name;

    QString m_ctags;
    int m_timer_ms;

private:
    QWebChannel *m_channel;
    std::shared_ptr<QWebEnginePage> m_page;
    std::shared_ptr<QTimer> m_timer;
};

#endif // MAINWINDOW_H
