cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(tools)

# set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable unicode
add_definitions(-DUNICODE -D_UNICODE)

add_executable(shader_header_gen "shader_header_gen.cpp")