#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>

MyThread* MainWindow::m_thread = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_model(NULL),
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

    QString welcome = "";
    m_fileWeight = QCoreApplication::applicationDirPath() + "/2b-it.sbs";
    if(loadFile(m_fileWeight)) {
        welcome += m_fileWeight + " loaded.\n\n";
    }
    else {
        qDebug() << m_fileWeight;
        m_fileWeight = "";
        welcome += "Weight file missing.\n\n";
    }
    m_fileTokenizer = QCoreApplication::applicationDirPath() + "/tokenizer.spm";
    if(loadFile(m_fileTokenizer)) {
        welcome += m_fileTokenizer + " loaded.\n\n";
    }
    else {
        qDebug() << m_fileTokenizer;
        m_fileTokenizer = "";
        welcome += "Tokenizer file missing.\n\n";
    }
    welcome += "---\n";

    m_content.setText(welcome);
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(m_channel);

    connect(ui->actionLoadT, &QAction::triggered, this, &MainWindow::onLoadTokenizer);
    connect(ui->actionLoadW, &QAction::triggered, this, &MainWindow::onLoadWeight);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);

    QShortcut *shortcut = new QShortcut(Qt::Key_Return, this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_send_clicked);

    m_thread = new MyThread(this);
    gemmaInit();
}

MainWindow::~MainWindow()
{
    gemmaUninit();
    delete ui;
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

void MainWindow::onLoadWeight()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Weight File"), "", tr("Weight File (*.sbs)"));
    if (loadFile(path)) {
        m_fileWeight = path;
        m_content.appendText(path + " loaded.\n\n");
        gemmaInit();
    }
}

void MainWindow::onLoadTokenizer()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Tokenizer File"), "", tr("Tokenizer File (*.spm)"));
    if (loadFile(path)) {
        m_fileTokenizer = path;
        m_content.appendText(path + " loaded.\n\n");
        gemmaInit();
    }
}

void MainWindow::onSaveAs()
{
    QString filter = "Markdown File (*.md)";
    QString path = QFileDialog::getSaveFileName(this,
        tr("Open Markdown File to Save"), "", tr("Markdown File (*.md)"));
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

void MainWindow::on_doGemma(QString text)
{
    text.replace("\n", "\n\n");
    m_content.appendText(text);
}

void MainWindow::on_doGemmaFinished()
{
    m_content.appendText("\n\n---\n");
}

void MainWindow::on_send_clicked()
{
    if(m_thread->isRunning()) {
    }
    else {
        QString text = ui->prompt->text();
        if(text != "") {
            m_content.appendText("\n\n**" + text + "**\n\n");
        }
        m_thread->start();

        ui->prompt->selectAll();
    }
}

void MainWindow::on_load_clicked()
{
    if(m_thread->isRunning()) {
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
                m_thread->setPrompt(pre + lines.toStdString());
                m_thread->start();
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
    m_abs_pos = 0;
    m_content.setText("");
}
