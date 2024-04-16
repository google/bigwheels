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

#ifndef ppx_grfx_vk_image_h
#define ppx_grfx_vk_image_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_image.h"

namespace ppx {
namespace grfx {
namespace vk {

class Image
    : public grfx::Image
{
public:
    Image() {}
    virtual ~Image() {}

    VkImagePtr         GetVkImage() const { return mImage; }
    VkFormat           GetVkFormat() const { return mVkFormat; }
    VkImageAspectFlags GetVkImageAspectFlags() const { return mImageAspect; }

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) override;
    virtual void   UnmapMemory() override;

protected:
    virtual Result CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkImagePtr         mImage;
    VmaAllocationPtr   mAllocation;
    VmaAllocationInfo  mAllocationInfo = {};
    VkFormat           mVkFormat       = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags mImageAspect    = ppx::InvalidValue<VkImageAspectFlags>();
};

// -------------------------------------------------------------------------------------------------

namespace internal {

class ImageResourceView
    : public grfx::internal::ImageResourceView
{
public:
    ImageResourceView(VkImageViewPtr vkImageView, VkImageLayout layout)
        : mImageView(vkImageView), mImageLayout(layout) {}
    virtual ~ImageResourceView() {}

    VkImageViewPtr GetVkImageView() const { return mImageView; }
    VkImageLayout  GetVkImageLayout() const { return mImageLayout; }

private:
    VkImageViewPtr mImageView;
    VkImageLayout  mImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

} // namespace internal

// -------------------------------------------------------------------------------------------------

class Sampler
    : public grfx::Sampler
{
public:
    Sampler() {}
    virtual ~Sampler() {}

    VkSamplerPtr GetVkSampler() const { return mSampler; }

protected:
    virtual Result CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkSamplerPtr mSampler;
};

// -------------------------------------------------------------------------------------------------

class DepthStencilView
    : public grfx::DepthStencilView
{
public:
    DepthStencilView() {}
    virtual ~DepthStencilView() {}

    VkImageViewPtr GetVkImageView() const { return mImageView; }

protected:
    virtual Result CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkImageViewPtr mImageView;
};

// -------------------------------------------------------------------------------------------------

class RenderTargetView
    : public grfx::RenderTargetView
{
public:
    RenderTargetView() {}
    virtual ~RenderTargetView() {}

    VkImageViewPtr GetVkImageView() const { return mImageView; }

protected:
    virtual Result CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkImageViewPtr mImageView;
};

// -------------------------------------------------------------------------------------------------

class SampledImageView
    : public grfx::SampledImageView
{
public:
    SampledImageView() {}
    virtual ~SampledImageView() {}

    VkImageViewPtr GetVkImageView() const { return mImageView; }

protected:
    virtual Result CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkImageViewPtr mImageView;
};

// -------------------------------------------------------------------------------------------------

class SamplerYcbcrConversion
    : public grfx::SamplerYcbcrConversion
{
public:
    SamplerYcbcrConversion() {}
    virtual ~SamplerYcbcrConversion() {}

    VkSamplerYcbcrConversionPtr GetVkSamplerYcbcrConversion() const { return mSamplerYcbcrConversion; }

protected:
    virtual Result CreateApiObjects(const grfx::SamplerYcbcrConversionCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkSamplerYcbcrConversionPtr mSamplerYcbcrConversion;
};

// -------------------------------------------------------------------------------------------------

class StorageImageView
    : public grfx::StorageImageView
{
public:
    StorageImageView() {}
    virtual ~StorageImageView() {}

    VkImageViewPtr GetVkImageView() const { return mImageView; }

protected:
    virtual Result CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkImageViewPtr mImageView;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_image_h
