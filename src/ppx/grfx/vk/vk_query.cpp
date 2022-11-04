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

#include "ppx/grfx/vk/vk_query.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_instance.h"
#include "ppx/grfx/vk/vk_command.h"
#include "ppx/grfx/vk/vk_buffer.h"

namespace ppx {
namespace grfx {
namespace vk {

Query::Query()
    : mType(VK_QUERY_TYPE_MAX_ENUM), mMultiplier(1)
{
}

uint32_t Query::GetQueryTypeSize(VkQueryType type, uint32_t multiplier) const
{
    uint32_t result = 0;
    switch (type) {
        case VK_QUERY_TYPE_OCCLUSION:
        case VK_QUERY_TYPE_TIMESTAMP:
            // this need VK_QUERY_RESULT_64_BIT to be set
            result = (uint32_t)sizeof(uint64_t);
            break;
        case VK_QUERY_TYPE_PIPELINE_STATISTICS:
            // this need VK_QUERY_RESULT_64_BIT to be set
            result = (uint32_t)sizeof(uint64_t) * multiplier;
            break;
        default:
            PPX_ASSERT_MSG(false, "Unsupported query type");
            break;
    }
    return result;
}

Result Query::CreateApiObjects(const grfx::QueryCreateInfo* pCreateInfo)
{
    VkQueryPoolCreateInfo vkci = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    vkci.flags                 = 0;
    vkci.queryType             = ToVkQueryType(pCreateInfo->type);
    vkci.queryCount            = pCreateInfo->count;
    vkci.pipelineStatistics    = 0;

    mType       = vkci.queryType;
    mMultiplier = 1;

    if (vkci.queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
        vkci.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
                                  VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
        std::bitset<32> flagbit(vkci.pipelineStatistics);
        mMultiplier = (uint32_t)flagbit.count();
    }

    VkResult vkres = vkCreateQueryPool(ToApi(GetDevice())->GetVkDevice(), &vkci, nullptr, &mQueryPool);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    grfx::BufferCreateInfo createInfo  = {};
    createInfo.size                    = vkci.queryCount * GetQueryTypeSize(mType, mMultiplier);
    createInfo.structuredElementStride = GetQueryTypeSize(mType, mMultiplier);
    createInfo.usageFlags              = grfx::BUFFER_USAGE_TRANSFER_DST;
    createInfo.memoryUsage             = grfx::MEMORY_USAGE_GPU_TO_CPU;
    createInfo.initialState            = grfx::RESOURCE_STATE_COPY_DST;
    createInfo.ownership               = grfx::OWNERSHIP_REFERENCE;

    // Create buffer
    Result ppxres = GetDevice()->CreateBuffer(&createInfo, &mBuffer);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Query::DestroyApiObjects()
{
    if (mQueryPool) {
        vkDestroyQueryPool(
            ToApi(GetDevice())->GetVkDevice(),
            mQueryPool,
            nullptr);

        mQueryPool.Reset();
    }

    if (mBuffer) {
        mBuffer.Reset();
    }
}

void Query::Reset(uint32_t firstQuery, uint32_t queryCount)
{
#ifdef VK_API_VERION_1_2
    vkResetQueryPool(ToApi(GetDevice())->GetVkDevice(), mQueryPool, firstQuery, queryCount);
#else
    ToApi(GetDevice())->ResetQueryPoolEXT(mQueryPool, firstQuery, queryCount);
#endif
}

Result Query::GetData(void* pDstData, uint64_t dstDataSize)
{
    void*  pMappedAddress = 0;
    Result ppxres         = mBuffer->MapMemory(0, &pMappedAddress);
    if (Failed(ppxres)) {
        return ppxres;
    }

    size_t copySize = std::min<size_t>(dstDataSize, mBuffer->GetSize());
    memcpy(pDstData, pMappedAddress, copySize);

    mBuffer->UnmapMemory();

    return ppx::SUCCESS;
}

VkBufferPtr Query::GetReadBackBuffer() const
{
    return ToApi(mBuffer)->GetVkBuffer();
}

} // namespace vk
} // namespace grfx
} // namespace ppx
