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

#include "ppx/grfx/dx11/dx11_image.h"
#include "ppx/grfx/dx11/dx11_device.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// Image
// -------------------------------------------------------------------------------------------------
Result Image::CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo)
{
    if (IsNull(pCreateInfo->pApiObject)) {
        D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

        switch (pCreateInfo->type) {
            default: break;
            case grfx::IMAGE_TYPE_1D: {
                D3D11_TEXTURE1D_DESC desc = {};
                desc.Width                = static_cast<UINT>(pCreateInfo->width);
                desc.MipLevels            = static_cast<UINT>(pCreateInfo->mipLevelCount);
                desc.ArraySize            = static_cast<UINT>(pCreateInfo->arrayLayerCount);
                desc.Format               = dx::ToDxgiFormat(pCreateInfo->format);
                desc.Usage                = D3D11_USAGE_DEFAULT;
                desc.BindFlags            = ToD3D11BindFlags(pCreateInfo->usageFlags);
                desc.CPUAccessFlags       = 0;
                desc.MiscFlags            = 0;

                HRESULT hr = device->CreateTexture1D(&desc, nullptr, &mTexture1D);
                if (FAILED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }
            } break;

            case grfx::IMAGE_TYPE_2D:
            case grfx::IMAGE_TYPE_CUBE: {
                //
                // If we need a format that's compatible with both DXGI_FORMAT_D32_FLOAT (DSV)
                // and DXGI_FORMAT_R32_FLOAT (SRV) then use DXGI_FORMAT_R32_TYPELESS.
                //
                DXGI_FORMAT format = dx::ToDxgiFormat(pCreateInfo->format);
                if (pCreateInfo->usageFlags.bits.sampled && pCreateInfo->usageFlags.bits.depthStencilAttachment) {
                    if (format == DXGI_FORMAT_D32_FLOAT) {
                        format = DXGI_FORMAT_R32_TYPELESS;
                    }
                }

                D3D11_TEXTURE2D_DESC1 desc = {};
                desc.Width                 = static_cast<UINT>(pCreateInfo->width);
                desc.Height                = static_cast<UINT>(pCreateInfo->height);
                desc.MipLevels             = static_cast<UINT>(pCreateInfo->mipLevelCount);
                desc.ArraySize             = static_cast<UINT>(pCreateInfo->arrayLayerCount);
                desc.Format                = format;
                desc.SampleDesc.Count      = static_cast<UINT>(pCreateInfo->sampleCount);
                desc.SampleDesc.Quality    = 0;
                desc.Usage                 = D3D11_USAGE_DEFAULT;
                desc.BindFlags             = ToD3D11BindFlags(pCreateInfo->usageFlags);
                desc.CPUAccessFlags        = 0;
                desc.MiscFlags             = (pCreateInfo->type == grfx::IMAGE_TYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
                desc.TextureLayout         = D3D11_TEXTURE_LAYOUT_UNDEFINED;

                HRESULT hr = device->CreateTexture2D1(&desc, nullptr, &mTexture2D);
                if (FAILED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }
            } break;

            case grfx::IMAGE_TYPE_3D: {
                D3D11_TEXTURE3D_DESC1 desc = {};
                desc.Width                 = static_cast<UINT>(pCreateInfo->width);
                desc.Height                = static_cast<UINT>(pCreateInfo->height);
                desc.Depth                 = static_cast<UINT>(pCreateInfo->depth);
                desc.MipLevels             = static_cast<UINT>(pCreateInfo->mipLevelCount);
                desc.Format                = dx::ToDxgiFormat(pCreateInfo->format);
                desc.Usage                 = D3D11_USAGE_DEFAULT;
                desc.BindFlags             = ToD3D11BindFlags(pCreateInfo->usageFlags);
                desc.CPUAccessFlags        = 0;
                desc.MiscFlags             = 0;
                desc.TextureLayout         = D3D11_TEXTURE_LAYOUT_UNDEFINED;

                HRESULT hr = device->CreateTexture3D1(&desc, nullptr, &mTexture3D);
                if (FAILED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }
            } break;
        }
    }
    else {
        switch (pCreateInfo->type) {
            default: break;

            case grfx::IMAGE_TYPE_1D: {
                mTexture1D = static_cast<typename D3D11Texture1DPtr::InterfaceType*>(pCreateInfo->pApiObject);
            } break;

            case grfx::IMAGE_TYPE_2D: {
                mTexture2D = static_cast<typename D3D11Texture2DPtr::InterfaceType*>(pCreateInfo->pApiObject);
            } break;

            case grfx::IMAGE_TYPE_3D: {
                mTexture3D = static_cast<typename D3D11Texture3DPtr::InterfaceType*>(pCreateInfo->pApiObject);
            } break;
        }
    }

    // Resource
    switch (pCreateInfo->type) {
        default: break;
        case grfx::IMAGE_TYPE_1D: {
            mResource = mTexture1D.Get();
        } break;

        case grfx::IMAGE_TYPE_2D:
        case grfx::IMAGE_TYPE_CUBE: {
            mResource = mTexture2D.Get();
        } break;

        case grfx::IMAGE_TYPE_3D: {
            mResource = mTexture3D.Get();
        } break;
    }

    return ppx::SUCCESS;
}

void Image::DestroyApiObjects()
{
    if (mResource) {
        mResource.Reset();
    }

    // Reset if resource isn't external
    if (IsNull(mCreateInfo.pApiObject)) {
        if (mTexture1D) {
            mTexture1D.Reset();
        }

        if (mTexture2D) {
            mTexture2D.Reset();
        }

        if (mTexture3D) {
            mTexture3D.Reset();
        }
    }
    else {
        // Deatch if the resource is external

        if (mTexture1D) {
            mTexture1D.Detach();
        }

        if (mTexture2D) {
            mTexture2D.Detach();
        }

        if (mTexture3D) {
            mTexture3D.Detach();
        }
    }
}

Result Image::MapMemory(uint64_t offset, void** ppMappedAddress)
{
    PPX_ASSERT_MSG(false, "memory mapping of textures is not availalble in D3D11");
    return ppx::ERROR_FAILED;
}

void Image::UnmapMemory()
{
    PPX_ASSERT_MSG(false, "memory mapping of textures is not availalble in D3D11");
}

// -------------------------------------------------------------------------------------------------
// Sampler
// -------------------------------------------------------------------------------------------------
Result Sampler::CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    D3D11_SAMPLER_DESC desc = {};

    // minFilter, maxFilter, mipmapMode are all NEAREST
    D3D11_FILTER filter = pCreateInfo->compareEnable ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;

    desc.Filter         = filter;
    desc.AddressU       = ToD3D11TextureAddressMode(pCreateInfo->addressModeU);
    desc.AddressV       = ToD3D11TextureAddressMode(pCreateInfo->addressModeV);
    desc.AddressW       = ToD3D11TextureAddressMode(pCreateInfo->addressModeW);
    desc.MipLODBias     = static_cast<FLOAT>(pCreateInfo->mipLodBias);
    desc.MaxAnisotropy  = 1;
    desc.ComparisonFunc = ToD3D11ComparisonFunc(pCreateInfo->compareOp);
    desc.BorderColor[0] = 0;
    desc.BorderColor[1] = 0;
    desc.BorderColor[2] = 0;
    desc.BorderColor[3] = 0;
    desc.MinLOD         = static_cast<FLOAT>(pCreateInfo->minLod);
    desc.MaxLOD         = static_cast<FLOAT>(pCreateInfo->maxLod);

    D3D11_FILTER_TYPE           minFilter = ToD3D11FilterType(pCreateInfo->minFilter);
    D3D11_FILTER_TYPE           magFilter = ToD3D11FilterType(pCreateInfo->magFilter);
    D3D11_FILTER_TYPE           mipFilter = ToD3D11FilterType(pCreateInfo->mipmapMode);
    D3D11_FILTER_REDUCTION_TYPE reduction = pCreateInfo->compareEnable ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;
    if (pCreateInfo->anisotropyEnable) {
        desc.Filter        = D3D11_ENCODE_ANISOTROPIC_FILTER(reduction);
        desc.MaxAnisotropy = static_cast<UINT>(std::max(1.0f, std::min(16.0f, pCreateInfo->maxAnisotropy)));
    }
    else {
        desc.Filter = D3D11_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, reduction);
    }

    if ((pCreateInfo->borderColor == grfx::BORDER_COLOR_FLOAT_OPAQUE_WHITE) || (pCreateInfo->borderColor == grfx::BORDER_COLOR_INT_OPAQUE_WHITE)) {
        desc.BorderColor[0] = 1;
        desc.BorderColor[1] = 1;
        desc.BorderColor[2] = 1;
        desc.BorderColor[3] = 1;
    }

    HRESULT hr = device->CreateSamplerState(&desc, &mSamplerState);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Sampler::DestroyApiObjects()
{
}

