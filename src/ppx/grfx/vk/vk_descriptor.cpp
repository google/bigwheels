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

#include "ppx/grfx/vk/vk_descriptor.h"
#include "ppx/grfx/vk/vk_buffer.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_image.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// DescriptorPool
// -------------------------------------------------------------------------------------------------
Result DescriptorPool::CreateApiObjects(const grfx::DescriptorPoolCreateInfo* pCreateInfo)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    // clang-format off
    if (pCreateInfo->sampler              > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER               , pCreateInfo->sampler             });
    if (pCreateInfo->combinedImageSampler > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pCreateInfo->combinedImageSampler});
    if (pCreateInfo->sampledImage         > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         , pCreateInfo->sampledImage        });
    if (pCreateInfo->storageImage         > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         , pCreateInfo->storageImage        });
    if (pCreateInfo->uniformTexelBuffer   > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  , pCreateInfo->uniformTexelBuffer  });
    if (pCreateInfo->storageTexelBuffer   > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  , pCreateInfo->storageTexelBuffer  });
    if (pCreateInfo->uniformBuffer        > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        , pCreateInfo->uniformBuffer       });
    if (pCreateInfo->rawStorageBuffer        > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        , pCreateInfo->rawStorageBuffer       });
    if (pCreateInfo->uniformBufferDynamic > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pCreateInfo->uniformBufferDynamic});
    if (pCreateInfo->storageBufferDynamic > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pCreateInfo->storageBufferDynamic});
    if (pCreateInfo->inputAttachment      > 0) poolSizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT      , pCreateInfo->inputAttachment     });
    // clang-format on
    if (pCreateInfo->structuredBuffer > 0) {
        auto it = FindIf(
            poolSizes,
            [](const VkDescriptorPoolSize& elem) -> bool {
                bool isSame = elem.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
                return isSame; });
        if (it != std::end(poolSizes)) {
            it->descriptorCount += pCreateInfo->structuredBuffer;
        }
        else {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pCreateInfo->structuredBuffer});
        }
    }

    // Flags
    uint32_t flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkDescriptorPoolCreateInfo vkci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    vkci.flags                      = flags;
    vkci.maxSets                    = PPX_MAX_SETS_PER_POOL;
    vkci.poolSizeCount              = CountU32(poolSizes);
    vkci.pPoolSizes                 = DataPtr(poolSizes);

    VkResult vkres = vkCreateDescriptorPool(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mDescriptorPool);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateDescriptorPool failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void DescriptorPool::DestroyApiObjects()
{
    if (mDescriptorPool) {
        vkDestroyDescriptorPool(ToApi(GetDevice())->GetVkDevice(), mDescriptorPool, nullptr);
        mDescriptorPool.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// DescriptorSet
// -------------------------------------------------------------------------------------------------
Result DescriptorSet::CreateApiObjects(const grfx::internal::DescriptorSetCreateInfo* pCreateInfo)
{
    mDescriptorPool = ToApi(pCreateInfo->pPool)->GetVkDescriptorPool();

    VkDescriptorSetLayout layout = ToApi(pCreateInfo->pLayout)->GetVkDescriptorSetLayout();

    VkDescriptorSetAllocateInfo vkai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    vkai.descriptorPool              = mDescriptorPool;
    vkai.descriptorSetCount          = 1;
    vkai.pSetLayouts                 = &layout;

    VkResult vkres = vk::AllocateDescriptorSets(
        ToApi(GetDevice())->GetVkDevice(),
        &vkai,
        &mDescriptorSet);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkAllocateDescriptorSets failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    // Allocate 32 entries initially
    const uint32_t count = 32;
    mWriteStore.resize(count);
    mImageInfoStore.resize(count);
    mBufferInfoStore.resize(count);
    mTexelBufferStore.resize(count);

    return ppx::SUCCESS;
}

void DescriptorSet::DestroyApiObjects()
{
    if (mDescriptorSet) {
        vk::FreeDescriptorSets(
            ToApi(GetDevice())->GetVkDevice(),
            mDescriptorPool,
            1,
            mDescriptorSet);

        mDescriptorSet.Reset();
    }

    if (mDescriptorPool) {
        mDescriptorPool.Reset();
    }
}

Result DescriptorSet::UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites)
{
    if (writeCount == 0) {
        return ppx::ERROR_UNEXPECTED_COUNT_VALUE;
    }

    if (CountU32(mWriteStore) < writeCount) {
        mWriteStore.resize(writeCount);
        mImageInfoStore.resize(writeCount);
        mBufferInfoStore.resize(writeCount);
        mTexelBufferStore.resize(writeCount);
    }

    mImageCount       = 0;
    mBufferCount      = 0;
    mTexelBufferCount = 0;
    for (mWriteCount = 0; mWriteCount < writeCount; ++mWriteCount) {
        const grfx::WriteDescriptor& srcWrite = pWrites[mWriteCount];

        VkDescriptorImageInfo*  pImageInfo       = nullptr;
        VkBufferView*           pTexelBufferView = nullptr;
        VkDescriptorBufferInfo* pBufferInfo      = nullptr;

        VkDescriptorType descriptorType = ToVkDescriptorType(srcWrite.type);
        switch (descriptorType) {
            default: {
                PPX_ASSERT_MSG(false, "unknown descriptor type: " << ToString(descriptorType) << "(" << descriptorType << ")");
                return ppx::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
            } break;

            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                PPX_ASSERT_MSG(mImageCount < mImageInfoStore.size(), "image count exceeds image store capacity");
                pImageInfo = &mImageInfoStore[mImageCount];
                // Fill out info
                pImageInfo->sampler     = VK_NULL_HANDLE;
                pImageInfo->imageView   = VK_NULL_HANDLE;
                pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                switch (descriptorType) {
                    default: break;
                    case VK_DESCRIPTOR_TYPE_SAMPLER: {
                        pImageInfo->sampler = ToApi(srcWrite.pSampler)->GetVkSampler();
                    } break;

                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                        pImageInfo->sampler     = ToApi(srcWrite.pSampler)->GetVkSampler();
                        pImageInfo->imageView   = ToApi(srcWrite.pImageView->GetResourceView())->GetVkImageView();
                        pImageInfo->imageLayout = ToApi(srcWrite.pImageView->GetResourceView())->GetVkImageLayout();
                    } break;

                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                        pImageInfo->imageView   = ToApi(srcWrite.pImageView->GetResourceView())->GetVkImageView();
                        pImageInfo->imageLayout = ToApi(srcWrite.pImageView->GetResourceView())->GetVkImageLayout();
                    } break;
                }
                // Increment count
                mImageCount += 1;
            } break;

            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
                PPX_ASSERT_MSG(false, "TEXEL BUFFER NOT IMPLEMENTED");
                PPX_ASSERT_MSG(mTexelBufferCount < mImageInfoStore.size(), "texel buffer count exceeds texel buffer store capacity");
                pTexelBufferView = &mTexelBufferStore[mTexelBufferCount];
                // Fill out info
                // Increment count
                mTexelBufferCount += 1;
            } break;

            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                PPX_ASSERT_MSG(mBufferCount < mBufferInfoStore.size(), "buffer count exceeds buffer store capacity");
                pBufferInfo = &mBufferInfoStore[mBufferCount];
                // Fill out info
                pBufferInfo->buffer = ToApi(srcWrite.pBuffer)->GetVkBuffer();
                pBufferInfo->offset = srcWrite.bufferOffset;
                pBufferInfo->range  = (srcWrite.bufferRange == PPX_WHOLE_SIZE) ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(srcWrite.bufferRange);
                // Increment count
                mBufferCount += 1;
            } break;
        }

        VkWriteDescriptorSet& vkWrite = mWriteStore[mWriteCount];
        vkWrite                       = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        vkWrite.dstSet                = mDescriptorSet;
        vkWrite.dstBinding            = srcWrite.binding;
        vkWrite.dstArrayElement       = srcWrite.arrayIndex;
        vkWrite.descriptorCount       = 1;
        vkWrite.descriptorType        = descriptorType;
        vkWrite.pImageInfo            = pImageInfo;
        vkWrite.pBufferInfo           = pBufferInfo;
        vkWrite.pTexelBufferView      = pTexelBufferView;
    }

    vk::UpdateDescriptorSets(
        ToApi(GetDevice())->GetVkDevice(),
        mWriteCount,
        mWriteStore.data(),
        0,
        nullptr);

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// DescriptorSetLayout
// -------------------------------------------------------------------------------------------------
Result DescriptorSetLayout::CreateApiObjects(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo)
{
    // Make sure the device has VK_KHR_push_descriptors if pushable is turned on
    if (pCreateInfo->flags.bits.pushable && (ToApi(GetDevice())->GetMaxPushDescriptors() == 0)) {
        PPX_ASSERT_MSG(false, "Descriptor set layout create info has pushable flag but device does not support VK_KHR_push_descriptor");
        return ppx::ERROR_REQUIRED_FEATURE_UNAVAILABLE;
    }

    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    for (size_t i = 0; i < pCreateInfo->bindings.size(); ++i) {
        const grfx::DescriptorBinding& baseBinding = pCreateInfo->bindings[i];

        VkDescriptorSetLayoutBinding vkBinding = {};
        vkBinding.binding                      = baseBinding.binding;
        vkBinding.descriptorType               = ToVkDescriptorType(baseBinding.type);
        vkBinding.descriptorCount              = baseBinding.arrayCount;
        vkBinding.stageFlags                   = ToVkShaderStageFlags(baseBinding.shaderVisiblity);
        vkBinding.pImmutableSamplers           = nullptr;
        vkBindings.push_back(vkBinding);
    }

    VkDescriptorSetLayoutCreateInfo vkci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    vkci.bindingCount                    = CountU32(vkBindings);
    vkci.pBindings                       = DataPtr(vkBindings);

    if (pCreateInfo->flags.bits.pushable) {
        vkci.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    }
    else {
        vkci.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    }

    VkResult vkres = vkCreateDescriptorSetLayout(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mDescriptorSetLayout);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateDescriptorSetLayout failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void DescriptorSetLayout::DestroyApiObjects()
{
    if (mDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(ToApi(GetDevice())->GetVkDevice(), mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout.Reset();
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
