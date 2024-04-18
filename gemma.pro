TEMPLATE = app

QT += webenginewidgets
QT += webchannel
QT += websockets

HEADERS += \
    mainwindow.h \
    document.h \
    websocketclient.h \
    dialogs/setting.h \
    dialogs/parsetext.h \
    dialogs/parsefile.h \
    dialogs/parsefunction.h \
    dialogs/promptedit.h \
    dialogs/sessionlist.h \
    dialogs/about.h

SOURCES = \
    main.cpp \
    mainwindow.cpp \
    websocketclient.cpp \
    dialogs/setting.cpp \
    dialogs/parsetext.cpp \
    dialogs/parsefile.cpp \
    dialogs/parsefunction.cpp \
    dialogs/promptedit.cpp \
    dialogs/sessionlist.cpp

RESOURCES = \
    resources/gemma.qrc

# Disable Qt Quick compiler because the example doesn't use QML, but more importantly so that
# the source code of the .js files is not removed from the embedded qrc file.
CONFIG -= qtquickcompiler

CONFIG += c++17
CONFIG += debug

# Enable ASAN to detect errors :  -fno-omit-frame-pointer
QMAKE_CXXFLAGS += "-fsanitize=address"
QMAKE_CFLAGS   += "-fsanitize=address"
QMAKE_LFLAGS   += "-fsanitize=address"

INCLUDEPATH += dialogs/

INCLUDEPATH += server/
INCLUDEPATH += 3rdparty/gemma.cpp/build/_deps/highway-src
INCLUDEPATH += 3rdparty/gemma.cpp/build/_deps/sentencepiece-src/
INCLUDEPATH += 3rdparty/gemma.cpp/
INCLUDEPATH += 3rdparty/json/include
INCLUDEPATH += 3rdparty/IXWebSocket

LIBS += build/server/libGemmaCore.a
LIBS += 3rdparty/gemma.cpp/build/libgemma.a
LIBS += 3rdparty/gemma.cpp/build/_deps/highway-build/libhwy.a
LIBS += 3rdparty/gemma.cpp/build/_deps/highway-build/libhwy_contrib.a
LIBS += 3rdparty/gemma.cpp/build/_deps/sentencepiece-build/src/libsentencepiece.a
LIBS += build/3rdparty/IXWebSocket/libixwebsocket.a -lz

FORMS += \
    mainwindow.ui \
    dialogs/setting.ui \
    dialogs/parsefile.ui \
    dialogs/parsefunction.ui

DISTFILES += \
    resources/3rdparty/MARKDOWN-LICENSE.txt \
    resources/3rdparty/MARKED-LICENSE.txt

defineReplace(GetCommitID) {
    COMMITID=$$system(git -c 3rdparty.fileMode=false describe --always --dirty)
    return ($$COMMITID)
}
COMMIT_NAME = $$GetCommitID()
DEFINES += COMMIT_NAME=\\\"$$COMMIT_NAME\\\"
