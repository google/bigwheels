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

#include "ppx/grfx/dx12/dx12_image.h"
#include "ppx/grfx/dx12/dx12_device.h"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// Image
// -------------------------------------------------------------------------------------------------
Image::~Image()
{
}

Result Image::CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo)
{
    // NOTE: D3D12 does not support mapping of texture resources without the
    //       use of the a custom heap. No plans to make this work currently.
    //       All texture resources must be GPU only.
    //
    if (pCreateInfo->memoryUsage != grfx::MEMORY_USAGE_GPU_ONLY) {
        PPX_ASSERT_MSG(false, "memory mapping of textures is not availalble in D3D12");
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    if (IsNull(pCreateInfo->pApiObject)) {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if (pCreateInfo->usageFlags.bits.storage) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (pCreateInfo->usageFlags.bits.colorAttachment) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (pCreateInfo->usageFlags.bits.depthStencilAttachment) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (pCreateInfo->concurrentMultiQueueUsage) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
        }

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension           = ToD3D12TextureResourceDimension(pCreateInfo->type);
        resourceDesc.Alignment           = 0;
        resourceDesc.Width               = static_cast<UINT64>(pCreateInfo->width);
        resourceDesc.Height              = static_cast<UINT64>(pCreateInfo->height);
        resourceDesc.DepthOrArraySize    = static_cast<UINT16>((pCreateInfo->type == grfx::IMAGE_TYPE_3D) ? pCreateInfo->depth : pCreateInfo->arrayLayerCount);
        resourceDesc.MipLevels           = static_cast<UINT16>(pCreateInfo->mipLevelCount);
        resourceDesc.Format              = dx::ToDxgiFormat(pCreateInfo->format);
        resourceDesc.SampleDesc.Count    = static_cast<UINT>(pCreateInfo->sampleCount);
        resourceDesc.SampleDesc.Quality  = 0;
        resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags               = flags;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType                 = ToD3D12HeapType(pCreateInfo->memoryUsage);

        D3D12_RESOURCE_STATES initialResourceState = ToD3D12ResourceStates(pCreateInfo->initialState, grfx::COMMAND_TYPE_GRAPHICS);

        // Optimized clear values
        bool              useClearValue = (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) || (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clearValue    = {};
        clearValue.Format               = resourceDesc.Format;
        if (pCreateInfo->usageFlags.bits.depthStencilAttachment) {
            clearValue.DepthStencil.Depth   = static_cast<FLOAT>(pCreateInfo->DSVClearValue.depth);
            clearValue.DepthStencil.Stencil = static_cast<UINT8>(pCreateInfo->DSVClearValue.stencil);
        }
        else {
            clearValue.Color[0] = static_cast<FLOAT>(pCreateInfo->RTVClearValue.rgba[0]);
            clearValue.Color[1] = static_cast<FLOAT>(pCreateInfo->RTVClearValue.rgba[1]);
            clearValue.Color[2] = static_cast<FLOAT>(pCreateInfo->RTVClearValue.rgba[2]);
            clearValue.Color[3] = static_cast<FLOAT>(pCreateInfo->RTVClearValue.rgba[3]);
        }

        dx12::Device* pDevice = ToApi(GetDevice());
        HRESULT       hr      = pDevice->GetAllocator()->CreateResource(
            &allocationDesc,
            &resourceDesc,
            initialResourceState,
            useClearValue ? &clearValue : nullptr,
            &mAllocation,
            IID_PPV_ARGS(&mResource));
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(D3D12Resource(Image), mResource.Get());
    }
    else {
        CComPtr<ID3D12Resource> resource = static_cast<ID3D12Resource*>(pCreateInfo->pApiObject);
        HRESULT                 hr       = resource.QueryInterface(&mResource);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "failed casting to ID3D12Resource");
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(D3D12Resource(Image | External), mResource.Get());
    }

    return ppx::SUCCESS;
}

void Image::DestroyApiObjects()
{
    // Reset if resource isn't external
    if (IsNull(mCreateInfo.pApiObject)) {
        if (mResource) {
            mResource.Reset();
        }
    }
    else {
        // Deatch if the resource is external
        mResource.Detach();
    }

    if (mAllocation) {
        mAllocation->Release();
        mAllocation.Reset();
    }
}

Result Image::MapMemory(uint64_t offset, void** ppMappedAddress)
{
    PPX_ASSERT_MSG(false, "memory mapping of textures is not availalble in D3D12");
    return ppx::ERROR_FAILED;
}

void Image::UnmapMemory()
{
    PPX_ASSERT_MSG(false, "memory mapping of textures is not availalble in D3D12");
}

