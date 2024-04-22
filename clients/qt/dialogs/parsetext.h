#ifndef PARSETEXT_H
#define PARSETEXT_H

#include <QFile>
#include <QString>
#include <QStringList>

#include <vector>

class MainWindow;
class ParseText {
public:
    ParseText() {}

protected:
    QString getExtension(QString path);
    bool isCLike(QString path);
    bool isPythonLike(QString path);

    QString getFuncBody(QFile *f, int sl, int el, bool bracket);
    bool getSymbolList(QString ctags, QString path, std::vector<QStringList> &data, int &funcCount);

protected:
    std::string m_type;
};

#endif // PARSETEXT_H
