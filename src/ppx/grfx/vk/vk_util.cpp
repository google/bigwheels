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

#include "ppx/grfx/vk/vk_util.h"

namespace ppx {
namespace grfx {
namespace vk {

const char* ToString(VkResult value)
{
    // clang-format off
    switch (value) {
        default: break;
        case VK_SUCCESS                                            : return "VK_SUCCESS"; break;
        case VK_NOT_READY                                          : return "VK_NOT_READY"; break;
        case VK_TIMEOUT                                            : return "VK_TIMEOUT"; break;
        case VK_EVENT_SET                                          : return "VK_EVENT_SET"; break;
        case VK_EVENT_RESET                                        : return "VK_EVENT_RESET"; break;
        case VK_INCOMPLETE                                         : return "VK_INCOMPLETE"; break;
        case VK_ERROR_OUT_OF_HOST_MEMORY                           : return "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY                         : return "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
        case VK_ERROR_INITIALIZATION_FAILED                        : return "VK_ERROR_INITIALIZATION_FAILED"; break;
        case VK_ERROR_DEVICE_LOST                                  : return "VK_ERROR_DEVICE_LOST"; break;
        case VK_ERROR_MEMORY_MAP_FAILED                            : return "VK_ERROR_MEMORY_MAP_FAILED"; break;
        case VK_ERROR_LAYER_NOT_PRESENT                            : return "VK_ERROR_LAYER_NOT_PRESENT"; break;
        case VK_ERROR_EXTENSION_NOT_PRESENT                        : return "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
        case VK_ERROR_FEATURE_NOT_PRESENT                          : return "VK_ERROR_FEATURE_NOT_PRESENT"; break;
        case VK_ERROR_INCOMPATIBLE_DRIVER                          : return "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
        case VK_ERROR_TOO_MANY_OBJECTS                             : return "VK_ERROR_TOO_MANY_OBJECTS"; break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED                         : return "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
        case VK_ERROR_FRAGMENTED_POOL                              : return "VK_ERROR_FRAGMENTED_POOL"; break;
        case VK_ERROR_OUT_OF_POOL_MEMORY                           : return "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE                      : return "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break;
        case VK_ERROR_FRAGMENTATION                                : return "VK_ERROR_FRAGMENTATION"; break;
        case VK_ERROR_SURFACE_LOST_KHR                             : return "VK_ERROR_SURFACE_LOST_KHR"; break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR                     : return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
        case VK_SUBOPTIMAL_KHR                                     : return "VK_SUBOPTIMAL_KHR"; break;
        case VK_ERROR_OUT_OF_DATE_KHR                              : return "VK_ERROR_OUT_OF_DATE_KHR"; break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR                     : return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
        case VK_ERROR_VALIDATION_FAILED_EXT                        : return "VK_ERROR_VALIDATION_FAILED_EXT"; break;
        case VK_ERROR_INVALID_SHADER_NV                            : return "VK_ERROR_INVALID_SHADER_NV"; break;
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT : return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break;
        case VK_ERROR_NOT_PERMITTED_EXT                            : return "VK_ERROR_NOT_PERMITTED_EXT"; break;
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT          : return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
#if PPX_VULKAN_VERSION >= PPX_VULKAN_VERSION_1_2
        case VK_ERROR_UNKNOWN                                      : return "VK_ERROR_UNKNOWN"; break;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS               : return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"; break;
        //case VK_ERROR_INCOMPATIBLE_VERSION_KHR                     : return "VK_ERROR_INCOMPATIBLE_VERSION_KHR"; break;
        case VK_THREAD_IDLE_KHR                                    : return "VK_THREAD_IDLE_KHR"; break;
        case VK_THREAD_DONE_KHR                                    : return "VK_THREAD_DONE_KHR"; break;
        case VK_OPERATION_DEFERRED_KHR                             : return "VK_OPERATION_DEFERRED_KHR"; break;
        case VK_OPERATION_NOT_DEFERRED_KHR                         : return "VK_OPERATION_NOT_DEFERRED_KHR"; break;
        case VK_PIPELINE_COMPILE_REQUIRED_EXT                      : return "VK_PIPELINE_COMPILE_REQUIRED_EXT"; break;
#endif // PPX_VULKAN_VERSION >= PPX_VERSION_VULKAN_1_2
    }
    // clang-format on
    return "<unknown VkResult value>";
}

const char* ToString(VkDescriptorType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case VK_DESCRIPTOR_TYPE_SAMPLER                : return "VK_DESCRIPTOR_TYPE_SAMPLER"; break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"; break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE          : return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE"; break;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE          : return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"; break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER   : return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"; break;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER   : return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"; break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER         : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"; break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER         : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"; break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"; break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC"; break;
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT       : return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT"; break;
    }
    // clang-format on
    return "<unknown VkDescriptorType value>";
}

const char* ToString(VkPresentModeKHR value)
{
    // clang-format off
    switch (value) {
        default: break;
        case VK_PRESENT_MODE_IMMEDIATE_KHR    : return "VK_PRESENT_MODE_IMMEDIATE_KHR"; break;
        case VK_PRESENT_MODE_MAILBOX_KHR      : return "VK_PRESENT_MODE_MAILBOX_KHR"; break;
        case VK_PRESENT_MODE_FIFO_KHR         : return "VK_PRESENT_MODE_FIFO_KHR"; break;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR : return "VK_PRESENT_MODE_FIFO_RELAXED_KHR"; break;
    }
    // clang-format on
    return "<unknown VkPresentModeKHR value>";
}

VkAttachmentLoadOp ToVkAttachmentLoadOp(grfx::AttachmentLoadOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::ATTACHMENT_LOAD_OP_LOAD      : return VK_ATTACHMENT_LOAD_OP_LOAD; break;
        case grfx::ATTACHMENT_LOAD_OP_CLEAR     : return VK_ATTACHMENT_LOAD_OP_CLEAR; break;
        case grfx::ATTACHMENT_LOAD_OP_DONT_CARE : return VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkAttachmentLoadOp>();
}

VkAttachmentStoreOp ToVkAttachmentStoreOp(grfx::AttachmentStoreOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::ATTACHMENT_STORE_OP_STORE     : return VK_ATTACHMENT_STORE_OP_STORE; break;
        case grfx::ATTACHMENT_STORE_OP_DONT_CARE : return VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkAttachmentStoreOp>();
}

VkBlendFactor ToVkBlendFactor(grfx::BlendFactor value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::BLEND_FACTOR_ZERO                     : return VK_BLEND_FACTOR_ZERO                    ; break;
        case grfx::BLEND_FACTOR_ONE                      : return VK_BLEND_FACTOR_ONE                     ; break;
        case grfx::BLEND_FACTOR_SRC_COLOR                : return VK_BLEND_FACTOR_SRC_COLOR               ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC_COLOR      : return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR     ; break;
        case grfx::BLEND_FACTOR_DST_COLOR                : return VK_BLEND_FACTOR_DST_COLOR               ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_DST_COLOR      : return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR     ; break;
        case grfx::BLEND_FACTOR_SRC_ALPHA                : return VK_BLEND_FACTOR_SRC_ALPHA               ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      : return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA     ; break;
        case grfx::BLEND_FACTOR_DST_ALPHA                : return VK_BLEND_FACTOR_DST_ALPHA               ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_DST_ALPHA      : return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA     ; break;
        case grfx::BLEND_FACTOR_CONSTANT_COLOR           : return VK_BLEND_FACTOR_CONSTANT_COLOR          ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR : return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; break;
        case grfx::BLEND_FACTOR_CONSTANT_ALPHA           : return VK_BLEND_FACTOR_CONSTANT_ALPHA          ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA : return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; break;
        case grfx::BLEND_FACTOR_SRC_ALPHA_SATURATE       : return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE      ; break;
        case grfx::BLEND_FACTOR_SRC1_COLOR               : return VK_BLEND_FACTOR_SRC1_COLOR              ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC1_COLOR     : return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR    ; break;
        case grfx::BLEND_FACTOR_SRC1_ALPHA               : return VK_BLEND_FACTOR_SRC1_ALPHA              ; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA     : return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA    ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkBlendFactor>();
}

VkBlendOp ToVkBlendOp(grfx::BlendOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::BLEND_OP_ADD              : return VK_BLEND_OP_ADD             ; break;
        case grfx::BLEND_OP_SUBTRACT         : return VK_BLEND_OP_SUBTRACT        ; break;
        case grfx::BLEND_OP_REVERSE_SUBTRACT : return VK_BLEND_OP_REVERSE_SUBTRACT; break;
        case grfx::BLEND_OP_MIN              : return VK_BLEND_OP_MIN             ; break;
        case grfx::BLEND_OP_MAX              : return VK_BLEND_OP_MAX             ; break;

#if defined(PPX_VK_BLEND_OPERATION_ADVANCED)
        // Provdied by VK_blend_operation_advanced
        case grfx::BLEND_OP_ZERO               : return  VK_BLEND_OP_ZERO_EXT; break;
        case grfx::BLEND_OP_SRC                : return  VK_BLEND_OP_SRC_EXT; break;
        case grfx::BLEND_OP_DST                : return  VK_BLEND_OP_DST_EXT; break;
        case grfx::BLEND_OP_SRC_OVER           : return  VK_BLEND_OP_SRC_OVER_EXT; break;
        case grfx::BLEND_OP_DST_OVER           : return  VK_BLEND_OP_DST_OVER_EXT; break;
        case grfx::BLEND_OP_SRC_IN             : return  VK_BLEND_OP_SRC_IN_EXT; break;
        case grfx::BLEND_OP_DST_IN             : return  VK_BLEND_OP_DST_IN_EXT; break;
        case grfx::BLEND_OP_SRC_OUT            : return  VK_BLEND_OP_SRC_OUT_EXT; break;
        case grfx::BLEND_OP_DST_OUT            : return  VK_BLEND_OP_DST_OUT_EXT; break;
        case grfx::BLEND_OP_SRC_ATOP           : return  VK_BLEND_OP_SRC_ATOP_EXT; break;
        case grfx::BLEND_OP_DST_ATOP           : return  VK_BLEND_OP_DST_ATOP_EXT; break;
        case grfx::BLEND_OP_XOR                : return  VK_BLEND_OP_XOR_EXT; break;
        case grfx::BLEND_OP_MULTIPLY           : return  VK_BLEND_OP_MULTIPLY_EXT; break;
        case grfx::BLEND_OP_SCREEN             : return  VK_BLEND_OP_SCREEN_EXT; break;
        case grfx::BLEND_OP_OVERLAY            : return  VK_BLEND_OP_OVERLAY_EXT; break;
        case grfx::BLEND_OP_DARKEN             : return  VK_BLEND_OP_DARKEN_EXT; break;
        case grfx::BLEND_OP_LIGHTEN            : return  VK_BLEND_OP_LIGHTEN_EXT; break;
        case grfx::BLEND_OP_COLORDODGE         : return  VK_BLEND_OP_COLORDODGE_EXT; break;
        case grfx::BLEND_OP_COLORBURN          : return  VK_BLEND_OP_COLORBURN_EXT; break;
        case grfx::BLEND_OP_HARDLIGHT          : return  VK_BLEND_OP_HARDLIGHT_EXT; break;
        case grfx::BLEND_OP_SOFTLIGHT          : return  VK_BLEND_OP_SOFTLIGHT_EXT; break;
        case grfx::BLEND_OP_DIFFERENCE         : return  VK_BLEND_OP_DIFFERENCE_EXT; break;
        case grfx::BLEND_OP_EXCLUSION          : return  VK_BLEND_OP_EXCLUSION_EXT; break;
        case grfx::BLEND_OP_INVERT             : return  VK_BLEND_OP_INVERT_EXT; break;
        case grfx::BLEND_OP_INVERT_RGB         : return  VK_BLEND_OP_INVERT_RGB_EXT; break;
        case grfx::BLEND_OP_LINEARDODGE        : return  VK_BLEND_OP_LINEARDODGE_EXT; break;
        case grfx::BLEND_OP_LINEARBURN         : return  VK_BLEND_OP_LINEARBURN_EXT; break;
        case grfx::BLEND_OP_VIVIDLIGHT         : return  VK_BLEND_OP_VIVIDLIGHT_EXT; break;
        case grfx::BLEND_OP_LINEARLIGHT        : return  VK_BLEND_OP_LINEARLIGHT_EXT; break;
        case grfx::BLEND_OP_PINLIGHT           : return  VK_BLEND_OP_PINLIGHT_EXT; break;
        case grfx::BLEND_OP_HARDMIX            : return  VK_BLEND_OP_HARDMIX_EXT; break;
        case grfx::BLEND_OP_HSL_HUE            : return  VK_BLEND_OP_HSL_HUE_EXT; break;
        case grfx::BLEND_OP_HSL_SATURATION     : return  VK_BLEND_OP_HSL_SATURATION_EXT; break;
        case grfx::BLEND_OP_HSL_COLOR          : return  VK_BLEND_OP_HSL_COLOR_EXT; break;
        case grfx::BLEND_OP_HSL_LUMINOSITY     : return  VK_BLEND_OP_HSL_LUMINOSITY_EXT; break;
        case grfx::BLEND_OP_PLUS               : return  VK_BLEND_OP_PLUS_EXT; break;
        case grfx::BLEND_OP_PLUS_CLAMPED       : return  VK_BLEND_OP_PLUS_CLAMPED_EXT; break;
        case grfx::BLEND_OP_PLUS_CLAMPED_ALPHA : return  VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT; break;
        case grfx::BLEND_OP_PLUS_DARKER        : return  VK_BLEND_OP_PLUS_DARKER_EXT; break;
        case grfx::BLEND_OP_MINUS              : return  VK_BLEND_OP_MINUS_EXT; break;
        case grfx::BLEND_OP_MINUS_CLAMPED      : return  VK_BLEND_OP_MINUS_CLAMPED_EXT; break;
        case grfx::BLEND_OP_CONTRAST           : return  VK_BLEND_OP_CONTRAST_EXT; break;
        case grfx::BLEND_OP_INVERT_OVG         : return  VK_BLEND_OP_INVERT_OVG_EXT; break;
        case grfx::BLEND_OP_RED                : return  VK_BLEND_OP_RED_EXT; break;
        case grfx::BLEND_OP_GREEN              : return  VK_BLEND_OP_GREEN_EXT; break;
        case grfx::BLEND_OP_BLUE               : return  VK_BLEND_OP_BLUE_EXT; break;
#endif // defined(PPX_VK_BLEND_OPERATION_ADVANCED)
    }
    // clang-format on
    return ppx::InvalidValue<VkBlendOp>();
}

VkBorderColor ToVkBorderColor(grfx::BorderColor value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::BORDER_COLOR_FLOAT_TRANSPARENT_BLACK : return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK; break;
        case grfx::BORDER_COLOR_INT_TRANSPARENT_BLACK   : return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK  ; break;
        case grfx::BORDER_COLOR_FLOAT_OPAQUE_BLACK      : return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK     ; break;
        case grfx::BORDER_COLOR_INT_OPAQUE_BLACK        : return VK_BORDER_COLOR_INT_OPAQUE_BLACK       ; break;
        case grfx::BORDER_COLOR_FLOAT_OPAQUE_WHITE      : return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE     ; break;
        case grfx::BORDER_COLOR_INT_OPAQUE_WHITE        : return VK_BORDER_COLOR_INT_OPAQUE_WHITE       ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkBorderColor>();
}

VkBufferUsageFlags ToVkBufferUsageFlags(const grfx::BufferUsageFlags& value)
{
    VkBufferUsageFlags flags = 0;
    // clang-format off
    if (value.bits.transferSrc                   ) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (value.bits.transferDst                   ) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (value.bits.uniformTexelBuffer            ) flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    if (value.bits.storageTexelBuffer            ) flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if (value.bits.uniformBuffer                 ) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (value.bits.rawStorageBuffer              ) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (value.bits.roStructuredBuffer            ) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (value.bits.rwStructuredBuffer            ) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (value.bits.indexBuffer                   ) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (value.bits.vertexBuffer                  ) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (value.bits.indirectBuffer                ) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (value.bits.conditionalRendering          ) flags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
    if (value.bits.transformFeedbackBuffer       ) flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    if (value.bits.transformFeedbackCounterBuffer) flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
    if (value.bits.shaderDeviceAddress           ) flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    // clang-format on
    return flags;
}

VkChromaLocation ToVkChromaLocation(grfx::ChromaLocation value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::CHROMA_LOCATION_COSITED_EVEN : return VK_CHROMA_LOCATION_COSITED_EVEN;
        case grfx::CHROMA_LOCATION_MIDPOINT     : return VK_CHROMA_LOCATION_MIDPOINT;
    }
    // clang-format on
    return ppx::InvalidValue<VkChromaLocation>();
}

