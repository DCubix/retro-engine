set(LIBNAME TwisterEngineLib)

set(CMAKE_DEBUG_POSTFIX "")

# set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get shader sources
set(SHADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/shaders")
set(SHADER_HEADERS_OUT "${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/shaders/generated")
file(GLOB_RECURSE SHADERS
    "${SHADER_ROOT}/*.vert"
    "${SHADER_ROOT}/*.frag"
    "${SHADER_ROOT}/*.geom"
    "${SHADER_ROOT}/*.comp"
    "${SHADER_ROOT}/*.glsl"
)

set(GENERATED_SHADERS)

# Generate header files with the shader sources
foreach(SHADER_FILE ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    string(REPLACE " " "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "-" "_" SHADER_NAME ${SHADER_NAME})
    string(REPLACE "." "" SHADER_EXT ${SHADER_EXT})
    string(TOLOWER ${SHADER_NAME} SHADER_NAME)
    string(TOLOWER ${SHADER_EXT} SHADER_EXT)

    set(OUTPUT_FILE "${SHADER_HEADERS_OUT}/${SHADER_NAME}_${SHADER_EXT}.h")
    list(APPEND GENERATED_SHADERS ${OUTPUT_FILE})

    get_filename_component(SHADER_FN ${SHADER_FILE} NAME)
    get_filename_component(SHADER_HN ${OUTPUT_FILE} NAME)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND $<TARGET_FILE:shader_header_gen> ${OUTPUT_FILE} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE} shader_header_gen
        COMMENT "Generating ${SHADER_HN} from ${SHADER_FN}"
        VERBATIM
    )
endforeach()

add_custom_target(embed_shaders ALL
    DEPENDS ${GENERATED_SHADERS}
)

### Add CPM modules

# Find SDL3 package
CPMAddPackage(
    NAME SDL3
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.4.2
    OPTIONS
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_X11_XSCRNSAVER OFF"
)

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
)

CPMAddPackage(
    NAME physfs
    GIT_TAG release-3.2.0
    GITHUB_REPOSITORY icculus/physfs
    OPTIONS
        "PHYSFS_BUILD_SHARED OFF"
        "PHYSFS_BUILD_STATIC ON"
        "PHYSFS_BUILD_TEST OFF"
        "PHYSFS_ARCHIVE_ZIP ON"
        "PHYSFS_ARCHIVE_7Z OFF"
        "PHYSFS_ARCHIVE_GRP OFF"
        "PHYSFS_ARCHIVE_WAD OFF"
        "PHYSFS_ARCHIVE_CSM OFF"
        "PHYSFS_ARCHIVE_HOG OFF"
        "PHYSFS_ARCHIVE_MVL OFF"
        "PHYSFS_ARCHIVE_QPAK OFF"
        "PHYSFS_ARCHIVE_SLB OFF"
        "PHYSFS_ARCHIVE_VDF OFF"
        "PHYSFS_ARCHIVE_LECARCHIVES OFF"
        "PHYSFS_BUILD_DOCS OFF"
)

CPMAddPackage(
    NAME assimp
    GIT_TAG v6.0.4
    GITHUB_REPOSITORY assimp/assimp
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "ASSIMP_NO_EXPORT ON"
        "ASSIMP_BUILD_TESTS OFF"
)

CPMAddPackage(
    NAME RmlUi
    GITHUB_REPOSITORY mikke89/RmlUi
    GIT_TAG 6.1
    OPTIONS
        "BUILD_SAMPLES OFF"
        "BUILD_TESTING OFF"
        "RMLUI_BACKEND_SUPPORT OFF"
)

# Cross platform IS_DEBUG flag
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DIS_DEBUG)
endif()

# check if IS_DEBUG is defined
if (DEFINED IS_DEBUG)
    message(STATUS "IS_DEBUG is defined")
endif()

# Get sources from src directory
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.c" "*.cc" "*.cxx")
file(GLOB_RECURSE ENGINE_HEADERS "*.h" "*.hpp" "*.hxx")

# Add executable
add_library(${LIBNAME} STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(${LIBNAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(${LIBNAME} embed_shaders)

if (SDL3_ADDED)
    target_include_directories(${LIBNAME} PUBLIC ${SDL3_SOURCE_DIR}/include)
    target_link_libraries(${LIBNAME} PUBLIC SDL3::SDL3-static)
endif()

if (fmt_ADDED)
    target_include_directories(${LIBNAME} PUBLIC ${fmt_SOURCE_DIR}/include)
    target_link_libraries(${LIBNAME} PUBLIC fmt::fmt)
endif()

if (physfs_ADDED)
    target_include_directories(${LIBNAME} PUBLIC ${physfs_SOURCE_DIR}/src)
    target_link_libraries(${LIBNAME} PUBLIC physfs-static)
endif()

if (assimp_ADDED)
    target_include_directories(${LIBNAME} PUBLIC ${assimp_SOURCE_DIR}/include)
    target_link_libraries(${LIBNAME} PUBLIC assimp)
endif()

if (RmlUi_ADDED)
    target_include_directories(${LIBNAME} PUBLIC ${RmlUi_SOURCE_DIR}/Include)
    target_link_libraries(${LIBNAME} PUBLIC rmlui_core)
endif()
