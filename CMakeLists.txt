cmake_minimum_required(VERSION 3.18)
project(Zugzwang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/bitboard.cpp
    src/movegen.cpp
    src/position.cpp
    src/uci.cpp
    src/evaluation.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Interprocedural Optimization (LTO/FLTO)
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Performance Flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
endif()

# Includes and PCH
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)