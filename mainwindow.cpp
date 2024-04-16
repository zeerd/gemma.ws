#include "mainwindow.h"
#include "gemmathread.h"

#include "setting.h"
#include "parsefile.h"
#include "parsefunction.h"
#include "about.h"

#include <filesystem>

#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QStandardItem>
#include <QMetaProperty>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

    m_gemma = std::make_shared<GemmaThread>();
    readConfig();

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

    if(m_WebSocketOpt) {
        // m_ws = std::make_shared<WebSocketServer>(this);
        // m_ws->start(m_port);
        m_ws = std::make_shared<WebSocketServer>(m_gemma);
    }

    prepareThread();
    m_gemma->start();
}

MainWindow::~MainWindow()
{
    // m_timer-stop();
    m_gemma->gemmaUninit();
    delete m_channel;
    delete ui;
}

void MainWindow::readConfig()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("Setting");

    m_gemma->m_fileWeight = settings.value("Weight").toString().toStdString();
    m_gemma->m_fileTokenizer = settings.value("Tokenizer").toString().toStdString();
    m_gemma->m_model_type = settings.value("ModelType", "2b-it").toString().toStdString();

    m_gemma->m_config.max_tokens = settings.value("MaxTokens", 3072).toInt();
    m_gemma->m_config.max_generated_tokens = settings.value("MaxGeneratedTokens", 2048).toInt();
    m_gemma->m_config.temperature = settings.value("Temperature", 1.0).toFloat();
    m_gemma->m_config.verbosity = settings.value("Verbosity", 2).toInt();

    m_ctags = settings.value("ctags").toString();
    m_timer_ms = settings.value("timer", 10000).toInt();

    m_port = settings.value("WebSocketPort").toInt();
    m_WebSocketOpt = settings.value("WebSocketOpt").toBool();
    settings.endGroup();
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
    Setting dlg(this);
    if(dlg.exec() == QDialog::Accepted) {
        m_ctags = dlg.ctags();
        m_WebSocketOpt = dlg.websocket();
        prepareThread();
        m_gemma->start();
    }
}

void MainWindow::onParseFile()
{
    if(m_gemma->m_model == NULL) {
        QMessageBox::warning(this, windowTitle(), "Model had not loaded.");
    }
    else if(m_gemma->isRunning()) {
        QMessageBox::warning(this, windowTitle(), "You need to stop the current work first.");
    }
    else {
        ParseFile dlg(this);
        if(dlg.exec() == QDialog::Accepted) {
            if(dlg.parse()) {
                prepareThread();
                m_gemma->start();
            }
        }
    }
}

void MainWindow::onParseFunction()
{
    if(m_gemma->m_model == NULL) {
        QMessageBox::warning(this, windowTitle(), "Model had not loaded.");
    }
    else if(m_gemma->isRunning()) {
        QMessageBox::warning(this, windowTitle(), "You need to stop the current work first.");
    }
    else {
        ParseFunction dlg(this);
        if(dlg.exec() == QDialog::Accepted) {
            if(dlg.parse()) {
                prepareThread();
                m_gemma->start();
            }
        }
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
    if(m_session_name != "") {
        QDir dir;
        QString path = dir.homePath() + "/.Gemma.QT/";
        dir.mkdir(path);

        QFile f(path + m_session_name);
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
    m_content.appendText("\n\n---\n");
    ui->progress->setValue(0);
    ui->send->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
}

void MainWindow::prepareThread(bool restart)
{
    m_gemma->m_break = false;
    QString stop = QCoreApplication::translate("MainWindow", "Stop", nullptr);
    // ui->send->setText(stop);
    int propertyIndex = ui->send->metaObject()->indexOfProperty("text");
    QMetaProperty property = ui->send->metaObject()->property(propertyIndex);
    property.write(ui->send, stop);

    // ui->progress->setValue(1);
    QMetaObject::invokeMethod(ui->progress, "setValue", Q_ARG(int, 1));

    if(m_session_name == "") {
        on_newSession_clicked();
    }

    if(restart) {
        if(m_gemma->isRunning()) {
            m_gemma->terminate();
        }
    }

    m_gemma->setCallback([&](int progress, int max, std::string text, bool eos) {
        QMetaObject::invokeMethod(ui->progress, "setValue", Q_ARG(int, progress));
        QMetaObject::invokeMethod(ui->progress,
                                    "setRange", Q_ARG(int, 0), Q_ARG(int, max));
        if(eos) {
            // this->on_doGemmaFinished();
            QMetaObject::invokeMethod(this, "on_doGemmaFinished");
        }
        else {
            // this->on_doGemma(QString(text.c_str()));
            QMetaObject::invokeMethod(this, "on_doGemma",
                        Q_ARG(QString, QString(text.c_str())));
        }
    });
}

void MainWindow::on_send_clicked()
{
    if(m_gemma->isRunning()) {
        m_gemma->m_break = true;
    }
    else {
        QString text = ui->prompt->toPlainText();
        // text.replace('\n', ' ');
        m_gemma->setPrompt(m_session_name.toStdString(), text.toStdString());
        prepareThread();
        m_gemma->start();

        ui->prompt->selectAll();
    }
}

void MainWindow::on_reset_clicked()
{
    m_gemma->cleanPrompt();
    m_content.setText("");
}

void MainWindow::on_newSession_clicked()
{
    m_session_name = QDateTime::currentDateTime()
                                        .toString("yyyy-MM-dd hh:mm:ss.zzz");
    ui->listSessions->appendRow(m_session_name);
}
