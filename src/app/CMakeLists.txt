# set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DSDL_MAIN_HANDLED)

add_executable(EngineApp main.cpp)
target_link_libraries(EngineApp PRIVATE TwisterEngineLib)