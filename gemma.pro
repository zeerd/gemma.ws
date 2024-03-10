TEMPLATE = app

QT += webenginewidgets
QT += webchannel

HEADERS += \
    mainwindow.h \
    document.h

SOURCES = \
    main.cpp \
    mainwindow.cpp \
    gemmamain.cpp

RESOURCES = \
    resources/gemma.qrc

# Disable Qt Quick compiler because the example doesn't use QML, but more importantly so that
# the source code of the .js files is not removed from the embedded qrc file.
CONFIG -= qtquickcompiler
CONFIG += c++17

INCLUDEPATH += /usr/local/Qt-5.15.6/include/QtWidgets/
INCLUDEPATH += gemma.cpp/
INCLUDEPATH += gemma.cpp/build/_deps/highway-src
INCLUDEPATH += gemma.cpp/build/_deps/sentencepiece-src/

LIBS+=-Lgemma.cpp/build/
LIBS+=-Lgemma.cpp/build/_deps/highway-build
LIBS+=-lgemma -lhwy -lhwy_contrib

LIBS+=gemma.cpp/build/_deps/sentencepiece-build/src/libsentencepiece.a

FORMS += \
    mainwindow.ui

DISTFILES += \
    resources/3rdparty/MARKDOWN-LICENSE.txt \
    resources/3rdparty/MARKED-LICENSE.txt

# install
# target.path = $$[QT_INSTALL_EXAMPLES]/webenginewidgets/gemma
# INSTALLS += target
