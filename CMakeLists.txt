cmake_minimum_required(VERSION 3.16)
project(Zugzwang)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/bitboard.cpp
    src/movegen.cpp
    src/position.cpp
    src/uci.cpp

    src/bitboard.h
    src/movegen.h
    src/position.h
    src/uci.h
    src/types.h

    src/pch.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -march=native
        -flto
    )
endif()