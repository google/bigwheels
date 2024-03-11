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

#include "ppx/grfx/vk/vk_image.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_queue.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// Image
// -------------------------------------------------------------------------------------------------
Result Image::CreateApiObjects(const grfx::ImageCreateInfo* pCreateInfo)
{
    if (IsNull(pCreateInfo->pApiObject)) {
        // Create image
        {
            VkExtent3D extent = {};
            extent.width      = pCreateInfo->width;
            extent.height     = pCreateInfo->height;
            extent.depth      = pCreateInfo->depth;

            VkImageCreateFlags createFlags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            if (pCreateInfo->type == grfx::IMAGE_TYPE_CUBE) {
                createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }

            if (pCreateInfo->createFlags.bits.subsampledFormat) {
                createFlags |= VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
            }
            auto queueIndices = ToApi(GetDevice())->GetAllQueueFamilyIndices();

            VkImageCreateInfo vkci = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            vkci.flags             = createFlags;
            vkci.imageType         = ToVkImageType(pCreateInfo->type);
            vkci.format            = ToVkFormat(pCreateInfo->format);
            vkci.extent            = extent;
            vkci.mipLevels         = pCreateInfo->mipLevelCount;
            vkci.arrayLayers       = pCreateInfo->arrayLayerCount;
            vkci.samples           = ToVkSampleCount(pCreateInfo->sampleCount);
            vkci.tiling            = pCreateInfo->memoryUsage == grfx::MEMORY_USAGE_GPU_TO_CPU ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
            vkci.usage             = ToVkImageUsageFlags(pCreateInfo->usageFlags);
            vkci.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            if (pCreateInfo->concurrentMultiQueueUsage) {
                vkci.sharingMode           = VK_SHARING_MODE_CONCURRENT;
                vkci.queueFamilyIndexCount = 3;
                vkci.pQueueFamilyIndices   = queueIndices.data();
            }
            else {
                vkci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
                vkci.queueFamilyIndexCount = 0;
                vkci.pQueueFamilyIndices   = nullptr;
            }

            VkAllocationCallbacks* pAllocator = nullptr;

            VkResult vkres = vk::CreateImage(ToApi(GetDevice())->GetVkDevice(), &vkci, pAllocator, &mImage);
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkCreateImage failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }

        // Allocate memory
        {
            VmaMemoryUsage memoryUsage = ToVmaMemoryUsage(pCreateInfo->memoryUsage);
            if (memoryUsage == VMA_MEMORY_USAGE_UNKNOWN) {
                PPX_ASSERT_MSG(false, "unknown memory usage");
                return ppx::ERROR_API_FAILURE;
            }

            VmaAllocationCreateFlags createFlags = 0;

            if ((memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY) || (memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)) {
                createFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            }

            VmaAllocationCreateInfo vma_alloc_ci = {};
            vma_alloc_ci.flags                   = createFlags;
            vma_alloc_ci.usage                   = memoryUsage;
            vma_alloc_ci.requiredFlags           = 0;
            vma_alloc_ci.preferredFlags          = 0;
            vma_alloc_ci.memoryTypeBits          = 0;
            vma_alloc_ci.pool                    = VK_NULL_HANDLE;
            vma_alloc_ci.pUserData               = nullptr;

            VkResult vkres = vmaAllocateMemoryForImage(
                ToApi(GetDevice())->GetVmaAllocator(),
                mImage,
                &vma_alloc_ci,
                &mAllocation,
                &mAllocationInfo);
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vmaAllocateMemoryForImage failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }

        // Bind memory
        {
            VkResult vkres = vmaBindImageMemory(
                ToApi(GetDevice())->GetVmaAllocator(),
                mAllocation,
                mImage);
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vmaBindImageMemory failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }
    }
    else {
        mImage = reinterpret_cast<VkImage>(pCreateInfo->pApiObject);
    }

    mVkFormat    = ToVkFormat(pCreateInfo->format);
    mImageAspect = DetermineAspectMask(mVkFormat);

    if ((pCreateInfo->initialState != grfx::RESOURCE_STATE_UNDEFINED) && IsNull(pCreateInfo->pApiObject)) {
        vk::Device* pDevice = ToApi(GetDevice());

        grfx::QueuePtr grfxQueue = GetDevice()->GetAnyAvailableQueue();
        if (!grfxQueue) {
            return ppx::ERROR_FAILED;
        }

        // Determine pipeline stage and layout from the initial state
        VkPipelineStageFlags pipelineStage = 0;
        VkAccessFlags        accessMask    = 0;
        VkImageLayout        layout        = VK_IMAGE_LAYOUT_UNDEFINED;
        Result               ppxres        = ToVkBarrierDst(
            pCreateInfo->initialState,
            grfxQueue->GetCommandType(),
            pDevice->GetDeviceFeatures(),
            pipelineStage,
            accessMask,
            layout);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "couldn't determine pipeline stage and layout from initial state");
            return ppxres;
        }

        vk::Queue* pQueue = ToApi(grfxQueue.Get());

        VkResult vkres = pQueue->TransitionImageLayout(
            mImage,                       // image
            mImageAspect,                 // aspectMask
            0,                            // baseMipLevel
            pCreateInfo->mipLevelCount,   // levelCount
            0,                            // baseArrayLayer
            pCreateInfo->arrayLayerCount, // layerCount
            VK_IMAGE_LAYOUT_UNDEFINED,    // oldLayout
            layout,                       // newLayout
            pipelineStage);               // newPipelineStage)
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vk::Queue::TransitionImageLayout failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

