find_package(PkgConfig)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -std=gnu++17 -g -O0")
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

################################################################################

find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
add_definitions(-DMAGICKCORE_HDRI_ENABLE=1)
add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16)

################################################################################

ADD_LIBRARY(GemmaCore STATIC
    "gemmathread.cpp"
    "run.cpp"
    "setting.cpp"
)

include_directories(
    ${ImageMagick_Magick++_INCLUDE_DIR}
    ${ImageMagick_Magick++_ARCH_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(
   GemmaCore
    pthread
    libgemma
    hwy hwy_contrib
    sentencepiece
    ixwebsocket
    ${PC_Magick++_LIBRARIES}
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
