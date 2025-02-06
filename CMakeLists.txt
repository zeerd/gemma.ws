cmake_minimum_required (VERSION 3.3)
project(Gemma.QT)
find_package(PkgConfig)
set(CMAKE_VERBOSE_MAKEFILE ON)

execute_process(COMMAND git -c core.fileMode=false describe --always --dirty
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE COMMIT_NAME)
string(REPLACE "\n" "" COMMIT_NAME ${COMMIT_NAME})
ADD_DEFINITIONS( -DCOMMIT_NAME="${COMMIT_NAME}")

include_directories(
    ${PROJECT_SOURCE_DIR}/build/_deps/highway-src
    ${PROJECT_SOURCE_DIR}/build/_deps/sentencepiece-src/
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/gemma
    ${PROJECT_SOURCE_DIR}/3rdparty/json/include
    ${PROJECT_SOURCE_DIR}/3rdparty/IXWebSocket
)

ADD_SUBDIRECTORY(3rdparty/IXWebSocket)
ADD_SUBDIRECTORY(3rdparty/gemma.cpp)
ADD_SUBDIRECTORY(server)
ADD_SUBDIRECTORY(clients/qt)