// -------------------------------------------------------------------------------------------------
// DepthStencilView
// -------------------------------------------------------------------------------------------------
Result DepthStencilView::CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    ID3D11Resource* pResource = nullptr;
    switch (pCreateInfo->pImage->GetType()) {
        default: break;
        case grfx::IMAGE_TYPE_1D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture1D();
        } break;
        case grfx::IMAGE_TYPE_2D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture2D();
        } break;
        case grfx::IMAGE_TYPE_3D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture3D();
        } break;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
    desc.Format                        = dx::ToDxgiFormat(pCreateInfo->format);
    desc.ViewDimension                 = ToD3D11DSVDimension(pCreateInfo->imageViewType);

    switch (desc.ViewDimension) {
        default: {
            PPX_ASSERT_MSG(false, "unknown DSV dimension");
        } break;

        case D3D11_DSV_DIMENSION_TEXTURE1D: {
            desc.Texture1D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D11_DSV_DIMENSION_TEXTURE1DARRAY: {
            desc.Texture1DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            desc.Texture1DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            desc.Texture1DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D11_DSV_DIMENSION_TEXTURE2D: {
            desc.Texture2D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D11_DSV_DIMENSION_TEXTURE2DARRAY: {
            desc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            desc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;
    }

    HRESULT hr = device->CreateDepthStencilView(pResource, &desc, &mDepthStencilView);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void DepthStencilView::DestroyApiObjects()
{
    if (mDepthStencilView) {
        mDepthStencilView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// RenderTargetView
// -------------------------------------------------------------------------------------------------
Result RenderTargetView::CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    ID3D11Resource* pResource = nullptr;
    switch (pCreateInfo->pImage->GetType()) {
        default: break;
        case grfx::IMAGE_TYPE_1D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture1D();
        } break;
        case grfx::IMAGE_TYPE_2D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture2D();
        } break;
        case grfx::IMAGE_TYPE_3D: {
            pResource = ToApi(pCreateInfo->pImage)->GetDxTexture3D();
        } break;
    }

    D3D11_RENDER_TARGET_VIEW_DESC1 desc = {};
    desc.Format                         = dx::ToDxgiFormat(pCreateInfo->format);
    desc.ViewDimension                  = ToD3D11RTVDimension(pCreateInfo->imageViewType);

    switch (desc.ViewDimension) {
        default: {
            PPX_ASSERT_MSG(false, "unknown RTV dimension");
        } break;

        case D3D11_RTV_DIMENSION_TEXTURE1D: {
            desc.Texture1D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY: {
            desc.Texture1DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            desc.Texture1DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            desc.Texture1DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D11_RTV_DIMENSION_TEXTURE2D: {
            desc.Texture2D.MipSlice = static_cast<UINT>(pCreateInfo->mipLevel);
        } break;

        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY: {
            desc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
            desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            desc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;

        case D3D11_RTV_DIMENSION_TEXTURE3D: {
            desc.Texture3D.MipSlice    = static_cast<UINT>(pCreateInfo->mipLevel);
            desc.Texture3D.FirstWSlice = static_cast<UINT>(pCreateInfo->arrayLayer);
            desc.Texture3D.WSize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        } break;
    }

    HRESULT hr = device->CreateRenderTargetView1(pResource, &desc, &mRenderTargetView);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void RenderTargetView::DestroyApiObjects()
{
    if (mRenderTargetView) {
        mRenderTargetView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// SampledImageView
// -------------------------------------------------------------------------------------------------
Result SampledImageView::CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    // Intial format
    DXGI_FORMAT format = dx::ToDxgiFormat(pCreateInfo->format);

    // D3D12 doesn't allow the usage of D32_FLOAT in SRV so we'll need to readjust it
    if (format == DXGI_FORMAT_D32_FLOAT) {
        format = DXGI_FORMAT_R32_FLOAT;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC1 desc = {};

    desc.Format        = format;
    desc.ViewDimension = ToD3D11SRVDimension(pCreateInfo->imageViewType, pCreateInfo->arrayLayerCount);
    if (pCreateInfo->arrayLayerCount > 1) {
        desc.TextureCube.MostDetailedMip = static_cast<UINT>(pCreateInfo->mipLevel);
        desc.TextureCube.MipLevels       = static_cast<UINT>(pCreateInfo->mipLevelCount);

        desc.Texture2DArray.MostDetailedMip = static_cast<UINT>(pCreateInfo->mipLevel);
        desc.Texture2DArray.MipLevels       = static_cast<UINT>(pCreateInfo->mipLevelCount);
        desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
        desc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        desc.Texture2DArray.PlaneSlice      = 0;
    }
    else {
        desc.Texture2D.MostDetailedMip = static_cast<UINT>(pCreateInfo->mipLevel);
        desc.Texture2D.MipLevels       = static_cast<UINT>(pCreateInfo->mipLevelCount);
        desc.Texture2D.PlaneSlice      = 0;
    }

    HRESULT hr = device->CreateShaderResourceView1(
        ToApi(pCreateInfo->pImage)->GetDxResource(),
        &desc,
        &mShaderResourceView);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void SampledImageView::DestroyApiObjects()
{
    if (mShaderResourceView) {
        mShaderResourceView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// StorageImageView
// -------------------------------------------------------------------------------------------------
Result StorageImageView::CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc = {};

    desc.Format        = dx::ToDxgiFormat(pCreateInfo->format);
    desc.ViewDimension = ToD3D11UAVDimension(pCreateInfo->imageViewType, pCreateInfo->arrayLayerCount);
    if (pCreateInfo->arrayLayerCount > 1) {
        desc.Texture2DArray.MipSlice        = static_cast<UINT>(pCreateInfo->mipLevel);
        desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(pCreateInfo->arrayLayer);
        desc.Texture2DArray.ArraySize       = static_cast<UINT>(pCreateInfo->arrayLayerCount);
        desc.Texture2DArray.PlaneSlice      = 0;
    }
    else {
        desc.Texture2D.MipSlice   = static_cast<UINT>(pCreateInfo->mipLevel);
        desc.Texture2D.PlaneSlice = 0;
    }

    HRESULT hr = device->CreateUnorderedAccessView1(
        ToApi(pCreateInfo->pImage)->GetDxResource(),
        &desc,
        &mUnorderedAccessView);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void StorageImageView::DestroyApiObjects()
{
    if (mUnorderedAccessView) {
        mUnorderedAccessView.Reset();
    }
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
