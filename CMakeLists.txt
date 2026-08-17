cmake_minimum_required(VERSION 3.20)

project(FlooFIX
    VERSION 0.1.0
    DESCRIPTION "Low-latency C++20 FIX protocol engine"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Language
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Project options
# ---------------------------------------------------------------------------

option(FLOOFIX_BUILD_TESTS "Build FlooFIX tests" ON)
option(FLOOFIX_BUILD_BENCHMARKS "Build FlooFIX benchmarks" OFF)
option(FLOOFIX_ENABLE_SANITIZERS "Enable sanitizers" OFF)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------

add_library(floofix
    src/parser.cpp
    src/tokenizer.cpp
    src/validator.cpp
    src/ingestion.cpp
    src/fix_engine.cpp
)

target_include_directories(floofix
    PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
)

target_compile_features(floofix
    PUBLIC
        cxx_std_20
)

# ---------------------------------------------------------------------------
# Warnings
# ---------------------------------------------------------------------------

if(MSVC)

    target_compile_options(floofix PRIVATE
        /W4
        /permissive-
    )

else()

    target_compile_options(floofix PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wold-style-cast
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
    )

endif()

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------

add_executable(floofix_demo
    main.cpp
)

target_link_libraries(floofix_demo
    PRIVATE
        floofix
)

# ---------------------------------------------------------------------------
# Release configuration
# ---------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Build type"
        FORCE
    )
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug
    Release
    RelWithDebInfo
    MinSizeRel
)

# ---------------------------------------------------------------------------
# Sanitizers
# ---------------------------------------------------------------------------

if(FLOOFIX_ENABLE_SANITIZERS)

    if(MSVC)

        message(WARNING
            "Sanitizers are not configured for MSVC in this project."
        )

    else()

        target_compile_options(floofix PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(floofix PRIVATE
            -fsanitize=address,undefined
        )

        target_compile_options(floofix_demo PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(floofix_demo PRIVATE
            -fsanitize=address,undefined
        )

    endif()

endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(FLOOFIX_BUILD_TESTS)

    enable_testing()

    add_subdirectory(tests)

endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

if(FLOOFIX_BUILD_BENCHMARKS)

    add_subdirectory(benchmarks)

endif()