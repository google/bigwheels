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

project(ppx)

# ------------------------------------------------------------------------------
# Vulkan header and libraries
# ------------------------------------------------------------------------------

# On Android, the location of libs and headers to be determined using find_package
if (ANDROID)
    find_package(Vulkan REQUIRED)
endif()

# Detect Vulkan directories
if (PPX_VULKAN)
    # Determine Vulkan's include directory
    if (NOT VULKAN_INCLUDE_DIR)
        if (LINUX)
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/include")
            endif()
        elseif (ANDROID)
            set(VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIR})
        elseif (APPLE OR WIN32)
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
            endif()
        endif()
    endif()

    # Determine Vulkan's library directory
    if (NOT VULKAN_LIBRARY_DIR)
        if (LINUX)
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/lib")
            endif()
        elseif (ANDROID)
            get_filename_component(VULKAN_LIBRARY_DIR ${Vulkan_LIBRARIES} DIRECTORY)
        elseif (APPLE OR WIN32)
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/Lib")
            endif()
        endif()
    endif()

    # Bail if Vulkan's include directory is not set
    if (NOT VULKAN_INCLUDE_DIR)
        message(FATAL_ERROR "VULKAN_INCLUDE_DIR not specified and could not be determined using environment variable VULKAN_SDK")
    endif()

    # Bail if Vulkan's library directory is not set
    if (NOT VULKAN_LIBRARY_DIR)
        message(FATAL_ERROR "VULKAN_LIBRARY_DIR not specified and could not be determined using environment variable VULKAN_SDK")
    endif()

    message("Found Vulkan: ${VULKAN_DIR}")
    message("  Vulkan include directory: ${VULKAN_INCLUDE_DIR}")
    message("  Vulkan library directory: ${VULKAN_LIBRARY_DIR}")
endif()

# ------------------------------------------------------------------------------
# Vulkan Linux surface flags
# ------------------------------------------------------------------------------
#
# Defaults to XCB if nothing is specified
#
if (PPX_LINUX_XCB)
    set(PPX_LINUX_VULKAN_SURFACE "PPX_LINUX_XCB")
elseif(PPX_LINUX_XLIB)
    set(PPX_LINUX_VULKAN_SURFACE "PPX_LINUX_XLIB")
elseif(PPX_LINUX_WAYLAND)
    set(PPX_LINUX_VULKAN_SURFACE "PPX_LINUX_WAYLAND")
elseif(PPX_LINUX_HEADLESS)
    set(PPX_LINUX_VULKAN_SURFACE "PPX_LINUX_HEADLESS")
else()
    set(PPX_LINUX_VULKAN_SURFACE "PPX_LINUX_XCB")
endif()

# ------------------------------------------------------------------------------
# Add header and source files
# ------------------------------------------------------------------------------

set(INC_DIR    ${PPX_DIR}/include)
set(SRC_DIR    ${PPX_DIR}/src)
set(IMGUI_DIR  ${PPX_DIR}/third_party/imgui)
set(JSON_DIR   ${PPX_DIR}/third_party/json/single_include)

list(APPEND IMGUI_HEADER_FILES
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/backends/imgui_impl_glfw.h
)