VkClearColorValue ToVkClearColorValue(const grfx::RenderTargetClearValue& value)
{
    VkClearColorValue res = {};
    res.float32[0]        = value.rgba[0];
    res.float32[1]        = value.rgba[1];
    res.float32[2]        = value.rgba[2];
    res.float32[3]        = value.rgba[3];
    return res;
}

VkClearDepthStencilValue ToVkClearDepthStencilValue(const grfx::DepthStencilClearValue& value)
{
    VkClearDepthStencilValue res = {};
    res.depth                    = value.depth;
    res.stencil                  = value.stencil;
    return res;
}

VkColorComponentFlags ToVkColorComponentFlags(const grfx::ColorComponentFlags& value)
{
    VkColorComponentFlags flags = 0;
    // clang-format off
    if (value.bits.r) flags |= VK_COLOR_COMPONENT_R_BIT;
    if (value.bits.g) flags |= VK_COLOR_COMPONENT_G_BIT;
    if (value.bits.b) flags |= VK_COLOR_COMPONENT_B_BIT;
    if (value.bits.a) flags |= VK_COLOR_COMPONENT_A_BIT;
    // clang-format on;
    return flags;
}

VkCompareOp ToVkCompareOp(grfx::CompareOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::COMPARE_OP_NEVER            : return VK_COMPARE_OP_NEVER           ; break;
        case grfx::COMPARE_OP_LESS             : return VK_COMPARE_OP_LESS            ; break;
        case grfx::COMPARE_OP_EQUAL            : return VK_COMPARE_OP_EQUAL           ; break;
        case grfx::COMPARE_OP_LESS_OR_EQUAL    : return VK_COMPARE_OP_LESS_OR_EQUAL   ; break;
        case grfx::COMPARE_OP_GREATER          : return VK_COMPARE_OP_GREATER         ; break;
        case grfx::COMPARE_OP_NOT_EQUAL        : return VK_COMPARE_OP_NOT_EQUAL       ; break;
        case grfx::COMPARE_OP_GREATER_OR_EQUAL : return VK_COMPARE_OP_GREATER_OR_EQUAL; break;
        case grfx::COMPARE_OP_ALWAYS           : return VK_COMPARE_OP_ALWAYS          ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkCompareOp>();
}

