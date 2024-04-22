#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QString>

class Document : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text MEMBER m_text NOTIFY textChanged FINAL)
public:
    explicit Document(QObject *parent = nullptr) : QObject(parent) {}

    void setText(const QString &text) {
        if (text == m_text)
            return;
        m_text = text;
        emit textChanged(m_text);
    }
    void appendText(const QString &text) {
        setText(m_text + text);
    }
    QString text() { return m_text; }

signals:
    void textChanged(const QString &text);

private:
    QString m_text;
};

#endif // DOCUMENT_H
