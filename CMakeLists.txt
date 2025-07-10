# As of writing, CMake 3.13 or greater is required, although it is recommended to use the latest version
# So, you can pick whatever version you have
cmake_minimum_required(VERSION 3.24)
project(deobfninja CXX)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

# Define a macro in the build to be called by the program
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

# Binary Ninja uses C++17 features, and requires a C++17 compatible compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unless we are writing a plugin that needs Qt's UI, we'll specify this
set(HEADLESS 1)

# ===================== Binary Ninja =====================
find_path(
    BN_API_PATH
    NAMES binaryninjaapi.h
    # List of paths to search for the clone of the api
    HINTS ../.. binaryninjaapi $ENV{BN_API_PATH}
    REQUIRED
)
add_subdirectory(${BN_API_PATH} api)

# ===================== Sources ==========================
file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC binaryninjaapi)

# Tell `cmake --install` to copy the plugin to the plugins directory
bn_install_plugin(${PROJECT_NAME})

# ===================== Tests ============================
include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
add_executable(${PROJECT_NAME}_gtest ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}_gtest PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_gtest
    PRIVATE
        gtest_main
        binaryninjaapi
        ${PROJECT_NAME}
)

add_test(NAME DeobfuscatorTests COMMAND ${PROJECT_NAME}_gtest)

# ===================== Third-party libraries ============================

# magic_enum (header-only)
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

target_link_libraries(${PROJECT_NAME} PUBLIC magic_enum)
target_link_libraries(${PROJECT_NAME}_gtest PRIVATE magic_enum)
