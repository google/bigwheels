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

#ifndef ppx_grfx_dx12_util_h
#define ppx_grfx_dx12_util_h

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/dx/dx_util.h"

#include <d3d12.h>

namespace ppx {
namespace grfx {
namespace dx12 {

D3D12_BLEND                    ToD3D12Blend(grfx::BlendFactor value);
D3D12_BLEND_OP                 ToD3D12BlendOp(grfx::BlendOp value);
D3D12_COMPARISON_FUNC          ToD3D12ComparisonFunc(grfx::CompareOp value);
D3D12_CULL_MODE                ToD3D12CullMode(grfx::CullMode value);
D3D12_DSV_DIMENSION            ToD3D12DSVDimension(grfx::ImageViewType value);
D3D12_FILL_MODE                ToD3D12FillMode(grfx::PolygonMode value);
D3D12_FILTER_TYPE              ToD3D12FilterType(grfx::Filter value);
D3D12_FILTER_TYPE              ToD3D12FilterType(grfx::SamplerMipmapMode value);
D3D12_HEAP_TYPE                ToD3D12HeapType(grfx::MemoryUsage value);
DXGI_FORMAT                    ToD3D12IndexFormat(grfx::IndexType value);
D3D12_LOGIC_OP                 ToD3D12LogicOp(grfx::LogicOp value);
D3D12_PRIMITIVE_TOPOLOGY_TYPE  ToD3D12PrimitiveTopology(grfx::PrimitiveTopology value);
D3D12_QUERY_TYPE               ToD3D12QueryType(grfx::QueryType value);
D3D12_QUERY_HEAP_TYPE          ToD3D12QueryHeapType(grfx::QueryType value);
D3D12_DESCRIPTOR_RANGE_TYPE    ToD3D12RangeType(grfx::DescriptorType value);
D3D12_RESOURCE_STATES          ToD3D12ResourceStates(grfx::ResourceState value, grfx::CommandType commandType);
D3D12_RTV_DIMENSION            ToD3D12RTVDimension(grfx::ImageViewType value);
D3D12_SHADER_COMPONENT_MAPPING ToD3D12ShaderComponentMapping(grfx::ComponentSwizzle value, D3D12_SHADER_COMPONENT_MAPPING identity);
D3D12_SHADER_VISIBILITY        ToD3D12ShaderVisibliity(grfx::ShaderStageBits value);
D3D12_SRV_DIMENSION            ToD3D12SRVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount);
D3D12_STENCIL_OP               ToD3D12StencilOp(grfx::StencilOp value);
D3D12_TEXTURE_ADDRESS_MODE     ToD3D12TextureAddressMode(grfx::SamplerAddressMode value);
D3D12_RESOURCE_DIMENSION       ToD3D12TextureResourceDimension(grfx::ImageType value);
D3D12_UAV_DIMENSION            ToD3D12UAVDimension(grfx::ImageViewType value, uint32_t arrayLayerCount);
UINT8                          ToD3D12WriteMask(uint32_t value);

UINT ToSubresourceIndex(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize);

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_util_h
