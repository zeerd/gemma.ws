#ifndef PARSEFILE_H
#define PARSEFILE_H

#include <QDialog>
#include <QFile>

#include "ui_parsefile.h"
#include "parsetext.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ParseFile;
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

class MainWindow;
class ParseFile : public QDialog, public ParseText
{
    Q_OBJECT

public:
    explicit ParseFile(QWidget *parent = nullptr);
    ~ParseFile();

    bool parse();

private slots:
    void on_button_OK_clicked();
    void on_button_Cancel_clicked();
    void on_button_Browse_clicked();
    void on_radioFile_clicked();
    void on_radioFunction_clicked();

private:
    void parseEachFunc(QString path);
    QString getPath() { return ui->edit_FolderFile->text(); }
    bool isFile() { return ui->radioFile->isChecked(); }
    QString ctags();

private:
    MainWindow *m_mainWindow;
    Ui::ParseFile *ui = nullptr;
};

#endif // PARSEFILE_H
