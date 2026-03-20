# set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DSDL_MAIN_HANDLED)

add_executable(EngineApp main.cpp)
target_link_libraries(EngineApp PRIVATE TwisterEngineLib)

# Copy the assets folder next to the executable
add_custom_command(TARGET EngineApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:EngineApp>/assets
    COMMENT "Copying assets folder"
)