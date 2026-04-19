cmake_minimum_required(VERSION 3.20)
project(PillarEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_EDITOR "Build the editor" ON)
option(BUILD_GAME   "Build standalone game" ON)

# Find packages
find_package(OpenGL REQUIRED)

# Vendor includes
add_subdirectory(vendor)

# Engine library
add_subdirectory(Engine)

# Editor executable
if(BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Game executable
if(BUILD_GAME)
    add_subdirectory(Game)
endif()

# Packer tool
add_subdirectory(Tools/Packer)
