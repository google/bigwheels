# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
include(Utils)
include(CMakeParseArguments)

function(add_custom_target_in_folder TARGET_NAME)
    set(oneValueArgs FOLDER)
    set(multiValueArgs DEPENDS SOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 "ARG" "" "${oneValueArgs}" "${multiValueArgs}")

    if (DEFINED ARG_DEPENDS)
        add_custom_target("${TARGET_NAME}" DEPENDS ${ARG_DEPENDS} SOURCES ${ARG_SOURCES})
    else ()
        add_custom_target("${TARGET_NAME}" SOURCES ${ARG_SOURCES})
    endif ()
    set_target_properties("${TARGET_NAME}" PROPERTIES FOLDER "shaders/${ARG_FOLDER}")
endfunction()

# Target to depend on all shaders, to force-build all shaders.
add_custom_target("all-shaders")

function(make_output_dir OUTPUT_FILE)
    get_filename_component(PARENT_DIR ${OUTPUT_FILE} DIRECTORY)
    if (NOT EXISTS "${PARENT_DIR}")
        file(MAKE_DIRECTORY "${PARENT_DIR}")
        message(STATUS "creating output directory: ${PARENT_DIR}")
    endif()
endfunction()

function(internal_add_compile_shader_target TARGET_NAME)
    set(oneValueArgs COMPILER_PATH SOURCES OUTPUT_FILE SHADER_STAGE OUTPUT_FORMAT TARGET_FOLDER)
    set(multiValueArgs COMPILER_FLAGS INCLUDE_DIRS INCLUDES)
    cmake_parse_arguments(PARSE_ARGV 1 "ARG" "" "${oneValueArgs}" "${multiValueArgs}")

    set(INCLUDE_DIRS "")
    foreach(DIR ${ARG_INCLUDE_DIRS})
        set(INCLUDE_DIRS "${INCLUDE_DIRS} -I \"${DIR}\"")
    endforeach()
    string(STRIP "${INCLUDE_DIRS}" INCLUDE_DIRS)

    string(TOUPPER "${ARG_SHADER_STAGE}" ARG_SHADER_STAGE)

    make_output_dir("${ARG_OUTPUT_FILE}")
    add_custom_command(
        OUTPUT "${ARG_OUTPUT_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "------ Compiling ${ARG_SHADER_STAGE} Shader [${ARG_OUTPUT_FORMAT}] ------"
        MAIN_DEPENDENCY "${ARG_SOURCE}"
        DEPENDS ${ARG_INCLUDES}
        COMMAND ${CMAKE_COMMAND} -E echo "[${ARG_OUTPUT_FORMAT}] Compiling ${ARG_SHADER_STAGE} ${ARG_SOURCE} to ${ARG_OUTPUT_FILE}"
        COMMAND "${ARG_COMPILER_PATH}" ${ARG_COMPILER_FLAGS} ${INCLUDE_DIRS} -Fo "${ARG_OUTPUT_FILE}" "${ARG_SOURCE}"
    )
    add_custom_target_in_folder("${TARGET_NAME}" DEPENDS "${ARG_OUTPUT_FILE}" SOURCES "${ARG_SOURCE}" ${ARG_INCLUDES} FOLDER "${ARG_TARGET_FOLDER}")
endfunction()

