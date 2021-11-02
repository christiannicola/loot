cmake_minimum_required(VERSION 3.10)

project(loot VERSION 0.0)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if (MSVC OR MSYS OR MINGW)
    add_executable(loot.exe win32_loot.cpp)
endif()

if (UNIX AND NOT APPLE)
    add_executable(loot linux_loot.cpp)
endif()

if (APPLE)
    message(SEND_ERROR "osx is currently not supported")
endif()
