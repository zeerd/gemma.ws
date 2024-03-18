#include "gemmathread.h"
#include "mainwindow.h"

#include "setting.h"

#include <QtWidgets>

Setting::Setting(QWidget *parent)
    : QDialog(parent),
      m_mainWindow((MainWindow*)parent),
      ui(new Ui::Setting)
{
    ui->setupUi(this);

    ui->edit_Weights->setText(m_mainWindow->m_gemma->m_fileWeight);
    ui->edit_Tokenizer->setText(m_mainWindow->m_gemma->m_fileTokenizer);

    QString max_tokens;
    max_tokens.setNum(m_mainWindow->m_gemma->m_config.max_tokens);
    ui->edit_MaxTokens->setText(max_tokens);
    QString max_generated_tokens;
    max_generated_tokens.setNum(m_mainWindow->m_gemma->m_config.max_generated_tokens);
    ui->edit_MaxGeneratedTokens->setText(max_generated_tokens);

    QString timer;
    timer.setNum(m_mainWindow->m_timer_ms);
    ui->edit_AutoSave->setText(timer);

    QString ctags = m_mainWindow->m_ctags;
    if(ctags == "" && QFile::exists("/usr/bin/ctags")) {
        ctags = "/usr/bin/ctags";
    }
    ui->edit_ctags->setText(ctags);

    ui->comboModelType->addItem("2b-it");
    ui->comboModelType->addItem("7b-it");
    ui->comboModelType->addItem("2b-pt");
    ui->comboModelType->addItem("7b-pt");

    int index = ui->comboModelType->findText(m_mainWindow->m_gemma->m_model_type);
    if (index != -1) {
        ui->comboModelType->setCurrentIndex(index);
    }

    connect(ui->comboModelType, &QComboBox::currentTextChanged, this, &Setting::onCurrentTextChanged);
}

Setting::~Setting()
{
    delete ui;
}

void Setting::onCurrentTextChanged(const QString &text)
{
    m_mainWindow->m_gemma->m_model_type = text;
}

void Setting::on_button_OK_clicked()
{
    QSettings settings("Gemma.QT", "Setting");
    settings.beginGroup("Setting");

    settings.setValue("Weight", ui->edit_Weights->text());
    settings.setValue("Tokenizer", ui->edit_Tokenizer->text());
    settings.setValue("ModelType", ui->comboModelType->currentText());

    settings.setValue("MaxTokens", ui->edit_MaxTokens->text());
    settings.setValue("MaxGeneratedTokens", ui->edit_MaxGeneratedTokens->text());

    settings.setValue("ctags", ui->edit_ctags->text());

    settings.endGroup();
    accept();
}

void Setting::on_button_Cancel_clicked()
{
    reject();
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

void Setting::on_button_BrowseCtags_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Point to ctags program"), "", tr("Ctags (ctags)"));
    if (m_mainWindow->loadFile(path)) {
        ui->edit_ctags->setText(path);
    }
}
