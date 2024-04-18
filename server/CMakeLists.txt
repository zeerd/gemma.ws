set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -std=gnu++17 -g -O0")
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

################################################################################

ADD_LIBRARY(GemmaCore STATIC
    "gemmathread.cpp"
    "setting.cpp"
)

TARGET_LINK_LIBRARIES(
   GemmaCore
    -pthread
   -L${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/build/
   -L${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/build/_deps/highway-build
   -lgemma -lhwy -lhwy_contrib
   -L${PROJECT_SOURCE_DIR}/3rdparty/gemma.cpp/build/_deps/sentencepiece-build/src/
    -lsentencepiece
   -L${PROJECT_SOURCE_DIR}/build/3rdparty/IXWebSocket/
    -lixwebsocket -lz
)

################################################################################

ADD_EXECUTABLE (
    ServerGemma
    "ServerGemma.cpp"
)

TARGET_LINK_LIBRARIES(
    ServerGemma
    GemmaCore
)
