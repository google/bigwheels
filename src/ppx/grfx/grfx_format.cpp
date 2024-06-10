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

#include "ppx/grfx/grfx_format.h"
#include "ppx/config.h"

namespace ppx {
namespace grfx {

#define UNCOMPRESSED_FORMAT(Name, Type, Aspect, BytesPerTexel, BytesPerComponent, Layout, Component, ComponentOffsets) \
    {                                                                                                                  \
        #Name,                                                                                                         \
            FORMAT_DATA_TYPE_##Type,                                                                                   \
            FORMAT_ASPECT_##Aspect,                                                                                    \
            BytesPerTexel,                                                                                             \
            /* blockWidth= */ 1,                                                                                       \
            BytesPerComponent,                                                                                         \
            FORMAT_LAYOUT_##Layout,                                                                                    \
            FORMAT_COMPONENT_##Component,                                                                              \
            OFFSETS_##ComponentOffsets,                                                                                \
            FORMAT_CHROMA_SUBSAMPLING_UNDEFINED,                                                                       \
            /*isPlanar=*/false                                                                                         \
    }

#define COMPRESSED_FORMAT(Name, Type, BytesPerBlock, BlockWidth, Component) \
    {                                                                       \
        #Name,                                                              \
            FORMAT_DATA_TYPE_##Type,                                        \
            FORMAT_ASPECT_COLOR,                                            \
            BytesPerBlock,                                                  \
            BlockWidth,                                                     \
            /*bytesPerComponent=*/-1,                                       \
            FORMAT_LAYOUT_COMPRESSED,                                       \
            FORMAT_COMPONENT_##Component,                                   \
            OFFSETS_UNDEFINED,                                              \
            FORMAT_CHROMA_SUBSAMPLING_UNDEFINED,                            \
            /*isPlanar=*/false                                              \
    }

#define UNCOMP_PLANAR_FORMAT(Name, Type, Aspect, BytesPerTexel, Layout, Component, ChromaSubsampling) \
    {                                                                                                 \
        #Name,                                                                                        \
            FORMAT_DATA_TYPE_##Type,                                                                  \
            FORMAT_ASPECT_##Aspect,                                                                   \
            BytesPerTexel,                                                                            \
            /*blockWidth=*/1,                                                                         \
            /*bytesPerComponent=*/-1,                                                                 \
            FORMAT_LAYOUT_##Layout,                                                                   \
            FORMAT_COMPONENT_##Component,                                                             \
            OFFSETS_UNDEFINED,                                                                        \
            FORMAT_CHROMA_SUBSAMPLING_##ChromaSubsampling,                                            \
            /*isPlanar=*/true                                                                         \
    }

// clang-format off
#define OFFSETS_UNDEFINED        { -1, -1, -1, -1 }
#define OFFSETS_R(R)             { R, -1, -1, -1 }
#define OFFSETS_RG(R, G)         { R,  G, -1, -1 }
#define OFFSETS_RGB(R, G, B)     { R,  G,  B, -1 }
#define OFFSETS_RGBA(R, G, B, A) { R,  G,  B,  A }
// clang-format on

#define PLANE_MEMBER(Component, Type, BitSize)   \
    FormatPlaneDesc::Member                      \
    {                                            \
        FORMAT_PLANE_COMPONENT_TYPE_##Component, \
            FORMAT_PLANE_CHROMA_TYPE_##Type,     \
            BitSize                              \
    }

FormatPlaneDesc::FormatPlaneDesc(std::initializer_list<std::initializer_list<FormatPlaneDesc::Member>>&& planes)
{
    for (auto it = planes.begin(); it != planes.end(); ++it) {
        this->planes.push_back(FormatPlaneDesc::Plane{*it});
    }
}

// A static registry of format descriptions.
// The order must match the order of the grfx::Format enum, so that
// retrieving the description for a given format can be done in
// constant time.
constexpr FormatDesc formatDescs[] = {
    // clang-format off
    //                 +----------------------------------------------------------------------------------------------------------+
    //                 |                                              ,-> bytes per texel                                         |
    //                 |                                              |   ,-> bytes per component                                 |
    //                 | format name      | type     | aspect         |   |   Layout   | Components            | Offsets          |
    UNCOMPRESSED_FORMAT(UNDEFINED,          UNDEFINED, UNDEFINED,     0,  0,  UNDEFINED, UNDEFINED,              UNDEFINED),
    UNCOMPRESSED_FORMAT(R8_SNORM,           SNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R8G8_SNORM,         SNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
    UNCOMPRESSED_FORMAT(R8G8B8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
    UNCOMPRESSED_FORMAT(R8G8B8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
    UNCOMPRESSED_FORMAT(B8G8R8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
    UNCOMPRESSED_FORMAT(B8G8R8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

    UNCOMPRESSED_FORMAT(R8_UNORM,           UNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R8G8_UNORM,         UNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
    UNCOMPRESSED_FORMAT(R8G8B8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
    UNCOMPRESSED_FORMAT(R8G8B8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
    UNCOMPRESSED_FORMAT(B8G8R8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
    UNCOMPRESSED_FORMAT(B8G8R8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

    UNCOMPRESSED_FORMAT(R8_SINT,            SINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R8G8_SINT,          SINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
    UNCOMPRESSED_FORMAT(R8G8B8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
    UNCOMPRESSED_FORMAT(R8G8B8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
    UNCOMPRESSED_FORMAT(B8G8R8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
    UNCOMPRESSED_FORMAT(B8G8R8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

    UNCOMPRESSED_FORMAT(R8_UINT,            UINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R8G8_UINT,          UINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
    UNCOMPRESSED_FORMAT(R8G8B8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
    UNCOMPRESSED_FORMAT(R8G8B8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
    UNCOMPRESSED_FORMAT(B8G8R8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
    UNCOMPRESSED_FORMAT(B8G8R8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

    UNCOMPRESSED_FORMAT(R16_SNORM,          SNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R16G16_SNORM,       SNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
    UNCOMPRESSED_FORMAT(R16G16B16_SNORM,    SNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
    UNCOMPRESSED_FORMAT(R16G16B16A16_SNORM, SNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

    UNCOMPRESSED_FORMAT(R16_UNORM,          UNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R16G16_UNORM,       UNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
    UNCOMPRESSED_FORMAT(R16G16B16_UNORM,    UNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
    UNCOMPRESSED_FORMAT(R16G16B16A16_UNORM, UNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

    UNCOMPRESSED_FORMAT(R16_SINT,           SINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R16G16_SINT,        SINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
    UNCOMPRESSED_FORMAT(R16G16B16_SINT,     SINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
    UNCOMPRESSED_FORMAT(R16G16B16A16_SINT,  SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

    UNCOMPRESSED_FORMAT(R16_UINT,           UINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R16G16_UINT,        UINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
    UNCOMPRESSED_FORMAT(R16G16B16_UINT,     UINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
    UNCOMPRESSED_FORMAT(R16G16B16A16_UINT,  UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

    UNCOMPRESSED_FORMAT(R16_FLOAT,          FLOAT,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R16G16_FLOAT,       FLOAT,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
    UNCOMPRESSED_FORMAT(R16G16B16_FLOAT,    FLOAT,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
    UNCOMPRESSED_FORMAT(R16G16B16A16_FLOAT, FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

    UNCOMPRESSED_FORMAT(R32_SINT,           SINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R32G32_SINT,        SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
    UNCOMPRESSED_FORMAT(R32G32B32_SINT,     SINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
    UNCOMPRESSED_FORMAT(R32G32B32A32_SINT,  SINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

    UNCOMPRESSED_FORMAT(R32_UINT,           UINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R32G32_UINT,        UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
    UNCOMPRESSED_FORMAT(R32G32B32_UINT,     UINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
    UNCOMPRESSED_FORMAT(R32G32B32A32_UINT,  UINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

    UNCOMPRESSED_FORMAT(R32_FLOAT,          FLOAT,     COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R32G32_FLOAT,       FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
    UNCOMPRESSED_FORMAT(R32G32B32_FLOAT,    FLOAT,     COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
    UNCOMPRESSED_FORMAT(R32G32B32A32_FLOAT, FLOAT,     COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

    UNCOMPRESSED_FORMAT(S8_UINT,            UINT,      STENCIL,       1,  1,  LINEAR,    STENCIL,                  RG(-1, 0)),
    UNCOMPRESSED_FORMAT(D16_UNORM,          UNORM,     DEPTH,         2,  2,  LINEAR,    DEPTH,                    RG(0, -1)),
    UNCOMPRESSED_FORMAT(D32_FLOAT,          FLOAT,     DEPTH,         4,  4,  LINEAR,    DEPTH,                    RG(0, -1)),

    UNCOMPRESSED_FORMAT(D16_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 3,  2,  LINEAR,    DEPTH_STENCIL,            RG(0, 2)),
    UNCOMPRESSED_FORMAT(D24_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 4,  3,  LINEAR,    DEPTH_STENCIL,            RG(0, 3)),
    UNCOMPRESSED_FORMAT(D32_FLOAT_S8_UINT,  FLOAT,     DEPTH_STENCIL, 5,  4,  LINEAR,    DEPTH_STENCIL,            RG(0, 4)),

    UNCOMPRESSED_FORMAT(R8_SRGB,            SRGB,     COLOR,          1,  1,  LINEAR,    RED,                       R(0)),
    UNCOMPRESSED_FORMAT(R8G8_SRBG,          SRGB,     COLOR,          2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
    UNCOMPRESSED_FORMAT(R8G8B8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
    UNCOMPRESSED_FORMAT(R8G8B8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
    UNCOMPRESSED_FORMAT(B8G8R8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
    UNCOMPRESSED_FORMAT(B8G8R8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

    // We don't support retrieving component size or byte offsets for packed formats.
    UNCOMPRESSED_FORMAT(R10G10B10A2_UNORM,  UNORM,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE_ALPHA,   UNDEFINED),
    UNCOMPRESSED_FORMAT(R11G11B10_FLOAT,    FLOAT,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE,         UNDEFINED),

    // We don't support retrieving component size or byte offsets for compressed formats.
    // We don't support non-square blocks for compressed textures.
    //                +-----------------------------------------------------+
    //                |                       ,-> bytes per block           |
    //                |                       |  ,-> block width            |
    //                | format name | type    |  |   Components             |
    COMPRESSED_FORMAT(BC1_RGBA_SRGB , SRGB,   8, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC1_RGBA_UNORM, UNORM,  8, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC1_RGB_SRGB  , SRGB,   8, 4,  RED_GREEN_BLUE),
    COMPRESSED_FORMAT(BC1_RGB_UNORM , UNORM,  8, 4,  RED_GREEN_BLUE),
    COMPRESSED_FORMAT(BC2_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC2_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC3_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC3_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC4_UNORM     , UNORM,  8, 4,  RED),
    COMPRESSED_FORMAT(BC4_SNORM     , SNORM,  8, 4,  RED),
    COMPRESSED_FORMAT(BC5_UNORM     , UNORM, 16, 4,  RED_GREEN),
    COMPRESSED_FORMAT(BC5_SNORM     , SNORM, 16, 4,  RED_GREEN),
    COMPRESSED_FORMAT(BC6H_UFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
    COMPRESSED_FORMAT(BC6H_SFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
    COMPRESSED_FORMAT(BC7_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
    COMPRESSED_FORMAT(BC7_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),

    // We don't support retrieving component size (which are typically non-uniform) or byte offsets for uncompressed planar formats.
    //                 +---------------------------------------------------------------------------------------+
    //                 |                                         ,-> bytes per texel                           |
    //                 | format name             | type | aspect | layout | components    | chroma subsampling |
    UNCOMP_PLANAR_FORMAT(G8_B8R8_2PLANE_420_UNORM, UNORM, COLOR, 2, PACKED, RED_GREEN_BLUE, 420),

#undef UNCOMP_PLANAR_FORMAT
#undef COMPRESSED_FORMAT
#undef UNCOMPRESSED_FORMAT
#undef OFFSETS_UNDEFINED
#undef OFFSETS_R
#undef OFFSETS_RG
#undef OFFSETS_RGB
#undef OFFSETS_RGBA
};
// clang-format on
constexpr size_t formatDescsSize = sizeof(formatDescs) / sizeof(FormatDesc);
static_assert(formatDescsSize == FORMAT_COUNT, "Missing format descriptions");

const FormatDesc* GetFormatDescription(grfx::Format format)
{
    uint32_t formatIndex = static_cast<uint32_t>(format);
    PPX_ASSERT_MSG(format != grfx::FORMAT_UNDEFINED && formatIndex < formatDescsSize, "invalid format");
    return &formatDescs[formatIndex];
}

// Definitions for image planes - component types and bit sizes.
const std::optional<FormatPlaneDesc> GetFormatPlaneDescription(
    grfx::Format format)
{
    switch (format) {
        case FORMAT_G8_B8R8_2PLANE_420_UNORM:
            return FormatPlaneDesc{
                {PLANE_MEMBER(GREEN, LUMA, 8)},
                {PLANE_MEMBER(BLUE, CHROMA, 8), PLANE_MEMBER(RED, CHROMA, 8)}};
        default:
            // We'll log a warning and return nullopt outside of the switch
            // statement. This is for the compiler.
            break;
    }

    const FormatDesc* description = GetFormatDescription(format);
    if (description == nullptr || !description->isPlanar) {
        PPX_LOG_WARN("Attempted to get planes for format " << format << ", which is non-planar");
    }
    else {
        PPX_LOG_ERROR("Could not find planes for planar format " << format << "; returning null.");
    }

    return std::nullopt;
}
#undef PLANE_MEMBER

const char* ToString(grfx::Format format)
{
    uint32_t formatIndex = static_cast<uint32_t>(format);
    PPX_ASSERT_MSG(formatIndex < formatDescsSize, "invalid format");
    return formatDescs[formatIndex].name;
}

} // namespace grfx
} // namespace ppx
