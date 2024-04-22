#include "parsetext.h"
#include "mainwindow.h"

#include <QProcess>

QString ParseText::getExtension(QString path)
{
    int lastDotIndex = path.lastIndexOf(".");
    return lastDotIndex == -1 ? "" : path.mid(lastDotIndex + 1);
}

bool ParseText::isCLike(QString path)
{
    QStringList exts = {"h", "hpp", "c", "cpp", "cc"};
    return exts.contains(getExtension(path));
}

bool ParseText::isPythonLike(QString path)
{
    QStringList exts = {"py"};
    return exts.contains(getExtension(path));
}

bool ParseText::getSymbolList(QString ctags, QString path,
        std::vector<QStringList> &data, int &funcCount)
{

    m_type = "";
    QStringList arguments;
    if(isCLike(path)) {
        m_type = "C++ ";
        arguments << "-x" << "--c++-kinds=pf" << "--language-force=c++" << path;
    }
    else if(isPythonLike(path)) {
        m_type = "Python ";
        arguments << "-x" << "--python-kinds=f" << "--language-force=python" << path;
    }
    else {
        arguments << "-x" << path;
    }

    // qDebug() << ctags << arguments.join(" ");
    QProcess process(NULL);
    process.start(ctags, arguments);
    process.waitForFinished();

    funcCount = 0;
    QTextStream stream(&process);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList strList = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if(strList.size() > 3) {
            data.push_back(strList);
            if(strList[1] == "function") {
                funcCount++;
            }
        }
    }
    sort(data.begin(), data.end(),
                [](const QStringList& a, const QStringList& b) {
        return a[2].toInt() < b[2].toInt();
    });

    return data.size() > 0;
}

QString ParseText::getFuncBody(QFile *f, int sl, int el, bool bracket)
{
    QTextStream in(f);
    f->seek(0);

    QString line;
    int lineNumber = 0;
    while (!in.atEnd()) {
        line = in.readLine();
        lineNumber++;
        if ((lineNumber + 1) == sl) {
            break;
        }
    }
    QString lines = "";
    while (!in.atEnd()) {
        line = in.readLine();
        lines += line + "\n";
        lineNumber++;
        if (lineNumber == el) {
            break;
        }
    }

    if(bracket) {
        int i, count = 0;
        bool found = false;
        for (i = 0; i < lines.size(); ++i) {
          if (lines[i] == '{') {
            ++count;
            found = true;
          }
          else if (lines[i] == '}') {
            --count;
          }
          if(found && count == 0) {
            break;
          }
        }
        lines = lines.mid(0, i + 1);
    }

    return lines;
}
