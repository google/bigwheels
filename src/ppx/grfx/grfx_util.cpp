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

#include "ppx/grfx/grfx_util.h"
#include "gli/target.hpp"

namespace ppx {
namespace grfx {

const char* ToString(grfx::Api value)
{
    switch (value) {
        default: break;
        case grfx::API_VK_1_1: return "Vulkan 1.1"; break;
        case grfx::API_VK_1_2: return "Vulkan 1.2"; break;
        case grfx::API_DX_11_0: return "Direct3D 11.0"; break;
        case grfx::API_DX_11_1: return "Direct3D 11.1"; break;
        case grfx::API_DX_12_0: return "Direct3D 12.0"; break;
        case grfx::API_DX_12_1: return "Direct3D 12.1"; break;
    }
    return "<unknown graphics API>";
}

const char* ToString(grfx::DescriptorType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::DESCRIPTOR_TYPE_SAMPLER                : return "grfx::DESCRIPTOR_TYPE_SAMPLER"; break;
        case grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : return "grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"; break;
        case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE          : return "grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE"; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE          : return "grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE"; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER   : return "grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  "; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER   : return "grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  "; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER         : return "grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER"; break;
        case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER     : return "grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER"; break;
        case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : return "grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"; break;
        case grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : return "grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC"; break;
        case grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT       : return "grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT"; break;
    }
    // clang-format on
    return "<unknown descriptor type>";
}

const char* ToString(grfx::VertexSemantic value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::VERTEX_SEMANTIC_POSITION   : return "POSITION"; break;
        case grfx::VERTEX_SEMANTIC_NORMAL     : return "NORMAL"; break;
        case grfx::VERTEX_SEMANTIC_COLOR      : return "COLOR"; break;
        case grfx::VERTEX_SEMANTIC_TANGENT    : return "TANGENT"; break;
        case grfx::VERTEX_SEMANTIC_BITANGENT  : return "BITANGENT"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD   : return "TEXCOORD"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD0  : return "TEXCOORD0"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD1  : return "TEXCOORD1"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD2  : return "TEXCOORD2"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD3  : return "TEXCOORD3"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD4  : return "TEXCOORD4"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD5  : return "TEXCOORD5"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD6  : return "TEXCOORD6"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD7  : return "TEXCOORD7"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD8  : return "TEXCOORD8"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD9  : return "TEXCOORD9"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD10 : return "TEXCOORD10"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD11 : return "TEXCOORD11"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD12 : return "TEXCOORD12"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD13 : return "TEXCOORD13"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD14 : return "TEXCOORD14"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD15 : return "TEXCOORD15"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD16 : return "TEXCOORD16"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD17 : return "TEXCOORD17"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD18 : return "TEXCOORD18"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD19 : return "TEXCOORD19"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD20 : return "TEXCOORD20"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD21 : return "TEXCOORD21"; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD22 : return "TEXCOORD22"; break;       
    }
    // clang-format on
    return "";
}

uint32_t IndexTypeSize(grfx::IndexType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::INDEX_TYPE_UINT16: return sizeof(uint16_t); break;
        case grfx::INDEX_TYPE_UINT32: return sizeof(uint32_t); break;
    }
    // clang-format on
    return 0;
}

grfx::Format VertexSemanticFormat(grfx::VertexSemantic value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::VERTEX_SEMANTIC_POSITION   : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_NORMAL     : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_COLOR      : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TANGENT    : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_BITANGENT  : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD   : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD0  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD1  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD2  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD3  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD4  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD5  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD6  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD7  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD8  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD9  : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD10 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD11 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD12 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD13 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD14 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD15 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD16 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD17 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD18 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD19 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD20 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD21 : return grfx::FORMAT_R32G32_FLOAT; break;
        case grfx::VERTEX_SEMANTIC_TEXCOORD22 : return grfx::FORMAT_R32G32_FLOAT; break;       
    }
    // clang-format on
    return grfx::FORMAT_UNDEFINED;
}