// -------------------------------------------------------------------------------------------------
// Sampler
// -------------------------------------------------------------------------------------------------
Result Sampler::CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo)
{
    // minFilter, maxFilter, mipmapMode are all NEAREST
    D3D12_FILTER filter = pCreateInfo->compareEnable ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;

    mDesc.Filter         = D3D12_FILTER_MIN_MAG_MIP_POINT;
    mDesc.AddressU       = ToD3D12TextureAddressMode(pCreateInfo->addressModeU);
    mDesc.AddressV       = ToD3D12TextureAddressMode(pCreateInfo->addressModeV);
    mDesc.AddressW       = ToD3D12TextureAddressMode(pCreateInfo->addressModeW);
    mDesc.MipLODBias     = static_cast<FLOAT>(pCreateInfo->mipLodBias);
    mDesc.MaxAnisotropy  = 1;
    mDesc.ComparisonFunc = ToD3D12ComparisonFunc(pCreateInfo->compareOp);
    mDesc.BorderColor[0] = 0;
    mDesc.BorderColor[1] = 0;
    mDesc.BorderColor[2] = 0;
    mDesc.BorderColor[3] = 0;
    mDesc.MinLOD         = static_cast<FLOAT>(pCreateInfo->minLod);
    mDesc.MaxLOD         = static_cast<FLOAT>(pCreateInfo->maxLod);

    D3D12_FILTER_TYPE           minFilter = ToD3D12FilterType(pCreateInfo->minFilter);
    D3D12_FILTER_TYPE           magFilter = ToD3D12FilterType(pCreateInfo->magFilter);
    D3D12_FILTER_TYPE           mipFilter = ToD3D12FilterType(pCreateInfo->mipmapMode);
    D3D12_FILTER_REDUCTION_TYPE reduction = pCreateInfo->compareEnable ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;
    if (pCreateInfo->anisotropyEnable) {
        mDesc.Filter        = D3D12_ENCODE_ANISOTROPIC_FILTER(reduction);
        mDesc.MaxAnisotropy = static_cast<UINT>(std::max(1.0f, std::min(16.0f, pCreateInfo->maxAnisotropy)));
    }
    else {
        mDesc.Filter = D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, reduction);
    }

    if ((pCreateInfo->borderColor == grfx::BORDER_COLOR_FLOAT_OPAQUE_WHITE) || (pCreateInfo->borderColor == grfx::BORDER_COLOR_INT_OPAQUE_WHITE)) {
        mDesc.BorderColor[0] = 1;
        mDesc.BorderColor[1] = 1;
        mDesc.BorderColor[2] = 1;
        mDesc.BorderColor[3] = 1;
    }

    return ppx::SUCCESS;
}

void Sampler::DestroyApiObjects()
{
    mDesc = {};
}

