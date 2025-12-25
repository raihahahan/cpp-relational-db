cmake_minimum_required(VERSION 3.14)
project(PostgresClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- GoogleTest setup ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --------------------------------------------------------------
# Library: storage
# --------------------------------------------------------------
add_library(storage
    src/storage/disk_manager/disk_manager.cpp
    src/storage/buffer_manager/buffer_manager.cpp
    src/storage/buffer_manager/free_list.cpp
    src/storage/buffer_manager/replacement_policies/clock_policy.cpp
    src/storage/page/slotted_page.cpp
    src/access/heap/heap_file.cpp
    src/access/heap/heap_iterator.cpp
)

# PUBLIC headers for library consumers
target_include_directories(storage PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# --------------------------------------------------------------
# Tests under tests/storage/*
# --------------------------------------------------------------
file(GLOB_RECURSE TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/tests/*/*/*.cpp
)

message(STATUS "Found test files:")
foreach(test_src ${TEST_SOURCES})
    message(STATUS "  ${test_src}")
endforeach()

include(GoogleTest)
foreach(test_src ${TEST_SOURCES})

    # extract test name from filename (without extension)
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    # tests need access to:
    # - include/ (public API)
    # - tests/   (mock headers)
    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tests
    )

    target_link_libraries(${test_name} PRIVATE
        storage
        GTest::gtest_main
    )

    # register tests
    gtest_discover_tests(${test_name})

endforeach()
