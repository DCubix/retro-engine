cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(TwisterEngine LANGUAGES C CXX)

# Download CPM.cmake into the cmake folder
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
endif()

# Download CPM
# TODO: Get the latest version from: https://github.com/cpm-cmake/CPM.cmake/releases/latest
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bin2h.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tools)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/engine)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/app)