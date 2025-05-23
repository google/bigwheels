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
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

cmake_policy(SET CMP0054 NEW)

project(BigWheels)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
# Options.
# ------------------------------------------------------------------------------
option(PPX_BUILD_PROJECTS "Build sample projets" ON)
option(PPX_BUILD_BENCHMARKS "Build benchmarks projects" ON)

# ------------------------------------------------------------------------------
# Detect DXC presence. This is REQUIRED to compile DXIL and SPIR-V shaders.
# ------------------------------------------------------------------------------

message(CHECK_START "Looking for DXC")
if (NOT DXC_PATH)
    message(STATUS "DXC_PATH not set. Looking for Vulkan SDK path in ENV")
    if (DEFINED ENV{VULKAN_SDK})
        if (CMAKE_HOST_WIN32)
            set(DXC_PATH "$ENV{VULKAN_SDK}\\Bin\\dxc.exe")
        else ()
            set(DXC_PATH "$ENV{VULKAN_SDK}/bin/dxc")
        endif ()
    endif ()
endif ()

if ((NOT DXC_PATH) OR (NOT EXISTS "${DXC_PATH}"))
    message(CHECK_FAIL "Cannot open DXC at ${DXC_PATH}")
    message(FATAL_ERROR "DXC is required.")
else ()
    message(CHECK_PASS "found")
endif ()

message(STATUS "DXC_PATH: ${DXC_PATH}")

# ------------------------------------------------------------------------------
# Detect glslc presence. Required to compile GLSL shaders.
# ------------------------------------------------------------------------------

message(CHECK_START "Looking for GLSLC")
if (NOT GLSLC_PATH)
    message(STATUS "GLSLC_PATH not set. Looking for Vulkan SDK path in ENV")
    if (DEFINED ENV{VULKAN_SDK})
        if (CMAKE_HOST_WIN32)
            set(GLSLC_PATH "$ENV{VULKAN_SDK}\\Bin\\glslc.exe")
        else ()
            set(GLSLC_PATH "$ENV{VULKAN_SDK}/bin/glslc")
        endif ()
    endif ()
endif ()

if ((NOT GLSLC_PATH) OR (NOT EXISTS "${GLSLC_PATH}"))
    message(CHECK_FAIL "Cannot open GLSLC at ${GLSLC_PATH}")
else ()
    message(CHECK_PASS "found")
endif ()

message(STATUS "GLSLC_PATH: ${GLSLC_PATH}")

# ------------------------------------------------------------------------------
# Configure target platform.
# ------------------------------------------------------------------------------

# Detect Linux.
if (UNIX AND NOT (APPLE OR ANDROID))
    set(LINUX TRUE)
endif()

if (NOT (PPX_LINUX OR PPX_MSW OR PPX_ANDROID))
    message("No PPX target plaform specified - configuring based on current host")
    if (LINUX)
        set(PPX_LINUX TRUE)
        message("Target platform       : Linux")
    elseif (ANDROID)
        set(PPX_ANDROID TRUE)
        message("Target platform       : Android")
    elseif (WIN32)
        set(PPX_MSW TRUE)
        message("Target platform       : Windows")
    endif()
endif()

# ------------------------------------------------------------------------------
# Configure graphics API(s).
# ------------------------------------------------------------------------------

if (NOT DEFINED PPX_VULKAN)
    set(PPX_VULKAN TRUE)
endif ()

if (NOT DEFINED PPX_D3D12)
    if (WIN32)
        set(PPX_D3D12 TRUE)
    else ()
        set(PPX_D3D12 FALSE)
    endif ()
endif ()

message("Graphics API D3D12    : ${PPX_D3D12}")
message("Graphics API Vulkan   : ${PPX_VULKAN}")

# ------------------------------------------------------------------------------
# Add CMake modules for shader compilation and sample creation.
# Needs to come after platform and API config.
# ------------------------------------------------------------------------------

include(ShaderCompile)
include(GraphicsSample)

# ------------------------------------------------------------------------------
# Configure C++ flags.
# ------------------------------------------------------------------------------

if(MSVC)
    # Disable the following warnings:
    # 26812: unscoped enums are widely used by the project and dependencies.
    set(MSVC_DISABLED_WARNINGS "/wd26812")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${MSVC_DISABLED_WARNINGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_DISABLED_WARNINGS} /MP /Zc:__cplusplus /std:c++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -Werror \
        -std=c++17 \
        -fdiagnostics-color=always \
        -Wno-nullability-completeness \
        -Wno-deprecated-anon-enum-enum-conversion \
        -Wno-missing-field-initializers \
        -Wno-unused-parameter \
        -Wno-ignored-qualifiers \
        -Wno-deprecated-copy \
        -Wno-deprecated-enum-enum-conversion \
        -Wno-pointer-bool-conversion \
    ")

endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.0)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-template-arg-list-after-template-kw")
endif()

set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------------------
# Configure PPX directories.
# ------------------------------------------------------------------------------

set(PPX_DIR             "${CMAKE_CURRENT_SOURCE_DIR}")
set(PPX_THIRD_PARTY_DIR "${PPX_DIR}/third_party")
set(PPX_TESTS_DIR       "${PPX_DIR}/tests")

# ------------------------------------------------------------------------------
# Configure output directories.
# ------------------------------------------------------------------------------

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(PPX_ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(PPX_3P_ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assets")

# Determine relative path from binary location to the build root.
cmake_path(SET BUILD_DIRECTORY NORMALIZE "${CMAKE_BINARY_DIR}")
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # Visual Studio doesn't follow CMAKE_RUNTIME_OUTPUT_DIRECTORY, but
    # creates a subdirectory with the config name.
    cmake_path(SET EXECUTABLE_DIRECTORY NORMALIZE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/subfolder")
else ()
    cmake_path(SET EXECUTABLE_DIRECTORY NORMALIZE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()
cmake_path(RELATIVE_PATH BUILD_DIRECTORY BASE_DIRECTORY ${EXECUTABLE_DIRECTORY} OUTPUT_VARIABLE BUILD_TO_EXECUTABLE_PATH)
add_compile_definitions(RELATIVE_PATH_TO_PROJECT_ROOT="${BUILD_TO_EXECUTABLE_PATH}")

add_custom_target(ppx_assets)

# When using a custom output directory, make sure to copy over the assets.
if(NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR))
    add_custom_target(ppx_copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PPX_ASSET_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PPX_3P_ASSET_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/third_party/assets"
    )
    add_dependencies(ppx_assets ppx_copy_assets)
endif()

# ------------------------------------------------------------------------------
# Enable testing support for CMake.
# ------------------------------------------------------------------------------
option(PPX_BUILD_TESTS "Build tests" ON)
if (PPX_BUILD_TESTS)
    enable_testing()
endif()

# ------------------------------------------------------------------------------
# Configure GLFW.
# ------------------------------------------------------------------------------
if (NOT PPX_ANDROID)
    set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    FALSE CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     FALSE CACHE BOOL "" FORCE)
    set(GLFW_VULKAN_STATIC  FALSE CACHE BOOL "" FORCE)
    if (PPX_LINUX_WAYLAND)
        set(GLFW_USE_WAYLAND TRUE CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(${PPX_THIRD_PARTY_DIR}/glfw)
endif()

# ------------------------------------------------------------------------------
# Add native activity for ANDROID
# ------------------------------------------------------------------------------
if (PPX_ANDROID)
    include_directories("${ANDROID_NDK}/sources/android/native_app_glue/")
    add_library(android_native_app_glue
        OBJECT
        "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
    target_compile_options(android_native_app_glue PRIVATE -fPIC)
endif()

# ------------------------------------------------------------------------------
# Add cpu_features.
# ------------------------------------------------------------------------------
if (NOT PPX_ANDROID)
    function(add_cpu_features)
        set(BUILD_TESTING OFF)
        add_subdirectory(${PPX_THIRD_PARTY_DIR}/cpu_features)
    endfunction()

    add_cpu_features()
endif()

# ------------------------------------------------------------------------------
# Add OpenXR.
# ------------------------------------------------------------------------------
if (PPX_BUILD_XR)
    message("Building OpenXR support")
    configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR})
    if (PPX_LINUX)
        add_compile_options(-Wno-tautological-pointer-compare -Wno-error=address)
    endif()
    add_subdirectory(third_party/OpenXR-SDK-Source)
endif()

# ------------------------------------------------------------------------------
# Add PPX library.
# ------------------------------------------------------------------------------
add_subdirectory(src)

# ------------------------------------------------------------------------------
# Add assets.
# ------------------------------------------------------------------------------
add_subdirectory(assets)

# ------------------------------------------------------------------------------
# Add projects.
# ------------------------------------------------------------------------------
if (PPX_BUILD_PROJECTS)
    message("Building sample projects")
    add_subdirectory(projects)
endif()

# ------------------------------------------------------------------------------
# Add benchmarks.
# ------------------------------------------------------------------------------
if (PPX_BUILD_BENCHMARKS)
    message("Building benchmarks projects")
    add_subdirectory(benchmarks)
endif()