function(internal_generate_rules_for_shader TARGET_NAME)
    set(oneValueArgs SOURCE SHADER_STAGE)
    set(multiValueArgs INCLUDE_DIRS INCLUDES)
    cmake_parse_arguments(PARSE_ARGV 1 "ARG" "" "${oneValueArgs}" "${multiValueArgs}")

    string(REPLACE ".hlsl" "" BASE_NAME "${ARG_SOURCE}")
    get_filename_component(BASE_NAME "${BASE_NAME}" NAME)
    file(RELATIVE_PATH PATH_PREFIX "${PPX_DIR}" "${ARG_SOURCE}")
    get_filename_component(PATH_PREFIX "${PATH_PREFIX}" DIRECTORY)

    # D3D12, dxil, sm 6_5.
    if (PPX_D3D12)
        internal_add_compile_shader_target(
            "dx12_${TARGET_NAME}_${ARG_SHADER_STAGE}"
            COMPILER_PATH "${DXC_PATH}"
            SOURCE "${ARG_SOURCE}"
            INCLUDE_DIRS ${ARG_INCLUDE_DIRS}
            INCLUDES ${ARG_INCLUDES}
            OUTPUT_FILE "${CMAKE_BINARY_DIR}/${PATH_PREFIX}/dxil/${BASE_NAME}.${ARG_SHADER_STAGE}.dxil"
            SHADER_STAGE "${ARG_SHADER_STAGE}"
            OUTPUT_FORMAT "DXIL_6_5"
            TARGET_FOLDER "${TARGET_NAME}"
            COMPILER_FLAGS "-T" "${ARG_SHADER_STAGE}_6_5" "-E" "${ARG_SHADER_STAGE}main" "-DPPX_DX12=1")
        add_dependencies("dx12_${TARGET_NAME}" "dx12_${TARGET_NAME}_${ARG_SHADER_STAGE}")
    endif ()

    # Vulkan, spv, sm 6_6.
    if (PPX_VULKAN)
        set(SHADER_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${PATH_PREFIX}/spv/${BASE_NAME}.${ARG_SHADER_STAGE}.spv")
        if (PPX_ANDROID)
            # Android has a designated asset folder, which is set to the root asset folder
            # The compiled SPVs must be placed there. There might be a way to set a secondary asset folder, but so far
            # attempts at doing that have failed
            set(SHADER_OUTPUT_PATH "spv/${BASE_NAME}.${ARG_SHADER_STAGE}.spv")
        endif()

        internal_add_compile_shader_target(
            "vk_${TARGET_NAME}_${ARG_SHADER_STAGE}"
            COMPILER_PATH "${DXC_PATH}"
            SOURCE "${ARG_SOURCE}"
            INCLUDE_DIRS ${ARG_INCLUDE_DIRS}
            INCLUDES ${ARG_INCLUDES}
            OUTPUT_FILE "${SHADER_OUTPUT_PATH}"
            SHADER_STAGE "${ARG_SHADER_STAGE}"
            OUTPUT_FORMAT "SPV_6_6"
            TARGET_FOLDER "${TARGET_NAME}"
            COMPILER_FLAGS "-spirv" "-fspv-flatten-resource-arrays" "-fspv-target-env=vulkan1.1" "-fvk-use-dx-layout" "-DPPX_VULKAN=1" "-T" "${ARG_SHADER_STAGE}_6_6" "-E" "${ARG_SHADER_STAGE}main")
        add_dependencies("vk_${TARGET_NAME}" "vk_${TARGET_NAME}_${ARG_SHADER_STAGE}")
    endif ()
endfunction()

function(generate_rules_for_shader TARGET_NAME)
    set(oneValueArgs SOURCE)
    set(multiValueArgs INCLUDE_DIRS INCLUDES STAGES)
    cmake_parse_arguments(PARSE_ARGV 1 "ARG" "" "${oneValueArgs}" "${multiValueArgs}")

    add_custom_target_in_folder("${TARGET_NAME}" SOURCES "${ARG_SOURCE}" ${ARG_INCLUDES} FOLDER "${TARGET_NAME}")
    message(STATUS "creating shader target ${TARGET_NAME}.")
    add_dependencies("all-shaders" "${TARGET_NAME}")

    if (PPX_D3D12)
        add_custom_target_in_folder("dx12_${TARGET_NAME}" SOURCES "${ARG_SOURCE}" ${ARG_INCLUDES} FOLDER "${TARGET_NAME}")
        add_dependencies("${TARGET_NAME}" "dx12_${TARGET_NAME}")
    endif ()
    if (PPX_VULKAN)
        add_custom_target_in_folder("vk_${TARGET_NAME}" SOURCES "${ARG_SOURCE}" ${ARG_INCLUDES} FOLDER "${TARGET_NAME}")
        add_dependencies("${TARGET_NAME}" "vk_${TARGET_NAME}")
    endif ()

    foreach (STAGE ${ARG_STAGES})
        internal_generate_rules_for_shader("${TARGET_NAME}" SOURCE "${ARG_SOURCE}" INCLUDE_DIRS ${ARG_INCLUDE_DIRS} INCLUDES ${ARG_INCLUDES} SHADER_STAGE "${STAGE}")
    endforeach ()
endfunction()

function(generate_group_rule_for_shader TARGET_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 "ARG" "" NONE "CHILDREN")

    add_custom_target_in_folder("${TARGET_NAME}" DEPENDS ${ARG_CHILDREN} FOLDER "${TARGET_NAME}")

    if (PPX_D3D12)
        prefix_all(PREFIXED_CHILDREN LIST ${ARG_CHILDREN} PREFIX "dx12_")
        add_custom_target_in_folder("dx12_${TARGET_NAME}" DEPENDS ${PREFIXED_CHILDREN} FOLDER "${TARGET_NAME}")
    endif ()
    if (PPX_VULKAN)
        prefix_all(PREFIXED_CHILDREN LIST ${ARG_CHILDREN} PREFIX "vk_")
        add_custom_target_in_folder("vk_${TARGET_NAME}" DEPENDS ${PREFIXED_CHILDREN} FOLDER "${TARGET_NAME}")
    endif ()
endfunction()
