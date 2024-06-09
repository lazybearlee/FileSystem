cmake_minimum_required(VERSION 3.10)
project(SimpleFileSystem)

set(CMAKE_CXX_STANDARD 14)

# Include src directory
include_directories(src)

# Add subdirectories
add_subdirectory(src)
#add_subdirectory(tests)
