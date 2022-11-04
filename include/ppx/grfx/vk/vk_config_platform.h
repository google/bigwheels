// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ppx_grfx_vk_config_platform_h
#define ppx_grfx_vk_config_platform_h

// clang-format off
#if defined(PPX_GGP)
#   if ! defined(VK_USE_PLATFORM_GGP)
#       define VK_USE_PLATFORM_GGP
#   endif
#elif defined(PPX_LINUX)
#   if defined(PPX_LINUX_XCB)
#       if ! defined(VK_USE_PLATFORM_XCB_KHR)
#           define VK_USE_PLATFORM_XCB_KHR
#       endif
#   elif defined(PPX_LINUX_XLIB)
#       if ! defined(VK_USE_PLATFORM_XLIB_KHR)
#           define VK_USE_PLATFORM_XLIB_KHR
#       endif
#   elif defined(PPX_LINUX_WAYLAND)
#       if ! defined(VK_USE_PLATFORM_WAYLAND_KHR)
#           define VK_USE_PLATFORM_WAYLAND_KHR
#       endif
#   endif
#elif defined(PPX_MSW)
#   if ! defined(VK_USE_PLATFORM_WIN32_KHR)
#       define VK_USE_PLATFORM_WIN32_KHR
#   endif
#endif
// clang-format on
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#define PPX_VULKAN_VERSION_1_1 110
#define PPX_VULKAN_VERSION_1_2 120

//
// If VK_HEADER_VERSION_COMPLETE isn't defined then platform is Vulkan 1.1.
//
#if defined(VK_HEADER_VERSION_COMPLETE)
#define PPX_VULKAN_VERSION PPX_VULKAN_VERSION_1_2
#else
#define PPX_VULKAN_VERSION PPX_VULKAN_VERSION_1_1
#endif // defined(VK_HEADER_VERSION_COMPLETE)

#if PPX_VULKAN_VERSION < PPX_VULKAN_VERSION_1_2
#define VK_ERROR_FRAGMENTATION                          VK_ERROR_FRAGMENTATION_EXT
#define VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT
#define VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR             VK_BUFFER_USAGE_RAY_TRACING_BIT_NV
#endif

#define VK_LAYER_KHRONOS_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

#endif // ppx_grfx_vk_config_platform_h
