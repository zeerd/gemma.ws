#include "parsefile.h"
#include "mainwindow.h"
#include "gemmathread.h"


#include <QtWidgets>

ParseFile::ParseFile(QWidget *parent)
    : QDialog(parent),
      m_mainWindow((MainWindow*)parent),
      ui(new Ui::ParseFile)
{
    ui->setupUi(this);

    ui->radioFile->setChecked(true);
    connect(ui->radioFile, &QRadioButton::clicked, this, &ParseFile::on_radioFile_clicked);
    connect(ui->radioFunction, &QRadioButton::clicked, this, &ParseFile::on_radioFunction_clicked);

    ui->checkWriteBack->setToolTip("Write Function's Summarization back to file as comment(dangers).");
    ui->checkBracket->setToolTip("Use '{}' to define the scope of the function body.");

    on_radioFile_clicked();
}

ParseFile::~ParseFile()
{
    delete ui;
}

QString ParseFile::ctags()
{
    return m_mainWindow->m_ctags;
}

void ParseFile::on_button_Browse_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open PlainText File"), "", tr("PlainText File (*.*)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_FolderFile->setText(path);
        if(isCLike(path)) {
            ui->checkBracket->setChecked(true);
        }
    }
}

void ParseFile::on_radioFile_clicked()
{
    ui->checkWriteBack->setEnabled(false);
    ui->checkBracket->setEnabled(false);
}

void ParseFile::on_radioFunction_clicked()
{
    // ui->checkWriteBack->setEnabled(true);
    ui->checkBracket->setEnabled(true);
}

bool ParseFile::parse()
{
    bool ret = false;
    QString path = getPath();
    if (!path.isEmpty()) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_mainWindow->m_content.appendText(
                "\n\nSummarize Content of " + path + " : " + "\n\n");
            QByteArray lines = f.readAll();
            if(isFile()) {
                // lines.replace('\n', ' ');
                std::string pre = "What does this text do :";
                m_mainWindow->m_gemma->setPrompt(pre + lines.toStdString());
            }
            else {
                parseEachFunc(path);
            }
            ret = true;
        }
        else {
            QMessageBox::warning(this, windowTitle(),
                            tr("Could not open file %1: %2").arg(
                            QDir::toNativeSeparators(path), f.errorString()));
        }
    }
    return ret;
}

void ParseFile::parseEachFunc(QString path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {

        std::vector<QStringList> data;
        int funcCount = 0;
        getSymbolList(m_mainWindow->m_ctags, path, data, funcCount);

        bool bracket = ui->checkBracket->isChecked();
        for (auto it = data.begin(); it != data.end(); ++it) {
            // Access the current element
            if((*it)[1] == "function") {
                int sl = (*it)[2].toInt();
                int el = -1;
                if (std::next(it) != data.end()) {
                    el = (*(std::next(it)))[2].toInt() - 1;
                }
                QString funcline = getFuncBody(&f, sl, el, bracket);
                // This prompt comes from Gemma's README.md
                // I tested many words and this one is the best.
                // Maybe it's about fine-tune.
                std::string pre = "What does this " + m_type + "code do:\n";
                m_mainWindow->m_gemma->appendPrompt(pre + funcline.toStdString());
                // qDebug() << pre.c_str() << funcline.toStdString().c_str();
            }
        }
    }
}

void ParseFile::on_button_OK_clicked()
{
    accept();
}

void ParseFile::on_button_Cancel_clicked()
{
    reject();
}
