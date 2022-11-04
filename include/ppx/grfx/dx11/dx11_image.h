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

#ifndef ppx_grfx_dx11_image_h
#define ppx_grfx_dx11_image_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_image.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Image
    : public grfx::Image
{
public:
    Image() {}
    virtual ~Image() {}

    typename D3D11Texture1DPtr::InterfaceType* GetDxTexture1D() const { return mTexture1D.Get(); }
    typename D3D11Texture2DPtr::InterfaceType* GetDxTexture2D() const { return mTexture2D.Get(); }
    typename D3D11Texture3DPtr::InterfaceType* GetDxTexture3D() const { return mTexture3D.Get(); }
    typename D3D11ResourcePtr::InterfaceType*  GetDxResource() const { return mResource.Get(); }

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) override;
    virtual void   UnmapMemory() override;

protected:
    virtual Result CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11Texture1DPtr mTexture1D;
    D3D11Texture2DPtr mTexture2D;
    D3D11Texture3DPtr mTexture3D;
    D3D11ResourcePtr  mResource;
};

// -------------------------------------------------------------------------------------------------

class Sampler
    : public grfx::Sampler
{
public:
    Sampler() {}
    virtual ~Sampler() {}

    typename D3D11SamplerStatePtr::InterfaceType* GetDxSamplerState() const { return mSamplerState.Get(); }

protected:
    virtual Result CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11SamplerStatePtr mSamplerState;
};

// -------------------------------------------------------------------------------------------------

class DepthStencilView
    : public grfx::DepthStencilView
{
public:
    DepthStencilView() {}
    virtual ~DepthStencilView() {}

    typename D3D11DepthStencilViewPtr::InterfaceType* GetDxDepthStencilView() const { return mDepthStencilView.Get(); }
    typename D3D11ResourcePtr::InterfaceType*         GetDxResource() const { return ToApi(mCreateInfo.pImage)->GetDxResource(); }

protected:
    virtual Result CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11DepthStencilViewPtr mDepthStencilView;
};

// -------------------------------------------------------------------------------------------------

class RenderTargetView
    : public grfx::RenderTargetView
{
public:
    RenderTargetView() {}
    virtual ~RenderTargetView() {}

    typename D3D11RenderTargetViewPtr::InterfaceType* GetDxRenderTargetView() const { return mRenderTargetView.Get(); }
    typename D3D11ResourcePtr::InterfaceType*         GetDxResource() const { return ToApi(mCreateInfo.pImage)->GetDxResource(); }

protected:
    virtual Result CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11RenderTargetViewPtr mRenderTargetView;
};

// -------------------------------------------------------------------------------------------------

class SampledImageView
    : public grfx::SampledImageView
{
public:
    SampledImageView() {}
    virtual ~SampledImageView() {}

    typename D3D11ShaderResourceViewPtr::InterfaceType* GetDxShaderResourceView() const { return mShaderResourceView.Get(); }
    typename D3D11ResourcePtr::InterfaceType*           GetDxResource() const { return ToApi(mCreateInfo.pImage)->GetDxResource(); }

protected:
    virtual Result CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11ShaderResourceViewPtr mShaderResourceView;
};

// -------------------------------------------------------------------------------------------------

class StorageImageView
    : public grfx::StorageImageView
{
public:
    StorageImageView() {}
    virtual ~StorageImageView() {}

    typename D3D11UnorderedAccessViewPtr::InterfaceType* GetDxUnorderedAccessView() const { return mUnorderedAccessView.Get(); }
    typename D3D11ResourcePtr::InterfaceType*            GetDxResource() const { return ToApi(mCreateInfo.pImage)->GetDxResource(); }

protected:
    virtual Result CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11UnorderedAccessViewPtr mUnorderedAccessView;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_image_h