VkComponentSwizzle ToVkComponentSwizzle(grfx::ComponentSwizzle value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::COMPONENT_SWIZZLE_IDENTITY : return VK_COMPONENT_SWIZZLE_IDENTITY; break;
        case grfx::COMPONENT_SWIZZLE_ZERO     : return VK_COMPONENT_SWIZZLE_ZERO    ; break;
        case grfx::COMPONENT_SWIZZLE_ONE      : return VK_COMPONENT_SWIZZLE_ONE     ; break;
        case grfx::COMPONENT_SWIZZLE_R        : return VK_COMPONENT_SWIZZLE_R       ; break;
        case grfx::COMPONENT_SWIZZLE_G        : return VK_COMPONENT_SWIZZLE_G       ; break;
        case grfx::COMPONENT_SWIZZLE_B        : return VK_COMPONENT_SWIZZLE_B       ; break;
        case grfx::COMPONENT_SWIZZLE_A        : return VK_COMPONENT_SWIZZLE_A       ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkComponentSwizzle>();
}

VkComponentMapping ToVkComponentMapping(const grfx::ComponentMapping& value)
{
    VkComponentMapping res = {};
    res.r                  = ToVkComponentSwizzle(value.r);
    res.g                  = ToVkComponentSwizzle(value.g);
    res.b                  = ToVkComponentSwizzle(value.b);
    res.a                  = ToVkComponentSwizzle(value.a);
    return res;
}