list(APPEND IMGUI_SOURCE_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

list(APPEND JSON_HEADER_FILES
    ${JSON_DIR}/nlohmann/json.hpp)

if (NOT PPX_ANDROID)
    list(APPEND IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    )
endif()

list(APPEND XXHASH_FILES
    ${PPX_DIR}/third_party/xxHash/xxhash.h
    ${PPX_DIR}/third_party/xxHash/xxhash.c
)

if (PPX_VULKAN)
    list(APPEND IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.h
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
endif()

if (PPX_ANDROID)
    list(APPEND IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/backends/imgui_impl_android.h
        ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    )
endif()

if (PPX_D3D12)
    list(APPEND IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/backends/imgui_impl_dx12.h
        ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
    )
endif()

list(
    APPEND PPX_HEADER_FILES
    ${INC_DIR}/ppx/config.h
    ${INC_DIR}/ppx/math_config.h
    ${INC_DIR}/ppx/application.h
    ${INC_DIR}/ppx/base_application.h
    ${INC_DIR}/ppx/bitmap.h
    ${INC_DIR}/ppx/bounding_volume.h
    ${INC_DIR}/ppx/camera.h
    ${INC_DIR}/ppx/ccomptr.h
    ${INC_DIR}/ppx/command_line_parser.h
    ${INC_DIR}/ppx/csv_file_log.h
    ${INC_DIR}/ppx/font.h
    ${INC_DIR}/ppx/fs.h
    ${INC_DIR}/ppx/generate_mip_shader_DX.h
    ${INC_DIR}/ppx/generate_mip_shader_VK.h
    ${INC_DIR}/ppx/geometry.h
    ${INC_DIR}/ppx/graphics_util.h
    ${INC_DIR}/ppx/imgui_impl.h
    ${INC_DIR}/ppx/knob.h
    ${INC_DIR}/ppx/log.h
    ${INC_DIR}/ppx/metrics.h
    ${INC_DIR}/ppx/mipmap.h
    ${INC_DIR}/ppx/obj_ptr.h
    ${INC_DIR}/ppx/platform.h
    ${INC_DIR}/ppx/ppx.h
    ${INC_DIR}/ppx/ppm_export.h
    ${INC_DIR}/ppx/profiler.h
    ${INC_DIR}/ppx/random.h
    ${INC_DIR}/ppx/string_util.h
    ${INC_DIR}/ppx/timer.h
    ${INC_DIR}/ppx/transform.h
    ${INC_DIR}/ppx/tri_mesh.h
    ${INC_DIR}/ppx/util.h
    ${INC_DIR}/ppx/window.h
    ${INC_DIR}/ppx/wire_mesh.h
    ${INC_DIR}/ppx/xr_component.h
    ${INC_DIR}/ppx/xr_composition_layers.h
)

list(
    APPEND PPX_SOURCE_FILES
    ${SRC_DIR}/ppx/application.cpp
    ${SRC_DIR}/ppx/base_application.cpp
    ${SRC_DIR}/ppx/bitmap.cpp
    ${SRC_DIR}/ppx/bounding_volume.cpp
    ${SRC_DIR}/ppx/camera.cpp
    ${SRC_DIR}/ppx/command_line_parser.cpp
    ${SRC_DIR}/ppx/csv_file_log.cpp
    ${SRC_DIR}/ppx/font.cpp
    ${SRC_DIR}/ppx/fs.cpp
    ${SRC_DIR}/ppx/geometry.cpp
    ${SRC_DIR}/ppx/graphics_util.cpp
    ${SRC_DIR}/ppx/imgui_impl.cpp
    ${SRC_DIR}/ppx/knob.cpp
    ${SRC_DIR}/ppx/log.cpp
    ${SRC_DIR}/ppx/math_config.cpp
    ${SRC_DIR}/ppx/metrics.cpp
    ${SRC_DIR}/ppx/mipmap.cpp
    ${SRC_DIR}/ppx/platform.cpp
    ${SRC_DIR}/ppx/ppm_export.cpp
    ${SRC_DIR}/ppx/profiler.cpp
    ${SRC_DIR}/ppx/single_header_libs_impl.cpp
    ${SRC_DIR}/ppx/string_util.cpp
    ${SRC_DIR}/ppx/timer.cpp
    ${SRC_DIR}/ppx/transform.cpp
    ${SRC_DIR}/ppx/tri_mesh.cpp
    ${SRC_DIR}/ppx/window_android.cpp
    ${SRC_DIR}/ppx/window_glfw.cpp
    ${SRC_DIR}/ppx/window.cpp
    ${SRC_DIR}/ppx/wire_mesh.cpp
    ${SRC_DIR}/ppx/xr_component.cpp
    ${SRC_DIR}/ppx/xr_composition_layers.cpp
    ${SRC_DIR}/ppx/imgui/font_inconsolata.h
    ${SRC_DIR}/ppx/imgui/font_inconsolata.cpp
)

list(
    APPEND PPX_GRFX_HEADER_FILES
    ${INC_DIR}/ppx/grfx/grfx_config.h
    ${INC_DIR}/ppx/grfx/grfx_buffer.h
    ${INC_DIR}/ppx/grfx/grfx_command.h
    ${INC_DIR}/ppx/grfx/grfx_constants.h
    ${INC_DIR}/ppx/grfx/grfx_descriptor.h
    ${INC_DIR}/ppx/grfx/grfx_device.h
    ${INC_DIR}/ppx/grfx/grfx_draw_pass.h
    ${INC_DIR}/ppx/grfx/grfx_enums.h
    ${INC_DIR}/ppx/grfx/grfx_format.h
    ${INC_DIR}/ppx/grfx/grfx_fullscreen_quad.h
    ${INC_DIR}/ppx/grfx/grfx_gpu.h
    ${INC_DIR}/ppx/grfx/grfx_helper.h
    ${INC_DIR}/ppx/grfx/grfx_image.h
    ${INC_DIR}/ppx/grfx/grfx_instance.h
    ${INC_DIR}/ppx/grfx/grfx_mesh.h
    ${INC_DIR}/ppx/grfx/grfx_pipeline.h
    ${INC_DIR}/ppx/grfx/grfx_query.h
    ${INC_DIR}/ppx/grfx/grfx_queue.h
    ${INC_DIR}/ppx/grfx/grfx_render_pass.h
    ${INC_DIR}/ppx/grfx/grfx_scope.h
    ${INC_DIR}/ppx/grfx/grfx_shader.h
    ${INC_DIR}/ppx/grfx/grfx_swapchain.h
    ${INC_DIR}/ppx/grfx/grfx_sync.h
    ${INC_DIR}/ppx/grfx/grfx_text_draw.h
    ${INC_DIR}/ppx/grfx/grfx_texture.h
    ${INC_DIR}/ppx/grfx/grfx_util.h
)

list(
    APPEND PPX_GRFX_SOURCE_FILES
    ${SRC_DIR}/ppx/grfx/grfx_buffer.cpp
    ${SRC_DIR}/ppx/grfx/grfx_command.cpp
    ${SRC_DIR}/ppx/grfx/grfx_descriptor.cpp
    ${SRC_DIR}/ppx/grfx/grfx_device.cpp
    ${SRC_DIR}/ppx/grfx/grfx_draw_pass.cpp
    ${SRC_DIR}/ppx/grfx/grfx_format.cpp
    ${SRC_DIR}/ppx/grfx/grfx_fullscreen_quad.cpp
    ${SRC_DIR}/ppx/grfx/grfx_gpu.cpp
    ${SRC_DIR}/ppx/grfx/grfx_helper.cpp
    ${SRC_DIR}/ppx/grfx/grfx_image.cpp
    ${SRC_DIR}/ppx/grfx/grfx_instance.cpp
    ${SRC_DIR}/ppx/grfx/grfx_mesh.cpp
    ${SRC_DIR}/ppx/grfx/grfx_pipeline.cpp
    ${SRC_DIR}/ppx/grfx/grfx_query.cpp
    ${SRC_DIR}/ppx/grfx/grfx_queue.cpp
    ${SRC_DIR}/ppx/grfx/grfx_render_pass.cpp
    ${SRC_DIR}/ppx/grfx/grfx_scope.cpp
    ${SRC_DIR}/ppx/grfx/grfx_shader.cpp
    ${SRC_DIR}/ppx/grfx/grfx_swapchain.cpp
    ${SRC_DIR}/ppx/grfx/grfx_sync.cpp
    ${SRC_DIR}/ppx/grfx/grfx_text_draw.cpp
    ${SRC_DIR}/ppx/grfx/grfx_texture.cpp
    ${SRC_DIR}/ppx/grfx/grfx_util.cpp
)

list(
    APPEND PPX_SCENE_HEADER_FILES
    ${INC_DIR}/ppx/scene/scene_config.h
    ${INC_DIR}/ppx/scene/scene_material.h
    ${INC_DIR}/ppx/scene/scene_mesh.h
    ${INC_DIR}/ppx/scene/scene_node.h
    ${INC_DIR}/ppx/scene/scene_resource_manager.h
    ${INC_DIR}/ppx/scene/scene_scene.h
)

list(
    APPEND PPX_SCENE_SOURCE_FILES
    ${SRC_DIR}/ppx/scene/scene_material.cpp
    ${SRC_DIR}/ppx/scene/scene_mesh.cpp
    ${SRC_DIR}/ppx/scene/scene_node.cpp
    ${SRC_DIR}/ppx/scene/scene_resource_manager.cpp
    ${SRC_DIR}/ppx/scene/scene_scene.cpp
)

if (PPX_D3D12)
    list(
        APPEND PPX_GRFX_DX_HEADER_FILES
        ${INC_DIR}/ppx/grfx/dx/dx_util.h
    )

    list(
        APPEND PPX_GRFX_DX_SOURCE_FILES
        ${SRC_DIR}/ppx/grfx/dx/dx_util.cpp
    )
endif()

if (PPX_D3D12)
    list(
        APPEND PPX_GRFX_DX12_HEADER_FILES
        ${INC_DIR}/ppx/grfx/dx12/dx12_config.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_buffer.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_command.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_descriptor.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_descriptor_helper.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_device.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_gpu.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_image.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_instance.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_pipeline.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_query.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_queue.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_render_pass.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_shader.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_swapchain.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_sync.h
        ${INC_DIR}/ppx/grfx/dx12/dx12_util.h
    )

    list(
        APPEND PPX_GRFX_DX12_SOURCE_FILES
        ${SRC_DIR}/ppx/grfx/dx12/dx12_buffer.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_command.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_descriptor.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_descriptor_helper.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_device.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_gpu.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_image.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_instance.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_pipeline.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_query.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_queue.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_render_pass.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_shader.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_swapchain.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_sync.cpp
        ${SRC_DIR}/ppx/grfx/dx12/dx12_util.cpp
        ${PPX_THIRD_PARTY_DIR}/D3D12MemoryAllocator/include/D3D12MemAlloc.h
        ${PPX_THIRD_PARTY_DIR}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp
    )
endif()

if (PPX_VULKAN)
    list(
        APPEND PPX_GRFX_VK_HEADER_FILES
        ${INC_DIR}/ppx/grfx/vk/vk_config.h
        ${INC_DIR}/ppx/grfx/vk/vk_config_platform.h
        ${INC_DIR}/ppx/grfx/vk/vk_buffer.h
        ${INC_DIR}/ppx/grfx/vk/vk_command.h
        ${INC_DIR}/ppx/grfx/vk/vk_descriptor.h
        ${INC_DIR}/ppx/grfx/vk/vk_device.h
        ${INC_DIR}/ppx/grfx/vk/vk_gpu.h
        ${INC_DIR}/ppx/grfx/vk/vk_image.h
        ${INC_DIR}/ppx/grfx/vk/vk_instance.h
        ${INC_DIR}/ppx/grfx/vk/vk_pipeline.h
        ${INC_DIR}/ppx/grfx/vk/vk_query.h
        ${INC_DIR}/ppx/grfx/vk/vk_queue.h
        ${INC_DIR}/ppx/grfx/vk/vk_render_pass.h
        ${INC_DIR}/ppx/grfx/vk/vk_shader.h
        ${INC_DIR}/ppx/grfx/vk/vk_swapchain.h
        ${INC_DIR}/ppx/grfx/vk/vk_sync.h
        ${INC_DIR}/ppx/grfx/vk/vk_util.h
    )

    list(
        APPEND PPX_GRFX_VK_SOURCE_FILES
        ${SRC_DIR}/ppx/grfx/vk/vk_buffer.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_command.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_descriptor.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_device.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_gpu.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_image.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_instance.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_pipeline.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_profiler_fn_wrapper.h
        ${SRC_DIR}/ppx/grfx/vk/vk_profiler_fn_wrapper.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_query.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_queue.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_render_pass.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_shader.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_swapchain.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_sync.cpp
        ${SRC_DIR}/ppx/grfx/vk/vk_util.cpp
        ${PPX_THIRD_PARTY_DIR}/VulkanMemoryAllocator/include/vk_mem_alloc.h
    )
endif()

# ------------------------------------------------------------------------------
# Source group
# ------------------------------------------------------------------------------

source_group("ppx\\header"            FILES ${PPX_HEADER_FILES})
source_group("ppx\\source"            FILES ${PPX_SOURCE_FILES})
source_group("ppx-grfx\\header"       FILES ${PPX_GRFX_HEADER_FILES})
source_group("ppx-grfx\\source"       FILES ${PPX_GRFX_SOURCE_FILES})
source_group("ppx-grfx-dx\\header"    FILES ${PPX_GRFX_DX_HEADER_FILES})
source_group("ppx-grfx-dx\\source"    FILES ${PPX_GRFX_DX_SOURCE_FILES})
source_group("ppx-grfx-dx12\\header"  FILES ${PPX_GRFX_DX12_HEADER_FILES})
source_group("ppx-grfx-dx12\\source"  FILES ${PPX_GRFX_DX12_SOURCE_FILES})
source_group("ppx-grfx-vk\\header"    FILES ${PPX_GRFX_VK_HEADER_FILES})
source_group("ppx-grfx-vk\\source"    FILES ${PPX_GRFX_VK_SOURCE_FILES})
source_group("ppx-scene\\header"      FILES ${PPX_SCENE_HEADER_FILES})
source_group("ppx-scene\\source"      FILES ${PPX_SCENE_SOURCE_FILES})
source_group("third_party\\imgui"     FILES ${IMGUI_HEADER_FILES} ${IMGUI_SOURCE_FILES})
source_group("third_party\\json"      FILES ${JSON_HEADER_FILES})
source_group("third_party\\xxHash"    FILES ${XXHASH_FILES})

# ------------------------------------------------------------------------------
# Add library
# ------------------------------------------------------------------------------

add_library(
    ${PROJECT_NAME} STATIC
    ${PPX_HEADER_FILES}
    ${PPX_SOURCE_FILES}
    ${PPX_GRFX_HEADER_FILES}
    ${PPX_GRFX_SOURCE_FILES}
    ${PPX_GRFX_DX_HEADER_FILES}
    ${PPX_GRFX_DX_SOURCE_FILES}
    ${PPX_GRFX_DX12_HEADER_FILES}
    ${PPX_GRFX_DX12_SOURCE_FILES}
    ${PPX_GRFX_VK_HEADER_FILES}
    ${PPX_GRFX_VK_SOURCE_FILES}
    ${PPX_SCENE_HEADER_FILES}
    ${PPX_SCENE_SOURCE_FILES}
    ${CONTRIB_FILES}
    ${IMGUI_HEADER_FILES}
    ${IMGUI_SOURCE_FILES}
    ${XXHASH_FILES}
    ${JSON_HEADER_FILES}
)

if (PPX_LINUX)
    set_target_properties(
        ${PROJECT_NAME} PROPERTIES PREFIX "lib"
    )
endif()

# ------------------------------------------------------------------------------
# Include directories
# ------------------------------------------------------------------------------

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${SRC_DIR}
    PUBLIC  ${INC_DIR}
            ${PPX_THIRD_PARTY_DIR}/glfw/include
            ${PPX_THIRD_PARTY_DIR}/glm
            ${PPX_THIRD_PARTY_DIR}/gli
            ${PPX_THIRD_PARTY_DIR}/stb
            ${PPX_THIRD_PARTY_DIR}/imgui
            ${PPX_THIRD_PARTY_DIR}/tinyobjloader
            ${PPX_THIRD_PARTY_DIR}/pcg32
            ${PPX_THIRD_PARTY_DIR}/cgltf
            ${PPX_THIRD_PARTY_DIR}/xxHash
            ${PPX_THIRD_PARTY_DIR}/utfcpp/source
            ${JSON_DIR}
)

# Include D3D12 directories if D3D12 is enabled
if (PPX_D3D12)
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE ${PPX_THIRD_PARTY_DIR}/D3D12MemoryAllocator/include
        PRIVATE ${PPX_THIRD_PARTY_DIR}/D3D12MemoryAllocator/src
    )
