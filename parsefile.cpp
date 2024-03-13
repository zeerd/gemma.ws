#include "parsefile.h"
#include "mainwindow.h"
#include "gemmathread.h"

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
    ui->split_line1->setFrameShape(QFrame::HLine);
    ui->split_line1->setFrameShadow(QFrame::Plain);
    ui->split_line2->setFrameShape(QFrame::HLine);
    ui->split_line2->setFrameShadow(QFrame::Plain);

    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("ParseFile");
    QString ctags = settings.value("ctags").toString();
    settings.endGroup();

    if(ctags == "" && QFile::exists("/usr/bin/ctags")) {
        ctags = "/usr/bin/ctags";
    }

    ui->edit_ctags->setText(ctags);

    ui->radioFile->setChecked(true);
    connect(ui->radioFunction, &QRadioButton::clicked, this, &ParseFile::on_radioFunction_clicked);
}

ParseFile::~ParseFile()
{
    delete ui;
}

void ParseFile::on_button_Browse_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open PlainText File"), "", tr("PlainText File (*.*)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_FolderFile->setText(path);
    }
}

void ParseFile::on_button_BrowseCtags_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Point to ctags program"), "", tr("Ctags (ctags)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_ctags->setText(path);
    }
}

void ParseFile::on_radioFunction_clicked()
{

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
                lines.replace('\n', ' ');
                std::string pre = "What does this texts do ";
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

QString ParseFile::getFuncBody(QFile *f, int ln)
{
    f->seek(0);

    QTextStream in(f);
    QString line;
    int lineNumber = 0;
    while (!in.atEnd()) {
        line = in.readLine();
        lineNumber++;
        if (lineNumber == ln) {
            break;
        }
    }
    QString lines = "";
    while (!in.atEnd()) {
        line = in.readLine();
        line.replace('\n', ' ');
        lines += line;
    }

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
    return lines.mid(0, i + 1);
}

void ParseFile::parseEachFunc(QString ctags, QString path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {

        QStringList arguments;
        arguments << "-x" << "--c++-kinds=pf" << "--language-force=c++" << path;
        QProcess process(this);
        process.start(ctags, arguments);
        process.waitForFinished();

        QTextStream stream(&process);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList strList = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if(strList.size() > 3 && strList[1] == "function") {
                int ln = strList[2].toInt();

                QString funcline = getFuncBody(&f, ln - 1);
                std::string pre = "Summarize those codes without example : ";
                m_mainWindow->m_gemma->appendPrompt(
                    "Summarize Function : " + strList[0].toStdString(),
                    pre + funcline.toStdString());
                // qDebug() << pre.c_str() << funcline.toStdString().c_str();
            }
        }
    }
}

void ParseFile::on_button_OK_clicked()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("ParseFile");
    settings.setValue("ctags", ui->edit_ctags->text());
    settings.endGroup();

    accept();
}

void ParseFile::on_button_Cancel_clicked()
{
    reject();
}