VkCullModeFlagBits ToVkCullMode(grfx::CullMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::CULL_MODE_NONE  : return VK_CULL_MODE_NONE; break;
        case grfx::CULL_MODE_FRONT : return VK_CULL_MODE_FRONT_BIT; break;
        case grfx::CULL_MODE_BACK  : return VK_CULL_MODE_BACK_BIT; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkCullModeFlagBits>();
}

VkDescriptorBindingFlags ToVkDescriptorBindingFlags(const grfx::DescriptorBindingFlags& value)
{
    VkDescriptorBindingFlags flags = 0;
    if (value.bits.updatable) {
        flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }
    if (value.bits.partiallyBound) {
        flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }
    return flags;
}

VkDescriptorType ToVkDescriptorType(grfx::DescriptorType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::DESCRIPTOR_TYPE_SAMPLER                : return VK_DESCRIPTOR_TYPE_SAMPLER               ; break;
        case grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
        case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE          : return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         ; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE          : return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         ; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER   : return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  ; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER   : return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  ; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER         : return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        ; break;
        case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER     : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        ; break;
        case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER   : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        ; break;
        case grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER   : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        ; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; break;
        case grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT       : return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT      ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkDescriptorType>();
}

VkFilter ToVkFilter(grfx::Filter value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::FILTER_NEAREST : return VK_FILTER_NEAREST; break;
        case grfx::FILTER_LINEAR  : return VK_FILTER_LINEAR; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkFilter>();
}

