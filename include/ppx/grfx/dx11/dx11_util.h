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

#ifndef ppx_grfx_dx11_util_h
#define ppx_grfx_dx11_util_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/dx/dx_util.h"
#include "ppx/grfx/dx/d3dcompile_util.h"

#include <d3d11.h>

namespace ppx {
namespace grfx {
namespace dx11 {

D3D11_BLEND                ToD3D11Blend(grfx::BlendFactor value);
D3D11_BLEND_OP             ToD3D11BlendOp(grfx::BlendOp value);
UINT                       ToD3D11BindFlags(const grfx::BufferUsageFlags& value);
UINT                       ToD3D11BindFlags(const grfx::ImageUsageFlags& value);
D3D11_COMPARISON_FUNC      ToD3D11ComparisonFunc(grfx::CompareOp value);
D3D11_CULL_MODE            ToD3D11CullMode(grfx::CullMode value);
D3D11_DSV_DIMENSION        ToD3D11DSVDimension(grfx::ImageViewType value);
D3D11_FILL_MODE            ToD3D11FillMode(grfx::PolygonMode value);
D3D11_FILTER_TYPE          ToD3D11FilterType(grfx::Filter value);
D3D11_FILTER_TYPE          ToD3D11FilterType(grfx::SamplerMipmapMode value);
DXGI_FORMAT                ToD3D11IndexFormat(grfx::IndexType value);
UINT                       ToD3D11LogicOp(grfx::LogicOp value);
D3D11_PRIMITIVE_TOPOLOGY   ToD3D11PrimitiveTopology(grfx::PrimitiveTopology value);
D3D11_QUERY                ToD3D11QueryType(grfx::QueryType value);
D3D11_RTV_DIMENSION        ToD3D11RTVDimension(grfx::ImageViewType value);
D3D11_STENCIL_OP           ToD3D11StencilOp(grfx::StencilOp value);
D3D11_SRV_DIMENSION        ToD3D11SRVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount);
D3D11_TEXTURE_ADDRESS_MODE ToD3D11TextureAddressMode(grfx::SamplerAddressMode value);
D3D11_RESOURCE_DIMENSION   ToD3D11TextureResourceDimension(grfx::ImageType value);
D3D11_UAV_DIMENSION        ToD3D11UAVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount);
D3D11_USAGE                ToD3D11Usage(grfx::MemoryUsage value, bool dynamic = false);
UINT8                      ToD3D11WriteMask(uint32_t value);

UINT ToSubresourceIndex(uint32_t mipSlice, uint32_t arraySlice, uint32_t mipLevels);

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_util_h
