#ifndef PARSEFUNCTION_H
#define PARSEFUNCTION_H

#include <QDialog>

#include "ui_parsefunction.h"
#include "parsetext.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ParseFile;
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

class MainWindow;
class ParseFunction : public QDialog, public ParseText
{
    Q_OBJECT

public:
    explicit ParseFunction(QWidget *parent = nullptr);
    ~ParseFunction();

    bool parse();

private slots:
    void on_button_OK_clicked();
    void on_button_Cancel_clicked();
    void on_button_Browse_clicked();
    void on_button_Load_clicked();

private:
    MainWindow *m_mainWindow;
    Ui::ParseFunction *ui = nullptr;
};

#endif // PARSEFUNCTION_H
