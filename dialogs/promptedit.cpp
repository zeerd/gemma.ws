#include "mainwindow.h"
#include "promptedit.h"

#include <QKeyEvent>

PromptEdit::PromptEdit(QWidget *parent)
    : QTextEdit(parent)
{
    QString stylesheet =
      "QTextEdit:placeholder {"  // Define the placeholder style
      "  color: gray;"
      "  font-style: italic;"
      "  opacity: 0.5;"  // Set transparency for hint text
      "}";
    setStyleSheet(stylesheet);
    setPlaceholderText("Press 'Enter' to send, 'Alt+Enter' to wrap.");
}

void PromptEdit::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return) {
        // Check if Alt key is also pressed
        if (event->modifiers() & Qt::AltModifier) {
            // Emit a custom signal or perform your desired action for Alt+Enter
            // emit AltEnterPressed();
            insertPlainText("\n");
            // You can also consume the event to prevent default Enter behavior
            event->accept();
        } else {
            // Default behavior for Enter (optional, can be removed)
            // ...
            m_mainWindow->ui->send->click();
        }
    } else {
        // Pass through other key presses for normal functionality
        QTextEdit::keyPressEvent(event);
    }
}
