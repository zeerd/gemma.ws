#include "mainwindow.h"
#include "gemmathread.h"
#include "ui_mainwindow.h"

#include "setting.h"
#include "ui_setting.h"

#include <filesystem>

#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QSettings>
#include <QStyle>

GemmaThread* MainWindow::m_gemma = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    setWindowState(Qt::WindowMaximized);

    ui->setupUi(this);
    ui->progress->setFormat("Parsing Progress: %p%");

    QRect viewGeometry(QPoint(0, 0), size());
    QWebEnginePage *page = new QWebEnginePage(this);
    ui->webView->setContextMenuPolicy(Qt::NoContextMenu);
    ui->webView->setPage(page);
    ui->webView->setGeometry(viewGeometry);
    ui->webView->setUrl(QUrl("qrc:/index.html"));

    m_gemma = new GemmaThread(this);
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
    welcome += "\n";

    m_content.setText(welcome);
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(m_channel);

    connect(ui->actionSetting, &QAction::triggered, this, &MainWindow::onSetting);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

    QShortcut *shortcut = new QShortcut(Qt::Key_Return, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_send_clicked);

    m_gemma->gemmaInit();
}

MainWindow::~MainWindow()
{
    m_gemma->gemmaUninit();
    delete m_gemma;
    delete m_channel;
    delete ui;
}

void MainWindow::saveConfig()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("Setting");
    settings.setValue("Weight", m_gemma->m_fileWeight);
    settings.setValue("Tokenizer", m_gemma->m_fileTokenizer);
    settings.endGroup();
}

void MainWindow::readConfig()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("Setting");
    m_gemma->m_fileWeight = settings.value("Weight").toString();
    m_gemma->m_fileTokenizer = settings.value("Tokenizer").toString();
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
    Setting s(this);
    s.exec();
    m_gemma->gemmaInit();
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

void MainWindow::onAbout()
{
    std::stringstream txt;
    txt << "Prefill Token Batch Size      : " << gcpp::kPrefillBatchSize
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
    text.replace("\n", "\n\n");
    m_content.appendText(text);
}

void MainWindow::on_doGemmaFinished()
{
    m_content.appendText("\n\n---\n");
    ui->progress->setValue(0);
    ui->send->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
    ui->load->setText(QCoreApplication::translate("MainWindow", "Load", nullptr));
}

void MainWindow::startThread()
{
    m_gemma->m_break = false;
    ui->send->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
    ui->load->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
    m_gemma->start();
}

void MainWindow::on_send_clicked()
{
    if(m_gemma->m_model == NULL) {
        QMessageBox::warning(this, windowTitle(), "Model had not loaded.");
    }
    else if(m_gemma->isRunning()) {
        m_gemma->m_break = true;
    }
    else {
        QString text = ui->prompt->text();
        if(text != "") {
            m_content.appendText("\n\n**" + text + "**\n\n");
        }
        startThread();

        ui->prompt->selectAll();
    }
}

void MainWindow::on_load_clicked()
{
    if(m_gemma->m_model == NULL) {
        QMessageBox::warning(this, windowTitle(), "Model had not loaded.");
    }
    else if(m_gemma->isRunning()) {
        m_gemma->m_break = true;
    }
    else {
        QString path = QFileDialog::getOpenFileName(this,
            tr("Open PlainText File"), "", tr("PlainText File (*.*)"));
        if (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QByteArray lines = f.readAll();
                lines.replace('\n', ' ');

                m_content.appendText(
                     "\n\nSummarize Content of " + path + " : " + "\n\n");

                std::string pre = "What does this texts do ";
                m_gemma->setPrompt(pre + lines.toStdString());
                startThread();
            }
            else {
                QMessageBox::warning(this, windowTitle(),
                                tr("Could not open file %1: %2").arg(
                                QDir::toNativeSeparators(path), f.errorString()));
            }
        }
    }
}

void MainWindow::on_reset_clicked()
{
    m_gemma->m_abs_pos = 0;
    m_content.setText("");
}
