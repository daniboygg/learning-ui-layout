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
target_compile_options(learning_ui_layout PRIVATE -Wall -Wextra -Werror=switch)
target_link_libraries(learning_ui_layout raylib)

add_custom_command(TARGET learning_ui_layout POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:learning_ui_layout>/assets
)
