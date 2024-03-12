#ifndef SETTING_H
#define SETTING_H

#include <QDialog>
#include "mainwindow.h"

QT_BEGIN_NAMESPACE

namespace Ui {

class Setting;

}

QT_END_NAMESPACE

QT_USE_NAMESPACE

class Setting : public QDialog
{
    Q_OBJECT

public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();

private slots:
    void on_load_Weights_clicked();
    void on_load_Tokenizer_clicked();

private:
    MainWindow *m_mainWindow;
    Ui::Setting *ui = nullptr;
};

#endif // SETTING_H
