#ifndef ABOUT_H
#define ABOUT_H

#include <sstream>
#include <QMessageBox>

class AboutBox : public QMessageBox {
public:
    AboutBox() {
        std::stringstream txt;
        txt << ""
            << "Commit Version                : " << COMMIT_NAME << "\n"
            << "\n"
            << "Prefill Token Batch Size      : " << gcpp::kPrefillBatchSize
            << "\n"
            << "Hardware concurrency          : "
            << std::thread::hardware_concurrency() << std::endl
            << "Instruction set               : "
            << hwy::TargetName(hwy::DispatchedTarget()) << " ("
            << hwy::VectorBytes() * 8 << " bits)"
            << "\n"
            << "Weight Type                   : "
            << gcpp::TypeName(gcpp::GemmaWeightT()) << "\n"
            << "EmbedderInput Type            : "
            << gcpp::TypeName(gcpp::EmbedderInputT()) << "\n"
            << "\n"
            << "Gemma.qt : https://github.com/zeerd/gemma.qt\n"
               "[Apache2.0/BSD3]Gemma.cpp : https://github.com/google/gemma.cpp\n"
               "[MIT]marked.js : https://github.com/chjj/marked\n"
               "[Apache2.0]Markdown.css : https://kevinburke.bitbucket.io/markdowncss/\n"
               "[BSD]MarkdownEditor : https://doc.qt.io/qt-5/"
                   "qtwebengine-webenginewidgets-markdowneditor-example.html\n"
               "[MIT]json : https://github.com/nlohmann/json.git\n"
               "[BSD]IXWebSocket : https://github.com/machinezone/IXWebSocket\n"
               ;

        setStyleSheet("QDialog { font: 8pt Consolas; }");
        setText(txt.str().c_str());
    }
};

#endif /* ABOUT_H */
