#include "mainwindow.h"
#include "gemmathread.h"

#include "setting.h"
#include "parsefile.h"
#include "parsefunction.h"
#include "about.h"
#include "logger.h"

#include <filesystem>

#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QStandardItem>
#include <QMetaProperty>

LogFunc logger::func = nullptr;
bool logger::trace_off = true;
int logger::threshold = logger::I;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_processing(false)
{
    setWindowState(Qt::WindowMaximized);

    ui->setupUi(this);
    ui->progress->setFormat("Parsing Progress: %p%");

    QRect viewGeometry(QPoint(0, 0), size());
    m_page = std::make_shared<QWebEnginePage>(this);
    ui->webView->setContextMenuPolicy(Qt::NoContextMenu);
    ui->webView->setPage(m_page.get());
    ui->webView->setGeometry(viewGeometry);
    ui->webView->setUrl(QUrl("qrc:/index.html"));

    ui->listSessions->setMainWindow(this);
    ui->prompt->setMainWindow(this);

    ui->splitter1->addWidget(ui->frameSession);
    ui->splitter1->addWidget(ui->frameView);
    ui->splitter1->setStretchFactor(0, 1);
    ui->splitter1->setStretchFactor(1, 9);

    ui->splitter2->addWidget(ui->frameMain);
    ui->splitter2->addWidget(ui->framePrompt);
    ui->splitter2->setStretchFactor(0, 9);
    ui->splitter2->setStretchFactor(1, 1);

    m_gemma = std::make_shared<GemmaThread>(readConfig().toStdString());

    m_content.setText("");
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("content"), &m_content);
    m_page->setWebChannel(m_channel);

    connect(ui->actionSetting, &QAction::triggered, this, &MainWindow::onSetting);
    connect(ui->actionParseFile, &QAction::triggered, this, &MainWindow::onParseFile);
    connect(ui->actionParseFunction, &QAction::triggered, this, &MainWindow::onParseFunction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

    // QShortcut *shortcut = new QShortcut(Qt::Key_Return, this);
    // connect(shortcut, &QShortcut::activated, this, &MainWindow::on_send_clicked);

    m_timer = std::make_shared<QTimer>(this);
    connect(m_timer.get(), &QTimer::timeout, this, &MainWindow::onTimerSave);
    m_timer->start(m_timer_ms);

    m_ws = std::make_shared<WebSocketClient>(this);

    const char* instructions = "\n"
        "**Usage**\n\n"
        "- Enter an instruction and press enter.\n\n"
        "**Examples**\n\n"
        "- Write an email to grandma thanking her for the cookies.\n"
        "- What are some historical attractions to visit around "
        "Massachusetts?\n"
        "- Compute the nth fibonacci number in javascript.\n"
        "- Write a standup comedy bit about GPU programming.\n"
        "\n\n---\n";
    m_content.appendText(instructions);
}

MainWindow::~MainWindow()
{
    m_gemma->stop();
    m_gemma->join();

    delete m_channel;
    delete ui;
}

QString MainWindow::readConfig()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("Setting");

    m_ctags = settings.value("ctags").toString();
    m_timer_ms = settings.value("timer", 10000).toInt();

    m_port = settings.value("WebSocketPort").toInt();
    settings.endGroup();

    return settings.fileName();
}

bool MainWindow::loadFile(const QString &path)
{
    bool ret = false;
    if (!path.isEmpty()) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            ret = true;
        }
        else {
            m_content.appendText(
                  "Could not open file" + QDir::toNativeSeparators(path)
                + " : " + f.errorString() + "\n\n");
        }
    }
    return ret;
}

void MainWindow::onSetting()
{
    logger(logger::TI).os << __FUNCTION__;
    Setting dlg(this);
    if(dlg.exec() == QDialog::Accepted) {
        m_ctags = dlg.ctags();
        m_gemma->stop();
        m_gemma->join();
        m_gemma = nullptr;
        m_ws = nullptr;
        m_gemma = std::make_shared<GemmaThread>(readConfig().toStdString());
        m_ws = std::make_shared<WebSocketClient>(this);
    }
    logger(logger::TO).os << __FUNCTION__;
}

