cmake_minimum_required(VERSION 3.16)
project(latex-fmt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main executable — always built, no external deps
add_executable(latex-fmt src/main.cpp)
target_include_directories(latex-fmt PRIVATE src)

# Testing — optional, requires Catch2
option(BUILD_TESTS "Build tests (requires Catch2 v3)" OFF)

if(BUILD_TESTS)
    enable_testing()

    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/install")

    find_package(Catch2 3 CONFIG QUIET)

    if(NOT Catch2_FOUND)
        message(WARNING
            "Catch2 v3 not found. Please run:  scripts/setup.sh\n"
            "Then re-run cmake with:  cmake .. -DBUILD_TESTS=ON"
        )
    else()
        add_executable(latex-fmt-tests
            tests/test_main.cpp
            tests/test_unicode_width.cpp
            tests/test_registry.cpp
            tests/test_lexer.cpp
            tests/test_formatter.cpp
        )

        target_compile_definitions(latex-fmt-tests PRIVATE
            SNAPSHOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/snapshots/"
        )

        target_include_directories(latex-fmt-tests PRIVATE src)

        target_link_libraries(latex-fmt-tests PRIVATE Catch2::Catch2WithMain)

        include(Catch)
        catch_discover_tests(latex-fmt-tests)
    endif()
endif()
