#ifndef PARSEFUNCTION_H
#define PARSEFUNCTION_H

#include <QDialog>
#include <QFile>

#include "ui_parsefunction.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ParseFile;
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

class MainWindow;
class ParseFunction : public QDialog
{
    Q_OBJECT

public:
    explicit ParseFunction(QWidget *parent = nullptr);
    ~ParseFunction();

    bool parse();

private:
    QString getFuncBody(QFile *f, int sl, int el, bool bracket);

private slots:
    void on_button_OK_clicked();
    void on_button_Cancel_clicked();
    void on_button_Browse_clicked();
    void on_button_Load_clicked();
    // void on_button_Parse_clicked();

private:
    MainWindow *m_mainWindow;
    Ui::ParseFunction *ui = nullptr;
    std::string m_type;
};

#endif // PARSEFUNCTION_H
