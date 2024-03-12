#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "document.h"

#include <QMainWindow>
#include <QString>
#include <QWebChannel>
#include <QThread>

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
    void onSaveAs();
    void onAbout();

    void on_doGemma(QString text);
    void on_doGemmaFinished();

    void on_send_clicked();
    void on_load_clicked();
    void on_reset_clicked();

private:
    void startThread();

public:
    void saveConfig();
    void readConfig();

public:
    static GemmaThread *m_gemma;

    Ui::MainWindow *ui;

    QWebChannel *m_channel;
    Document m_content;
};

#endif // MAINWINDOW_H
