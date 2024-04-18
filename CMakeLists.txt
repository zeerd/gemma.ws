cmake_minimum_required (VERSION 3.0)
project(Gemma.QT)
find_package(PkgConfig)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/build/_deps/highway-src
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/build/_deps/sentencepiece-src/
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/
    ${PROJECT_SOURCE_DIR}/3rdparty/json/include
    ${PROJECT_SOURCE_DIR}/3rdparty/IXWebSocket
)

ADD_SUBDIRECTORY(3rdparty/IXWebSocket)
ADD_SUBDIRECTORY(server)
