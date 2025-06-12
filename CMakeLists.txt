# As of writing, CMake 3.13 or greater is required, although it is recommended to use the latest version
# So, you can pick whatever version you have
cmake_minimum_required(VERSION 3.24)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
project(deobfninja CXX)

# Define a macro in the build to be called by the program
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

# Binary Ninja uses C++17 features, and requires a C++17 compatible compiler
set(CMAKE_CXX_STANDARD 17)

# Unless we are writing a plugin that needs Qt's UI, we'll specify this
set(HEADLESS 1)

# Point CMake to the API and include it for use
find_path(
        BN_API_PATH
        NAMES binaryninjaapi.h
        # List of paths to search for the clone of the api
        HINTS ../.. binaryninjaapi $ENV{BN_API_PATH}
        REQUIRED
)
add_subdirectory(${BN_API_PATH} api)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link with Binary Ninja
target_link_libraries(${PROJECT_NAME} PUBLIC binaryninjaapi)

# Tell `cmake --install` to copy the plugin to the plugins directory
bn_install_plugin(${PROJECT_NAME})