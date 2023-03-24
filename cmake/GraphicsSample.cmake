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

function(_add_sample_internal)
    set(oneValueArgs NAME API_TAG SHADER_FORMAT)
    set(multiValueArgs API_DEFINES SOURCES DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "${oneValueArgs}" "${multiValueArgs}")

    set (TARGET_NAME "${ARG_API_TAG}_${ARG_NAME}")
    add_executable("${TARGET_NAME}" ${ARG_SOURCES})
    set_target_properties("${TARGET_NAME}" PROPERTIES FOLDER "ppx/samples/${ARG_API_TAG}")

    target_include_directories("${TARGET_NAME}" PUBLIC ${PPX_DIR}/include ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
    target_compile_definitions("${TARGET_NAME}" PRIVATE ${ARG_API_DEFINES})

    target_link_libraries("${TARGET_NAME}" PUBLIC ppx glfw)

    # OpenXR libs
    if (PPX_BUILD_XR)
        if (PPX_MSW OR PPX_LINUX)
            target_link_libraries(${TARGET_NAME} PUBLIC openxr_loader)
        else ()
            message(FATAL_ERROR "OpenXR not supported on GGP")
        endif ()
    endif()

    add_dependencies("${TARGET_NAME}" ppx_assets)
    if (DEFINED ARG_DEPENDENCIES)
        add_dependencies("${TARGET_NAME}" ${ARG_DEPENDENCIES})
    endif ()

    if (NOT TARGET all-${ARG_API_TAG})
        add_custom_target(all-${ARG_API_TAG})
    endif()
    add_dependencies(all-${ARG_API_TAG} "${TARGET_NAME}")
endfunction()

function(add_vk_sample)
    set(multiValueArgs SOURCES DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")
    if (PPX_VULKAN)
        _add_sample_internal(NAME ${ARG_NAME}
                   API_TAG "vk"
                   SHADER_FORMAT "spv"
                   API_DEFINES "USE_VK"
                   SOURCES ${ARG_SOURCES}
                   DEPENDENCIES ${ARG_DEPENDENCIES}
                   ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
    endif()
endfunction()

function(add_dx11_sample)
    set(multiValueArgs SOURCES DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")
    if (PPX_D3D11)
        _add_sample_internal(NAME ${ARG_NAME}
                   API_TAG "dx11"
                   SHADER_FORMAT "dxbc50"
                   API_DEFINES "USE_DX11"
                   SOURCES ${ARG_SOURCES}
                   DEPENDENCIES ${ARG_DEPENDENCIES}
                   ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
    endif()
endfunction()

function(add_dx12_sample)
    set(multiValueArgs SOURCES DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")
    if (PPX_D3D12)
        _add_sample_internal(NAME ${ARG_NAME}
                   API_TAG "dx12"
                   SHADER_FORMAT "dxbc51"
                   API_DEFINES "USE_DX12"
                   SOURCES ${ARG_SOURCES}
                   DEPENDENCIES ${ARG_DEPENDENCIES}
                   ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
    endif()
endfunction()

function(add_dxil_sample)
    set(multiValueArgs SOURCES DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")
    if (PPX_D3D12)
        _add_sample_internal(NAME ${ARG_NAME}
                   API_TAG "dxil"
                   SHADER_FORMAT "dxil"
                   API_DEFINES "USE_DX12" "USE_DXIL"
                   SOURCES ${ARG_SOURCES}
                   DEPENDENCIES ${ARG_DEPENDENCIES}
                   ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
    endif()
endfunction()

function(add_samples)
    set(multiValueArgs TARGET_APIS SOURCES DEPENDENCIES SHADER_DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")

    foreach(target_api ${ARG_TARGET_APIS})
        if(target_api STREQUAL "dx11")
            prefix_all(PREFIXED_SHADERS_DEPENDENCIES LIST ${ARG_SHADER_DEPENDENCIES} PREFIX "d3d11_")
            add_dx11_sample(NAME ${ARG_NAME} SOURCES ${ARG_SOURCES} DEPENDENCIES ${ARG_DEPENDENCIES} ${PREFIXED_SHADERS_DEPENDENCIES} ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
        elseif(target_api STREQUAL "dx12")
            prefix_all(PREFIXED_SHADERS_DEPENDENCIES LIST ${ARG_SHADER_DEPENDENCIES} PREFIX "d3d12_")
            add_dx12_sample(NAME ${ARG_NAME} SOURCES ${ARG_SOURCES} DEPENDENCIES ${ARG_DEPENDENCIES} ${PREFIXED_SHADERS_DEPENDENCIES} ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
        elseif(target_api STREQUAL "dxil")
            prefix_all(PREFIXED_SHADERS_DEPENDENCIES LIST ${ARG_SHADER_DEPENDENCIES} PREFIX "dxil_")
            add_dxil_sample(NAME ${ARG_NAME} SOURCES ${ARG_SOURCES} DEPENDENCIES ${ARG_DEPENDENCIES} ${PREFIXED_SHADERS_DEPENDENCIES} ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
        elseif(target_api STREQUAL "vk")
            prefix_all(PREFIXED_SHADERS_DEPENDENCIES LIST ${ARG_SHADER_DEPENDENCIES} PREFIX "vk_")
            add_vk_sample(NAME ${ARG_NAME} SOURCES ${ARG_SOURCES} DEPENDENCIES ${ARG_DEPENDENCIES} ${PREFIXED_SHADERS_DEPENDENCIES} ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES})
        else()
            message(FATAL_ERROR "Invalid target API \"${target_api}\"" )
        endif()
    endforeach()
endfunction()

function(add_samples_for_all_apis)
    set(multiValueArgs SOURCES DEPENDENCIES SHADER_DEPENDENCIES ADDITIONAL_INCLUDE_DIRECTORIES)
    cmake_parse_arguments(PARSE_ARGV 0 "ARG" "" "NAME" "${multiValueArgs}")
    add_samples(
        NAME ${ARG_NAME}
        TARGET_APIS "dx12" "dx11" "vk" "dxil"
        SOURCES ${ARG_SOURCES}
        DEPENDENCIES ${ARG_DEPENDENCIES}
        SHADER_DEPENDENCIES ${ARG_SHADER_DEPENDENCIES}
        ADDITIONAL_INCLUDE_DIRECTORIES ${ARG_ADDITIONAL_INCLUDE_DIRECTORIES}
    )
endfunction()
