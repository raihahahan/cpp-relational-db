cmake_minimum_required(VERSION 3.14)
project(PostgresClone LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- GoogleTest setup ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(main
    src/storage/disk_manager/disk_manager.cpp
    src/storage/buffer_manager/buffer_manager.cpp
    src/storage/buffer_manager/free_list.cpp
    src/storage/buffer_manager/replacement_policies/clock_policy.cpp
    src/storage/page/slotted_page.cpp
    src/access/heap/heap_file.cpp
    src/access/heap/heap_iterator.cpp
    src/catalog/catalog_codec.cpp
    src/catalog/catalog_tables.cpp
    src/catalog/catalog.cpp
    src/server/server.cpp
    src/model/dynamic_codec.cpp
    src/model/relation.cpp
    src/model/user_table.cpp
    src/model/table_manager.cpp
    src/common/tuple.cpp
    src/executor/operators/filter_op.cpp
    src/executor/operators/seq_scan_op.cpp
    src/executor/operators/projection_op.cpp
    src/executor/operators/limit_op.cpp
    src/executor/predicate.cpp
    src/executor/executor.cpp
)

# Disk Manager
add_executable(dm 
    src/storage/disk_manager/main.cpp
    src/storage/disk_manager/disk_manager.cpp)
target_include_directories(dm PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(main PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ASAN on main library
target_compile_options(main PRIVATE
    -fsanitize=address
    -fno-omit-frame-pointer
)
target_link_options(main PRIVATE
    -fsanitize=address
)

# --------------------------------------------------------------
# Tests under tests/*/*
# --------------------------------------------------------------
file(GLOB_RECURSE TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/tests/**/*.cpp
)

include(GoogleTest)
foreach(test_src ${TEST_SOURCES})

    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tests
    )

    target_link_libraries(${test_name} PRIVATE
        main
        GTest::gtest_main
    )

    # ASAN on test binaries
    target_compile_options(${test_name} PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(${test_name} PRIVATE
        -fsanitize=address
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

endforeach()