const char* ToString(const gli::target& target)
{
    // clang-format off
    switch (target) {
        case gli::TARGET_1D:         return "TARGET_1D";
        case gli::TARGET_1D_ARRAY:   return "TARGET_1D_ARRAY";
        case gli::TARGET_2D:         return "TARGET_2D";
        case gli::TARGET_2D_ARRAY:   return "TARGET_2D_ARRAY";
        case gli::TARGET_3D:         return "TARGET_3D";
        case gli::TARGET_RECT:       return "TARGET_RECT";
        case gli::TARGET_RECT_ARRAY: return "TARGET_RECT_ARRAY";
        case gli::TARGET_CUBE:       return "TARGET_CUBE";
        case gli::TARGET_CUBE_ARRAY: return "TARGET_CUBE_ARRAY";
    }
    // clang-format on
    return "TARGET_UNKNOWN";
}

const char* ToString(const gli::format& format)
{
    switch (format) {
        case gli::FORMAT_UNDEFINED: return "FORMAT_UNDEFINED";
        case gli::FORMAT_RG4_UNORM_PACK8: return "FORMAT_RG4_UNORM_PACK8";
        case gli::FORMAT_RGBA4_UNORM_PACK16: return "FORMAT_RGBA4_UNORM_PACK16";
        case gli::FORMAT_BGRA4_UNORM_PACK16: return "FORMAT_BGRA4_UNORM_PACK16";
        case gli::FORMAT_R5G6B5_UNORM_PACK16: return "FORMAT_R5G6B5_UNORM_PACK16";
        case gli::FORMAT_B5G6R5_UNORM_PACK16: return "FORMAT_B5G6R5_UNORM_PACK16";
        case gli::FORMAT_RGB5A1_UNORM_PACK16: return "FORMAT_RGB5A1_UNORM_PACK16";
        case gli::FORMAT_BGR5A1_UNORM_PACK16: return "FORMAT_BGR5A1_UNORM_PACK16";
        case gli::FORMAT_A1RGB5_UNORM_PACK16: return "FORMAT_A1RGB5_UNORM_PACK16";

        case gli::FORMAT_R8_UNORM_PACK8: return "FORMAT_R8_UNORM_PACK8";
        case gli::FORMAT_R8_SNORM_PACK8: return "FORMAT_R8_SNORM_PACK8";
        case gli::FORMAT_R8_USCALED_PACK8: return "FORMAT_R8_USCALED_PACK8";
        case gli::FORMAT_R8_SSCALED_PACK8: return "FORMAT_R8_SSCALED_PACK8";
        case gli::FORMAT_R8_UINT_PACK8: return "FORMAT_R8_UINT_PACK8";
        case gli::FORMAT_R8_SINT_PACK8: return "FORMAT_R8_SINT_PACK8";
        case gli::FORMAT_R8_SRGB_PACK8: return "FORMAT_R8_SRGB_PACK8";

        case gli::FORMAT_RG8_UNORM_PACK8: return "FORMAT_RG8_UNORM_PACK8";
        case gli::FORMAT_RG8_SNORM_PACK8: return "FORMAT_RG8_SNORM_PACK8";
        case gli::FORMAT_RG8_USCALED_PACK8: return "FORMAT_RG8_USCALED_PACK8";
        case gli::FORMAT_RG8_SSCALED_PACK8: return "FORMAT_RG8_SSCALED_PACK8";
        case gli::FORMAT_RG8_UINT_PACK8: return "FORMAT_RG8_UINT_PACK8";
        case gli::FORMAT_RG8_SINT_PACK8: return "FORMAT_RG8_SINT_PACK8";
        case gli::FORMAT_RG8_SRGB_PACK8: return "FORMAT_RG8_SRGB_PACK8";

        case gli::FORMAT_RGB8_UNORM_PACK8: return "FORMAT_RGB8_UNORM_PACK8";
        case gli::FORMAT_RGB8_SNORM_PACK8: return "FORMAT_RGB8_SNORM_PACK8";
        case gli::FORMAT_RGB8_USCALED_PACK8: return "FORMAT_RGB8_USCALED_PACK8";
        case gli::FORMAT_RGB8_SSCALED_PACK8: return "FORMAT_RGB8_SSCALED_PACK8";
        case gli::FORMAT_RGB8_UINT_PACK8: return "FORMAT_RGB8_UINT_PACK8";
        case gli::FORMAT_RGB8_SINT_PACK8: return "FORMAT_RGB8_SINT_PACK8";
        case gli::FORMAT_RGB8_SRGB_PACK8: return "FORMAT_RGB8_SRGB_PACK8";

        case gli::FORMAT_BGR8_UNORM_PACK8: return "FORMAT_BGR8_UNORM_PACK8";
        case gli::FORMAT_BGR8_SNORM_PACK8: return "FORMAT_BGR8_SNORM_PACK8";
        case gli::FORMAT_BGR8_USCALED_PACK8: return "FORMAT_BGR8_USCALED_PACK8";
        case gli::FORMAT_BGR8_SSCALED_PACK8: return "FORMAT_BGR8_SSCALED_PACK8";
        case gli::FORMAT_BGR8_UINT_PACK8: return "FORMAT_BGR8_UINT_PACK8";
        case gli::FORMAT_BGR8_SINT_PACK8: return "FORMAT_BGR8_SINT_PACK8";
        case gli::FORMAT_BGR8_SRGB_PACK8: return "FORMAT_BGR8_SRGB_PACK8";

        case gli::FORMAT_RGBA8_UNORM_PACK8: return "FORMAT_RGBA8_UNORM_PACK8";
        case gli::FORMAT_RGBA8_SNORM_PACK8: return "FORMAT_RGBA8_SNORM_PACK8";
        case gli::FORMAT_RGBA8_USCALED_PACK8: return "FORMAT_RGBA8_USCALED_PACK8";
        case gli::FORMAT_RGBA8_SSCALED_PACK8: return "FORMAT_RGBA8_SSCALED_PACK8";
        case gli::FORMAT_RGBA8_UINT_PACK8: return "FORMAT_RGBA8_UINT_PACK8";
        case gli::FORMAT_RGBA8_SINT_PACK8: return "FORMAT_RGBA8_SINT_PACK8";
        case gli::FORMAT_RGBA8_SRGB_PACK8: return "FORMAT_RGBA8_SRGB_PACK8";

        case gli::FORMAT_BGRA8_UNORM_PACK8: return "FORMAT_BGRA8_UNORM_PACK8";
        case gli::FORMAT_BGRA8_SNORM_PACK8: return "FORMAT_BGRA8_SNORM_PACK8";
        case gli::FORMAT_BGRA8_USCALED_PACK8: return "FORMAT_BGRA8_USCALED_PACK8";
        case gli::FORMAT_BGRA8_SSCALED_PACK8: return "FORMAT_BGRA8_SSCALED_PACK8";
        case gli::FORMAT_BGRA8_UINT_PACK8: return "FORMAT_BGRA8_UINT_PACK8";
        case gli::FORMAT_BGRA8_SINT_PACK8: return "FORMAT_BGRA8_SINT_PACK8";
        case gli::FORMAT_BGRA8_SRGB_PACK8: return "FORMAT_BGRA8_SRGB_PACK8";

        case gli::FORMAT_RGBA8_UNORM_PACK32: return "FORMAT_RGBA8_UNORM_PACK32";
        case gli::FORMAT_RGBA8_SNORM_PACK32: return "FORMAT_RGBA8_SNORM_PACK32";
        case gli::FORMAT_RGBA8_USCALED_PACK32: return "FORMAT_RGBA8_USCALED_PACK32";
        case gli::FORMAT_RGBA8_SSCALED_PACK32: return "FORMAT_RGBA8_SSCALED_PACK32";
        case gli::FORMAT_RGBA8_UINT_PACK32: return "FORMAT_RGBA8_UINT_PACK32";
        case gli::FORMAT_RGBA8_SINT_PACK32: return "FORMAT_RGBA8_SINT_PACK32";
        case gli::FORMAT_RGBA8_SRGB_PACK32: return "FORMAT_RGBA8_SRGB_PACK32";

        case gli::FORMAT_RGB10A2_UNORM_PACK32: return "FORMAT_RGB10A2_UNORM_PACK32";
        case gli::FORMAT_RGB10A2_SNORM_PACK32: return "FORMAT_RGB10A2_SNORM_PACK32";
        case gli::FORMAT_RGB10A2_USCALED_PACK32: return "FORMAT_RGB10A2_USCALED_PACK32";
        case gli::FORMAT_RGB10A2_SSCALED_PACK32: return "FORMAT_RGB10A2_SSCALED_PACK32";
        case gli::FORMAT_RGB10A2_UINT_PACK32: return "FORMAT_RGB10A2_UINT_PACK32";
        case gli::FORMAT_RGB10A2_SINT_PACK32: return "FORMAT_RGB10A2_SINT_PACK32";

        case gli::FORMAT_BGR10A2_UNORM_PACK32: return "FORMAT_BGR10A2_UNORM_PACK32";
        case gli::FORMAT_BGR10A2_SNORM_PACK32: return "FORMAT_BGR10A2_SNORM_PACK32";
        case gli::FORMAT_BGR10A2_USCALED_PACK32: return "FORMAT_BGR10A2_USCALED_PACK32";
        case gli::FORMAT_BGR10A2_SSCALED_PACK32: return "FORMAT_BGR10A2_SSCALED_PACK32";
        case gli::FORMAT_BGR10A2_UINT_PACK32: return "FORMAT_BGR10A2_UINT_PACK32";
        case gli::FORMAT_BGR10A2_SINT_PACK32: return "FORMAT_BGR10A2_SINT_PACK32";

        case gli::FORMAT_R16_UNORM_PACK16: return "FORMAT_R16_UNORM_PACK16";
        case gli::FORMAT_R16_SNORM_PACK16: return "FORMAT_R16_SNORM_PACK16";
        case gli::FORMAT_R16_USCALED_PACK16: return "FORMAT_R16_USCALED_PACK16";
        case gli::FORMAT_R16_SSCALED_PACK16: return "FORMAT_R16_SSCALED_PACK16";
        case gli::FORMAT_R16_UINT_PACK16: return "FORMAT_R16_UINT_PACK16";
        case gli::FORMAT_R16_SINT_PACK16: return "FORMAT_R16_SINT_PACK16";
        case gli::FORMAT_R16_SFLOAT_PACK16: return "FORMAT_R16_SFLOAT_PACK16";

        case gli::FORMAT_RG16_UNORM_PACK16: return "FORMAT_RG16_UNORM_PACK16";
        case gli::FORMAT_RG16_SNORM_PACK16: return "FORMAT_RG16_SNORM_PACK16";
        case gli::FORMAT_RG16_USCALED_PACK16: return "FORMAT_RG16_USCALED_PACK16";
        case gli::FORMAT_RG16_SSCALED_PACK16: return "FORMAT_RG16_SSCALED_PACK16";
        case gli::FORMAT_RG16_UINT_PACK16: return "FORMAT_RG16_UINT_PACK16";
        case gli::FORMAT_RG16_SINT_PACK16: return "FORMAT_RG16_SINT_PACK16";
        case gli::FORMAT_RG16_SFLOAT_PACK16: return "FORMAT_RG16_SFLOAT_PACK16";

        case gli::FORMAT_RGB16_UNORM_PACK16: return "FORMAT_RGB16_UNORM_PACK16";
        case gli::FORMAT_RGB16_SNORM_PACK16: return "FORMAT_RGB16_SNORM_PACK16";
        case gli::FORMAT_RGB16_USCALED_PACK16: return "FORMAT_RGB16_USCALED_PACK16";
        case gli::FORMAT_RGB16_SSCALED_PACK16: return "FORMAT_RGB16_SSCALED_PACK16";
        case gli::FORMAT_RGB16_UINT_PACK16: return "FORMAT_RGB16_UINT_PACK16";
        case gli::FORMAT_RGB16_SINT_PACK16: return "FORMAT_RGB16_SINT_PACK16";
        case gli::FORMAT_RGB16_SFLOAT_PACK16: return "FORMAT_RGB16_SFLOAT_PACK16";

        case gli::FORMAT_RGBA16_UNORM_PACK16: return "FORMAT_RGBA16_UNORM_PACK16";
        case gli::FORMAT_RGBA16_SNORM_PACK16: return "FORMAT_RGBA16_SNORM_PACK16";
        case gli::FORMAT_RGBA16_USCALED_PACK16: return "FORMAT_RGBA16_USCALED_PACK16";
        case gli::FORMAT_RGBA16_SSCALED_PACK16: return "FORMAT_RGBA16_SSCALED_PACK16";
        case gli::FORMAT_RGBA16_UINT_PACK16: return "FORMAT_RGBA16_UINT_PACK16";
        case gli::FORMAT_RGBA16_SINT_PACK16: return "FORMAT_RGBA16_SINT_PACK16";
        case gli::FORMAT_RGBA16_SFLOAT_PACK16: return "FORMAT_RGBA16_SFLOAT_PACK16";

        case gli::FORMAT_R32_UINT_PACK32: return "FORMAT_R32_UINT_PACK32";
        case gli::FORMAT_R32_SINT_PACK32: return "FORMAT_R32_SINT_PACK32";
        case gli::FORMAT_R32_SFLOAT_PACK32: return "FORMAT_R32_SFLOAT_PACK32";

        case gli::FORMAT_RG32_UINT_PACK32: return "FORMAT_RG32_UINT_PACK32";
        case gli::FORMAT_RG32_SINT_PACK32: return "FORMAT_RG32_SINT_PACK32";
        case gli::FORMAT_RG32_SFLOAT_PACK32: return "FORMAT_RG32_SFLOAT_PACK32";

        case gli::FORMAT_RGB32_UINT_PACK32: return "FORMAT_RGB32_UINT_PACK32";
        case gli::FORMAT_RGB32_SINT_PACK32: return "FORMAT_RGB32_SINT_PACK32";
        case gli::FORMAT_RGB32_SFLOAT_PACK32: return "FORMAT_RGB32_SFLOAT_PACK32";

        case gli::FORMAT_RGBA32_UINT_PACK32: return "FORMAT_RGBA32_UINT_PACK32";
        case gli::FORMAT_RGBA32_SINT_PACK32: return "FORMAT_RGBA32_SINT_PACK32";
        case gli::FORMAT_RGBA32_SFLOAT_PACK32: return "FORMAT_RGBA32_SFLOAT_PACK32";

        case gli::FORMAT_R64_UINT_PACK64: return "FORMAT_R64_UINT_PACK64";
        case gli::FORMAT_R64_SINT_PACK64: return "FORMAT_R64_SINT_PACK64";
        case gli::FORMAT_R64_SFLOAT_PACK64: return "FORMAT_R64_SFLOAT_PACK64";

        case gli::FORMAT_RG64_UINT_PACK64: return "FORMAT_RG64_UINT_PACK64";
        case gli::FORMAT_RG64_SINT_PACK64: return "FORMAT_RG64_SINT_PACK64";
        case gli::FORMAT_RG64_SFLOAT_PACK64: return "FORMAT_RG64_SFLOAT_PACK64";

        case gli::FORMAT_RGB64_UINT_PACK64: return "FORMAT_RGB64_UINT_PACK64";
        case gli::FORMAT_RGB64_SINT_PACK64: return "FORMAT_RGB64_SINT_PACK64";
        case gli::FORMAT_RGB64_SFLOAT_PACK64: return "FORMAT_RGB64_SFLOAT_PACK64";

        case gli::FORMAT_RGBA64_UINT_PACK64: return "FORMAT_RGBA64_UINT_PACK64";
        case gli::FORMAT_RGBA64_SINT_PACK64: return "FORMAT_RGBA64_SINT_PACK64";
        case gli::FORMAT_RGBA64_SFLOAT_PACK64: return "FORMAT_RGBA64_SFLOAT_PACK64";

        case gli::FORMAT_RG11B10_UFLOAT_PACK32: return "FORMAT_RG11B10_UFLOAT_PACK32";
        case gli::FORMAT_RGB9E5_UFLOAT_PACK32: return "FORMAT_RGB9E5_UFLOAT_PACK32";

        case gli::FORMAT_D16_UNORM_PACK16: return "FORMAT_D16_UNORM_PACK16";
        case gli::FORMAT_D24_UNORM_PACK32: return "FORMAT_D24_UNORM_PACK32";
        case gli::FORMAT_D32_SFLOAT_PACK32: return "FORMAT_D32_SFLOAT_PACK32";
        case gli::FORMAT_S8_UINT_PACK8: return "FORMAT_S8_UINT_PACK8";
        case gli::FORMAT_D16_UNORM_S8_UINT_PACK32: return "FORMAT_D16_UNORM_S8_UINT_PACK32";
        case gli::FORMAT_D24_UNORM_S8_UINT_PACK32: return "FORMAT_D24_UNORM_S8_UINT_PACK32";
        case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64: return "FORMAT_D32_SFLOAT_S8_UINT_PACK64";

        case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return "FORMAT_RGB_DXT1_UNORM_BLOCK8";
        case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return "FORMAT_RGB_DXT1_SRGB_BLOCK8";
        case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return "FORMAT_RGBA_DXT1_UNORM_BLOCK8";
        case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return "FORMAT_RGBA_DXT1_SRGB_BLOCK8";
        case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return "FORMAT_RGBA_DXT3_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return "FORMAT_RGBA_DXT3_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return "FORMAT_RGBA_DXT5_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return "FORMAT_RGBA_DXT5_SRGB_BLOCK16";
        case gli::FORMAT_R_ATI1N_UNORM_BLOCK8: return "FORMAT_R_ATI1N_UNORM_BLOCK8";
        case gli::FORMAT_R_ATI1N_SNORM_BLOCK8: return "FORMAT_R_ATI1N_SNORM_BLOCK8";
        case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16: return "FORMAT_RG_ATI2N_UNORM_BLOCK16";
        case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16: return "FORMAT_RG_ATI2N_SNORM_BLOCK16";
        case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16: return "FORMAT_RGB_BP_UFLOAT_BLOCK16";
        case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16: return "FORMAT_RGB_BP_SFLOAT_BLOCK16";
        case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return "FORMAT_RGBA_BP_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return "FORMAT_RGBA_BP_SRGB_BLOCK16";

        case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8: return "FORMAT_RGB_ETC2_UNORM_BLOCK8";
        case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8: return "FORMAT_RGB_ETC2_SRGB_BLOCK8";
        case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8: return "FORMAT_RGBA_ETC2_UNORM_BLOCK8";
        case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8: return "FORMAT_RGBA_ETC2_SRGB_BLOCK8";
        case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16: return "FORMAT_RGBA_ETC2_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16: return "FORMAT_RGBA_ETC2_SRGB_BLOCK16";
        case gli::FORMAT_R_EAC_UNORM_BLOCK8: return "FORMAT_R_EAC_UNORM_BLOCK8";
        case gli::FORMAT_R_EAC_SNORM_BLOCK8: return "FORMAT_R_EAC_SNORM_BLOCK8";
        case gli::FORMAT_RG_EAC_UNORM_BLOCK16: return "FORMAT_RG_EAC_UNORM_BLOCK16";
        case gli::FORMAT_RG_EAC_SNORM_BLOCK16: return "FORMAT_RG_EAC_SNORM_BLOCK16";

        case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16";

        case gli::FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32";
        case gli::FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32";
        case gli::FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32";
        case gli::FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32";
        case gli::FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32";
        case gli::FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32";
        case gli::FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32";
        case gli::FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32";
        case gli::FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8";
        case gli::FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8";
        case gli::FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8";
        case gli::FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8";

        case gli::FORMAT_RGB_ETC_UNORM_BLOCK8: return "FORMAT_RGB_ETC_UNORM_BLOCK8";
        case gli::FORMAT_RGB_ATC_UNORM_BLOCK8: return "FORMAT_RGB_ATC_UNORM_BLOCK8";
        case gli::FORMAT_RGBA_ATCA_UNORM_BLOCK16: return "FORMAT_RGBA_ATCA_UNORM_BLOCK16";
        case gli::FORMAT_RGBA_ATCI_UNORM_BLOCK16: return "FORMAT_RGBA_ATCI_UNORM_BLOCK16";

        case gli::FORMAT_L8_UNORM_PACK8: return "FORMAT_L8_UNORM_PACK8";
        case gli::FORMAT_A8_UNORM_PACK8: return "FORMAT_A8_UNORM_PACK8";
        case gli::FORMAT_LA8_UNORM_PACK8: return "FORMAT_LA8_UNORM_PACK8";
        case gli::FORMAT_L16_UNORM_PACK16: return "FORMAT_L16_UNORM_PACK16";
        case gli::FORMAT_A16_UNORM_PACK16: return "FORMAT_A16_UNORM_PACK16";
        case gli::FORMAT_LA16_UNORM_PACK16: return "FORMAT_LA16_UNORM_PACK16";

        case gli::FORMAT_BGR8_UNORM_PACK32: return "FORMAT_BGR8_UNORM_PACK32";
        case gli::FORMAT_BGR8_SRGB_PACK32: return "FORMAT_BGR8_SRGB_PACK32";

        case gli::FORMAT_RG3B2_UNORM_PACK8: return "FORMAT_RG3B2_UNORM_PACK8";
    }
    return "FORMAT_UNKNOWN";
}

} // namespace grfx
} // namespace ppx