void Image::DestroyApiObjects()
{
    // Don't destroy image unless we created it
    if (!IsNull(mCreateInfo.pApiObject)) {
        return;
    }

    if (mAllocation) {
        vmaFreeMemory(ToApi(GetDevice())->GetVmaAllocator(), mAllocation);
        mAllocation.Reset();

        mAllocationInfo = {};
    }

    if (mImage) {
        vkDestroyImage(ToApi(GetDevice())->GetVkDevice(), mImage, nullptr);
        mImage.Reset();
    }
}

Result Image::MapMemory(uint64_t offset, void** ppMappedAddress)
{
    if (IsNull(ppMappedAddress)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    VkResult vkres = vmaMapMemory(
        ToApi(GetDevice())->GetVmaAllocator(),
        mAllocation,
        ppMappedAddress);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vmaMapMemory failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Image::UnmapMemory()
{
    vmaUnmapMemory(
        ToApi(GetDevice())->GetVmaAllocator(),
        mAllocation);
}

// -------------------------------------------------------------------------------------------------
// Sampler
// -------------------------------------------------------------------------------------------------
Result Sampler::CreateApiObjects(const grfx::SamplerCreateInfo* pCreateInfo)
{
    VkSamplerCreateInfo vkci = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    vkci.flags               = 0;
    if (pCreateInfo->createFlags.bits.subsampledFormat) {
        vkci.flags |= VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT;
    }
    vkci.magFilter               = ToVkFilter(pCreateInfo->magFilter);
    vkci.minFilter               = ToVkFilter(pCreateInfo->minFilter);
    vkci.mipmapMode              = ToVkSamplerMipmapMode(pCreateInfo->mipmapMode);
    vkci.addressModeU            = ToVkSamplerAddressMode(pCreateInfo->addressModeU);
    vkci.addressModeV            = ToVkSamplerAddressMode(pCreateInfo->addressModeV);
    vkci.addressModeW            = ToVkSamplerAddressMode(pCreateInfo->addressModeW);
    vkci.mipLodBias              = pCreateInfo->mipLodBias;
    vkci.anisotropyEnable        = pCreateInfo->anisotropyEnable ? VK_TRUE : VK_FALSE;
    vkci.maxAnisotropy           = pCreateInfo->maxAnisotropy;
    vkci.compareEnable           = pCreateInfo->compareEnable ? VK_TRUE : VK_FALSE;
    vkci.compareOp               = ToVkCompareOp(pCreateInfo->compareOp);
    vkci.minLod                  = pCreateInfo->minLod;
    vkci.maxLod                  = pCreateInfo->maxLod;
    vkci.borderColor             = ToVkBorderColor(pCreateInfo->borderColor);
    vkci.unnormalizedCoordinates = VK_FALSE;

    VkResult vkres = vkCreateSampler(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mSampler);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateSampler failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Sampler::DestroyApiObjects()
{
    if (mSampler) {
        vkDestroySampler(ToApi(GetDevice())->GetVkDevice(), mSampler, nullptr);
        mSampler.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// DepthStencilView
// -------------------------------------------------------------------------------------------------
Result DepthStencilView::CreateApiObjects(const grfx::DepthStencilViewCreateInfo* pCreateInfo)
{
    VkImageViewCreateInfo vkci           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vkci.flags                           = 0;
    vkci.image                           = ToApi(pCreateInfo->pImage)->GetVkImage();
    vkci.viewType                        = ToVkImageViewType(pCreateInfo->imageViewType);
    vkci.format                          = ToVkFormat(pCreateInfo->format);
    vkci.components                      = ToVkComponentMapping(pCreateInfo->components);
    vkci.subresourceRange.aspectMask     = ToApi(pCreateInfo->pImage)->GetVkImageAspectFlags();
    vkci.subresourceRange.baseMipLevel   = pCreateInfo->mipLevel;
    vkci.subresourceRange.levelCount     = pCreateInfo->mipLevelCount;
    vkci.subresourceRange.baseArrayLayer = pCreateInfo->arrayLayer;
    vkci.subresourceRange.layerCount     = pCreateInfo->arrayLayerCount;

    VkResult vkres = vk::CreateImageView(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mImageView);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateImageView(DepthStencilView) failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    std::unique_ptr<grfx::internal::ImageResourceView> resourceView(new vk::internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    if (!resourceView) {
        PPX_ASSERT_MSG(false, "vk::internal::ImageResourceView(DepthStencilView) allocation failed");
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    SetResourceView(std::move(resourceView));

    return ppx::SUCCESS;
}

void DepthStencilView::DestroyApiObjects()
{
    if (mImageView) {
        vkDestroyImageView(
            ToApi(GetDevice())->GetVkDevice(),
            mImageView,
            nullptr);
        mImageView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// RenderTargetView
// -------------------------------------------------------------------------------------------------
Result RenderTargetView::CreateApiObjects(const grfx::RenderTargetViewCreateInfo* pCreateInfo)
{
    VkImageViewCreateInfo vkci           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vkci.flags                           = 0;
    vkci.image                           = ToApi(pCreateInfo->pImage)->GetVkImage();
    vkci.viewType                        = ToVkImageViewType(pCreateInfo->imageViewType);
    vkci.format                          = ToVkFormat(pCreateInfo->format);
    vkci.components                      = ToVkComponentMapping(pCreateInfo->components);
    vkci.subresourceRange.aspectMask     = ToApi(pCreateInfo->pImage)->GetVkImageAspectFlags();
    vkci.subresourceRange.baseMipLevel   = pCreateInfo->mipLevel;
    vkci.subresourceRange.levelCount     = pCreateInfo->mipLevelCount;
    vkci.subresourceRange.baseArrayLayer = pCreateInfo->arrayLayer;
    vkci.subresourceRange.layerCount     = pCreateInfo->arrayLayerCount;

    VkResult vkres = vk::CreateImageView(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mImageView);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateImageView(RenderTargetView) failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    std::unique_ptr<grfx::internal::ImageResourceView> resourceView(new vk::internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    if (!resourceView) {
        PPX_ASSERT_MSG(false, "vk::internal::ImageResourceView(RenderTargetView) allocation failed");
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    SetResourceView(std::move(resourceView));

    return ppx::SUCCESS;
}

void RenderTargetView::DestroyApiObjects()
{
    if (mImageView) {
        vkDestroyImageView(
            ToApi(GetDevice())->GetVkDevice(),
            mImageView,
            nullptr);
        mImageView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// SampledImageView
// -------------------------------------------------------------------------------------------------
Result SampledImageView::CreateApiObjects(const grfx::SampledImageViewCreateInfo* pCreateInfo)
{
    VkImageViewCreateInfo vkci           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vkci.flags                           = 0;
    vkci.image                           = ToApi(pCreateInfo->pImage)->GetVkImage();
    vkci.viewType                        = ToVkImageViewType(pCreateInfo->imageViewType);
    vkci.format                          = ToVkFormat(pCreateInfo->format);
    vkci.components                      = ToVkComponentMapping(pCreateInfo->components);
    vkci.subresourceRange.aspectMask     = ToApi(pCreateInfo->pImage)->GetVkImageAspectFlags();
    vkci.subresourceRange.baseMipLevel   = pCreateInfo->mipLevel;
    vkci.subresourceRange.levelCount     = pCreateInfo->mipLevelCount;
    vkci.subresourceRange.baseArrayLayer = pCreateInfo->arrayLayer;
    vkci.subresourceRange.layerCount     = pCreateInfo->arrayLayerCount;

    VkResult vkres = vk::CreateImageView(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mImageView);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateImageView(SampledImageView) failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    std::unique_ptr<grfx::internal::ImageResourceView> resourceView(new vk::internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    if (!resourceView) {
        PPX_ASSERT_MSG(false, "vk::internal::ImageResourceView(SampledImageView) allocation failed");
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    SetResourceView(std::move(resourceView));

    return ppx::SUCCESS;
}

void SampledImageView::DestroyApiObjects()
{
    if (mImageView) {
        vkDestroyImageView(
            ToApi(GetDevice())->GetVkDevice(),
            mImageView,
            nullptr);
        mImageView.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// StorageImageView
// -------------------------------------------------------------------------------------------------
Result StorageImageView::CreateApiObjects(const grfx::StorageImageViewCreateInfo* pCreateInfo)
{
    VkImageViewCreateInfo vkci           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vkci.flags                           = 0;
    vkci.image                           = ToApi(pCreateInfo->pImage)->GetVkImage();
    vkci.viewType                        = ToVkImageViewType(pCreateInfo->imageViewType);
    vkci.format                          = ToVkFormat(pCreateInfo->format);
    vkci.components                      = ToVkComponentMapping(pCreateInfo->components);
    vkci.subresourceRange.aspectMask     = ToApi(pCreateInfo->pImage)->GetVkImageAspectFlags();
    vkci.subresourceRange.baseMipLevel   = pCreateInfo->mipLevel;
    vkci.subresourceRange.levelCount     = pCreateInfo->mipLevelCount;
    vkci.subresourceRange.baseArrayLayer = pCreateInfo->arrayLayer;
    vkci.subresourceRange.layerCount     = pCreateInfo->arrayLayerCount;

    VkResult vkres = vk::CreateImageView(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mImageView);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateImageView(StorageImageView) failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    std::unique_ptr<grfx::internal::ImageResourceView> resourceView(new vk::internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_GENERAL));
    if (!resourceView) {
        PPX_ASSERT_MSG(false, "vk::internal::ImageResourceView(StorageImageView) allocation failed");
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    SetResourceView(std::move(resourceView));

    return ppx::SUCCESS;
}

void StorageImageView::DestroyApiObjects()
{
    if (mImageView) {
        vkDestroyImageView(
            ToApi(GetDevice())->GetVkDevice(),
            mImageView,
            nullptr);
        mImageView.Reset();
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