VkFormat ToVkFormat(grfx::Format value)
{
    // clang-format off
    switch (value) {
        default: break;

        // 8-bit signed normalized
        case FORMAT_R8_SNORM           : return VK_FORMAT_R8_SNORM; break;
        case FORMAT_R8G8_SNORM         : return VK_FORMAT_R8G8_SNORM; break;
        case FORMAT_R8G8B8_SNORM       : return VK_FORMAT_R8G8B8_SNORM; break;
        case FORMAT_R8G8B8A8_SNORM     : return VK_FORMAT_R8G8B8A8_SNORM; break;
        case FORMAT_B8G8R8_SNORM       : return VK_FORMAT_B8G8R8_SNORM; break;
        case FORMAT_B8G8R8A8_SNORM     : return VK_FORMAT_B8G8R8A8_SNORM; break;

        // 8-bit unsigned normalized
        case FORMAT_R8_UNORM           : return VK_FORMAT_R8_UNORM; break;
        case FORMAT_R8G8_UNORM         : return VK_FORMAT_R8G8_UNORM; break;
        case FORMAT_R8G8B8_UNORM       : return VK_FORMAT_R8G8B8_UNORM; break;
        case FORMAT_R8G8B8A8_UNORM     : return VK_FORMAT_R8G8B8A8_UNORM; break;
        case FORMAT_B8G8R8_UNORM       : return VK_FORMAT_B8G8R8_UNORM; break;
        case FORMAT_B8G8R8A8_UNORM     : return VK_FORMAT_B8G8R8A8_UNORM; break;

        // 8-bit signed integer
        case FORMAT_R8_SINT            : return VK_FORMAT_R8_SINT; break;
        case FORMAT_R8G8_SINT          : return VK_FORMAT_R8G8_SINT; break;
        case FORMAT_R8G8B8_SINT        : return VK_FORMAT_R8G8B8_SINT; break;
        case FORMAT_R8G8B8A8_SINT      : return VK_FORMAT_R8G8B8A8_SINT; break;
        case FORMAT_B8G8R8_SINT        : return VK_FORMAT_B8G8R8_SINT; break;
        case FORMAT_B8G8R8A8_SINT      : return VK_FORMAT_B8G8R8A8_SINT; break;

        // 8-bit unsigned integer
        case FORMAT_R8_UINT            : return VK_FORMAT_R8_UINT; break;
        case FORMAT_R8G8_UINT          : return VK_FORMAT_R8G8_UINT; break;
        case FORMAT_R8G8B8_UINT        : return VK_FORMAT_R8G8B8_UINT; break;
        case FORMAT_R8G8B8A8_UINT      : return VK_FORMAT_R8G8B8A8_UINT; break;
        case FORMAT_B8G8R8_UINT        : return VK_FORMAT_B8G8R8_UINT; break;
        case FORMAT_B8G8R8A8_UINT      : return VK_FORMAT_B8G8R8A8_UINT; break;

        // 16-bit signed normalized
        case FORMAT_R16_SNORM          : return VK_FORMAT_R16_SNORM; break;
        case FORMAT_R16G16_SNORM       : return VK_FORMAT_R16G16_SNORM; break;
        case FORMAT_R16G16B16_SNORM    : return VK_FORMAT_R16G16B16_SNORM; break;
        case FORMAT_R16G16B16A16_SNORM : return VK_FORMAT_R16G16B16A16_SNORM; break;

        // 16-bit unsigned normalized
        case FORMAT_R16_UNORM          : return VK_FORMAT_R16_UNORM; break;
        case FORMAT_R16G16_UNORM       : return VK_FORMAT_R16G16_UNORM; break;
        case FORMAT_R16G16B16_UNORM    : return VK_FORMAT_R16G16B16_UNORM; break;
        case FORMAT_R16G16B16A16_UNORM : return VK_FORMAT_R16G16B16A16_UNORM; break;

        // 16-bit signed integer
        case FORMAT_R16_SINT           : return VK_FORMAT_R16_SINT; break;
        case FORMAT_R16G16_SINT        : return VK_FORMAT_R16G16_SINT; break;
        case FORMAT_R16G16B16_SINT     : return VK_FORMAT_R16G16B16_SINT; break;
        case FORMAT_R16G16B16A16_SINT  : return VK_FORMAT_R16G16B16A16_SINT; break;

        // 16-bit unsigned integer
        case FORMAT_R16_UINT           : return VK_FORMAT_R16_UINT; break;
        case FORMAT_R16G16_UINT        : return VK_FORMAT_R16G16_UINT; break;
        case FORMAT_R16G16B16_UINT     : return VK_FORMAT_R16G16B16_UINT; break;
        case FORMAT_R16G16B16A16_UINT  : return VK_FORMAT_R16G16B16A16_UINT; break;

        // 16-bit float
        case FORMAT_R16_FLOAT          : return VK_FORMAT_R16_SFLOAT; break;
        case FORMAT_R16G16_FLOAT       : return VK_FORMAT_R16G16_SFLOAT; break;
        case FORMAT_R16G16B16_FLOAT    : return VK_FORMAT_R16G16B16_SFLOAT; break;
        case FORMAT_R16G16B16A16_FLOAT : return VK_FORMAT_R16G16B16A16_SFLOAT; break;

        // 32-bit signed integer
        case FORMAT_R32_SINT           : return VK_FORMAT_R32_SINT; break;
        case FORMAT_R32G32_SINT        : return VK_FORMAT_R32G32_SINT; break;
        case FORMAT_R32G32B32_SINT     : return VK_FORMAT_R32G32B32_SINT; break;
        case FORMAT_R32G32B32A32_SINT  : return VK_FORMAT_R32G32B32A32_SINT; break;

        // 32-bit unsigned integer
        case FORMAT_R32_UINT           : return VK_FORMAT_R32_UINT; break;
        case FORMAT_R32G32_UINT        : return VK_FORMAT_R32G32_UINT; break;
        case FORMAT_R32G32B32_UINT     : return VK_FORMAT_R32G32B32_UINT; break;
        case FORMAT_R32G32B32A32_UINT  : return VK_FORMAT_R32G32B32A32_UINT; break;

        // 32-bit float
        case FORMAT_R32_FLOAT          : return VK_FORMAT_R32_SFLOAT; break;
        case FORMAT_R32G32_FLOAT       : return VK_FORMAT_R32G32_SFLOAT; break;
        case FORMAT_R32G32B32_FLOAT    : return VK_FORMAT_R32G32B32_SFLOAT; break;
        case FORMAT_R32G32B32A32_FLOAT : return VK_FORMAT_R32G32B32A32_SFLOAT; break;

        // 8-bit unsigned integer stencil
        case FORMAT_S8_UINT            : return VK_FORMAT_S8_UINT; break;

        // 16-bit unsigned normalized depth
        case FORMAT_D16_UNORM          : return VK_FORMAT_D16_UNORM; break;

        // 32-bit float depth
        case FORMAT_D32_FLOAT          : return VK_FORMAT_D32_SFLOAT; break;

        // Depth/stencil combinations
        case FORMAT_D16_UNORM_S8_UINT  : return VK_FORMAT_D16_UNORM_S8_UINT; break;
        case FORMAT_D24_UNORM_S8_UINT  : return VK_FORMAT_D24_UNORM_S8_UINT; break;
        case FORMAT_D32_FLOAT_S8_UINT  : return VK_FORMAT_D32_SFLOAT_S8_UINT; break;

        // SRGB
        case FORMAT_R8_SRGB            : return VK_FORMAT_R8_SRGB; break;
        case FORMAT_R8G8_SRGB          : return VK_FORMAT_R8G8_SRGB; break;
        case FORMAT_R8G8B8_SRGB        : return VK_FORMAT_R8G8B8_SRGB; break;
        case FORMAT_R8G8B8A8_SRGB      : return VK_FORMAT_R8G8B8A8_SRGB; break;
        case FORMAT_B8G8R8_SRGB        : return VK_FORMAT_B8G8R8_SRGB; break;
        case FORMAT_B8G8R8A8_SRGB      : return VK_FORMAT_B8G8R8A8_SRGB; break;

        // 10-bit
        case FORMAT_R10G10B10A2_UNORM  : return VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;

        // 11-bit R, 11-bit G, 10-bit B packed
        case FORMAT_R11G11B10_FLOAT    : return VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;

        // Compressed images
        case FORMAT_BC1_RGBA_SRGB       : return VK_FORMAT_BC1_RGBA_SRGB_BLOCK; break;
        case FORMAT_BC1_RGBA_UNORM      : return VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
        case FORMAT_BC1_RGB_SRGB        : return VK_FORMAT_BC1_RGB_SRGB_BLOCK; break;
        case FORMAT_BC1_RGB_UNORM       : return VK_FORMAT_BC1_RGB_UNORM_BLOCK; break;
        case FORMAT_BC2_SRGB            : return VK_FORMAT_BC2_SRGB_BLOCK; break;
        case FORMAT_BC2_UNORM           : return VK_FORMAT_BC2_UNORM_BLOCK; break;
        case FORMAT_BC3_SRGB            : return VK_FORMAT_BC3_SRGB_BLOCK; break;
        case FORMAT_BC3_UNORM           : return VK_FORMAT_BC3_UNORM_BLOCK; break;
        case FORMAT_BC4_UNORM           : return VK_FORMAT_BC4_UNORM_BLOCK; break;
        case FORMAT_BC4_SNORM           : return VK_FORMAT_BC4_SNORM_BLOCK; break;
        case FORMAT_BC5_UNORM           : return VK_FORMAT_BC5_UNORM_BLOCK; break;
        case FORMAT_BC5_SNORM           : return VK_FORMAT_BC5_SNORM_BLOCK; break;
        case FORMAT_BC6H_UFLOAT         : return VK_FORMAT_BC6H_UFLOAT_BLOCK; break;
        case FORMAT_BC6H_SFLOAT         : return VK_FORMAT_BC6H_SFLOAT_BLOCK; break;
        case FORMAT_BC7_UNORM           : return VK_FORMAT_BC7_UNORM_BLOCK; break;
        case FORMAT_BC7_SRGB            : return VK_FORMAT_BC7_SRGB_BLOCK; break;
    }
    // clang-format on

    return VK_FORMAT_UNDEFINED;
}

