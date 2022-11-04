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

#ifndef ppx_grfx_dx11_config_h
#define ppx_grfx_dx11_config_h

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/dx11/dx11_util.h"

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#if defined(PPX_MSW)
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#else
template <typename T>
using ComPtr                      = ObjPtr<T>;
#endif

#if defined(PPX_ENABLE_LOG_OBJECT_CREATION)
#define PPX_LOG_OBJECT_CREATION(TAG, ADDR) \
    PPX_LOG_INFO("DX OBJECT CREATED: addr=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << reinterpret_cast<uintptr_t>(ADDR) << ", type="## #TAG)
#else
#define PPX_LOG_OBJECT_CREATION(TAG, ADDR)
#endif

namespace ppx {
namespace grfx {
namespace dx11 {

using DXGIAdapterPtr              = ComPtr<IDXGIAdapter4>;
using DXGIDebugPtr                = ComPtr<IDXGIDebug1>;
using DXGIInfoQueuePtr            = ComPtr<IDXGIInfoQueue>;
using DXGIFactoryPtr              = ComPtr<IDXGIFactory7>;
using DXGISwapChainPtr            = ComPtr<IDXGISwapChain4>;
using D3D11BlendStatePtr          = ComPtr<ID3D11BlendState1>;
using D3D11BufferPtr              = ComPtr<ID3D11Buffer>;
using D3D11DepthStencilStatePtr   = ComPtr<ID3D11DepthStencilState>;
using D3D11DepthStencilViewPtr    = ComPtr<ID3D11DepthStencilView>;
using D3D11DevicePtr              = ComPtr<ID3D11Device5>;
using D3D11DeviceContextPtr       = ComPtr<ID3D11DeviceContext3>;
using D3D11InputLayoutPtr         = ComPtr<ID3D11InputLayout>;
using D3D11RasterizerStatePtr     = ComPtr<ID3D11RasterizerState2>;
using D3D11RenderTargetViewPtr    = ComPtr<ID3D11RenderTargetView1>;
using D3D11ResourcePtr            = ComPtr<ID3D11Resource>;
using D3D11SamplerStatePtr        = ComPtr<ID3D11SamplerState>;
using D3D11ShaderResourceViewPtr  = ComPtr<ID3D11ShaderResourceView1>;
using D3D11Texture1DPtr           = ComPtr<ID3D11Texture1D>;
using D3D11Texture2DPtr           = ComPtr<ID3D11Texture2D1>;
using D3D11Texture3DPtr           = ComPtr<ID3D11Texture3D1>;
using D3D11UnorderedAccessViewPtr = ComPtr<ID3D11UnorderedAccessView1>;
using D3D11ComputeShaderPtr  = ComPtr<ID3D11ComputeShader>;
using D3D11DomainShaderPtr   = ComPtr<ID3D11DomainShader>;
using D3D11GeometryShaderPtr = ComPtr<ID3D11GeometryShader>;
using D3D11HullShaderPtr     = ComPtr<ID3D11HullShader>;
using D3D11PixelShaderPtr    = ComPtr<ID3D11PixelShader>;
using D3D11VertexShaderPtr   = ComPtr<ID3D11VertexShader>;

// -------------------------------------------------------------------------------------------------

class Buffer;
class CommandBuffer;
class CommandPool;
class ComputePipeline;
class DepthStencilView;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class Device;
class Fence;
class Gpu;
class GraphicsPipeline;
class Image;
class Instance;
class Pipeline;
class PipelineInterface;
class Queue;
class Query;
class RenderPass;
class RenderTargetView;
class Sampler;
class Semaphore;
class ShaderModule;
class Surface;
class Swapchain;

template <typename GrfxTypeT>
struct ApiObjectLookUp
{
};

template <>
struct ApiObjectLookUp<grfx::Buffer>
{
    using GrfxType = grfx::Buffer;
    using ApiType  = dx11::Buffer;
};

template <>
struct ApiObjectLookUp<grfx::CommandBuffer>
{
    using GrfxType = grfx::CommandBuffer;
    using ApiType  = dx11::CommandBuffer;
};

template <>
struct ApiObjectLookUp<grfx::CommandPool>
{
    using GrfxType = grfx::CommandPool;
    using ApiType  = dx11::CommandPool;
};

template <>
struct ApiObjectLookUp<grfx::ComputePipeline>
{
    using GrfxType = grfx::ComputePipeline;
    using ApiType  = dx11::ComputePipeline;
};

template <>
struct ApiObjectLookUp<grfx::DescriptorPool>
{
    using GrfxType = grfx::DescriptorPool;
    using ApiType  = dx11::DescriptorPool;
};

template <>
struct ApiObjectLookUp<grfx::DescriptorSet>
{
    using GrfxType = grfx::DescriptorSet;
    using ApiType  = dx11::DescriptorSet;
};

template <>
struct ApiObjectLookUp<grfx::DescriptorSetLayout>
{
    using GrfxType = grfx::DescriptorSetLayout;
    using ApiType  = dx11::DescriptorSetLayout;
};

template <>
struct ApiObjectLookUp<grfx::DepthStencilView>
{
    using GrfxType = grfx::DepthStencilView;
    using ApiType  = dx11::DepthStencilView;
};

template <>
struct ApiObjectLookUp<grfx::Device>
{
    using GrfxType = grfx::Device;
    using ApiType  = dx11::Device;
};

template <>
struct ApiObjectLookUp<grfx::Fence>
{
    using GrfxType = grfx::Fence;
    using ApiType  = dx11::Fence;
};

template <>
struct ApiObjectLookUp<grfx::GraphicsPipeline>
{
    using GrfxType = grfx::GraphicsPipeline;
    using ApiType  = dx11::GraphicsPipeline;
};

template <>
struct ApiObjectLookUp<grfx::Image>
{
    using GrfxType = grfx::Image;
    using ApiType  = dx11::Image;
};

template <>
struct ApiObjectLookUp<grfx::Instance>
{
    using GrfxType = grfx::Instance;
    using ApiType  = Instance;
};

template <>
struct ApiObjectLookUp<grfx::Gpu>
{
    using GrfxType = grfx::Gpu;
    using ApiType  = dx11::Gpu;
};

template <>
struct ApiObjectLookUp<grfx::Queue>
{
    using GrfxType = grfx::Queue;
    using ApiType  = dx11::Queue;
};

template <>
struct ApiObjectLookUp<grfx::Query>
{
    using GrfxType = grfx::Query;
    using ApiType  = dx11::Query;
};

template <>
struct ApiObjectLookUp<grfx::PipelineInterface>
{
    using GrfxType = grfx::PipelineInterface;
    using ApiType  = dx11::PipelineInterface;
};

template <>
struct ApiObjectLookUp<grfx::RenderPass>
{
    using GrfxType = grfx::RenderPass;
    using ApiType  = dx11::RenderPass;
};

template <>
struct ApiObjectLookUp<grfx::RenderTargetView>
{
    using GrfxType = grfx::RenderTargetView;
    using ApiType  = dx11::RenderTargetView;
};

template <>
struct ApiObjectLookUp<grfx::Sampler>
{
    using GrfxType = grfx::Sampler;
    using ApiType  = dx11::Sampler;
};

template <>
struct ApiObjectLookUp<grfx::Semaphore>
{
    using GrfxType = grfx::Semaphore;
    using ApiType  = dx11::Semaphore;
};

template <>
struct ApiObjectLookUp<grfx::ShaderModule>
{
    using GrfxType = grfx::ShaderModule;
    using ApiType  = dx11::ShaderModule;
};

template <>
struct ApiObjectLookUp<grfx::Surface>
{
    using GrfxType = grfx::Surface;
    using ApiType  = dx11::Surface;
};

template <>
struct ApiObjectLookUp<grfx::Swapchain>
{
    using GrfxType = grfx::Swapchain;
    using ApiType  = dx11::Swapchain;
};
template <typename GrfxTypeT>
typename ApiObjectLookUp<GrfxTypeT>::ApiType* ToApi(GrfxTypeT* pGrfxObject)
{
    using ApiType       = typename ApiObjectLookUp<GrfxTypeT>::ApiType;
    ApiType* pApiObject = static_cast<ApiType*>(pGrfxObject);
    return pApiObject;
}

template <typename GrfxTypeT>
const typename ApiObjectLookUp<GrfxTypeT>::ApiType* ToApi(const GrfxTypeT* pGrfxObject)
{
    using ApiType             = typename ApiObjectLookUp<GrfxTypeT>::ApiType;
    const ApiType* pApiObject = static_cast<const ApiType*>(pGrfxObject);
    return pApiObject;
}

template <typename GrfxTypeT>
typename ApiObjectLookUp<GrfxTypeT>::ApiType* ToApi(ObjPtr<GrfxTypeT>& pGrfxObject)
{
    using ApiType       = typename ApiObjectLookUp<GrfxTypeT>::ApiType;
    ApiType* pApiObject = static_cast<ApiType*>(pGrfxObject.Get());
    return pApiObject;
}

template <typename GrfxTypeT>
const typename ApiObjectLookUp<GrfxTypeT>::ApiType* ToApi(const ObjPtr<GrfxTypeT>& pGrfxObject)
{
    using ApiType       = typename ApiObjectLookUp<GrfxTypeT>::ApiType;
    ApiType* pApiObject = static_cast<ApiType*>(pGrfxObject.Get());
    return pApiObject;
}

// -------------------------------------------------------------------------------------------------

struct DescriptorArray
{
    uint32_t                binding          = UINT32_MAX;
    grfx::D3DDescriptorType descriptorType   = grfx::D3D_DESCRIPTOR_TYPE_UNDEFINED;
    grfx::ShaderStageBits   shaderVisibility = grfx::SHADER_STAGE_UNDEFINED;
    std::vector<void*>      resources;
};

// -------------------------------------------------------------------------------------------------

const uint32_t kInvalidStateIndex = InvalidValue<uint32_t>();

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_config_h