// -------------------------------------------------------------------------------------------------
// DepthStencilView
// -------------------------------------------------------------------------------------------------
Result DepthStencilView::CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo)
{
    Result ppxres = ToApi(GetDevice())->AllocateDSVHandle(&mDescriptor);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "dx::Device::AllocateDSVHandle failed");
        return ppxres;
    }

    mDesc               = {};
    mDesc.Format        = dx::ToDxgiFormat(pCreateInfo->format);
    mDesc.ViewDimension = ToD3D12DSVDimension(pCreateInfo->imageViewType);
    mDesc.Flags         = D3D12_DSV_FLAG_NONE;

    switch (mDesc.ViewDimension) {
        default: {
            PPX_ASSERT_MSG(false, "unknown DSV dimension");
        } break;

        case D3D12_DSV_DIMENSION_TEXTURE1D: {
            mDesc.Texture1D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY: {
            mDesc.Texture1DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            mDesc.Texture1DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            mDesc.Texture1DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D12_DSV_DIMENSION_TEXTURE2D: {
            mDesc.Texture2D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY: {
            mDesc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            mDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            mDesc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;
    }

    D3D12DevicePtr device = ToApi(GetDevice())->GetDxDevice();
    device->CreateDepthStencilView(
        ToApi(pCreateInfo->pImage)->GetDxResource(),
        &mDesc,
        mDescriptor.handle);

    return ppx::SUCCESS;
}

void DepthStencilView::DestroyApiObjects()
{
    if (mDescriptor) {
        ToApi(GetDevice())->FreeRTVHandle(&mDescriptor);
        mDescriptor.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// RenderTargetView
// -------------------------------------------------------------------------------------------------
Result RenderTargetView::CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo)
{
    Result ppxres = ToApi(GetDevice())->AllocateRTVHandle(&mDescriptor);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "dx::Device::AllocateRTVHandle failed");
        return ppxres;
    }

    mDesc               = {};
    mDesc.Format        = dx::ToDxgiFormat(pCreateInfo->format);
    mDesc.ViewDimension = ToD3D12RTVDimension(pCreateInfo->imageViewType);

    switch (mDesc.ViewDimension) {
        default: {
            PPX_ASSERT_MSG(false, "unknown RTV dimension");
        } break;

        case D3D12_RTV_DIMENSION_TEXTURE1D: {
            mDesc.Texture1D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY: {
            mDesc.Texture1DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            mDesc.Texture1DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            mDesc.Texture1DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D12_RTV_DIMENSION_TEXTURE2D: {
            mDesc.Texture2D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY: {
            mDesc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            mDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            mDesc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D12_RTV_DIMENSION_TEXTURE3D: {
            mDesc.Texture3D.MipSlice    = static_cast<UINT>(pCreateInfo->mipLevel);
            mDesc.Texture3D.FirstWSlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            mDesc.Texture3D.WSize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;
    }

    D3D12DevicePtr device = ToApi(GetDevice())->GetDxDevice();
    device->CreateRenderTargetView(
        ToApi(pCreateInfo->pImage)->GetDxResource(),
        &mDesc,
        mDescriptor.handle);

    return ppx::SUCCESS;
}

void RenderTargetView::DestroyApiObjects()
{
    if (mDescriptor) {
        ToApi(GetDevice())->FreeRTVHandle(&mDescriptor);
        mDescriptor.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// SampledImageView
// -------------------------------------------------------------------------------------------------
Result SampledImageView::CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo)
{
    D3D12_SHADER_COMPONENT_MAPPING src0 = ToD3D12ShaderComponentMapping(pCreateInfo->components.r, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
    D3D12_SHADER_COMPONENT_MAPPING src1 = ToD3D12ShaderComponentMapping(pCreateInfo->components.g, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1);
    D3D12_SHADER_COMPONENT_MAPPING src2 = ToD3D12ShaderComponentMapping(pCreateInfo->components.b, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2);
    D3D12_SHADER_COMPONENT_MAPPING src3 = ToD3D12ShaderComponentMapping(pCreateInfo->components.a, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);

    // Intial format
    DXGI_FORMAT format = dx::ToDxgiFormat(pCreateInfo->format);

    // D3D12 doesn't allow the usage of D32_FLOAT in SRV so we'll need to readjust it
    if (format == DXGI_FORMAT_D32_FLOAT) {
        format = DXGI_FORMAT_R32_FLOAT;
    }

    mDesc.Format                  = format;
    mDesc.ViewDimension           = ToD3D12SRVDimension(pCreateInfo->imageViewType, pCreateInfo->arrayLayerCount);
    mDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(src0, src1, src2, src3);
    if (pCreateInfo->arrayLayerCount > 1) {
        mDesc.Texture2DArray.MostDetailedMip     = 0;
        mDesc.Texture2DArray.MipLevels           = static_cast<UINT>(pCreateInfo->mipLevelCount);
        mDesc.Texture2DArray.FirstArraySlice     = static_cast<UINT>(pCreateInfo->arrayLayer);
        mDesc.Texture2DArray.ArraySize           = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        mDesc.Texture2DArray.PlaneSlice          = 0;
        mDesc.Texture2DArray.ResourceMinLODClamp = static_cast<FLOAT>(pCreateInfo->mipLevel);
    }
    else {
        mDesc.Texture2D.MostDetailedMip     = static_cast<UINT>(pCreateInfo->mipLevel);
        mDesc.Texture2D.MipLevels           = static_cast<UINT>(pCreateInfo->mipLevelCount);
        mDesc.Texture2D.PlaneSlice          = 0;
        mDesc.Texture2D.ResourceMinLODClamp = static_cast<FLOAT>(pCreateInfo->mipLevel);
    }

    return ppx::SUCCESS;
}

void SampledImageView::DestroyApiObjects()
{
    mDesc = {};
}

// -------------------------------------------------------------------------------------------------
// StorageImageView
// -------------------------------------------------------------------------------------------------
Result StorageImageView::CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo)
{
    mDesc.Format        = dx::ToDxgiFormat(pCreateInfo->format);
    mDesc.ViewDimension = ToD3D12UAVDimension(pCreateInfo->imageViewType, pCreateInfo->arrayLayerCount);
    if (pCreateInfo->arrayLayerCount > 1) {
        mDesc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
        mDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
        mDesc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        mDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else {
        mDesc.Texture2D.MipSlice   = static_cast<UINT>(pCreateInfo->mipLevel);
        mDesc.Texture2D.PlaneSlice = 0;
    }

    return ppx::SUCCESS;
}

void StorageImageView::DestroyApiObjects()
{
    mDesc = {};
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