VkFrontFace ToVkFrontFace(grfx::FrontFace value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::FRONT_FACE_CCW : return VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
        case grfx::FRONT_FACE_CW  : return VK_FRONT_FACE_CLOCKWISE; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkFrontFace>();
}

VkImageType ToVkImageType(grfx::ImageType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::IMAGE_TYPE_1D   : return VK_IMAGE_TYPE_1D; break;
        case grfx::IMAGE_TYPE_2D   : return VK_IMAGE_TYPE_2D; break;
        case grfx::IMAGE_TYPE_3D   : return VK_IMAGE_TYPE_3D; break;
        case grfx::IMAGE_TYPE_CUBE : return VK_IMAGE_TYPE_2D; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkImageType>();
}

VkImageUsageFlags ToVkImageUsageFlags(const grfx::ImageUsageFlags& value)
{
    VkImageUsageFlags flags = 0;
    // clang-format off
    if (value.bits.transferSrc           ) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (value.bits.transferDst           ) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (value.bits.sampled               ) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (value.bits.storage               ) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (value.bits.colorAttachment       ) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (value.bits.depthStencilAttachment) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (value.bits.transientAttachment   ) flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    if (value.bits.inputAttachment       ) flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (value.bits.fragmentDensityMap    ) flags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
    if (value.bits.fragmentShadingRateAttachment) flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    // clang-format on
    return flags;
}

VkImageViewType ToVkImageViewType(grfx::ImageViewType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::IMAGE_VIEW_TYPE_1D         : return VK_IMAGE_VIEW_TYPE_1D; break;
        case grfx::IMAGE_VIEW_TYPE_2D         : return VK_IMAGE_VIEW_TYPE_2D; break;
        case grfx::IMAGE_VIEW_TYPE_3D         : return VK_IMAGE_VIEW_TYPE_3D; break;
        case grfx::IMAGE_VIEW_TYPE_CUBE       : return VK_IMAGE_VIEW_TYPE_CUBE; break;
        case grfx::IMAGE_VIEW_TYPE_1D_ARRAY   : return VK_IMAGE_VIEW_TYPE_1D_ARRAY; break;
        case grfx::IMAGE_VIEW_TYPE_2D_ARRAY   : return VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
        case grfx::IMAGE_VIEW_TYPE_CUBE_ARRAY : return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkImageViewType>();
}

VkIndexType ToVkIndexType(grfx::IndexType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::INDEX_TYPE_UINT16 : return VK_INDEX_TYPE_UINT16; break;
        case grfx::INDEX_TYPE_UINT32 : return VK_INDEX_TYPE_UINT32; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkIndexType>();
}

VkLogicOp ToVkLogicOp(grfx::LogicOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::LOGIC_OP_CLEAR         : return VK_LOGIC_OP_CLEAR; break;
        case grfx::LOGIC_OP_AND           : return VK_LOGIC_OP_AND; break;
        case grfx::LOGIC_OP_AND_REVERSE   : return VK_LOGIC_OP_AND_REVERSE; break;
        case grfx::LOGIC_OP_COPY          : return VK_LOGIC_OP_COPY; break;
        case grfx::LOGIC_OP_AND_INVERTED  : return VK_LOGIC_OP_AND_INVERTED; break;
        case grfx::LOGIC_OP_NO_OP         : return VK_LOGIC_OP_NO_OP; break;
        case grfx::LOGIC_OP_XOR           : return VK_LOGIC_OP_XOR; break;
        case grfx::LOGIC_OP_OR            : return VK_LOGIC_OP_OR; break;
        case grfx::LOGIC_OP_NOR           : return VK_LOGIC_OP_NOR; break;
        case grfx::LOGIC_OP_EQUIVALENT    : return VK_LOGIC_OP_EQUIVALENT; break;
        case grfx::LOGIC_OP_INVERT        : return VK_LOGIC_OP_INVERT; break;
        case grfx::LOGIC_OP_OR_REVERSE    : return VK_LOGIC_OP_OR_REVERSE; break;
        case grfx::LOGIC_OP_COPY_INVERTED : return VK_LOGIC_OP_COPY_INVERTED; break;
        case grfx::LOGIC_OP_OR_INVERTED   : return VK_LOGIC_OP_OR_INVERTED; break;
        case grfx::LOGIC_OP_NAND          : return VK_LOGIC_OP_NAND; break;
        case grfx::LOGIC_OP_SET           : return VK_LOGIC_OP_SET; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkLogicOp>();
}

