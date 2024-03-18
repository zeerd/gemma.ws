#include "mainwindow.h"
#include "gemmathread.h"

#include "setting.h"
#include "parsefile.h"
#include "parsefunction.h"

#include <filesystem>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QStandardItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
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

    m_gemma = std::make_shared<GemmaThread>(this);
    readConfig();

    QString welcome = "**Start**\n";
    if(loadFile(m_gemma->m_fileWeight)) {
        welcome += "- " + m_gemma->m_fileWeight + " loaded.\n";
    }
    else {
        m_gemma->m_fileWeight = "";
        welcome += "- Weight file missing.\n";
    }
    if(loadFile(m_gemma->m_fileTokenizer)) {
        welcome += "- " + m_gemma->m_fileTokenizer + " loaded.\n";
    }
    else {
        m_gemma->m_fileTokenizer = "";
        welcome += "- Tokenizer file missing.\n";
    }
    if(m_gemma->m_model_type.length() == 0) {
        welcome += "- Model type not set.\n";
    }
    welcome += "\n";

    m_content.setText(welcome);
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

    m_gemma->m_fileWeight = settings.value("Weight").toString();
    m_gemma->m_fileTokenizer = settings.value("Tokenizer").toString();
    m_gemma->m_model_type = settings.value("ModelType", "2b-it").toString();

    m_gemma->m_config.max_tokens = settings.value("MaxTokens", 3072).toInt();
    m_gemma->m_config.max_generated_tokens = settings.value("MaxGeneratedTokens", 2048).toInt();
    m_gemma->m_config.temperature = settings.value("Temperature", 1.0).toFloat();
    m_gemma->m_config.verbosity = settings.value("Verbosity", 2).toInt();

    m_ctags = settings.value("ctags").toString();
    m_timer_ms = settings.value("timer", 10000).toInt();
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
        startThread();
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
                startThread();
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
                startThread();
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
    QDir dir;
    QString path = dir.homePath() + "/.Gemma.QT/";
    dir.mkdir(path);

    QFile f(path + "tmp.md");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(m_content.text().toUtf8());
        f.close();
    }
}

void MainWindow::onAbout()
{
    std::stringstream txt;
    txt << ""
        << "Commit Version                : " << COMMIT_NAME << "\n"
        << "\n"
        << "Prefill Token Batch Size      : " << gcpp::kPrefillBatchSize
        << "\n"
        << "Hardware concurrency          : "
        << std::thread::hardware_concurrency() << std::endl
        << "Instruction set               : "
        << hwy::TargetName(hwy::DispatchedTarget()) << " ("
        << hwy::VectorBytes() * 8 << " bits)"
        << "\n"
        << "Weight Type                   : "
        << gcpp::TypeName(gcpp::WeightT()) << "\n"
        << "EmbedderInput Type            : "
        << gcpp::TypeName(gcpp::EmbedderInputT()) << "\n"
        << "\n"
        << "Gemma.qt : https://github.com/zeerd/gemma.qt\n"
           "[Apache2.0/BSD3]Gemma.cpp : https://github.com/google/gemma.cpp\n"
           "[MIT]marked.js : https://github.com/chjj/marked\n"
           "[Apache2.0]Markdown.css : https://kevinburke.bitbucket.io/markdowncss/\n"
           "[BSD]MarkdownEditor : https://doc.qt.io/qt-5/"
               "qtwebengine-webenginewidgets-markdowneditor-example.html\n";

    QMessageBox msgBox;
    msgBox.setStyleSheet("QDialog { font: 8pt Consolas; }");
    msgBox.setText(txt.str().c_str());
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

void MainWindow::startThread()
{
    m_gemma->m_break = false;
    ui->send->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
    ui->progress->setValue(1);

    if(m_session_name == "") {
        m_session_name = QDateTime::currentDateTime()
                        .toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
        ui->listSessions->appendRow(m_session_name);
    }
    m_gemma->start();
}

void MainWindow::on_send_clicked()
{
    if(m_gemma->isRunning()) {
        m_gemma->m_break = true;
    }
    else {
        QString text = ui->prompt->toPlainText();
        // text.replace('\n', ' ');
        m_gemma->setPrompt(text.toStdString());
        startThread();

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
    std::string name = QDateTime::currentDateTime()
                        .toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
    ui->listSessions->appendRow(name);
}
