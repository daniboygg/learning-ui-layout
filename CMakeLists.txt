cmake_minimum_required(VERSION 3.16)
project(learning_ui_layout C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(learning_ui_layout
        main.c
)
target_compile_options(learning_ui_layout PRIVATE
        -Wall
        -Wextra
        -Wshadow                 # a local variable hides another one
        -Wconversion -Wno-sign-conversion  # silent float->int, int->short loss
        -Wdouble-promotion       # float silently becomes double (raylib uses float)
        -Wvla
        -Wformat=2
        -Wundef
        -Werror=switch
        -Werror=implicit-function-declaration
        -Werror=return-type
)

# AddressSanitizer with the GCC defaults: stops at the first memory error, reports leaks at exit.
# Off by default because it slows the start of the program. Turn on with -DUSE_ASAN=ON.
option(USE_ASAN "Build with AddressSanitizer" OFF)
if(USE_ASAN)
    target_compile_options(learning_ui_layout PRIVATE -fsanitize=address -g)
    target_link_options(learning_ui_layout PRIVATE -fsanitize=address)
endif()
target_link_libraries(learning_ui_layout raylib)

add_custom_command(TARGET learning_ui_layout POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:learning_ui_layout>/assets
)