endif()

# Include Vulkan directories if Vulkan is enabled
if (PPX_VULKAN)
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE ${PPX_THIRD_PARTY_DIR}/VulkanMemoryAllocator/include
        PRIVATE ${PPX_THIRD_PARTY_DIR}/VulkanMemoryAllocator/src
        PUBLIC  ${VULKAN_INCLUDE_DIR}
    )
endif()

# Include OpenXR directories if OpenXR is enabled.
# The OpenXR headers are generated at build time, so introduce a dependency
# and look for headers in the build directory.
if (PPX_BUILD_XR)
    add_dependencies(${PROJECT_NAME} generate_openxr_header)
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC ${CMAKE_BINARY_DIR}/third_party/OpenXR-SDK-Source/include
    )
endif()

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES FOLDER "ppx"
)

# ------------------------------------------------------------------------------
# Compile definitions
# ------------------------------------------------------------------------------
target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC  GLFW_INCLUDE_NONE
)

if (PPX_LINUX)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC  PPX_LINUX
    )
elseif (PPX_ANDROID)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC  PPX_ANDROID
    )
elseif (PPX_MSW)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC  PPX_MSW
                _CRT_SECURE_NO_WARNINGS
    )
endif()

if (PPX_BUILD_XR)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC PPX_BUILD_XR
    )
