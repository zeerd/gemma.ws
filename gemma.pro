TEMPLATE = app

QT += webenginewidgets
QT += webchannel

HEADERS += \
    mainwindow.h \
    document.h \
    core/gemmathread.h \
    core/websocket.h \
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
    core/gemmathread.cpp \
    core/websocket.cpp \
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

INCLUDEPATH += core/
INCLUDEPATH += dialogs/
INCLUDEPATH += core/gemma.cpp/
INCLUDEPATH += core/gemma.cpp/build/_deps/highway-src
INCLUDEPATH += core/gemma.cpp/build/_deps/sentencepiece-src/
INCLUDEPATH += core/json/include
INCLUDEPATH += core/IXWebSocket

LIBS += -Lcore/gemma.cpp/build/
LIBS += -Lcore/gemma.cpp/build/_deps/highway-build
LIBS += -lgemma -lhwy -lhwy_contrib
LIBS += core/gemma.cpp/build/_deps/sentencepiece-build/src/libsentencepiece.a
LIBS += core/IXWebSocket/build/libixwebsocket.a -lz

FORMS += \
    mainwindow.ui \
    dialogs/setting.ui \
    dialogs/parsefile.ui \
    dialogs/parsefunction.ui

DISTFILES += \
    resources/3rdparty/MARKDOWN-LICENSE.txt \
    resources/3rdparty/MARKED-LICENSE.txt

defineReplace(GetCommitID) {
    COMMITID=$$system(git -c core.fileMode=false describe --always)
    return ($$COMMITID)
}
COMMIT_NAME = $$GetCommitID()
DEFINES += COMMIT_NAME=\\\"$$COMMIT_NAME\\\"
