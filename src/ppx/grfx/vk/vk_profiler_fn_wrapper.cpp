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

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"
#include "ppx/profiler.h"

namespace ppx {
namespace grfx {
namespace vk {

static ProfilerEventToken s_vkCreateBuffer           = 0;
static ProfilerEventToken s_vkCreateImage            = 0;
static ProfilerEventToken s_vkCreateImageView        = 0;
static ProfilerEventToken s_vkCreateCommandPool      = 0;
static ProfilerEventToken s_vkCreateRenderPass       = 0;
static ProfilerEventToken s_vkCreateRenderPass2      = 0;
static ProfilerEventToken s_vkAllocateCommandBuffers = 0;
static ProfilerEventToken s_vkFreeCommandBuffers     = 0;
static ProfilerEventToken s_vkAllocateDescriptorSets = 0;
static ProfilerEventToken s_vkFreeDescriptorSets     = 0;
static ProfilerEventToken s_vkUpdateDescriptorSets   = 0;
static ProfilerEventToken s_vkQueuePresent           = 0;
static ProfilerEventToken s_vkQueueSubmit            = 0;
static ProfilerEventToken s_vkBeginCommandBuffer     = 0;
static ProfilerEventToken s_vkEndCommandBuffer       = 0;
static ProfilerEventToken s_vkCmdPipelineBarrier     = 0;
static ProfilerEventToken s_vkCmdBeginRenderPass     = 0;
static ProfilerEventToken s_vkCmdEndRenderPass       = 0;
static ProfilerEventToken s_vkCmdBindDescriptorSets  = 0;
static ProfilerEventToken s_vkCmdBindIndexBuffer     = 0;
static ProfilerEventToken s_vkCmdBindPipeline        = 0;
static ProfilerEventToken s_vkCmdBindVertexBuffers   = 0;
static ProfilerEventToken s_vkCmdDispatch            = 0;
static ProfilerEventToken s_vkCmdDraw                = 0;
static ProfilerEventToken s_vkCmdDrawIndexed         = 0;

void RegisterProfilerFunctions()
{
#define REGISTER_EVENT_PARAMS(VKFN) #VKFN, &s_##VKFN

    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCreateBuffer)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCreateImage)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCreateImageView)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCreateCommandPool)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCreateRenderPass)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkAllocateCommandBuffers)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkFreeCommandBuffers)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkAllocateDescriptorSets)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkFreeDescriptorSets)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkUpdateDescriptorSets)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkQueuePresent)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkQueueSubmit)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkBeginCommandBuffer)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkEndCommandBuffer)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdPipelineBarrier)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdBeginRenderPass)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdEndRenderPass)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdBindDescriptorSets)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdBindIndexBuffer)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdBindPipeline)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdBindVertexBuffers)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdDispatch)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdDraw)));
    PPX_CHECKED_CALL(Profiler::RegisterGrfxApiFnEvent(REGISTER_EVENT_PARAMS(vkCmdDrawIndexed)));

#undef REGISTER_EVENT_PARAMS
}

namespace {
struct FuncVkCreateRenderPass2KHR
{
    VkDevice                   mDevice = VK_NULL_HANDLE;
    PFN_vkCreateRenderPass2KHR mFunc   = nullptr;
    FuncVkCreateRenderPass2KHR(VkDevice device)
    {
        mDevice = device;
        mFunc   = reinterpret_cast<PFN_vkCreateRenderPass2KHR>(vkGetDeviceProcAddr(device, "vkCreateRenderPass2KHR"));
    }
    inline VkResult Call(
        VkDevice                       device,
        const VkRenderPassCreateInfo2* pCreateInfo,
        const VkAllocationCallbacks*   pAllocator,
        VkRenderPass*                  pRenderPass)
    {
        if (device != mDevice) {
            PPX_LOG_WARN("vkRenderPass2KHR called with different VkDevice");
            return VK_ERROR_UNKNOWN;
        }
        return mFunc(device, pCreateInfo, pAllocator, pRenderPass);
    }
};
} // namespace

#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)