VkPipelineStageFlagBits ToVkPipelineStage(grfx::PipelineStage value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT    : return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; break;
        case grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkPipelineStageFlagBits>();
}

VkPolygonMode ToVkPolygonMode(grfx::PolygonMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::POLYGON_MODE_FILL  : return VK_POLYGON_MODE_FILL; break;
        case grfx::POLYGON_MODE_LINE  : return VK_POLYGON_MODE_LINE; break;
        case grfx::POLYGON_MODE_POINT : return VK_POLYGON_MODE_POINT; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkPolygonMode>();
}

VkPresentModeKHR ToVkPresentMode(grfx::PresentMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::PRESENT_MODE_FIFO      : return VK_PRESENT_MODE_FIFO_KHR; break;
        case grfx::PRESENT_MODE_MAILBOX   : return VK_PRESENT_MODE_MAILBOX_KHR; break;
        case grfx::PRESENT_MODE_IMMEDIATE : return VK_PRESENT_MODE_IMMEDIATE_KHR; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkPresentModeKHR>();
}

VkPrimitiveTopology ToVkPrimitiveTopology(grfx::PrimitiveTopology value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST  : return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN   : return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
        case grfx::PRIMITIVE_TOPOLOGY_POINT_LIST     : return VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_LIST      : return VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_STRIP     : return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkPrimitiveTopology>();
}

VkQueryType ToVkQueryType(grfx::QueryType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::QUERY_TYPE_OCCLUSION           : return VK_QUERY_TYPE_OCCLUSION; break;
        case grfx::QUERY_TYPE_TIMESTAMP           : return VK_QUERY_TYPE_TIMESTAMP; break;
        case grfx::QUERY_TYPE_PIPELINE_STATISTICS : return VK_QUERY_TYPE_PIPELINE_STATISTICS; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkQueryType>();
}

VkSamplerAddressMode ToVkSamplerAddressMode(grfx::SamplerAddressMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::SAMPLER_ADDRESS_MODE_REPEAT           : return VK_SAMPLER_ADDRESS_MODE_REPEAT         ; break;
        case grfx::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT  : return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
        case grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE    : return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE  ; break;
        case grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER  : return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkSamplerAddressMode>();
}

VkSamplerMipmapMode ToVkSamplerMipmapMode(grfx::SamplerMipmapMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::SAMPLER_MIPMAP_MODE_NEAREST : return VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
        case grfx::SAMPLER_MIPMAP_MODE_LINEAR  : return VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkSamplerMipmapMode>();
}

VkSampleCountFlagBits ToVkSampleCount(grfx::SampleCount value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::SAMPLE_COUNT_1  : return VK_SAMPLE_COUNT_1_BIT; break;
        case grfx::SAMPLE_COUNT_2  : return VK_SAMPLE_COUNT_2_BIT; break;
        case grfx::SAMPLE_COUNT_4  : return VK_SAMPLE_COUNT_4_BIT; break;
        case grfx::SAMPLE_COUNT_8  : return VK_SAMPLE_COUNT_8_BIT; break;
        case grfx::SAMPLE_COUNT_16 : return VK_SAMPLE_COUNT_16_BIT; break;
        case grfx::SAMPLE_COUNT_32 : return VK_SAMPLE_COUNT_32_BIT; break;
        case grfx::SAMPLE_COUNT_64 : return VK_SAMPLE_COUNT_64_BIT; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkSampleCountFlagBits>();
}

VkShaderStageFlags ToVkShaderStageFlags(const grfx::ShaderStageFlags& value)
{
    VkShaderStageFlags flags = 0;
    // clang-format off
    if (value.bits.VS) flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (value.bits.HS) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (value.bits.DS) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (value.bits.GS) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (value.bits.PS) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (value.bits.CS) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    // clang-format on
    return flags;
}

VkStencilOp ToVkStencilOp(grfx::StencilOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::STENCIL_OP_KEEP                : return VK_STENCIL_OP_KEEP               ; break;
        case grfx::STENCIL_OP_ZERO                : return VK_STENCIL_OP_ZERO               ; break;
        case grfx::STENCIL_OP_REPLACE             : return VK_STENCIL_OP_REPLACE            ; break;
        case grfx::STENCIL_OP_INCREMENT_AND_CLAMP : return VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
        case grfx::STENCIL_OP_DECREMENT_AND_CLAMP : return VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
        case grfx::STENCIL_OP_INVERT              : return VK_STENCIL_OP_INVERT             ; break;
        case grfx::STENCIL_OP_INCREMENT_AND_WRAP  : return VK_STENCIL_OP_INCREMENT_AND_WRAP ; break;
        case grfx::STENCIL_OP_DECREMENT_AND_WRAP  : return VK_STENCIL_OP_DECREMENT_AND_WRAP ; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkStencilOp>();
}

VkTessellationDomainOrigin ToVkTessellationDomainOrigin(grfx::TessellationDomainOrigin value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT : return VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT; break;
        case grfx::TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT : return VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkTessellationDomainOrigin>();
}

VkVertexInputRate ToVkVertexInputRate(grfx::VertexInputRate value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::VERTEX_INPUT_RATE_VERTEX   : return VK_VERTEX_INPUT_RATE_VERTEX; break;
        case grfx::VERETX_INPUT_RATE_INSTANCE : return VK_VERTEX_INPUT_RATE_INSTANCE; break;
    }
    // clang-format on
    return ppx::InvalidValue<VkVertexInputRate>();
}

