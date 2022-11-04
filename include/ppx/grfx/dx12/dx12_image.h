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

#ifndef ppx_grfx_dx12_image_h
#define ppx_grfx_dx12_image_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/dx12/dx12_descriptor_helper.h"
#include "ppx/grfx/grfx_image.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class Image
    : public grfx::Image
{
public:
    Image() {}
    virtual ~Image();

    typename D3D12ResourcePtr::InterfaceType* GetDxResource() const { return mResource.Get(); }

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) override;
    virtual void   UnmapMemory() override;

protected:
    virtual Result CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12ResourcePtr            mResource;
    ObjPtr<D3D12MA::Allocation> mAllocation;
};

// -------------------------------------------------------------------------------------------------

class Sampler
    : public grfx::Sampler
{
public:
    Sampler() {}
    virtual ~Sampler() {}

    const D3D12_SAMPLER_DESC& GetDesc() const { return mDesc; }

protected:
    virtual Result CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12_SAMPLER_DESC mDesc = {};
};

// -------------------------------------------------------------------------------------------------

class DepthStencilView
    : public grfx::DepthStencilView
{
public:
    DepthStencilView() {}
    virtual ~DepthStencilView() {}

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle() const { return mDescriptor.handle; }

protected:
    virtual Result CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12_DEPTH_STENCIL_VIEW_DESC mDesc       = {};
    dx12::DescriptorHandle        mDescriptor = {};
};

// -------------------------------------------------------------------------------------------------

class RenderTargetView
    : public grfx::RenderTargetView
{
public:
    RenderTargetView() {}
    virtual ~RenderTargetView() {}

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle() const { return mDescriptor.handle; }

protected:
    virtual Result CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12_RENDER_TARGET_VIEW_DESC mDesc       = {};
    dx12::DescriptorHandle        mDescriptor = {};
};

// -------------------------------------------------------------------------------------------------

class SampledImageView
    : public grfx::SampledImageView
{
public:
    SampledImageView() {}
    virtual ~SampledImageView() {}

    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const { return mDesc; }

protected:
    virtual Result CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC mDesc = {};
};

// -------------------------------------------------------------------------------------------------

class StorageImageView
    : public grfx::StorageImageView
{
public:
    StorageImageView() {}
    virtual ~StorageImageView() {}

    const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const { return mDesc; }

protected:
    virtual Result CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12_UNORDERED_ACCESS_VIEW_DESC mDesc = {};
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_image_h
