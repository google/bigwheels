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

#ifndef ppx_grfx_format_h
#define ppx_grfx_format_h

#include <cstdint>
#include <optional>
#include <vector>

namespace ppx {
namespace grfx {

enum Format
{
    FORMAT_UNDEFINED = 0,

    // 8-bit signed normalized
    FORMAT_R8_SNORM,
    FORMAT_R8G8_SNORM,
    FORMAT_R8G8B8_SNORM,
    FORMAT_R8G8B8A8_SNORM,
    FORMAT_B8G8R8_SNORM,
    FORMAT_B8G8R8A8_SNORM,

    // 8-bit unsigned normalized
    FORMAT_R8_UNORM,
    FORMAT_R8G8_UNORM,
    FORMAT_R8G8B8_UNORM,
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_B8G8R8_UNORM,
    FORMAT_B8G8R8A8_UNORM,

    // 8-bit signed integer
    FORMAT_R8_SINT,
    FORMAT_R8G8_SINT,
    FORMAT_R8G8B8_SINT,
    FORMAT_R8G8B8A8_SINT,
    FORMAT_B8G8R8_SINT,
    FORMAT_B8G8R8A8_SINT,

    // 8-bit unsigned integer
    FORMAT_R8_UINT,
    FORMAT_R8G8_UINT,
    FORMAT_R8G8B8_UINT,
    FORMAT_R8G8B8A8_UINT,
    FORMAT_B8G8R8_UINT,
    FORMAT_B8G8R8A8_UINT,

    // 16-bit signed normalized
    FORMAT_R16_SNORM,
    FORMAT_R16G16_SNORM,
    FORMAT_R16G16B16_SNORM,
    FORMAT_R16G16B16A16_SNORM,

    // 16-bit unsigned normalized
    FORMAT_R16_UNORM,
    FORMAT_R16G16_UNORM,
    FORMAT_R16G16B16_UNORM,
    FORMAT_R16G16B16A16_UNORM,

    // 16-bit signed integer
    FORMAT_R16_SINT,
    FORMAT_R16G16_SINT,
    FORMAT_R16G16B16_SINT,
    FORMAT_R16G16B16A16_SINT,

    // 16-bit unsigned integer
    FORMAT_R16_UINT,
    FORMAT_R16G16_UINT,
    FORMAT_R16G16B16_UINT,
    FORMAT_R16G16B16A16_UINT,

    // 16-bit float
    FORMAT_R16_FLOAT,
    FORMAT_R16G16_FLOAT,
    FORMAT_R16G16B16_FLOAT,
    FORMAT_R16G16B16A16_FLOAT,

    // 32-bit signed integer
    FORMAT_R32_SINT,
    FORMAT_R32G32_SINT,
    FORMAT_R32G32B32_SINT,
    FORMAT_R32G32B32A32_SINT,

    // 32-bit unsigned integer
    FORMAT_R32_UINT,
    FORMAT_R32G32_UINT,
    FORMAT_R32G32B32_UINT,
    FORMAT_R32G32B32A32_UINT,

    // 32-bit float
    FORMAT_R32_FLOAT,
    FORMAT_R32G32_FLOAT,
    FORMAT_R32G32B32_FLOAT,
    FORMAT_R32G32B32A32_FLOAT,

    // 8-bit unsigned integer stencil
    FORMAT_S8_UINT,

    // 16-bit unsigned normalized depth
    FORMAT_D16_UNORM,

    // 32-bit float depth
    FORMAT_D32_FLOAT,

    // Depth/stencil combinations
    FORMAT_D16_UNORM_S8_UINT,
    FORMAT_D24_UNORM_S8_UINT,
    FORMAT_D32_FLOAT_S8_UINT,

    // SRGB
    FORMAT_R8_SRGB,
    FORMAT_R8G8_SRGB,
    FORMAT_R8G8B8_SRGB,
    FORMAT_R8G8B8A8_SRGB,
    FORMAT_B8G8R8_SRGB,
    FORMAT_B8G8R8A8_SRGB,

    // 10-bit RGB, 2-bit A packed
    FORMAT_R10G10B10A2_UNORM,

    // 11-bit R, 11-bit G, 10-bit B packed
    FORMAT_R11G11B10_FLOAT,

    // Compressed formats
    FORMAT_BC1_RGBA_SRGB,
    FORMAT_BC1_RGBA_UNORM,
    FORMAT_BC1_RGB_SRGB,
    FORMAT_BC1_RGB_UNORM,
    FORMAT_BC2_SRGB,
    FORMAT_BC2_UNORM,
    FORMAT_BC3_SRGB,
    FORMAT_BC3_UNORM,
    FORMAT_BC4_UNORM,
    FORMAT_BC4_SNORM,
    FORMAT_BC5_UNORM,
    FORMAT_BC5_SNORM,
    FORMAT_BC6H_UFLOAT,
    FORMAT_BC6H_SFLOAT,
    FORMAT_BC7_UNORM,
    FORMAT_BC7_SRGB,

    FORMAT_G8_B8R8_2PLANE_420_UNORM,

    FORMAT_COUNT,
};

enum FormatAspectBit
{
    FORMAT_ASPECT_UNDEFINED = 0x0,
    FORMAT_ASPECT_COLOR     = 0x1,
    FORMAT_ASPECT_DEPTH     = 0x2,
    FORMAT_ASPECT_STENCIL   = 0x4,

