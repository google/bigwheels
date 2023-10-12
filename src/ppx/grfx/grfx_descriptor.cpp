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

#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_texture.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// DescriptorSet
// -------------------------------------------------------------------------------------------------
Result DescriptorSet::UpdateSampler(
    uint32_t             binding,
    uint32_t             arrayIndex,
    const grfx::Sampler* pSampler)
{
    grfx::WriteDescriptor write = {};
    write.binding               = binding;
    write.arrayIndex            = arrayIndex;
    write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
    write.pSampler              = pSampler;

    Result ppxres = UpdateDescriptors(1, &write);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result DescriptorSet::UpdateSampledImage(
    uint32_t                      binding,
    uint32_t                      arrayIndex,
    const grfx::SampledImageView* pImageView)
{
    grfx::WriteDescriptor write = {};
    write.binding               = binding;
    write.arrayIndex            = arrayIndex;
    write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.pImageView            = pImageView;

    Result ppxres = UpdateDescriptors(1, &write);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result DescriptorSet::UpdateSampledImage(
    uint32_t             binding,
    uint32_t             arrayIndex,
    const grfx::Texture* pTexture)
{
    grfx::WriteDescriptor write = {};
    write.binding               = binding;
    write.arrayIndex            = arrayIndex;
    write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.pImageView            = pTexture->GetSampledImageView();

    Result ppxres = UpdateDescriptors(1, &write);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result DescriptorSet::UpdateStorageImage(
    uint32_t             binding,
    uint32_t             arrayIndex,
    const grfx::Texture* pTexture)
{
    grfx::WriteDescriptor write = {};
    write.binding               = binding;
    write.arrayIndex            = arrayIndex;
    write.type                  = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageView            = pTexture->GetStorageImageView();

    Result ppxres = UpdateDescriptors(1, &write);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result DescriptorSet::UpdateUniformBuffer(
    uint32_t            binding,
    uint32_t            arrayIndex,
    const grfx::Buffer* pBuffer,
    uint64_t            offset,
    uint64_t            range)
{
    grfx::WriteDescriptor write = {};
    write.binding               = binding;
    write.arrayIndex            = arrayIndex;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = offset;
    write.bufferRange           = range;
    write.pBuffer               = pBuffer;

    Result ppxres = UpdateDescriptors(1, &write);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// DescriptorSetLayout
// -------------------------------------------------------------------------------------------------
Result DescriptorSetLayout::Create(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo)
{
    // Bail if there's any binding overlaps - overlaps are not permitted to
    // make D3D12 and Vulkan agreeable.
    //
    std::vector<ppx::RangeU32> ranges;
    const size_t               bindingCount = pCreateInfo->bindings.size();
    for (size_t i = 0; i < bindingCount; ++i) {
        const grfx::DescriptorBinding& binding = pCreateInfo->bindings[i];

        // Calculate range
        ppx::RangeU32 range = {};
        range.start         = binding.binding;
        range.end           = binding.binding + binding.arrayCount;

        size_t rangeCount = ranges.size();
        for (size_t j = 0; j < rangeCount; ++j) {
            bool overlaps = HasOverlapHalfOpen(range, ranges[j]);
            if (overlaps) {
                std::stringstream ss;
                ss << "[DESCRIPTOR BINDING RANGE ALIASES]: "
                   << "binding at entry " << i << " aliases with binding at entry " << j;
                PPX_ASSERT_MSG(false, ss.str());
                return ppx::ERROR_RANGE_ALIASING_NOT_ALLOWED;
            }
        }

        ranges.push_back(range);
    }

    Result ppxres = grfx::DeviceObject<grfx::DescriptorSetLayoutCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

} // namespace grfx
} // namespace ppx
