#include "parsefile.h"
#include "mainwindow.h"
#include "gemmathread.h"

#include <iostream>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>
#include <QProcess>

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
                parseEachFunc(ctags(), path);
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

QString ParseFile::getFuncBody(QFile *f, int sl, int el, bool bracket)
{
    QTextStream in(f);
    f->seek(0);

    QString line;
    int lineNumber = 0;
    while (!in.atEnd()) {
        line = in.readLine();
        lineNumber++;
        if ((lineNumber + 1) == sl) {
            break;
        }
    }
    QString lines = "";
    while (!in.atEnd()) {
        line = in.readLine();
        lines += line + "\n";
        lineNumber++;
        if (lineNumber == el) {
            break;
        }
    }

    if(bracket) {
        int i, count = 0;
        bool found = false;
        for (i = 0; i < lines.size(); ++i) {
          if (lines[i] == '{') {
            ++count;
            found = true;
          }
          else if (lines[i] == '}') {
            --count;
          }
          if(found && count == 0) {
            break;
          }
        }
        lines = lines.mid(0, i + 1);
    }

    return lines;
}

void ParseFile::parseEachFunc(QString ctags, QString path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {

        auto getExtension = [](const QString& fileName) -> QString {
            int lastDotIndex = fileName.lastIndexOf(".");
            return lastDotIndex == -1 ? "" : fileName.mid(lastDotIndex + 1);
        };

        QStringList clike = {"h", "hpp", "c", "cpp", "cc"};
        QStringList pylike = {"py"};

        std::string type = "";
        QString ext = getExtension(path);
        QStringList arguments;
        if(clike.contains(ext)) {
            type = "C++ ";
            arguments << "-x" << "--c++-kinds=pf" << "--language-force=c++" << path;
        }
        else if(pylike.contains(ext)) {
            type = "Python ";
            arguments << "-x" << "--python-kinds=f" << "--language-force=python" << path;
        }
        else {
            arguments << "-x" << path;
        }

        // qDebug() << ctags << arguments.join(" ");
        QProcess process(this);
        process.start(ctags, arguments);
        process.waitForFinished();

        std::vector<QStringList> data;
        QTextStream stream(&process);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList strList = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if(strList.size() > 3) {
                data.push_back(strList);
            }
        }
        sort(data.begin(), data.end(),
                    [](const QStringList& a, const QStringList& b) {
            return atoi(a[2].toStdString().c_str()) < atoi(b[2].toStdString().c_str());
        });

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
                // This prompt comes from Gemma's Readme.md
                // I tested many words and this one is the best.
                // Maybe it's about fine-tune.
                std::string pre = "What does this " + type + "code do:\n";
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