VkResult CreateBuffer(
    VkDevice                     device,
    const VkBufferCreateInfo*    pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer*                    pBuffer)
{
    ProfilerScopedEventSample eventSample(s_vkCreateBuffer);
    return vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

VkResult CreateImage(
    VkDevice                     device,
    const VkImageCreateInfo*     pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage*                     pImage)
{
    ProfilerScopedEventSample eventSample(s_vkCreateImage);
    return vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

VkResult CreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView*                 pView)
{
    ProfilerScopedEventSample eventSample(s_vkCreateImageView);
    return vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

VkResult CreateCommandPool(
    VkDevice                       device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*   pAllocator,
    VkCommandPool*                 pCommandPool)
{
    ProfilerScopedEventSample eventSample(s_vkCreateCommandPool);
    return vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

VkResult CreateRenderPass(
    VkDevice                      device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*  pAllocator,
    VkRenderPass*                 pRenderPass)
{
    ProfilerScopedEventSample eventSample(s_vkCreateRenderPass);
    return vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult CreateRenderPass(
    VkDevice                       device,
    const VkRenderPassCreateInfo2* pCreateInfo,
    const VkAllocationCallbacks*   pAllocator,
    VkRenderPass*                  pRenderPass)
{
    static FuncVkCreateRenderPass2KHR func(device);
    ProfilerScopedEventSample         eventSample(s_vkCreateRenderPass2);
    return func.Call(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult AllocateCommandBuffers(
    VkDevice                           device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer*                   pCommandBuffers)
{
    ProfilerScopedEventSample eventSample(s_vkAllocateCommandBuffers);
    return vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

void FreeCommandBuffers(
    VkDevice               device,
    VkCommandPool          commandPool,
    uint32_t               commandBufferCount,
    const VkCommandBuffer* pCommandBuffers)
{
    ProfilerScopedEventSample eventSample(s_vkFreeCommandBuffers);
    vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult AllocateDescriptorSets(
    VkDevice                           device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet*                   pDescriptorSets)
{
    ProfilerScopedEventSample eventSample(s_vkAllocateDescriptorSets);
    return vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

void FreeDescriptorSets(
    VkDevice               device,
    VkDescriptorPool       descriptorPool,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets)
{
    ProfilerScopedEventSample eventSample(s_vkFreeDescriptorSets);
    vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void UpdateDescriptorSets(
    VkDevice                    device,
    uint32_t                    descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t                    descriptorCopyCount,
    const VkCopyDescriptorSet*  pDescriptorCopies)
{
    ProfilerScopedEventSample eventSample(s_vkUpdateDescriptorSets);
    vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult QueuePresent(
    VkQueue                 queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    ProfilerScopedEventSample eventSample(s_vkQueuePresent);
    return vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult QueueSubmit(
    VkQueue             queue,
    uint32_t            submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence             fence)
{
    ProfilerScopedEventSample eventSample(s_vkQueueSubmit);
    return vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult BeginCommandBuffer(
    VkCommandBuffer                 commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo)
{
    ProfilerScopedEventSample eventSample(s_vkBeginCommandBuffer);
    return vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VkResult EndCommandBuffer(
    VkCommandBuffer commandBuffer)
{
    ProfilerScopedEventSample eventSample(s_vkEndCommandBuffer);
    return vkEndCommandBuffer(commandBuffer);
}

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
    const VkImageMemoryBarrier*  pImageMemoryBarriers)
{
    ProfilerScopedEventSample eventSample(s_vkCmdPipelineBarrier);
    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void CmdBeginRenderPass(
    VkCommandBuffer              commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents            contents)
{
    ProfilerScopedEventSample eventSample(s_vkCmdBeginRenderPass);
    vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void CmdEndRenderPass(
    VkCommandBuffer commandBuffer)
{
    ProfilerScopedEventSample eventSample(s_vkCmdEndRenderPass);
    vkCmdEndRenderPass(commandBuffer);
}

void CmdBindDescriptorSets(
    VkCommandBuffer        commandBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t*        pDynamicOffsets)
{
    ProfilerScopedEventSample eventSample(s_vkCmdBindDescriptorSets);
    vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void CmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset,
    VkIndexType     indexType)
{
    ProfilerScopedEventSample eventSample(s_vkCmdBindIndexBuffer);
    vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void CmdBindPipeline(
    VkCommandBuffer     commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline)
{
    ProfilerScopedEventSample eventSample(s_vkCmdBindVertexBuffers);
    vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void CmdBindVertexBuffers(
    VkCommandBuffer     commandBuffer,
    uint32_t            firstBinding,
    uint32_t            bindingCount,
    const VkBuffer*     pBuffers,
    const VkDeviceSize* pOffsets)
{
    ProfilerScopedEventSample eventSample(s_vkCmdBindPipeline);
    vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void CmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t        groupCountX,
    uint32_t        groupCountY,
    uint32_t        groupCountZ)
{
    ProfilerScopedEventSample eventSample(s_vkCmdDispatch);
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void CmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t        vertexCount,
    uint32_t        instanceCount,
    uint32_t        firstVertex,
    uint32_t        firstInstance)
{
    ProfilerScopedEventSample eventSample(s_vkCmdDraw);
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t        indexCount,
    uint32_t        instanceCount,
    uint32_t        firstIndex,
    int32_t         vertexOffset,
    uint32_t        firstInstance)
{
    ProfilerScopedEventSample eventSample(s_vkCmdDrawIndexed);
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

#else

VkResult CreateRenderPass(
    VkDevice                       device,
    const VkRenderPassCreateInfo2* pCreateInfo,
    const VkAllocationCallbacks*   pAllocator,
    VkRenderPass*                  pRenderPass)
{
    static FuncVkCreateRenderPass2KHR func(device);
    return func.Call(device, pCreateInfo, pAllocator, pRenderPass);
}

#endif // defined(PPX_ENABLE_PROFILE_GRFX_FUNCTIONS)

} // namespace vk
} // namespace grfx
} // namespace ppx
