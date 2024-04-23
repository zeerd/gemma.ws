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
    pthread
    libgemma
    hwy hwy_contrib
    sentencepiece
    ixwebsocket
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
