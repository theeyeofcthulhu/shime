cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project(shime C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wshadow -pedantic")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "*.c")

set(BUILD_SOUND_PATH "\"${CMAKE_INSTALL_PREFIX}/Bell, Counter, A.wav\"")

find_package(Curses REQUIRED)
find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(NOTIFY libnotify)

add_executable(shime ${SOURCES})

target_include_directories(shime PRIVATE "${CURSES_INCLUDE_DIRS}" "${SDL2_INCLUDE_DIRS}")
target_link_libraries(shime "${SDL2_LIBRARIES}" "${CURSES_LIBRARIES}")

if(NOTIFY_FOUND)
    find_path(NOTIFY_INC notify.h
        PATHS
        HINTS ${PKG_LIBNOTIFY_INCLUDE_DIRS}
        PATH_SUFFIXES libnotify
    )

    target_include_directories(shime PRIVATE "${NOTIFY_INC}" "${NOTIFY_INCLUDE_DIRS}")
    target_link_libraries(shime "${NOTIFY_LDFLAGS}")

    set(USE_NOTIFY 1)
endif()

configure_file (
    "config.h.in"
    "config.h"
)

target_include_directories(shime PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
