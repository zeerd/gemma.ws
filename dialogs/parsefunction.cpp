#include "parsefunction.h"
#include "mainwindow.h"
#include "gemmathread.h"

#include <iostream>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>
#include <QProcess>

ParseFunction::ParseFunction(QWidget *parent)
    : QDialog(parent),
      m_mainWindow((MainWindow*)parent),
      ui(new Ui::ParseFunction)
{
    ui->setupUi(this);

    ui->checkWhole->setToolTip("Summarize the whole file before the selected function(s).");
    ui->checkWriteBack->setToolTip("Write Function's Summarization back to file as comment(dangers).");
    ui->checkBracket->setToolTip("Use '{}' to define the scope of the function body.");

    ui->tableFunctions->setColumnCount(5);
}

ParseFunction::~ParseFunction()
{
    delete ui;
}

void ParseFunction::on_button_OK_clicked()
{
    accept();
}

void ParseFunction::on_button_Cancel_clicked()
{
    reject();
}

void ParseFunction::on_button_Browse_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open PlainText File"), "", tr("PlainText File (*.*)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_FolderFile->setText(path);
    }
}

void ParseFunction::on_button_Load_clicked()
{
    QString path = ui->edit_FolderFile->text();
    if (!path.isEmpty()) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto getExtension = [](const QString& fileName) -> QString {
                int lastDotIndex = fileName.lastIndexOf(".");
                return lastDotIndex == -1 ? "" : fileName.mid(lastDotIndex + 1);
            };

            QStringList clike = {"h", "hpp", "c", "cpp", "cc"};
            QStringList pylike = {"py"};

            m_type = "";
            QString ext = getExtension(path);
            QStringList arguments;
            if(clike.contains(ext)) {
                m_type = "C++ ";
                arguments << "-x" << "--c++-kinds=pf" << "--language-force=c++" << path;
            }
            else if(pylike.contains(ext)) {
                m_type = "Python ";
                arguments << "-x" << "--python-kinds=f" << "--language-force=python" << path;
            }
            else {
                arguments << "-x" << path;
            }

            // qDebug() << ctags << arguments.join(" ");
            QProcess process(this);
            process.start(m_mainWindow->m_ctags, arguments);
            process.waitForFinished();

            int rowCount = 0;
            std::vector<QStringList> data;
            QTextStream stream(&process);
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                QStringList strList = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
                if(strList.size() > 3) {
                    data.push_back(strList);
                    if(strList[1] == "function") {
                        rowCount++;
                    }
                }
            }
            sort(data.begin(), data.end(),
                        [](const QStringList& a, const QStringList& b) {
                return atoi(a[2].toStdString().c_str()) < atoi(b[2].toStdString().c_str());
            });

            QStringList headerLabels;
            headerLabels << "" << "Start" << "End" << "Name" << "Full Content";
            ui->tableFunctions->setHorizontalHeaderLabels(headerLabels);

            ui->tableFunctions->setRowCount(rowCount);
            int i = 0;
            for (auto it = data.begin(); it != data.end(); ++it) {
                // Access the current element
                if((*it)[1] == "function") {
                    int el = -1;
                    if (std::next(it) != data.end()) {
                        el = (*(std::next(it)))[2].toInt() - 1;
                    }
                    QString end;
                    end.setNum(el);

                    QCheckBox *checkBox = new QCheckBox();
                    ui->tableFunctions->setCellWidget(i, 0, checkBox);
                    ui->tableFunctions->setItem(i, 1, new QTableWidgetItem((*it)[2]));
                    ui->tableFunctions->setItem(i, 2, new QTableWidgetItem(end));
                    ui->tableFunctions->setItem(i, 3, new QTableWidgetItem((*it)[0]));
                    ui->tableFunctions->setItem(i, 4, new QTableWidgetItem((*it).mid(4).join(" ")));
                    i++;
                }
            }
            ui->tableFunctions->resizeColumnsToContents();
        }
        else {
            QMessageBox::warning(this, windowTitle(),
                            tr("Could not open file %1: %2").arg(
                            QDir::toNativeSeparators(path), f.errorString()));
        }
    }

}

QString ParseFunction::getFuncBody(QFile *f, int sl, int el, bool bracket)
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

bool ParseFunction::parse()
{
    bool ret = false;

    QString path = ui->edit_FolderFile->text();
    if (!path.isEmpty()) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::string whole = "";
            if(ui->checkWhole->isChecked()) {
                whole = f.readAll().toStdString();
            }

            for (int i = 0; i < ui->tableFunctions->rowCount(); i++) {
                QCheckBox *check = (QCheckBox*)ui->tableFunctions->cellWidget(i, 0);
                QTableWidgetItem *s = ui->tableFunctions->item(i, 1);
                QTableWidgetItem *e = ui->tableFunctions->item(i, 2);
                if(check->isChecked()) {
                    // This prompt comes from Gemma's Readme.md
                    // I tested many words and this one is the best.
                    // Maybe it's about fine-tune.
                    std::string pre = "What does this " + m_type + "code do:\n";
                    if(whole != "") {
                        m_mainWindow->m_gemma->appendPrompt(pre + whole);
                        // qDebug() << pre.c_str() << whole.c_str();
                        whole = "";
                    }
                    QString funcline = getFuncBody(&f, s->text().toInt(), e->text().toInt(), ui->checkBracket->isChecked());
                    m_mainWindow->m_gemma->appendPrompt(pre + funcline.toStdString());
                    // qDebug() << pre.c_str() << funcline.toStdString().c_str();
                    ret = true;
                }
            }

        }
        else {
            QMessageBox::warning(this, windowTitle(),
                            tr("Could not open file %1: %2").arg(
                            QDir::toNativeSeparators(path), f.errorString()));
        }
    }

    return ret;
}
