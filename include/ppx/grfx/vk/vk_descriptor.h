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

#ifndef ppx_grfx_vk_descriptor_h
#define ppx_grfx_vk_descriptor_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_descriptor.h"

namespace ppx {
namespace grfx {
namespace vk {

class DescriptorPool
    : public grfx::DescriptorPool
{
public:
    DescriptorPool() {}
    virtual ~DescriptorPool() {}

    VkDescriptorPoolPtr GetVkDescriptorPool() const { return mDescriptorPool; }

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorPoolCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkDescriptorPoolPtr mDescriptorPool;
};

// -------------------------------------------------------------------------------------------------

class DescriptorSet
    : public grfx::DescriptorSet
{
public:
    DescriptorSet() {}
    virtual ~DescriptorSet() {}

    VkDescriptorSetPtr GetVkDescriptorSet() const { return mDescriptorSet; }

    virtual Result UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites) override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::DescriptorSetCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkDescriptorSetPtr  mDescriptorSet;
    VkDescriptorPoolPtr mDescriptorPool;

    // Reduce memory allocations during update process
    std::vector<VkWriteDescriptorSet>   mWriteStore;
    std::vector<VkDescriptorImageInfo>  mImageInfoStore;
    std::vector<VkBufferView>           mTexelBufferStore;
    std::vector<VkDescriptorBufferInfo> mBufferInfoStore;
    uint32_t                            mWriteCount       = 0;
    uint32_t                            mImageCount       = 0;
    uint32_t                            mTexelBufferCount = 0;
    uint32_t                            mBufferCount      = 0;
};

// -------------------------------------------------------------------------------------------------

class DescriptorSetLayout
    : public grfx::DescriptorSetLayout
{
public:
    DescriptorSetLayout() {}
    virtual ~DescriptorSetLayout() {}

    VkDescriptorSetLayoutPtr GetVkDescriptorSetLayout() const { return mDescriptorSetLayout; }

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkDescriptorSetLayoutPtr mDescriptorSetLayout;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_descriptor_h
