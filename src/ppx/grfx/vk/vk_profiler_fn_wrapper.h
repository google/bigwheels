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

#ifndef PPX_GRFX_VK_PROFILER_FN_WRAPPHER_H
#define PPX_GRFX_VK_PROFILER_FN_WRAPPHER_H

#include "ppx/grfx/vk//vk_config_platform.h"

namespace ppx {
namespace grfx {
namespace vk {

void RegisterProfilerFunctions();

#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)

VkResult CreateBuffer(
    VkDevice                     device,
    const VkBufferCreateInfo*    pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer*                    pBuffer);

VkResult CreateImage(
    VkDevice                     device,
    const VkImageCreateInfo*     pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage*                     pImage);

VkResult CreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView*                 pView);

VkResult CreateCommandPool(
    VkDevice                       device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*   pAllocator,
    VkCommandPool*                 pCommandPool);

VkResult CreateRenderPass(
    VkDevice                      device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*  pAllocator,
    VkRenderPass*                 pRenderPass);

VkResult AllocateCommandBuffers(
    VkDevice                           device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer*                   pCommandBuffers);

void FreeCommandBuffers(
    VkDevice               device,
    VkCommandPool          commandPool,
    uint32_t               commandBufferCount,
    const VkCommandBuffer* pCommandBuffers);

VkResult AllocateDescriptorSets(
    VkDevice                           device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet*                   pDescriptorSets);

void FreeDescriptorSets(
    VkDevice               device,
    VkDescriptorPool       descriptorPool,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets);

void UpdateDescriptorSets(
    VkDevice                    device,
    uint32_t                    descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t                    descriptorCopyCount,
    const VkCopyDescriptorSet*  pDescriptorCopies);

VkResult QueuePresent(
    VkQueue                 queue,
    const VkPresentInfoKHR* pPresentInfo);

VkResult QueueSubmit(
    VkQueue             queue,
    uint32_t            submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence             fence);

VkResult BeginCommandBuffer(
    VkCommandBuffer                 commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo);

VkResult EndCommandBuffer(
    VkCommandBuffer commandBuffer);

void CmdPipelineBarrier(
    VkCommandBuffer              commandBuffer,
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    VkDependencyFlags            dependencyFlags,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier*       pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier*  pImageMemoryBarriers);

void CmdBeginRenderPass(
    VkCommandBuffer              commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents            contents);

void CmdEndRenderPass(
    VkCommandBuffer commandBuffer);

void CmdBindDescriptorSets(
    VkCommandBuffer        commandBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t*        pDynamicOffsets);

void CmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset,
    VkIndexType     indexType);

void CmdBindPipeline(
    VkCommandBuffer     commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline);

void CmdBindVertexBuffers(
    VkCommandBuffer     commandBuffer,
    uint32_t            firstBinding,
    uint32_t            bindingCount,
    const VkBuffer*     pBuffers,
    const VkDeviceSize* pOffsets);

void CmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t        groupCountX,
    uint32_t        groupCountY,
    uint32_t        groupCountZ);

void CmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t        vertexCount,
    uint32_t        instanceCount,
    uint32_t        firstVertex,
    uint32_t        firstInstance);

void CmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t        indexCount,
    uint32_t        instanceCount,
    uint32_t        firstIndex,
    int32_t         vertexOffset,
    uint32_t        firstInstance);

#else

inline VkResult CreateBuffer(
    VkDevice                     device,
    const VkBufferCreateInfo*    pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer*                    pBuffer)
{
    return vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

inline VkResult CreateImage(
    VkDevice                     device,
    const VkImageCreateInfo*     pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage*                     pImage)
{
    return vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

inline VkResult CreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView*                 pView)
{
    return vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

inline VkResult CreateCommandPool(
    VkDevice                       device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*   pAllocator,
    VkCommandPool*                 pCommandPool)
{
    return vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

inline VkResult CreateRenderPass(
    VkDevice                      device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*  pAllocator,
    VkRenderPass*                 pRenderPass)
{
    return vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

inline VkResult AllocateCommandBuffers(
    VkDevice                           device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer*                   pCommandBuffers)
{
    return vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

inline void FreeCommandBuffers(
    VkDevice               device,
    VkCommandPool          commandPool,
    uint32_t               commandBufferCount,
    const VkCommandBuffer* pCommandBuffers)
{
    vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

inline VkResult AllocateDescriptorSets(
    VkDevice                           device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet*                   pDescriptorSets)
{
    return vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

inline void FreeDescriptorSets(
    VkDevice               device,
    VkDescriptorPool       descriptorPool,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets)
{
    vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

inline void UpdateDescriptorSets(
    VkDevice                    device,
    uint32_t                    descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t                    descriptorCopyCount,
    const VkCopyDescriptorSet*  pDescriptorCopies)
{
    vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

inline VkResult QueuePresent(
    VkQueue                 queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    return vkQueuePresentKHR(queue, pPresentInfo);
}

inline VkResult QueueSubmit(
    VkQueue             queue,
    uint32_t            submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence             fence)
{
    return vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

inline VkResult BeginCommandBuffer(
    VkCommandBuffer                 commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo)
{
    return vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

inline VkResult EndCommandBuffer(
    VkCommandBuffer commandBuffer)
{
    return vkEndCommandBuffer(commandBuffer);
}

inline void CmdPipelineBarrier(
    VkCommandBuffer              commandBuffer,
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    VkDependencyFlags            dependencyFlags,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier*       pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier*  pImageMemoryBarriers)
{
    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void CmdBeginRenderPass(
    VkCommandBuffer              commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents            contents)
{
    vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

inline void CmdEndRenderPass(
    VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

inline void CmdBindDescriptorSets(
    VkCommandBuffer        commandBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t*        pDynamicOffsets)
{
    vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

inline void CmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset,
    VkIndexType     indexType)
{
    vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

inline void CmdBindPipeline(
    VkCommandBuffer     commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline)
{
    vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

inline void CmdBindVertexBuffers(
    VkCommandBuffer     commandBuffer,
    uint32_t            firstBinding,
    uint32_t            bindingCount,
    const VkBuffer*     pBuffers,
    const VkDeviceSize* pOffsets)
{
    vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

inline void CmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t        groupCountX,
    uint32_t        groupCountY,
    uint32_t        groupCountZ)
{
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

inline void CmdCopyBuffer()
{
}

inline void CmdCopyBufferToImage()
{
}

inline void CmdCopyImage()
{
}

inline void CmdCopyImageToBuffer()
{
}

inline void CmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t        vertexCount,
    uint32_t        instanceCount,
    uint32_t        firstVertex,
    uint32_t        firstInstance)
{
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

inline void CmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t        indexCount,
    uint32_t        instanceCount,
    uint32_t        firstIndex,
    int32_t         vertexOffset,
    uint32_t        firstInstance)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

#endif // defined(PPX_ENABLE_PROFILE_GRFX_FUNCTIONS)

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // PPX_GRFX_VK_PROFILER_FN_WRAPPHER_H
