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


# Prevent GoogleTest from overriding compiler flags
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# source files
add_library(storage
    src/storage/disk_manager/disk_manager.cpp
)
target_include_directories(storage PUBLIC src)

# header files
include_directories(${PROJECT_SOURCE_DIR}/include)

# tests
add_executable(test_disk_manager tests/storage/disk_manager/test_disk_manager.cpp)
target_link_libraries(test_disk_manager
    PRIVATE
        storage
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_disk_manager)
