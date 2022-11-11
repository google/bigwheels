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

#include "ppx/grfx/dx11/dx11_util.h"

namespace ppx {
namespace grfx {
namespace dx11 {

D3D11_BLEND ToD3D11Blend(grfx::BlendFactor value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::BLEND_FACTOR_ZERO                     : return D3D11_BLEND_ZERO; break;
        case grfx::BLEND_FACTOR_ONE                      : return D3D11_BLEND_ONE; break;
        case grfx::BLEND_FACTOR_SRC_COLOR                : return D3D11_BLEND_SRC_COLOR; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC_COLOR      : return D3D11_BLEND_INV_SRC_COLOR; break;
        case grfx::BLEND_FACTOR_SRC_ALPHA                : return D3D11_BLEND_SRC_ALPHA; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      : return D3D11_BLEND_INV_SRC_ALPHA; break;
        case grfx::BLEND_FACTOR_DST_ALPHA                : return D3D11_BLEND_DEST_ALPHA; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_DST_ALPHA      : return D3D11_BLEND_INV_DEST_ALPHA; break;
        case grfx::BLEND_FACTOR_DST_COLOR                : return D3D11_BLEND_DEST_COLOR; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_DST_COLOR      : return D3D11_BLEND_INV_DEST_COLOR; break;
        case grfx::BLEND_FACTOR_SRC_ALPHA_SATURATE       : return D3D11_BLEND_SRC_ALPHA_SAT; break;
        case grfx::BLEND_FACTOR_CONSTANT_COLOR           : return D3D11_BLEND_BLEND_FACTOR; break;     // D3D11 does not have constants
        case grfx::BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR : return D3D11_BLEND_INV_BLEND_FACTOR; break; // for color and alpha blend
        case grfx::BLEND_FACTOR_CONSTANT_ALPHA           : return D3D11_BLEND_BLEND_FACTOR; break;     // constants. 
        case grfx::BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA : return D3D11_BLEND_INV_BLEND_FACTOR; break; //
        case grfx::BLEND_FACTOR_SRC1_COLOR               : return D3D11_BLEND_SRC1_COLOR; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC1_COLOR     : return D3D11_BLEND_INV_SRC1_COLOR; break;
        case grfx::BLEND_FACTOR_SRC1_ALPHA               : return D3D11_BLEND_SRC1_ALPHA; break;
        case grfx::BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA     : return D3D11_BLEND_INV_SRC1_ALPHA; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_BLEND>();
}

D3D11_BLEND_OP ToD3D11BlendOp(grfx::BlendOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::BLEND_OP_ADD              : return D3D11_BLEND_OP_ADD; break;
        case grfx::BLEND_OP_SUBTRACT         : return D3D11_BLEND_OP_SUBTRACT; break;
        case grfx::BLEND_OP_REVERSE_SUBTRACT : return D3D11_BLEND_OP_REV_SUBTRACT; break;
        case grfx::BLEND_OP_MIN              : return D3D11_BLEND_OP_MIN; break;
        case grfx::BLEND_OP_MAX              : return D3D11_BLEND_OP_MAX; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_BLEND_OP>();
}

UINT ToD3D11BindFlags(const grfx::BufferUsageFlags& value)
{
    UINT flags = 0;
    if (value.bits.uniformBuffer) {
        flags |= D3D11_BIND_CONSTANT_BUFFER;
    }
    if (value.bits.rawStorageBuffer) {
        flags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    if (value.bits.structuredBuffer) {
        flags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (value.bits.indexBuffer) {
        flags |= D3D11_BIND_INDEX_BUFFER;
    }
    if (value.bits.vertexBuffer) {
        flags |= D3D11_BIND_VERTEX_BUFFER;
    }
    return flags;
}

UINT ToD3D11BindFlags(const grfx::ImageUsageFlags& value)
{
    UINT flags = 0;
    if (value.bits.sampled) {
        flags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (value.bits.colorAttachment) {
        flags |= D3D11_BIND_RENDER_TARGET;
    }
    if (value.bits.depthStencilAttachment) {
        flags |= D3D11_BIND_DEPTH_STENCIL;
    }
    if (value.bits.storage) {
        flags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    return flags;
}

D3D11_COMPARISON_FUNC ToD3D11ComparisonFunc(grfx::CompareOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::COMPARE_OP_NEVER            : return D3D11_COMPARISON_NEVER; break;
        case grfx::COMPARE_OP_LESS             : return D3D11_COMPARISON_LESS; break;
        case grfx::COMPARE_OP_EQUAL            : return D3D11_COMPARISON_EQUAL; break;
        case grfx::COMPARE_OP_LESS_OR_EQUAL    : return D3D11_COMPARISON_LESS_EQUAL; break;
        case grfx::COMPARE_OP_GREATER          : return D3D11_COMPARISON_GREATER; break;
        case grfx::COMPARE_OP_NOT_EQUAL        : return D3D11_COMPARISON_NOT_EQUAL; break;
        case grfx::COMPARE_OP_GREATER_OR_EQUAL : return D3D11_COMPARISON_GREATER_EQUAL; break;
        case grfx::COMPARE_OP_ALWAYS           : return D3D11_COMPARISON_ALWAYS; break;      
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_COMPARISON_FUNC>();
}

D3D11_CULL_MODE ToD3D11CullMode(grfx::CullMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::CULL_MODE_NONE  : return D3D11_CULL_NONE; break;
        case grfx::CULL_MODE_FRONT : return D3D11_CULL_FRONT; break;
        case grfx::CULL_MODE_BACK  : return D3D11_CULL_BACK; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_CULL_MODE>();
}

D3D11_DSV_DIMENSION ToD3D11DSVDimension(grfx::ImageViewType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::IMAGE_VIEW_TYPE_1D       : return D3D11_DSV_DIMENSION_TEXTURE1D; break;
        case grfx::IMAGE_VIEW_TYPE_1D_ARRAY : return D3D11_DSV_DIMENSION_TEXTURE1DARRAY; break;
        case grfx::IMAGE_VIEW_TYPE_2D       : return D3D11_DSV_DIMENSION_TEXTURE2D; break;
        case grfx::IMAGE_VIEW_TYPE_2D_ARRAY : return D3D11_DSV_DIMENSION_TEXTURE2DARRAY; break;
    }
    // clang-format on
    return D3D11_DSV_DIMENSION_UNKNOWN;
}

D3D11_FILL_MODE ToD3D11FillMode(grfx::PolygonMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case POLYGON_MODE_FILL  : return D3D11_FILL_SOLID; break;
        case POLYGON_MODE_LINE  : return D3D11_FILL_WIREFRAME; break;
        case POLYGON_MODE_POINT : break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_FILL_MODE>();
}

D3D11_FILTER_TYPE ToD3D11FilterType(grfx::Filter value)
{
    // clang-format off
    switch (value) {
        default: break;
        case FILTER_NEAREST : return D3D11_FILTER_TYPE_POINT; break;
        case FILTER_LINEAR  : return D3D11_FILTER_TYPE_LINEAR; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_FILTER_TYPE>();
}

D3D11_FILTER_TYPE ToD3D11FilterType(grfx::SamplerMipmapMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case SAMPLER_MIPMAP_MODE_NEAREST : return D3D11_FILTER_TYPE_POINT; break;
        case SAMPLER_MIPMAP_MODE_LINEAR  : return D3D11_FILTER_TYPE_LINEAR; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_FILTER_TYPE>();
}

DXGI_FORMAT ToD3D11IndexFormat(grfx::IndexType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::INDEX_TYPE_UINT16 : return DXGI_FORMAT_R16_UINT; break;
        case grfx::INDEX_TYPE_UINT32 : return DXGI_FORMAT_R32_UINT; break;
    }
    // clang-format on
    return DXGI_FORMAT_UNKNOWN;
}

UINT ToD3D11LogicOp(grfx::LogicOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::LOGIC_OP_CLEAR         : return D3D11_LOGIC_OP_CLEAR; break;
        case grfx::LOGIC_OP_SET           : return D3D11_LOGIC_OP_SET; break;
        case grfx::LOGIC_OP_COPY          : return D3D11_LOGIC_OP_COPY; break;
        case grfx::LOGIC_OP_COPY_INVERTED : return D3D11_LOGIC_OP_COPY_INVERTED; break;
        case grfx::LOGIC_OP_NO_OP         : return D3D11_LOGIC_OP_NOOP; break;
        case grfx::LOGIC_OP_INVERT        : return D3D11_LOGIC_OP_INVERT; break;
        case grfx::LOGIC_OP_AND           : return D3D11_LOGIC_OP_AND; break;
        case grfx::LOGIC_OP_NAND          : return D3D11_LOGIC_OP_NAND; break;
        case grfx::LOGIC_OP_OR            : return D3D11_LOGIC_OP_OR; break;
        case grfx::LOGIC_OP_NOR           : return D3D11_LOGIC_OP_NOR; break;
        case grfx::LOGIC_OP_XOR           : return D3D11_LOGIC_OP_XOR; break;
        case grfx::LOGIC_OP_EQUIVALENT    : return D3D11_LOGIC_OP_EQUIV; break;
        case grfx::LOGIC_OP_AND_REVERSE   : return D3D11_LOGIC_OP_AND_REVERSE; break;
        case grfx::LOGIC_OP_AND_INVERTED  : return D3D11_LOGIC_OP_AND_INVERTED; break;
        case grfx::LOGIC_OP_OR_REVERSE    : return D3D11_LOGIC_OP_OR_REVERSE; break;
        case grfx::LOGIC_OP_OR_INVERTED   : return D3D11_LOGIC_OP_OR_INVERTED; break;
    }
    // clang-format on
    return ppx::InvalidValue<UINT>();
}

D3D11_PRIMITIVE_TOPOLOGY ToD3D11PrimitiveTopology(grfx::PrimitiveTopology value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST  : return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN   : break;
        case grfx::PRIMITIVE_TOPOLOGY_POINT_LIST     : return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_LIST      : return D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_STRIP     : return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case grfx::PRIMITIVE_TOPOLOGY_PATCH_LIST     : break;
    }
    // clang-format on
    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D11_QUERY ToD3D11QueryType(grfx::QueryType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::QUERY_TYPE_OCCLUSION: return D3D11_QUERY_OCCLUSION; break;
        case grfx::QUERY_TYPE_TIMESTAMP: return D3D11_QUERY_TIMESTAMP; break;
        case grfx::QUERY_TYPE_PIPELINE_STATISTICS: return D3D11_QUERY_PIPELINE_STATISTICS; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_QUERY>();
}

D3D11_RTV_DIMENSION ToD3D11RTVDimension(grfx::ImageViewType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::IMAGE_VIEW_TYPE_1D       : return D3D11_RTV_DIMENSION_TEXTURE1D; break;
        case grfx::IMAGE_VIEW_TYPE_1D_ARRAY : return D3D11_RTV_DIMENSION_TEXTURE1DARRAY; break;
        case grfx::IMAGE_VIEW_TYPE_2D       : return D3D11_RTV_DIMENSION_TEXTURE2D; break;
        case grfx::IMAGE_VIEW_TYPE_2D_ARRAY : return D3D11_RTV_DIMENSION_TEXTURE2DARRAY; break;
        case grfx::IMAGE_VIEW_TYPE_3D       : return D3D11_RTV_DIMENSION_TEXTURE3D; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_RTV_DIMENSION>();
}

D3D11_STENCIL_OP ToD3D11StencilOp(grfx::StencilOp value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::STENCIL_OP_KEEP                : return D3D11_STENCIL_OP_KEEP; break;
        case grfx::STENCIL_OP_ZERO                : return D3D11_STENCIL_OP_ZERO; break;
        case grfx::STENCIL_OP_REPLACE             : return D3D11_STENCIL_OP_REPLACE; break;
        case grfx::STENCIL_OP_INCREMENT_AND_CLAMP : return D3D11_STENCIL_OP_INCR_SAT; break;
        case grfx::STENCIL_OP_DECREMENT_AND_CLAMP : return D3D11_STENCIL_OP_DECR_SAT; break;
        case grfx::STENCIL_OP_INVERT              : return D3D11_STENCIL_OP_INVERT; break;
        case grfx::STENCIL_OP_INCREMENT_AND_WRAP  : return D3D11_STENCIL_OP_INCR; break;
        case grfx::STENCIL_OP_DECREMENT_AND_WRAP  : return D3D11_STENCIL_OP_DECR; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_STENCIL_OP>();
}

D3D11_SRV_DIMENSION ToD3D11SRVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount)
{
    // clang-format off
    switch (value) {
        default: break;
        case IMAGE_VIEW_TYPE_1D         : return D3D11_SRV_DIMENSION_TEXTURE1D; break;
        case IMAGE_VIEW_TYPE_2D         : return D3D11_SRV_DIMENSION_TEXTURE2D; break;
        case IMAGE_VIEW_TYPE_3D         : return D3D11_SRV_DIMENSION_TEXTURE3D; break;
        case IMAGE_VIEW_TYPE_CUBE       : return D3D11_SRV_DIMENSION_TEXTURECUBE; break;
        case IMAGE_VIEW_TYPE_1D_ARRAY   : return D3D11_SRV_DIMENSION_TEXTURE1DARRAY; break;
        case IMAGE_VIEW_TYPE_2D_ARRAY   : return D3D11_SRV_DIMENSION_TEXTURE2DARRAY; break;
        case IMAGE_VIEW_TYPE_CUBE_ARRAY : return D3D11_SRV_DIMENSION_TEXTURECUBEARRAY; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_SRV_DIMENSION>();
}

D3D11_TEXTURE_ADDRESS_MODE ToD3D11TextureAddressMode(grfx::SamplerAddressMode value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::SAMPLER_ADDRESS_MODE_REPEAT          : return D3D11_TEXTURE_ADDRESS_WRAP; break;
        case grfx::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT : return D3D11_TEXTURE_ADDRESS_MIRROR; break;
        case grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE   : return D3D11_TEXTURE_ADDRESS_CLAMP; break;
        case grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : return D3D11_TEXTURE_ADDRESS_BORDER; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_TEXTURE_ADDRESS_MODE>();
}

D3D11_RESOURCE_DIMENSION ToD3D11TextureResourceDimension(grfx::ImageType value)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::IMAGE_TYPE_1D   : return D3D11_RESOURCE_DIMENSION_TEXTURE1D; break;
        case grfx::IMAGE_TYPE_2D   : return D3D11_RESOURCE_DIMENSION_TEXTURE2D; break;
        case grfx::IMAGE_TYPE_3D   : return D3D11_RESOURCE_DIMENSION_TEXTURE3D; break;
        case grfx::IMAGE_TYPE_CUBE : return D3D11_RESOURCE_DIMENSION_TEXTURE2D; break;
    }
    // clang-format on
    return D3D11_RESOURCE_DIMENSION_UNKNOWN;
}

D3D11_UAV_DIMENSION ToD3D11UAVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount)
{
    // clang-format off
    switch (value) {
        default: break;
        case IMAGE_VIEW_TYPE_1D   : return (arrayLayerCount > 1) ? D3D11_UAV_DIMENSION_TEXTURE1DARRAY : D3D11_UAV_DIMENSION_TEXTURE1D; break;
        case IMAGE_VIEW_TYPE_2D   : return (arrayLayerCount > 1) ? D3D11_UAV_DIMENSION_TEXTURE2DARRAY : D3D11_UAV_DIMENSION_TEXTURE2D; break;
        case IMAGE_VIEW_TYPE_3D   : return D3D11_UAV_DIMENSION_TEXTURE3D;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_UAV_DIMENSION>();
}

D3D11_USAGE ToD3D11Usage(grfx::MemoryUsage value, bool dynamic)
{
    // clang-format off
    switch (value) {
        default: break;
        case grfx::MEMORY_USAGE_GPU_ONLY   : return D3D11_USAGE_DEFAULT; break;
        case grfx::MEMORY_USAGE_CPU_ONLY   : return D3D11_USAGE_STAGING; break;
        case grfx::MEMORY_USAGE_CPU_TO_GPU : return dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_STAGING; break;
        case grfx::MEMORY_USAGE_GPU_TO_CPU : return D3D11_USAGE_STAGING; break;
    }
    // clang-format on
    return ppx::InvalidValue<D3D11_USAGE>();
}

UINT8 ToD3D11WriteMask(uint32_t value)
{
    UINT8 mask = 0;
    if (value & grfx::COLOR_COMPONENT_R)
        mask |= D3D11_COLOR_WRITE_ENABLE_RED;
    if (value & grfx::COLOR_COMPONENT_G)
        mask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    if (value & grfx::COLOR_COMPONENT_B)
        mask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    if (value & grfx::COLOR_COMPONENT_A)
        mask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    return mask;
}

UINT ToSubresourceIndex(uint32_t mipSlice, uint32_t arraySlice, uint32_t mipLevels)
{
    return mipSlice + (arraySlice * mipLevels);
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