endif()

# ------------------------------------------------------------------------------
# Link libraries
# ------------------------------------------------------------------------------

if (NOT PPX_ANDROID)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC cpu_features
    )
endif()

if (PPX_MSW)
    target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC shcore.lib
    )
endif()

# ------------------------------------------------------------------------------
# Graphics API compile definitions
# ------------------------------------------------------------------------------
if (PPX_D3D12)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC  PPX_D3D12
    )
endif()

if (PPX_VULKAN)
    target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC  PPX_VULKAN
    )
    # Platform specific Vulkan defintions
    if (PPX_LINUX)
        target_compile_definitions(
            ${PROJECT_NAME}
            PUBLIC ${PPX_LINUX_VULKAN_SURFACE}
        )
    elseif (PPX_MSW)
        target_compile_definitions(
            ${PROJECT_NAME}
            PRIVATE
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Graphics API link libraries
# ------------------------------------------------------------------------------

if (PPX_D3D12)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC dxgi.lib dxguid.lib
    )
endif()

if (PPX_D3D12)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC d3d12.lib
    )
endif()

if (PPX_VULKAN)
    #Linux
    if (PPX_LINUX)
        # Link directories
        target_link_directories(${PROJECT_NAME}
            PUBLIC ${VULKAN_LIBRARY_DIR}
        )
        # Link libraries
        target_link_libraries(${PROJECT_NAME}
            PUBLIC vulkan
        )

        # Link properties
        set_target_properties(${PROJECT_NAME}
            PROPERTIES OUTPUT_NAME PPX
                       LINKER_LANGUAGE CXX
        )

        # Link XCB libraries if needed
        if (${PPX_LINUX_VULKAN_SURFACE} STREQUAL "PPX_LINUX_XCB")
            target_link_libraries(${PROJECT_NAME}
                PUBLIC xcb X11-xcb
            )
        endif()
    # Android
    elseif (PPX_ANDROID)
        target_link_libraries(${PROJECT_NAME}
            PUBLIC
            Vulkan::Vulkan
        )
    # Windows
    elseif (PPX_MSW)
        target_link_libraries(${PROJECT_NAME}
            PUBLIC "${VULKAN_LIBRARY_DIR}/vulkan-1.lib"
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Profiling
# ------------------------------------------------------------------------------
if (PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
    target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS
    )
endif()

# ------------------------------------------------------------------------------
# Unit Tests
# ------------------------------------------------------------------------------
# Test infrastructure uses gtest_discover_tests(), which is not cross-platform and
# fails for Android platforms
if (NOT ANDROID)
    if (PPX_BUILD_TESTS)
        enable_testing()
        include(GoogleTest)
        add_subdirectory(test)
    endif()
endif()

