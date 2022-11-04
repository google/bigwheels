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

#include "ppx/grfx/vk/vk_queue.h"
#include "ppx/grfx/vk/vk_command.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_swapchain.h"
#include "ppx/grfx/vk/vk_sync.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

Result Queue::CreateApiObjects(const grfx::internal::QueueCreateInfo* pCreateInfo)
{
    vkGetDeviceQueue(
        ToApi(GetDevice())->GetVkDevice(),
        pCreateInfo->queueFamilyIndex,
        pCreateInfo->queueIndex,
        &mQueue);

    VkCommandPoolCreateInfo vkci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    vkci.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkci.queueFamilyIndex        = pCreateInfo->queueFamilyIndex;

    VkResult vkres = vkCreateCommandPool(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mTransientPool);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateCommandPool failed" << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Queue::DestroyApiObjects()
{
    if (mTransientPool) {
        vkDestroyCommandPool(
            ToApi(GetDevice())->GetVkDevice(),
            mTransientPool,
            nullptr);
        mTransientPool.Reset();
    }

    if (mQueue) {
        WaitIdle();
        mQueue.Reset();
    }
}

Result Queue::WaitIdle()
{
    VkResult vkres = vkQueueWaitIdle(mQueue);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkQueueWaitIdle failed" << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result Queue::Submit(const grfx::SubmitInfo* pSubmitInfo)
{
    // Command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    for (uint32_t i = 0; i < pSubmitInfo->commandBufferCount; ++i) {
        commandBuffers.push_back(ToApi(pSubmitInfo->ppCommandBuffers[i])->GetVkCommandBuffer());
    }

    // Wait semaphores
    std::vector<VkSemaphore>          waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    for (uint32_t i = 0; i < pSubmitInfo->waitSemaphoreCount; ++i) {
        waitSemaphores.push_back(ToApi(pSubmitInfo->ppWaitSemaphores[i])->GetVkSemaphore());
        waitDstStageMasks.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }

    // Signal semaphores
    std::vector<VkSemaphore> signalSemaphores;
    for (uint32_t i = 0; i < pSubmitInfo->signalSemaphoreCount; ++i) {
        signalSemaphores.push_back(ToApi(pSubmitInfo->ppSignalSemaphores[i])->GetVkSemaphore());
    }

    VkSubmitInfo vksi         = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    vksi.waitSemaphoreCount   = CountU32(waitSemaphores);
    vksi.pWaitSemaphores      = DataPtr(waitSemaphores);
    vksi.pWaitDstStageMask    = DataPtr(waitDstStageMasks);
    vksi.commandBufferCount   = CountU32(commandBuffers);
    vksi.pCommandBuffers      = DataPtr(commandBuffers);
    vksi.signalSemaphoreCount = CountU32(signalSemaphores);
    vksi.pSignalSemaphores    = DataPtr(signalSemaphores);

    // Fence
    VkFence fence = VK_NULL_HANDLE;
    if (!IsNull(pSubmitInfo->pFence)) {
        fence = ToApi(pSubmitInfo->pFence)->GetVkFence();
    }

    VkResult vkres = vk::QueueSubmit(
        mQueue,
        1,
        &vksi,
        fence);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result Queue::GetTimestampFrequency(uint64_t* pFrequency) const
{
    if (IsNull(pFrequency)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    float  timestampPeriod = ToApi(GetDevice()->GetGpu())->GetTimestampPeriod();
    double ticksPerSecond  = 1000000000.0 / static_cast<double>(timestampPeriod);
    *pFrequency            = static_cast<uint64_t>(ticksPerSecond);

    return ppx::SUCCESS;
}

static VkResult CmdTransitionImageLayout(
    VkCommandBuffer      commandBuffer,
    VkImage              image,
    VkImageAspectFlags   aspectMask,
    uint32_t             baseMipLevel,
    uint32_t             levelCount,
    uint32_t             baseArrayLayer,
    uint32_t             layerCount,
    VkImageLayout        oldLayout,
    VkImageLayout        newLayout,
    VkPipelineStageFlags newPipelineStage)
{
    VkPipelineStageFlags srcStageMask    = 0;
    VkPipelineStageFlags dstStageMask    = 0;
    VkAccessFlags        srcAccessMask   = 0;
    VkAccessFlags        dstAccessMask   = 0;
    VkDependencyFlags    dependencyFlags = 0;

    switch (oldLayout) {
        default: {
            PPX_ASSERT_MSG(false, "invalid value for oldLayout");
            return VK_ERROR_INITIALIZATION_FAILED;
        } break;

        case VK_IMAGE_LAYOUT_UNDEFINED: {
            srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_GENERAL: {
            // @TODO: This may need tweaking.
            srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED: {
            srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
            // @TODO: This may need tweaking.
            srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
            // @TODO: This may need tweaking.
            srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
            srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        } break;
    }

    switch (newLayout) {
        default: {
            // Note: VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED cannot be a destination layout.
            PPX_ASSERT_MSG(false, "invalid value for newLayout");
            return VK_ERROR_INITIALIZATION_FAILED;
        } break;

        case VK_IMAGE_LAYOUT_GENERAL: {
            dstStageMask  = newPipelineStage;
            dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
            dstStageMask  = newPipelineStage;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            dstStageMask  = newPipelineStage;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        } break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
            dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
            dstStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstAccessMask = 0;
        } break;
    }

    VkImageMemoryBarrier barrier            = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask                   = srcAccessMask;
    barrier.dstAccessMask                   = dstAccessMask;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = aspectMask;
    barrier.subresourceRange.baseMipLevel   = baseMipLevel;
    barrier.subresourceRange.levelCount     = levelCount,
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount     = layerCount;

    vkCmdPipelineBarrier(
        commandBuffer,   // commandBuffer
        srcStageMask,    // srcStageMask
        dstStageMask,    // dstStageMask
        dependencyFlags, // dependencyFlags
        0,               // memoryBarrierCount
        nullptr,         // pMemoryBarriers
        0,               // bufferMemoryBarrierCount
        nullptr,         // pBufferMemoryBarriers
        1,               // imageMemoryBarrierCount
        &barrier);       // pImageMemoryBarriers);

    return VK_SUCCESS;
}

VkResult Queue::TransitionImageLayout(
    VkImage              image,
    VkImageAspectFlags   aspectMask,
    uint32_t             baseMipLevel,
    uint32_t             levelCount,
    uint32_t             baseArrayLayer,
    uint32_t             layerCount,
    VkImageLayout        oldLayout,
    VkImageLayout        newLayout,
    VkPipelineStageFlags newPipelineStage)
{
    VkCommandBufferAllocateInfo vkai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    vkai.commandPool                 = mTransientPool;
    vkai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkai.commandBufferCount          = 1;

    VkCommandBufferPtr commandBuffer;
    VkResult           vkres = vkAllocateCommandBuffers(
        ToApi(GetDevice())->GetVkDevice(),
        &vkai,
        &commandBuffer);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkAllocateCommandBuffers failed" << ToString(vkres));
        return vkres;
    }

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo         = nullptr;

    vkres = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (vkres != VK_SUCCESS) {
        vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);
        PPX_ASSERT_MSG(false, "vkBeginCommandBuffer failed" << ToString(vkres));
        return vkres;
    }

    vkres = CmdTransitionImageLayout(
        commandBuffer,
        image,
        aspectMask,
        baseMipLevel,
        levelCount,
        baseArrayLayer,
        layerCount,
        oldLayout,
        newLayout,
        newPipelineStage);
    if (vkres != VK_SUCCESS) {
        vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);
        PPX_ASSERT_MSG(false, "CmdTransitionImageLayout failed" << ToString(vkres));
        return vkres;
    }

    vkres = vkEndCommandBuffer(commandBuffer);
    if (vkres != VK_SUCCESS) {
        vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);
        PPX_ASSERT_MSG(false, "vkEndCommandBuffer failed" << ToString(vkres));
        return vkres;
    }

    VkSubmitInfo submitInfo         = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount   = 0;
    submitInfo.pWaitSemaphores      = nullptr;
    submitInfo.pWaitDstStageMask    = nullptr;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores    = nullptr;

    vkres = vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (vkres != VK_SUCCESS) {
        vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);
        PPX_ASSERT_MSG(false, "vkQueueSubmit failed" << ToString(vkres));
        return vkres;
    }

    vkres = vkQueueWaitIdle(mQueue);
    if (vkres != VK_SUCCESS) {
        vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);
        PPX_ASSERT_MSG(false, "vkQueueWaitIdle failed" << ToString(vkres));
        return vkres;
    }

    vkFreeCommandBuffers(ToApi(GetDevice())->GetVkDevice(), mTransientPool, 1, commandBuffer);

    return VK_SUCCESS;
}

} // namespace vk
} // namespace grfx
} // namespace ppx
