cmake_minimum_required(VERSION 3.10)

project(loot VERSION 0.0)

option(ENABLE_ASSERTS "Enable debug asserts" ON)

if (ENABLE_ASSERTS)
    add_compile_options("-DENABLE_ASSERTS=1")
endif(ENABLE_ASSERTS)

if (MSVC OR MSYS OR MINGW)
    set(LIBRARY_NAME ${PROJECT_NAME}.dll)

    add_compile_options("-g" "-ffast-math")

    add_library(${LIBRARY_NAME} SHARED loot.cpp)

    add_executable(${PROJECT_NAME}.exe win32.cpp)

    target_link_libraries(${PROJECT_NAME} user32.lib gdi32.lib winm.lib opengl32.lib)
endif()

if (UNIX AND NOT APPLE)
    add_compile_options("-g" "-ffast-math")

    add_library(${PROJECT_NAME} SHARED loot.cpp)

    add_executable(${PROJECT_NAME}_linux linux.cpp)

    target_link_libraries(${PROJECT_NAME}_linux pthread X11 dl GL)
endif()

if (APPLE)
    message(SEND_ERROR "osx is currently not supported")
endif()


