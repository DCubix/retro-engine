function(embed_shaders OUTPUT_DIR)
    # Remaining arguments are shader source files
    set(SHADERS ${ARGN})

    # List of headers for result
    set(HEADERS)

    foreach(SHADER_FILE IN LISTS SHADERS)
        get_filename_component(SHADER_NAME_WE "${SHADER_FILE}" NAME_WE)
        get_filename_component(SHADER_EXT "${SHADER_FILE}" EXT)
        string(SUBSTRING "${SHADER_EXT}" 1 -1 SHADER_EXT_NO_DOT)
        string(TOLOWER "${SHADER_NAME_WE}" SHADER_NAME_LOWER)
        string(TOLOWER "${SHADER_EXT_NO_DOT}" SHADER_EXT_LOWER)

        set(HEADER_FILENAME "${OUTPUT_DIR}/${SHADER_NAME_WE}_${SHADER_EXT_NO_DOT}.h")

        # Read shader content
        file(READ "${SHADER_FILE}" SHADER_CONTENT)

        # Write header file
        file(WRITE "${HEADER_FILENAME}" "#pragma once\n")
        file(APPEND "${HEADER_FILENAME}" "#include <string>\n")
        file(APPEND "${HEADER_FILENAME}" "const std::string ${SHADER_NAME_LOWER}_${SHADER_EXT_LOWER}_src = R\"(\n${SHADER_CONTENT}\n)\";\n")

        # Add the generated header to the list of sources
        list(APPEND HEADERS "${HEADER_FILENAME}")
    endforeach()

    # return the list of generated headers
    set(SHADER_HEADERS ${HEADERS} PARENT_SCOPE)
endfunction()