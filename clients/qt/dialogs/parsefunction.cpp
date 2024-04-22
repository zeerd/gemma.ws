#include "parsefunction.h"
#include "mainwindow.h"
#include "gemmathread.h"

#include <QtWidgets>

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
        if(isCLike(path)) {
            ui->checkBracket->setChecked(true);
        }
    }
}

void ParseFunction::on_button_Load_clicked()
{
    QString path = ui->edit_FolderFile->text();
    if (!path.isEmpty()) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::vector<QStringList> data;
            int funcCount = 0;
            getSymbolList(m_mainWindow->m_ctags, path, data, funcCount);

            QStringList headerLabels;
            headerLabels << "" << "Start" << "End" << "Name" << "Full Content";
            ui->tableFunctions->setHorizontalHeaderLabels(headerLabels);

            ui->tableFunctions->setRowCount(funcCount);
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
                    // This prompt comes from Gemma's README.md
                    // I tested many words and this one is the best.
                    // Maybe it's about fine-tune.
                    std::string pre = "What does this " + m_type + "code do:\n";
                    if(whole != "") {
                        // qDebug() << pre.c_str() << whole.c_str();
                        m_mainWindow->m_ws->sendMessage(QString(pre.c_str()) + whole.c_str());
                        whole = "";
                    }
                    QString funcline = getFuncBody(&f, s->text().toInt(), e->text().toInt(), ui->checkBracket->isChecked());
                    // qDebug() << pre.c_str() << funcline.toStdString().c_str();
                    m_mainWindow->m_ws->sendMessage(QString(pre.c_str()) + funcline);
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
