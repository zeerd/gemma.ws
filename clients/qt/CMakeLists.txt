
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5WebChannel REQUIRED)
find_package(Qt5WebSockets REQUIRED)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

include_directories(
    .
    dialogs
    ${PROJECT_SOURCE_DIR}/server
    ${PROJECT_BINARY_DIR}/clients/qt
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -std=gnu++17 -g -O0")

file(GLOB SRC
    "*.cpp"
    "*.ui"
    "dialogs/*.cpp"
    "dialogs/*.ui"
)

file(GLOB UI_INCs
    "*.h"
    "dialogs/*.h"
)

file(GLOB UI_SRC
    "*.ui"
    "dialogs/*.ui"
)

file(GLOB QRC_SRC
    "resources/*.qrc"
)


qt_wrap_ui(UI_INC ${UI_SRC})
qt_wrap_cpp(UI_MOC ${UI_INCs})
qt_add_resources(QRC_SRC ${QRC_SRC})

ADD_EXECUTABLE (
    Gemma.QT
    ${SRC}
    ${UI_MOC}
    ${QRC_SRC}
)

TARGET_LINK_LIBRARIES(
    Gemma.QT
    GemmaCore
    Qt5::WebEngineWidgets
    Qt5::WebChannel
    Qt5::WebSockets
    pthread
    libgemma
    hwy hwy_contrib
    sentencepiece
    ixwebsocket
)
