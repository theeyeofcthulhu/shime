cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project(shime C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wshadow -pedantic")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "*.c")

set(SOUND "Bell, Counter, A.wav")
set(SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share")

set(BUILD_SOUND_PATH "\"${SHARE_PATH}/${SOUND}\"")

find_package(Curses REQUIRED)

find_package(SDL2)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_search_module(NOTIFY libnotify)
endif()

add_executable(shime ${SOURCES})

target_include_directories(shime PRIVATE "${CURSES_INCLUDE_DIRS}")
target_link_libraries(shime "${CURSES_LIBRARIES}")

if (SDL2_FOUND)
    message("Using sdl2")

    target_include_directories(shime PRIVATE "${SDL2_INCLUDE_DIRS}")
    target_link_libraries(shime "${SDL2_LIBRARIES}")

    set(USE_SDL2 1)
else()
    message("Not using sdl2")
endif()

if(PkgConfig_FOUND)
    if(NOTIFY_FOUND)
        message("Using libnotify")

        target_include_directories(shime PRIVATE "${NOTIFY_INCLUDE_DIRS}")
        target_link_libraries(shime "${NOTIFY_LDFLAGS}")

        set(USE_NOTIFY 1)
    else()
        message("Not using libnotify")
    endif()
else()
    message("Not using pkg-config to locate libnotify")
endif()

configure_file (
    "config.h.in"
    "config.h"
)

# config.h is going to be here
target_include_directories(shime PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS shime)
install(FILES "${SOUND}" DESTINATION "${SHARE_PATH}")
