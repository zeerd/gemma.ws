#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "document.h"
#include "gemma.h"  // Gemma

#include "hwy/base.h"
#include "hwy/contrib/thread_pool/thread_pool.h"
#include "hwy/highway.h"
#include "hwy/per_target.h"
#include "hwy/profiler.h"
#include "hwy/timer.h"

#include <QMainWindow>
#include <QString>
#include <QWebChannel>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MyThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool loadFile(const QString &path);

private slots:
    void onLoadWeight();
    void onLoadTokenizer();
    void onSaveAs();

    void on_doGemma(QString text);
    void on_doGemmaFinished();

    void on_send_clicked();
    void on_load_clicked();
    void on_reset_clicked();

private:
    void gemmaInit();
    void gemmaUninit();

public:
    static MyThread *m_thread;
    gcpp::Gemma *m_model;
    gcpp::InferenceArgs *m_inference;
    hwy::ThreadPool *m_inner_pool;
    hwy::ThreadPool *m_pool;
    int m_abs_pos;
    int m_current_pos; // token index within the current turn

    Ui::MainWindow *ui;
    QString m_fileWeight;
    QString m_fileTokenizer;

    QWebChannel *m_channel;
    Document m_content;
};

class MyThread : public QThread {
    Q_OBJECT
public:
    MyThread(QObject *parent = nullptr) : m_prompt("") {
        m_mainWindow = (MainWindow*)parent; }
    void setPrompt(std::string prompt) { m_prompt = prompt; }
protected:
    void run() override;
private:
    MainWindow *m_mainWindow;
    std::string m_prompt;
    bool m_running;
};

#endif // MAINWINDOW_H