void MainWindow::onParseFile()
{
    if(m_processing) {
        QMessageBox::warning(this, windowTitle(), "You need to stop the current work first.");
    }
    else if(m_ws->isValid()) {
        ParseFile dlg(this);
        if(dlg.exec() == QDialog::Accepted) {
            if(dlg.parse()) {
                prepare();
            }
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(), "Gemma Server Lost.");
    }
}

void MainWindow::onParseFunction()
{
    if(m_processing) {
        QMessageBox::warning(this, windowTitle(), "You need to stop the current work first.");
    }
    else if(m_ws->isValid()) {
        ParseFunction dlg(this);
        if(dlg.exec() == QDialog::Accepted) {
            if(dlg.parse()) {
                prepare();
            }
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(), "Gemma Server Lost.");
    }
}

void MainWindow::onSaveAs()
{
    QString filter = "Markdown File (*.md)";
    QString path = QFileDialog::getSaveFileName(this,
        tr("Open Markdown File to Save"), "", tr("Markdown File (*.md)"));
    if(path.length() > 0) {
        if(path.right(3) != ".md" && path.right(9) != ".markdown") {
            path += ".md";
        }

        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(m_content.text().toUtf8());
            f.close();

            m_content.appendText("Saved to " + path + " .\n\n");
        }
        else {
            m_content.appendText(
                  "Could not save to file" + QDir::toNativeSeparators(path)
                + " : " + f.errorString() + "\n\n");
        }
    }
}

void MainWindow::onTimerSave()
{
    if(m_ws->session() != "") {
        QDir dir;
        QString path = dir.homePath() + "/.Gemma.QT/";
        dir.mkdir(path);

        QFile f(path + m_ws->session());
        if (f.open(QIODevice::WriteOnly)) {
            f.write(m_content.text().toUtf8());
            f.close();
        }
    }
}

void MainWindow::onAbout()
{
    AboutBox msgBox;
    msgBox.exec();
}

void MainWindow::on_doGemma(QString text)
{
    static int wait_for_spec = 0;
    if(text == "<end_of_turn>" && wait_for_spec == 0) {
        wait_for_spec++;
    }
    else if(wait_for_spec > 0) {
        if(text == "<start_of_turn>" && wait_for_spec == 1) {
            wait_for_spec++;
        }
        if(text == "model" && wait_for_spec == 2) {
            wait_for_spec = 0;
        }
    }
    else {
        // text.replace("\n", "\n\n");
        m_content.appendText(text);
    }
}

void MainWindow::on_doGemmaFinished()
{
    m_processing = false;
    m_content.appendText("\n\n---\n");
    ui->progress->setValue(0);
    ui->send->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
}

void MainWindow::prepare()
{
    QString stop = QCoreApplication::translate("MainWindow", "Stop", nullptr);
    int propertyIndex = ui->send->metaObject()->indexOfProperty("text");
    QMetaProperty property = ui->send->metaObject()->property(propertyIndex);
    property.write(ui->send, stop);

    QMetaObject::invokeMethod(ui->progress, "setValue", Q_ARG(int, 1));

    if(m_ws->session() == "") {
        on_newSession_clicked();
    }
    m_processing = true;
}

void MainWindow::on_send_clicked()
{
    if(m_ws->isValid()) {
        if(m_processing) {
            m_ws->sendStop();
        }
        else {
            prepare();
            QString text = ui->prompt->toPlainText();
            m_ws->sendMessage(text);
            ui->prompt->selectAll();
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(), "Gemma Server Lost.");
    }
}

void MainWindow::on_reset_clicked()
{
    m_content.setText("");
}

void MainWindow::on_newSession_clicked()
{
    QString session = QDateTime::currentDateTime()
                                        .toString("yyyy-MM-dd hh:mm:ss.zzz");
    m_ws->setSession(session);
    ui->listSessions->appendRow(session);
}
