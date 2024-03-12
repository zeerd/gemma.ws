#include "gemmathread.h"
#include "ui_mainwindow.h"

#include "setting.h"
#include "ui_setting.h"

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

Setting::Setting(QWidget *parent)
    : QDialog(parent),
      m_mainWindow((MainWindow*)parent),
      ui(new Ui::Setting)
{
    ui->setupUi(this);

    ui->edit_Weights->setText(m_mainWindow->m_gemma->m_fileWeight);
    ui->edit_Tokenizer->setText(m_mainWindow->m_gemma->m_fileTokenizer);
}

Setting::~Setting()
{
    delete ui;
}

void Setting::on_load_Weights_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Weight File"), "", tr("Weight File (*.sbs)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_Weights->setText(path);

        m_mainWindow->m_gemma->m_fileWeight = path;
        m_mainWindow->m_content.appendText("\n**Weight file**\n- " + path + " loaded.\n");
    }
}

void Setting::on_load_Tokenizer_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Tokenizer File"), "", tr("Tokenizer File (*.spm)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_Tokenizer->setText(path);

        m_mainWindow->m_gemma->m_fileTokenizer = path;
        m_mainWindow->m_content.appendText("\n**Tokenizer file**\n- " + path + " loaded.\n");
    }
}