static Result ToVkBarrier(
    ResourceState                   state,
    grfx::CommandType               commandType,
    const VkPhysicalDeviceFeatures& features,
    bool                            isSource,
    VkPipelineStageFlags&           stageMask,
    VkAccessFlags&                  accessMask,
    VkImageLayout&                  layout)
{
    VkPipelineStageFlags PIPELINE_STAGE_ALL_SHADER_STAGES = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if (commandType == grfx::CommandType::COMMAND_TYPE_GRAPHICS) {
        PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    VkPipelineStageFlags PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if (commandType == grfx::CommandType::COMMAND_TYPE_GRAPHICS) {
        PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }

    if (commandType == grfx::CommandType::COMMAND_TYPE_GRAPHICS && features.geometryShader) {
        PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    }
    if (commandType == grfx::CommandType::COMMAND_TYPE_GRAPHICS && features.tessellationShader) {
        PIPELINE_STAGE_ALL_SHADER_STAGES |=
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |=
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    }

    switch (state) {
        default: return ppx::ERROR_FAILED; break;

        case grfx::RESOURCE_STATE_UNDEFINED: {
            stageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_UNDEFINED;
        } break;

        case grfx::RESOURCE_STATE_GENERAL: {
            stageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_GENERAL;
        } break;

        case grfx::RESOURCE_STATE_CONSTANT_BUFFER:
        case grfx::RESOURCE_STATE_VERTEX_BUFFER: {
            stageMask  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | PIPELINE_STAGE_ALL_SHADER_STAGES;
            accessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_INDEX_BUFFER: {
            stageMask  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            accessMask = VK_ACCESS_INDEX_READ_BIT;
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_RENDER_TARGET: {
            stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_UNORDERED_ACCESS: {
            stageMask  = PIPELINE_STAGE_ALL_SHADER_STAGES;
            accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_GENERAL;
        } break;

        case grfx::RESOURCE_STATE_DEPTH_STENCIL_READ: {
            stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE: {
            stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_DEPTH_WRITE_STENCIL_READ: {
            stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_DEPTH_READ_STENCIL_WRITE: {
            stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: {
            stageMask  = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES;
            accessMask = VK_ACCESS_SHADER_READ_BIT;
            layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_SHADER_RESOURCE: {
            stageMask  = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES;
            accessMask = VK_ACCESS_SHADER_READ_BIT;
            layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE: {
            stageMask  = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            accessMask = VK_ACCESS_SHADER_READ_BIT;
            layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_STREAM_OUT: {
            stageMask  = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
            accessMask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_INDIRECT_ARGUMENT: {
            stageMask  = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            accessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_COPY_SRC: {
            stageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            accessMask = VK_ACCESS_TRANSFER_READ_BIT;
            layout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_COPY_DST: {
            stageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_RESOLVE_SRC: {
            stageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            accessMask = VK_ACCESS_TRANSFER_READ_BIT;
            layout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_RESOLVE_DST: {
            stageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        } break;

        case grfx::RESOURCE_STATE_PRESENT: {
            stageMask  = isSource ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            layout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        } break;

        case grfx::RESOURCE_STATE_PREDICATION: {
            stageMask  = VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
            accessMask = VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE: {
            stageMask  = InvalidValue<VkPipelineStageFlags>();
            accessMask = InvalidValue<VkAccessFlags>();
            layout     = InvalidValue<VkImageLayout>();
        } break;

        case grfx::RESOURCE_STATE_FRAGMENT_DENSITY_MAP_ATTACHMENT: {
            stageMask  = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
            accessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
            layout     = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        } break;

        case grfx::RESOURCE_STATE_FRAGMENT_SHADING_RATE_ATTACHMENT: {
            stageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            accessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            layout     = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        } break;
    }

    return ppx::SUCCESS;
}

Result ToVkBarrierSrc(
    ResourceState                   state,
    grfx::CommandType               commandType,
    const VkPhysicalDeviceFeatures& features,
    VkPipelineStageFlags&           stageMask,
    VkAccessFlags&                  accessMask,
    VkImageLayout&                  layout)
{
    return ToVkBarrier(state, commandType, features, true, stageMask, accessMask, layout);
}

Result ToVkBarrierDst(
    ResourceState                   state,
    grfx::CommandType               commandType,
    const VkPhysicalDeviceFeatures& features,
    VkPipelineStageFlags&           stageMask,
    VkAccessFlags&                  accessMask,
    VkImageLayout&                  layout)
{
    return ToVkBarrier(state, commandType, features, false, stageMask, accessMask, layout);
}

VkImageAspectFlags DetermineAspectMask(VkFormat format)
{
    // clang-format off
    switch (format) {
        // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT: {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        } break;

        // Stencil
        case VK_FORMAT_S8_UINT: {
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        } break;

        // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT: {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        } break;

            // Assume everything else is color
        default: break;
    }
    // clang-format on
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VmaMemoryUsage ToVmaMemoryUsage(grfx::MemoryUsage value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::MEMORY_USAGE_GPU_ONLY   : return VMA_MEMORY_USAGE_GPU_ONLY  ; break;
        case grfx::MEMORY_USAGE_CPU_ONLY   : return VMA_MEMORY_USAGE_CPU_ONLY  ; break;
        case grfx::MEMORY_USAGE_CPU_TO_GPU : return VMA_MEMORY_USAGE_CPU_TO_GPU; break;
        case grfx::MEMORY_USAGE_GPU_TO_CPU : return VMA_MEMORY_USAGE_GPU_TO_CPU; break;
    }
    // clang-forat on
    return VMA_MEMORY_USAGE_UNKNOWN;
}

VkSamplerYcbcrModelConversion ToVkYcbcrModelConversion(grfx::YcbcrModelConversion value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::YCBCR_MODEL_CONVERSION_RGB_IDENTITY   : return VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
        case grfx::YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY : return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY;
        case grfx::YCBCR_MODEL_CONVERSION_YCBCR_709      : return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
        case grfx::YCBCR_MODEL_CONVERSION_YCBCR_601      : return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
        case grfx::YCBCR_MODEL_CONVERSION_YCBCR_2020     : return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
    }
    // clang-format on
    return ppx::InvalidValue<VkSamplerYcbcrModelConversion>();
}

VkSamplerYcbcrRange ToVkYcbcrRange(grfx::YcbcrRange value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::YCBCR_RANGE_ITU_FULL   : return VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
        case grfx::YCBCR_RANGE_ITU_NARROW : return VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
    }
    // clang-format on
    return ppx::InvalidValue<VkSamplerYcbcrRange>();
}

} // namespace vk
} // namespace grfx
} // namespace ppx
