set(
    HOVER_SHADERCROSS_ROOT
    "${CMAKE_SOURCE_DIR}/.tools/shadercross/SDL3_shadercross-3.0.0-linux-x64"
    CACHE PATH
    "Root of the pinned SDL_shadercross host tool"
)

set(
    HOVER_SHADERCROSS_EXECUTABLE
    "${HOVER_SHADERCROSS_ROOT}/bin/shadercross"
    CACHE FILEPATH
    "Path to the pinned shadercross executable"
)

set(
    HOVER_SHADERCROSS_LIBRARY_DIR
    "${HOVER_SHADERCROSS_ROOT}/lib"
    CACHE PATH
    "Directory containing shadercross host-tool libraries"
)

if(NOT EXISTS "${HOVER_SHADERCROSS_EXECUTABLE}")
    message(FATAL_ERROR
        "Pinned shadercross tool not found. Run: tools/fetch-shadercross-linux-x64.sh"
    )
endif()

add_custom_target(hover_shaders)

function(hover_add_shader)
    set(options)
    set(one_value_args NAME SOURCE STAGE)
    set(multi_value_args)
    cmake_parse_arguments(HOVER_SHADER "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT HOVER_SHADER_NAME OR NOT HOVER_SHADER_SOURCE OR NOT HOVER_SHADER_STAGE)
        message(FATAL_ERROR "hover_add_shader requires NAME, SOURCE, and STAGE")
    endif()

    set(source_path "${CMAKE_SOURCE_DIR}/${HOVER_SHADER_SOURCE}")
    set(output_dir "${CMAKE_BINARY_DIR}/shaders")
    set(output_path "${output_dir}/${HOVER_SHADER_NAME}.spv")
    set(shader_target "hover_shader_${HOVER_SHADER_NAME}")

    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}"
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "LD_LIBRARY_PATH=${HOVER_SHADERCROSS_LIBRARY_DIR}"
            "${HOVER_SHADERCROSS_EXECUTABLE}"
            "${source_path}"
            --source HLSL
            --dest SPIRV
            --stage "${HOVER_SHADER_STAGE}"
            --entrypoint main
            --output "${output_path}"
        DEPENDS "${source_path}"
        COMMENT "Compiling ${HOVER_SHADER_SOURCE} to SPIR-V"
        VERBATIM
    )

    add_custom_target("${shader_target}" DEPENDS "${output_path}")
    add_dependencies(hover_shaders "${shader_target}")
endfunction()
