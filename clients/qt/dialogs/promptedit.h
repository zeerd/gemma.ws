#ifndef PROMPTEDIT_H
#define PROMPTEDIT_H

#include <QTextEdit>

class MainWindow;
class PromptEdit: public QTextEdit {
    Q_OBJECT

public:
    explicit PromptEdit(QWidget *parent = nullptr);
    void setMainWindow(MainWindow *win) { m_mainWindow = win; }

protected:
    void keyPressEvent(QKeyEvent *event) override;

// signals:
//     void AltEnterPressed();

private:
    MainWindow *m_mainWindow;
};

#endif // PROMPTEDIT_H
