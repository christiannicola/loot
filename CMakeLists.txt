cmake_minimum_required(VERSION 3.10)

project(loot VERSION 0.0)

set(CMAKE_CXX_STANDARD 98)

option(ENABLE_ASSERTS "Enable debug asserts" ON)

add_library(game SHARED loot.cpp)

if (ENABLE_ASSERTS)
    add_compile_options("-DENABLE_ASSERTS=1")
endif(ENABLE_ASSERTS)

if (MSVC OR MSYS OR MINGW)
    add_compile_options("/W3")

    add_executable(${PROJECT_NAME} WIN32 win32_platform.cpp)

    target_link_libraries(${PROJECT_NAME} user32.lib gdi32.lib winmm.lib)
endif()

if (UNIX AND NOT APPLE)
    add_compile_options("-g" "-ffast-math" "-Wall")

    add_executable(${PROJECT_NAME} linux_platform.cpp)

    target_link_libraries(${PROJECT_NAME}_linux pthread X11 dl GL)
endif()

if (APPLE)
    message(SEND_ERROR "osx is currently not supported")
endif()