    FORMAT_ASPECT_DEPTH_STENCIL = FORMAT_ASPECT_DEPTH | FORMAT_ASPECT_STENCIL,
};

enum FormatChromaSubsampling
{
    FORMAT_CHROMA_SUBSAMPLING_UNDEFINED = 0x0,
    FORMAT_CHROMA_SUBSAMPLING_444       = 0x1,
    FORMAT_CHROMA_SUBSAMPLING_422       = 0x2,
    FORMAT_CHROMA_SUBSAMPLING_420       = 0x3,
};

enum FormatComponentBit
{
    FORMAT_COMPONENT_UNDEFINED = 0x0,
    FORMAT_COMPONENT_RED       = 0x1,
    FORMAT_COMPONENT_GREEN     = 0x2,
    FORMAT_COMPONENT_BLUE      = 0x4,
    FORMAT_COMPONENT_ALPHA     = 0x8,
    FORMAT_COMPONENT_DEPTH     = 0x10,
    FORMAT_COMPONENT_STENCIL   = 0x20,

    FORMAT_COMPONENT_RED_GREEN            = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN,
    FORMAT_COMPONENT_RED_GREEN_BLUE       = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN | FORMAT_COMPONENT_BLUE,
    FORMAT_COMPONENT_RED_GREEN_BLUE_ALPHA = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN | FORMAT_COMPONENT_BLUE | FORMAT_COMPONENT_ALPHA,
    FORMAT_COMPONENT_DEPTH_STENCIL        = FORMAT_COMPONENT_DEPTH | FORMAT_COMPONENT_STENCIL,
};

enum FormatDataType
{
    FORMAT_DATA_TYPE_UNDEFINED = 0x0,
    FORMAT_DATA_TYPE_UNORM     = 0x1,
    FORMAT_DATA_TYPE_SNORM     = 0x2,
    FORMAT_DATA_TYPE_UINT      = 0x4,
    FORMAT_DATA_TYPE_SINT      = 0x8,
    FORMAT_DATA_TYPE_FLOAT     = 0x10,
    FORMAT_DATA_TYPE_SRGB      = 0x20,
};

enum FormatLayout
{
    FORMAT_LAYOUT_UNDEFINED  = 0x0,
    FORMAT_LAYOUT_LINEAR     = 0x1,
    FORMAT_LAYOUT_PACKED     = 0x2,
    FORMAT_LAYOUT_COMPRESSED = 0x4,
};

struct FormatComponentOffset
{
    union
    {
        struct
        {
            int32_t red   : 8;
            int32_t green : 8;
            int32_t blue  : 8;
            int32_t alpha : 8;
        };
        struct
        {
            int32_t depth   : 8;
            int32_t stencil : 8;
        };
    };
};

struct FormatDesc
{
    // BigWheels specific format name.
    const char* name;

    // The texel data type, e.g. UNORM, SNORM, UINT, etc.
    FormatDataType dataType;

    // The format aspect, i.e. color, depth, stencil, or depth-stencil.
    FormatAspectBit aspect;

    // The number of bytes per texel.
    // For compressed formats, this field is the size of a block.
    uint8_t bytesPerTexel;

    // The size in texels of the smallest supported size.
    // For compressed textures, that's the block size.
    // For uncompressed textures, the value is 1 (a pixel).
    uint8_t blockWidth;

    // The number of bytes per component (channel).
    // In case of combined depth-stencil formats, this is the size of the depth
    // component only.
    // In case of packed or compressed formats, this field is invalid
    // and will be set to -1.
    int8_t bytesPerComponent;

    // The layout of the format (linear, packed, or compressed).
    FormatLayout layout;

    // The components (channels) represented by the format,
    // e.g. RGBA, depth-stencil, or a subset of those.
    FormatComponentBit componentBits;

    // The offset, in bytes, of each component within the texel.
    // In case of packed or compressed formats, this field is invalid
    // and the offsets will be set to -1.
    FormatComponentOffset componentOffset;

    // In chroma-based formats, there can be subsampling of chroma color components
    // of an image, to reduce image size.
    FormatChromaSubsampling chromaSubsampling;

    // If true, this is a planar format that does not store all image components
    // in a single block. E.G. YCbCr formats, where Cb and Cr may be defined in
    // a separate plane than Y values, and have a different resolution.
    bool isPlanar;
};

enum FormatPlaneChromaType
{
    FORMAT_PLANE_CHROMA_TYPE_UNDEFINED,
    FORMAT_PLANE_CHROMA_TYPE_LUMA,
    FORMAT_PLANE_CHROMA_TYPE_CHROMA,
};

struct FormatPlaneDesc
{
    struct Member
    {
        // Note: it's expected that only one bit would be set here. That being
        // said, this is mostly to add clarity to plane component definitions.
        FormatComponentBit component;
        // This defines whether this is a luma value, chroma value, or neither
        // (will be set to undefined for non-YCbCr types).
        FormatPlaneChromaType type;
        // Number of bits used to describe this component.
        int bitCount;
    };

    struct Plane
    {
        std::vector<Member> members;
    };

    FormatPlaneDesc(std::initializer_list<std::initializer_list<Member>>&& planes);

    std::vector<Plane> planes;
};

//! @brief Gets a description of the given /b format.
const FormatDesc* GetFormatDescription(grfx::Format format);

// Gets a description of planes in the format, if the format is planar.
// If the format is not planar, returns nullopt.
const std::optional<FormatPlaneDesc> GetFormatPlaneDescription(
    grfx::Format format);

const char* ToString(grfx::Format format);

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_format_h
