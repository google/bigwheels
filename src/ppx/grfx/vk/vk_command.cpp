// Copyright 2024 Google LLC
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

#include "ppx/grfx/vk/vk_command.h"
#include "ppx/grfx/vk/vk_buffer.h"
#include "ppx/grfx/vk/vk_descriptor.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_image.h"
#include "ppx/grfx/vk/vk_query.h"
#include "ppx/grfx/vk/vk_queue.h"
#include "ppx/grfx/vk/vk_pipeline.h"
#include "ppx/grfx/vk/vk_render_pass.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// CommandBuffer
// -------------------------------------------------------------------------------------------------
Result CommandBuffer::CreateApiObjects(const grfx::internal::CommandBufferCreateInfo* pCreateInfo)
{
    VkCommandBufferAllocateInfo vkai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    vkai.commandPool                 = ToApi(pCreateInfo->pPool)->GetVkCommandPool();
    vkai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkai.commandBufferCount          = 1;

    VkResult vkres = vk::AllocateCommandBuffers(
        ToApi(GetDevice())->GetVkDevice(),
        &vkai,
        &mCommandBuffer);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkAllocateCommandBuffers failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void CommandBuffer::DestroyApiObjects()
{
    if (mCommandBuffer) {
        vk::FreeCommandBuffers(
            ToApi(GetDevice())->GetVkDevice(),
            ToApi(mCreateInfo.pPool)->GetVkCommandPool(),
            1,
            mCommandBuffer);

        mCommandBuffer.Reset();
    }
}

Result CommandBuffer::Begin()
{
    VkCommandBufferBeginInfo vkbi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    VkResult vkres = vk::BeginCommandBuffer(mCommandBuffer, &vkbi);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkBeginCommandBuffer failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result CommandBuffer::End()
{
    VkResult vkres = vk::EndCommandBuffer(mCommandBuffer);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkEndCommandBuffer failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void CommandBuffer::BeginRenderPassImpl(const grfx::RenderPassBeginInfo* pBeginInfo)
{
    VkRect2D rect = {};
    rect.offset   = {pBeginInfo->renderArea.x, pBeginInfo->renderArea.x};
    rect.extent   = {pBeginInfo->renderArea.width, pBeginInfo->renderArea.height};

    uint32_t     clearValueCount                         = 0;
    VkClearValue clearValues[PPX_MAX_RENDER_TARGETS + 1] = {};

    for (uint32_t i = 0; i < pBeginInfo->RTVClearCount; ++i) {
        clearValues[i].color = ToVkClearColorValue(pBeginInfo->RTVClearValues[i]);
        ++clearValueCount;
    }

    if (pBeginInfo->pRenderPass->GetDepthStencilView()) {
        uint32_t i                  = clearValueCount;
        clearValues[i].depthStencil = ToVkClearDepthStencilValue(pBeginInfo->DSVClearValue);
        ++clearValueCount;
    }

    VkRenderPassBeginInfo vkbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    vkbi.renderPass            = ToApi(pBeginInfo->pRenderPass)->GetVkRenderPass();
    vkbi.framebuffer           = ToApi(pBeginInfo->pRenderPass)->GetVkFramebuffer();
    vkbi.renderArea            = rect;
    vkbi.clearValueCount       = clearValueCount;
    vkbi.pClearValues          = clearValues;

    vk::CmdBeginRenderPass(mCommandBuffer, &vkbi, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPassImpl()
{
    vk::CmdEndRenderPass(mCommandBuffer);
}

void CommandBuffer::BeginRenderingImpl(const grfx::RenderingInfo* pRenderingInfo)
{
    vk::Device* pDevice = ToApi(GetDevice());
    if (!pDevice->DynamicRenderingSupported()) {
        PPX_ASSERT_MSG(false, "Device does not support VK_KHR_dynamic_rendering");
        return;
    }
#if defined(VK_KHR_dynamic_rendering)
    VkRect2D rect = {};
    rect.offset   = {pRenderingInfo->renderArea.x, pRenderingInfo->renderArea.x};
    rect.extent   = {pRenderingInfo->renderArea.width, pRenderingInfo->renderArea.height};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentDescs;
    for (uint32_t i = 0; i < pRenderingInfo->renderTargetCount; ++i) {
        const grfx::RenderTargetViewPtr& rtv             = pRenderingInfo->pRenderTargetViews[i];
        VkRenderingAttachmentInfo        colorAttachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colorAttachment.imageView                        = ToApi(rtv.Get())->GetVkImageView();
        colorAttachment.imageLayout                      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode                      = VK_RESOLVE_MODE_NONE;
        colorAttachment.loadOp                           = ToVkAttachmentLoadOp(rtv->GetLoadOp());
        colorAttachment.storeOp                          = ToVkAttachmentStoreOp(rtv->GetStoreOp());
        colorAttachment.clearValue.color                 = ToVkClearColorValue(pRenderingInfo->RTVClearValues[i]);
        colorAttachmentDescs.push_back(colorAttachment);
    }

    VkRenderingInfo vkri      = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    vkri.renderArea           = rect;
    vkri.layerCount           = 1;
    vkri.viewMask             = 0;
    vkri.colorAttachmentCount = CountU32(colorAttachmentDescs);
    vkri.pColorAttachments    = DataPtr(colorAttachmentDescs);

    if (pRenderingInfo->pDepthStencilView) {
        grfx::DepthStencilViewPtr dsv             = pRenderingInfo->pDepthStencilView;
        VkRenderingAttachmentInfo depthAttachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depthAttachment.imageView                 = ToApi(dsv.Get())->GetVkImageView();
        depthAttachment.imageLayout               = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.resolveMode               = VK_RESOLVE_MODE_NONE;
        depthAttachment.loadOp                    = ToVkAttachmentLoadOp(dsv->GetDepthLoadOp());
        depthAttachment.storeOp                   = ToVkAttachmentStoreOp(dsv->GetDepthStoreOp());
        depthAttachment.clearValue.depthStencil   = ToVkClearDepthStencilValue(pRenderingInfo->DSVClearValue);
        vkri.pDepthAttachment                     = &depthAttachment;
    }

    if (pRenderingInfo->flags.bits.resuming) {
        vkri.flags |= VK_RENDERING_RESUMING_BIT;
    }
    if (pRenderingInfo->flags.bits.suspending) {
        vkri.flags |= VK_RENDERING_SUSPENDING_BIT;
    }

    PPX_ASSERT_MSG(vk::CmdBeginRenderingKHR != nullptr, "Function not found");
    vk::CmdBeginRenderingKHR(mCommandBuffer, &vkri);
#endif
}

void CommandBuffer::EndRenderingImpl()
{
#if defined(VK_KHR_dynamic_rendering)
    vk::Device* pDevice = ToApi(GetDevice());
    if (!pDevice->DynamicRenderingSupported()) {
        PPX_ASSERT_MSG(false, "Device does not support VK_KHR_dynamic_rendering");
        return;
    }
    PPX_ASSERT_MSG(vk::CmdEndRenderingKHR != nullptr, "Function not found");

    vk::CmdEndRenderingKHR(mCommandBuffer);
#endif
}

void CommandBuffer::PushDescriptorImpl(
    grfx::CommandType              pipelineBindPoint,
    const grfx::PipelineInterface* pInterface,
    grfx::DescriptorType           descriptorType,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer,
    const grfx::SampledImageView*  pSampledImageView,
    const grfx::StorageImageView*  pStorageImageView,
    const grfx::Sampler*           pSampler)
{
    auto pLayout = pInterface->GetSetLayout(set);
    PPX_ASSERT_MSG((pLayout != nullptr), "set=" << set << " does not match a set layout in the pipeline interface");
    PPX_ASSERT_MSG(pLayout->IsPushable(), "set=" << set << " refers to a set layout that is not pushable");

    // Determine pipeline bind point
    VkPipelineBindPoint vulkanPipelineBindPoint = InvalidValue<VkPipelineBindPoint>();
    switch (pipelineBindPoint) {
        default: PPX_ASSERT_MSG(false, "invalid pipeline bindpoint"); break;
        case grfx::COMMAND_TYPE_GRAPHICS: vulkanPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; break;
        case grfx::COMMAND_TYPE_COMPUTE: vulkanPipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; break;
    }

    // Determine descriptor type and fill out buffer/image info
    VkDescriptorImageInfo         imageInfo            = {};
    VkDescriptorBufferInfo        bufferInfo           = {};
    const VkDescriptorImageInfo*  pImageInfo           = nullptr;
    const VkDescriptorBufferInfo* pBufferInfo          = nullptr;
    VkDescriptorType              vulkanDescriptorType = InvalidValue<VkDescriptorType>();

    switch (descriptorType) {
        default: PPX_ASSERT_MSG(false, "descriptor is not of pushable type binding=" << binding << ", set=" << set); break;

        case grfx::DESCRIPTOR_TYPE_SAMPLER: {
            vulkanDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            imageInfo.sampler    = ToApi(pSampler)->GetVkSampler();
            pImageInfo           = &imageInfo;
        } break;

        case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
            vulkanDescriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView   = ToApi(pSampledImageView)->GetVkImageView();
            pImageInfo            = &imageInfo;
        } break;

        case grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            vulkanDescriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView   = ToApi(pStorageImageView)->GetVkImageView();
            pImageInfo            = &imageInfo;
        } break;

        case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
            vulkanDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bufferInfo.buffer    = ToApi(pBuffer)->GetVkBuffer();
            bufferInfo.offset    = bufferOffset;
            bufferInfo.range     = VK_WHOLE_SIZE;
            pBufferInfo          = &bufferInfo;
        } break;

        case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER:
        case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER:
        case grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: {
            vulkanDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bufferInfo.buffer    = ToApi(pBuffer)->GetVkBuffer();
            bufferInfo.offset    = bufferOffset;
            bufferInfo.range     = VK_WHOLE_SIZE;
            pBufferInfo          = &bufferInfo;
        } break;
    }

    // Fill out most of descriptor write
    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.pNext                = nullptr;
    write.dstSet               = VK_NULL_HANDLE;
    write.dstBinding           = binding;
    write.dstArrayElement      = 0;
    write.descriptorCount      = 1;
    write.descriptorType       = vulkanDescriptorType;
    write.pImageInfo           = pImageInfo;
    write.pBufferInfo          = pBufferInfo;
    write.pTexelBufferView     = nullptr;

    vk::CmdPushDescriptorSetKHR(
        mCommandBuffer,                           // commandBuffer
        vulkanPipelineBindPoint,                  // pipelineBindPoint
        ToApi(pInterface)->GetVkPipelineLayout(), // layout
        set,                                      // set
        1,                                        // descriptorWriteCount
        &write);                                  // pDescriptorWrites;
}

void CommandBuffer::ClearRenderTarget(
    grfx::Image*                        pImage,
    const grfx::RenderTargetClearValue& clearValue)
{
    if (!HasActiveRenderPass()) {
        PPX_LOG_WARN("No active render pass.");
        return;
    }

    grfx::Rect renderArea;
    uint32_t   colorAttachment = UINT32_MAX;
    uint32_t   baseArrayLayer;

    // Dynamic render pass
    if (mDynamicRenderPassActive) {
        renderArea = mDynamicRenderPassInfo.mRenderArea;

        auto views = mDynamicRenderPassInfo.mRenderTargetViews;
        for (uint32_t i = 0; i < views.size(); ++i) {
            auto rtv   = views[i];
            auto image = rtv->GetImage();
            if (image.Get() == pImage) {
                colorAttachment = i;
                baseArrayLayer  = rtv->GetArrayLayer();
                break;
            }
        }
    }
    else {
        // active regular render pass
        auto pCurrentRenderPass = GetCurrentRenderPass();
        renderArea              = pCurrentRenderPass->GetRenderArea();

        // Make sure pImage is a render target in current render pass
        const uint32_t renderTargetIndex = pCurrentRenderPass->GetRenderTargetImageIndex(pImage);
        colorAttachment                  = renderTargetIndex;

        auto view      = pCurrentRenderPass->GetRenderTargetView(renderTargetIndex);
        baseArrayLayer = view->GetArrayLayer();
    }

    if (colorAttachment == UINT32_MAX) {
        PPX_ASSERT_MSG(false, "Passed image is not a render target.");
        return;
    }

    // Clear attachment
    VkClearAttachment attachment           = {};
    attachment.aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
    attachment.colorAttachment             = colorAttachment;
    attachment.clearValue.color.float32[0] = clearValue.r;
    attachment.clearValue.color.float32[1] = clearValue.g;
    attachment.clearValue.color.float32[2] = clearValue.b;
    attachment.clearValue.color.float32[3] = clearValue.a;

    // Clear rect
    VkClearRect clearRect    = {};
    clearRect.rect.offset    = {renderArea.x, renderArea.y};
    clearRect.rect.extent    = {renderArea.width, renderArea.height};
    clearRect.baseArrayLayer = baseArrayLayer;
    clearRect.layerCount     = 1;

    vkCmdClearAttachments(
        mCommandBuffer,
        1,
        &attachment,
        1,
        &clearRect);
}

void CommandBuffer::ClearDepthStencil(
    grfx::Image*                        pImage,
    const grfx::DepthStencilClearValue& clearValue,
    uint32_t                            clearFlags)
{
    if (!HasActiveRenderPass()) {
        PPX_LOG_WARN("No active render pass.");
        return;
    }

    grfx::Rect renderArea;
    uint32_t   baseArrayLayer;

    // Dynamic render pass
    if (mDynamicRenderPassActive) {
        renderArea = mDynamicRenderPassInfo.mRenderArea;

        baseArrayLayer = mDynamicRenderPassInfo.mDepthStencilView->GetArrayLayer();
    }
    else {
        // active regular render pass
        auto pCurrentRenderPass = GetCurrentRenderPass();

        auto view = pCurrentRenderPass->GetDepthStencilView();

        renderArea = pCurrentRenderPass->GetRenderArea();

        baseArrayLayer = view->GetArrayLayer();
    }

    // Aspect mask
    VkImageAspectFlags aspectMask = 0;
    if (clearFlags & grfx::CLEAR_FLAG_DEPTH) {
        aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (clearFlags & grfx::CLEAR_FLAG_STENCIL) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    // Clear attachment
    VkClearAttachment attachment               = {};
    attachment.aspectMask                      = aspectMask;
    attachment.clearValue.depthStencil.depth   = clearValue.depth;
    attachment.clearValue.depthStencil.stencil = clearValue.stencil;

    // Clear rect
    VkClearRect clearRect    = {};
    clearRect.rect.offset    = {renderArea.x, renderArea.y};
    clearRect.rect.extent    = {renderArea.width, renderArea.height};
    clearRect.baseArrayLayer = baseArrayLayer;
    clearRect.layerCount     = 1;

    vkCmdClearAttachments(
        mCommandBuffer,
        1,
        &attachment,
        1,
        &clearRect);
}

void CommandBuffer::TransitionImageLayout(
    const grfx::Image*  pImage,
    uint32_t            mipLevel,
    uint32_t            mipLevelCount,
    uint32_t            arrayLayer,
    uint32_t            arrayLayerCount,
    grfx::ResourceState beforeState,
    grfx::ResourceState afterState,
    const grfx::Queue*  pSrcQueue,
    const grfx::Queue*  pDstQueue)
{
    PPX_ASSERT_NULL_ARG(pImage);

    if ((!IsNull(pSrcQueue) && IsNull(pDstQueue)) || (IsNull(pSrcQueue) && !IsNull(pDstQueue))) {
        PPX_ASSERT_MSG(false, "queue family transfer requires both pSrcQueue and pDstQueue to be NOT NULL");
    }

    uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    if (!IsNull(pSrcQueue)) {
        srcQueueFamilyIndex = ToApi(pSrcQueue)->GetQueueFamilyIndex();
    }

    if (!IsNull(pDstQueue)) {
        dstQueueFamilyIndex = ToApi(pDstQueue)->GetQueueFamilyIndex();
    }

    if (srcQueueFamilyIndex == dstQueueFamilyIndex) {
        srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    if (beforeState == afterState && srcQueueFamilyIndex == dstQueueFamilyIndex) {
        return;
    }

    if (mipLevelCount == PPX_REMAINING_MIP_LEVELS) {
        mipLevelCount = pImage->GetMipLevelCount();
    }

    if (arrayLayerCount == PPX_REMAINING_ARRAY_LAYERS) {
        arrayLayerCount = pImage->GetArrayLayerCount();
    }

    const vk::Image* pApiImage = ToApi(pImage);

    VkPipelineStageFlags srcStageMask    = InvalidValue<VkPipelineStageFlags>();
    VkPipelineStageFlags dstStageMask    = InvalidValue<VkPipelineStageFlags>();
    VkAccessFlags        srcAccessMask   = InvalidValue<VkAccessFlags>();
    VkAccessFlags        dstAccessMask   = InvalidValue<VkAccessFlags>();
    VkImageLayout        oldLayout       = InvalidValue<VkImageLayout>();
    VkImageLayout        newLayout       = InvalidValue<VkImageLayout>();
    VkDependencyFlags    dependencyFlags = 0;

    vk::Device* pDevice = ToApi(GetDevice());

    grfx::CommandType commandType = GetCommandType();

    Result ppxres = ToVkBarrierSrc(
        beforeState,
        commandType,
        pDevice->GetDeviceFeatures(),
        srcStageMask,
        srcAccessMask,
        oldLayout);
    PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "couldn't get src barrier data");

    ppxres = ToVkBarrierDst(
        afterState,
        commandType,
        pDevice->GetDeviceFeatures(),
        dstStageMask,
        dstAccessMask,
        newLayout);
    PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "couldn't get dst barrier data");

    VkImageMemoryBarrier barrier            = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask                   = srcAccessMask;
    barrier.dstAccessMask                   = dstAccessMask;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = srcQueueFamilyIndex;
    barrier.dstQueueFamilyIndex             = dstQueueFamilyIndex;
    barrier.image                           = pApiImage->GetVkImage();
    barrier.subresourceRange.aspectMask     = pApiImage->GetVkImageAspectFlags();
    barrier.subresourceRange.baseMipLevel   = mipLevel;
    barrier.subresourceRange.levelCount     = mipLevelCount;
    barrier.subresourceRange.baseArrayLayer = arrayLayer;
    barrier.subresourceRange.layerCount     = arrayLayerCount;

    vk::CmdPipelineBarrier(
        mCommandBuffer,  // commandBuffer
        srcStageMask,    // srcStageMask
        dstStageMask,    // dstStageMask
        dependencyFlags, // dependencyFlags
        0,               // memoryBarrierCount
        nullptr,         // pMemoryBarriers
        0,               // bufferMemoryBarrierCount
        nullptr,         // pBufferMemoryBarriers
        1,               // imageMemoryBarrierCount
        &barrier);       // pImageMemoryBarriers);
}

void CommandBuffer::BufferResourceBarrier(
    const grfx::Buffer* pBuffer,
    grfx::ResourceState beforeState,
    grfx::ResourceState afterState,
    const grfx::Queue*  pSrcQueue,
    const grfx::Queue*  pDstQueue)
{
    PPX_ASSERT_NULL_ARG(pBuffer);

    if ((!IsNull(pSrcQueue) && IsNull(pDstQueue)) || (IsNull(pSrcQueue) && !IsNull(pDstQueue))) {
        PPX_ASSERT_MSG(false, "queue family transfer requires both pSrcQueue and pDstQueue to be NOT NULL");
    }

    uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    if (!IsNull(pSrcQueue)) {
        srcQueueFamilyIndex = ToApi(pSrcQueue)->GetQueueFamilyIndex();
    }

    if (!IsNull(pDstQueue)) {
        dstQueueFamilyIndex = ToApi(pDstQueue)->GetQueueFamilyIndex();
    }

    if (srcQueueFamilyIndex == dstQueueFamilyIndex) {
        srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }

    if (beforeState == afterState && srcQueueFamilyIndex == dstQueueFamilyIndex) {
        return;
    }

    VkPipelineStageFlags srcStageMask    = InvalidValue<VkPipelineStageFlags>();
    VkPipelineStageFlags dstStageMask    = InvalidValue<VkPipelineStageFlags>();
    VkAccessFlags        srcAccessMask   = InvalidValue<VkAccessFlags>();
    VkAccessFlags        dstAccessMask   = InvalidValue<VkAccessFlags>();
    VkImageLayout        oldLayout       = InvalidValue<VkImageLayout>();
    VkImageLayout        newLayout       = InvalidValue<VkImageLayout>();
    VkDependencyFlags    dependencyFlags = 0;

    vk::Device* pDevice = ToApi(GetDevice());

    grfx::CommandType commandType = GetCommandType();

    Result ppxres = ToVkBarrierSrc(
        beforeState,
        commandType,
        pDevice->GetDeviceFeatures(),
        srcStageMask,
        srcAccessMask,
        oldLayout);
    PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "couldn't get src barrier data");

    ppxres = ToVkBarrierDst(
        afterState,
        commandType,
        pDevice->GetDeviceFeatures(),
        dstStageMask,
        dstAccessMask,
        newLayout);
    PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "couldn't get dst barrier data");

    VkBufferMemoryBarrier barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask         = srcAccessMask;
    barrier.dstAccessMask         = dstAccessMask;
    barrier.srcQueueFamilyIndex   = srcQueueFamilyIndex;
    barrier.dstQueueFamilyIndex   = dstQueueFamilyIndex;
    barrier.buffer                = ToApi(pBuffer)->GetVkBuffer();
    barrier.offset                = static_cast<VkDeviceSize>(0);
    barrier.size                  = static_cast<VkDeviceSize>(pBuffer->GetSize());

    vkCmdPipelineBarrier(
        mCommandBuffer,  // commandBuffer
        srcStageMask,    // srcStageMask
        dstStageMask,    // dstStageMask
        dependencyFlags, // dependencyFlags
        0,               // memoryBarrierCount
        nullptr,         // pMemoryBarriers
        1,               // bufferMemoryBarrierCount
        &barrier,        // pBufferMemoryBarriers
        0,               // imageMemoryBarrierCount
        nullptr);        // pImageMemoryBarriers);
}

void CommandBuffer::SetViewports(uint32_t viewportCount, const grfx::Viewport* pViewports)
{
    VkViewport viewports[PPX_MAX_VIEWPORTS] = {};
    for (uint32_t i = 0; i < viewportCount; ++i) {
        // clang-format off
        viewports[i].x        =  pViewports[i].x;
        viewports[i].y        =  pViewports[i].height;
        viewports[i].width    =  pViewports[i].width;
        viewports[i].height   = -pViewports[i].height;
        viewports[i].minDepth =  pViewports[i].minDepth;
        viewports[i].maxDepth =  pViewports[i].maxDepth;
        // clang-format on
    }

    vkCmdSetViewport(
        mCommandBuffer,
        0,
        viewportCount,
        viewports);
}

void CommandBuffer::SetScissors(uint32_t scissorCount, const grfx::Rect* pScissors)
{
    vkCmdSetScissor(
        mCommandBuffer,
        0,
        scissorCount,
        reinterpret_cast<const VkRect2D*>(pScissors));
}

void CommandBuffer::BindDescriptorSets(
    VkPipelineBindPoint               bindPoint,
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    PPX_ASSERT_NULL_ARG(pInterface);

    // D3D12 needs the pipeline interface (root signature) bound even if there
    // aren't any descriptor sets. Since Vulkan doesn't require this, we'll
    // just treat it as a NOOP if setCount is zero.
    //
    if (setCount == 0) {
        return;
    }

    // Get set numbers
    const std::vector<uint32_t>& setNumbers = pInterface->GetSetNumbers();

    // setCount cannot exceed the number of sets in the pipeline interface
    uint32_t setNumberCount = CountU32(setNumbers);
    if (setCount > setNumberCount) {
        PPX_ASSERT_MSG(false, "setCount exceeds the number of sets in pipeline interface");
    }

    if (setCount > 0) {
        // Get Vulkan handles
        VkDescriptorSet vkSets[PPX_MAX_BOUND_DESCRIPTOR_SETS] = {VK_NULL_HANDLE};
        for (uint32_t i = 0; i < setCount; ++i) {
            vkSets[i] = ToApi(ppSets[i])->GetVkDescriptorSet();
        }

        // If we have consecutive set numbers we can bind just once...
        if (pInterface->HasConsecutiveSetNumbers()) {
            uint32_t firstSet = setNumbers[0];

            vk::CmdBindDescriptorSets(
                mCommandBuffer,                           // commandBuffer
                bindPoint,                                // pipelineBindPoint
                ToApi(pInterface)->GetVkPipelineLayout(), // layout
                firstSet,                                 // firstSet
                setCount,                                 // descriptorSetCount
                vkSets,                                   // pDescriptorSets
                0,                                        // dynamicOffsetCount
                nullptr);                                 // pDynamicOffsets
        }
        // ...otherwise we get to bind a bunch of times
        else {
            for (uint32_t i = 0; i < setCount; ++i) {
                uint32_t firstSet = setNumbers[i];

                vk::CmdBindDescriptorSets(
                    mCommandBuffer,                           // commandBuffer
                    bindPoint,                                // pipelineBindPoint
                    ToApi(pInterface)->GetVkPipelineLayout(), // layout
                    firstSet,                                 // firstSet
                    1,                                        // descriptorSetCount
                    &vkSets[i],                               // pDescriptorSets
                    0,                                        // dynamicOffsetCount
                    nullptr);                                 // pDynamicOffsets
            }
        }
    }
    else {
        vk::CmdBindDescriptorSets(
            mCommandBuffer,                           // commandBuffer
            bindPoint,                                // pipelineBindPoint
            ToApi(pInterface)->GetVkPipelineLayout(), // layout
            0,                                        // firstSet
            0,                                        // descriptorSetCount
            nullptr,                                  // pDescriptorSets
            0,                                        // dynamicOffsetCount
            nullptr);                                 // pDynamicOffsets
    }
}

void CommandBuffer::BindGraphicsDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pInterface, setCount, ppSets);
}

void CommandBuffer::PushConstants(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       count,
    const void*                    pValues,
    uint32_t                       dstOffset)
{
    PPX_ASSERT_NULL_ARG(pInterface);
    PPX_ASSERT_NULL_ARG(pValues);

    PPX_ASSERT_MSG(((dstOffset + count) <= PPX_MAX_PUSH_CONSTANTS), "dstOffset + count (" << (dstOffset + count) << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");

    const uint32_t sizeInBytes   = count * sizeof(uint32_t);
    const uint32_t offsetInBytes = dstOffset * sizeof(uint32_t);

    vkCmdPushConstants(
        mCommandBuffer,
        ToApi(pInterface)->GetVkPipelineLayout(),
        ToApi(pInterface)->GetPushConstantShaderStageFlags(),
        offsetInBytes,
        sizeInBytes,
        pValues);
}

void CommandBuffer::PushGraphicsConstants(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       count,
    const void*                    pValues,
    uint32_t                       dstOffset)
{
    PPX_ASSERT_MSG(((dstOffset + count) <= PPX_MAX_PUSH_CONSTANTS), "dstOffset + count (" << (dstOffset + count) << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");

    const VkShaderStageFlags shaderStageFlags = ToApi(pInterface)->GetPushConstantShaderStageFlags();
    if ((shaderStageFlags & VK_SHADER_STAGE_ALL_GRAPHICS) == 0) {
        PPX_ASSERT_MSG(false, "push constants shader visibility flags in pInterface does not have any graphics stages");
    }

    PushConstants(pInterface, count, pValues, dstOffset);
}

void CommandBuffer::BindGraphicsPipeline(const grfx::GraphicsPipeline* pPipeline)
{
    PPX_ASSERT_NULL_ARG(pPipeline);

    vk::CmdBindPipeline(
        mCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ToApi(pPipeline)->GetVkPipeline());
}

void CommandBuffer::BindComputeDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pInterface, setCount, ppSets);
}

void CommandBuffer::PushComputeConstants(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       count,
    const void*                    pValues,
    uint32_t                       dstOffset)
{
    PPX_ASSERT_MSG(((dstOffset + count) <= PPX_MAX_PUSH_CONSTANTS), "dstOffset + count (" << (dstOffset + count) << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");

    const VkShaderStageFlags shaderStageFlags = ToApi(pInterface)->GetPushConstantShaderStageFlags();
    if ((shaderStageFlags & VK_SHADER_STAGE_COMPUTE_BIT) == 0) {
        PPX_ASSERT_MSG(false, "push constants shader visibility flags in pInterface does not have compute stage");
    }

    PushConstants(pInterface, count, pValues, dstOffset);
}

void CommandBuffer::BindComputePipeline(const grfx::ComputePipeline* pPipeline)
{
    PPX_ASSERT_NULL_ARG(pPipeline);

    vk::CmdBindPipeline(
        mCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        ToApi(pPipeline)->GetVkPipeline());
}

void CommandBuffer::BindIndexBuffer(const grfx::IndexBufferView* pView)
{
    PPX_ASSERT_NULL_ARG(pView);
    PPX_ASSERT_NULL_ARG(pView->pBuffer);

    vk::CmdBindIndexBuffer(
        mCommandBuffer,
        ToApi(pView->pBuffer)->GetVkBuffer(),
        static_cast<VkDeviceSize>(pView->offset),
        ToVkIndexType(pView->indexType));
}

void CommandBuffer::BindVertexBuffers(uint32_t viewCount, const grfx::VertexBufferView* pViews)
{
    PPX_ASSERT_NULL_ARG(pViews);
    PPX_ASSERT_MSG(viewCount < PPX_MAX_VERTEX_BINDINGS, "viewCount exceeds PPX_MAX_VERTEX_ATTRIBUTES");

    VkBuffer     buffers[PPX_MAX_RENDER_TARGETS] = {VK_NULL_HANDLE};
    VkDeviceSize offsets[PPX_MAX_RENDER_TARGETS] = {0};

    for (uint32_t i = 0; i < viewCount; ++i) {
        buffers[i] = ToApi(pViews[i].pBuffer)->GetVkBuffer();
        offsets[i] = static_cast<VkDeviceSize>(pViews[i].offset);
    }

    vk::CmdBindVertexBuffers(
        mCommandBuffer,
        0,
        viewCount,
        buffers,
        offsets);
}

void CommandBuffer::Draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    vkCmdDraw(mCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t  vertexOffset,
    uint32_t firstInstance)
{
    vk::CmdDrawIndexed(mCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::Dispatch(
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ)
{
    vk::CmdDispatch(mCommandBuffer, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::CopyBufferToBuffer(
    const grfx::BufferToBufferCopyInfo* pCopyInfo,
    grfx::Buffer*                       pSrcBuffer,
    grfx::Buffer*                       pDstBuffer)
{
    VkBufferCopy region = {};
    region.srcOffset    = static_cast<VkDeviceSize>(pCopyInfo->srcBuffer.offset);
    region.dstOffset    = static_cast<VkDeviceSize>(pCopyInfo->dstBuffer.offset);
    region.size         = static_cast<VkDeviceSize>(pCopyInfo->size);

    vkCmdCopyBuffer(
        mCommandBuffer,
        ToApi(pSrcBuffer)->GetVkBuffer(),
        ToApi(pDstBuffer)->GetVkBuffer(),
        1,
        &region);
}

void CommandBuffer::CopyBufferToImage(
    const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
    grfx::Buffer*                                   pSrcBuffer,
    grfx::Image*                                    pDstImage)
{
    PPX_ASSERT_NULL_ARG(pSrcBuffer);
    PPX_ASSERT_NULL_ARG(pDstImage);

    std::vector<VkBufferImageCopy> regions(pCopyInfos.size());

    for (size_t i = 0; i < pCopyInfos.size(); i++) {
        regions[i].bufferOffset                    = static_cast<VkDeviceSize>(pCopyInfos[i].srcBuffer.footprintOffset);
        regions[i].bufferRowLength                 = pCopyInfos[i].srcBuffer.imageWidth;
        regions[i].bufferImageHeight               = pCopyInfos[i].srcBuffer.imageHeight;
        regions[i].imageSubresource.aspectMask     = ToApi(pDstImage)->GetVkImageAspectFlags();
        regions[i].imageSubresource.mipLevel       = pCopyInfos[i].dstImage.mipLevel;
        regions[i].imageSubresource.baseArrayLayer = pCopyInfos[i].dstImage.arrayLayer;
        regions[i].imageSubresource.layerCount     = pCopyInfos[i].dstImage.arrayLayerCount;
        regions[i].imageOffset.x                   = pCopyInfos[i].dstImage.x;
        regions[i].imageOffset.y                   = pCopyInfos[i].dstImage.y;
        regions[i].imageOffset.z                   = pCopyInfos[i].dstImage.z;
        regions[i].imageExtent.width               = pCopyInfos[i].dstImage.width;
        regions[i].imageExtent.height              = pCopyInfos[i].dstImage.height;
        regions[i].imageExtent.depth               = pCopyInfos[i].dstImage.depth;
    }

    vkCmdCopyBufferToImage(
        mCommandBuffer,
        ToApi(pSrcBuffer)->GetVkBuffer(),
        ToApi(pDstImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(pCopyInfos.size()),
        regions.data());
}

void CommandBuffer::CopyBufferToImage(
    const grfx::BufferToImageCopyInfo* pCopyInfo,
    grfx::Buffer*                      pSrcBuffer,
    grfx::Image*                       pDstImage)
{
    return CopyBufferToImage(std::vector<grfx::BufferToImageCopyInfo>{*pCopyInfo}, pSrcBuffer, pDstImage);
}

grfx::ImageToBufferOutputPitch CommandBuffer::CopyImageToBuffer(
    const grfx::ImageToBufferCopyInfo* pCopyInfo,
    grfx::Image*                       pSrcImage,
    grfx::Buffer*                      pDstBuffer)
{
    std::vector<VkBufferImageCopy> regions;

    VkBufferImageCopy region               = {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0; // Tightly-packed texels.
    region.bufferImageHeight               = 0; // Tightly-packed texels.
    region.imageSubresource.mipLevel       = pCopyInfo->srcImage.mipLevel;
    region.imageSubresource.baseArrayLayer = pCopyInfo->srcImage.arrayLayer;
    region.imageSubresource.layerCount     = pCopyInfo->srcImage.arrayLayerCount;
    region.imageOffset.x                   = pCopyInfo->srcImage.offset.x;
    region.imageOffset.y                   = pCopyInfo->srcImage.offset.y;
    region.imageOffset.z                   = pCopyInfo->srcImage.offset.z;
    region.imageExtent                     = {0, 1, 1};
    region.imageExtent.width               = pCopyInfo->extent.x;
    if (pSrcImage->GetType() != IMAGE_TYPE_1D) { // Can only be set for 2D and 3D textures.
        region.imageExtent.height = pCopyInfo->extent.y;
    }
    if (pSrcImage->GetType() == IMAGE_TYPE_3D) { // Can only be set for 3D textures.
        region.imageExtent.depth = pCopyInfo->extent.z;
    }

    const grfx::FormatDesc* srcDesc = grfx::GetFormatDescription(pSrcImage->GetFormat());

    // For depth-stencil images, each component must be copied separately.
    if (srcDesc->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL) {
        // First copy depth.
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        regions.push_back(region);

        // Compute the total size of the depth part to offset the stencil part.
        // We always copy tightly-packed texels, so we don't have to worry
        // about tiling.
        size_t depthTexelBytes = srcDesc->bytesPerTexel - 1; // Stencil is always 1 byte.
        size_t depthTotalBytes = depthTexelBytes * pCopyInfo->extent.x * pCopyInfo->extent.y;

        // Then copy stencil.
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        region.bufferOffset                = depthTotalBytes;
        regions.push_back(region);
    }
    else {
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions.push_back(region);
    }

    vkCmdCopyImageToBuffer(
        mCommandBuffer,
        ToApi(pSrcImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        ToApi(pDstBuffer)->GetVkBuffer(),
        static_cast<uint32_t>(regions.size()),
        regions.data());

    grfx::ImageToBufferOutputPitch outPitch;
    outPitch.rowPitch = srcDesc->bytesPerTexel * pCopyInfo->extent.x;
    return outPitch;
}

void CommandBuffer::CopyImageToImage(
    const grfx::ImageToImageCopyInfo* pCopyInfo,
    grfx::Image*                      pSrcImage,
    grfx::Image*                      pDstImage)
{
    bool isSourceDepthStencil = grfx::GetFormatDescription(pSrcImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    bool isDestDepthStencil   = grfx::GetFormatDescription(pDstImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    PPX_ASSERT_MSG(isSourceDepthStencil == isDestDepthStencil, "both images in an image copy must be depth-stencil if one is depth-stencil");

    VkImageSubresourceLayers srcSubresource = {};
    srcSubresource.aspectMask               = DetermineAspectMask(ToApi(pSrcImage)->GetVkFormat());
    srcSubresource.baseArrayLayer           = pCopyInfo->srcImage.arrayLayer;
    srcSubresource.layerCount               = pCopyInfo->srcImage.arrayLayerCount;
    srcSubresource.mipLevel                 = pCopyInfo->srcImage.mipLevel;

    VkImageSubresourceLayers dstSubresource = {};
    dstSubresource.aspectMask               = DetermineAspectMask(ToApi(pDstImage)->GetVkFormat());
    dstSubresource.baseArrayLayer           = pCopyInfo->dstImage.arrayLayer;
    dstSubresource.layerCount               = pCopyInfo->dstImage.arrayLayerCount;
    dstSubresource.mipLevel                 = pCopyInfo->dstImage.mipLevel;

    VkImageCopy region = {};
    region.srcOffset   = {
        static_cast<int32_t>(pCopyInfo->srcImage.offset.x),
        static_cast<int32_t>(pCopyInfo->srcImage.offset.y),
        static_cast<int32_t>(pCopyInfo->srcImage.offset.z)};
    region.srcSubresource = srcSubresource;
    region.dstOffset      = {
        static_cast<int32_t>(pCopyInfo->dstImage.offset.x),
        static_cast<int32_t>(pCopyInfo->dstImage.offset.y),
        static_cast<int32_t>(pCopyInfo->dstImage.offset.z)};
    region.dstSubresource = dstSubresource;
    region.extent         = {0, 1, 1};
    region.extent.width   = pCopyInfo->extent.x;
    if (pSrcImage->GetType() != IMAGE_TYPE_1D) { // Can only be set for 2D and 3D textures.
        region.extent.height = pCopyInfo->extent.y;
    }
    if (pSrcImage->GetType() == IMAGE_TYPE_3D) { // Can only be set for 3D textures.
        region.extent.height = pCopyInfo->extent.z;
    }

    vkCmdCopyImage(
        mCommandBuffer,
        ToApi(pSrcImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        ToApi(pDstImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void CommandBuffer::BlitImage(
    const grfx::ImageBlitInfo* pCopyInfo,
    grfx::Image*               pSrcImage,
    grfx::Image*               pDstImage)
{
    bool isSourceDepthStencil = grfx::GetFormatDescription(pSrcImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    bool isDestDepthStencil   = grfx::GetFormatDescription(pDstImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    PPX_ASSERT_MSG(isSourceDepthStencil == isDestDepthStencil, "both images in an image copy must be depth-stencil if one is depth-stencil");

    VkImageSubresourceLayers srcSubresource = {};
    srcSubresource.aspectMask               = DetermineAspectMask(ToApi(pSrcImage)->GetVkFormat());
    srcSubresource.baseArrayLayer           = pCopyInfo->srcImage.arrayLayer;
    srcSubresource.layerCount               = pCopyInfo->srcImage.arrayLayerCount;
    srcSubresource.mipLevel                 = pCopyInfo->srcImage.mipLevel;

    VkImageSubresourceLayers dstSubresource = {};
    dstSubresource.aspectMask               = DetermineAspectMask(ToApi(pDstImage)->GetVkFormat());
    dstSubresource.baseArrayLayer           = pCopyInfo->dstImage.arrayLayer;
    dstSubresource.layerCount               = pCopyInfo->dstImage.arrayLayerCount;
    dstSubresource.mipLevel                 = pCopyInfo->dstImage.mipLevel;

    VkImageBlit region    = {};
    region.srcSubresource = srcSubresource;
    region.dstSubresource = dstSubresource;
    for (int i = 0; i < 2; ++i) {
        region.srcOffsets[i] = {
            static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].x),
            static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].y),
            static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].z)};
        region.dstOffsets[i] = {
            static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].x),
            static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].y),
            static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].z)};
    }

    VkFilter filter;
    switch (pCopyInfo->filter) {
        case FILTER_NEAREST:
            filter = VK_FILTER_NEAREST;
            break;
        case FILTER_LINEAR:
            filter = VK_FILTER_LINEAR;
            break;
        default:
            PPX_ASSERT_MSG(false, "Invalid filter value: " << (int)pCopyInfo->filter);
            return;
    }

    vkCmdBlitImage(
        mCommandBuffer,
        ToApi(pSrcImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        ToApi(pDstImage)->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region,
        filter);
}

void CommandBuffer::BeginQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    VkQueryControlFlags flags = 0;
    if (pQuery->GetType() == grfx::QUERY_TYPE_OCCLUSION) {
        flags = VK_QUERY_CONTROL_PRECISE_BIT;
    }

    vkCmdBeginQuery(
        mCommandBuffer,
        ToApi(pQuery)->GetVkQueryPool(),
        queryIndex,
        flags);
}

void CommandBuffer::EndQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    vkCmdEndQuery(
        mCommandBuffer,
        ToApi(pQuery)->GetVkQueryPool(),
        queryIndex);
}

void CommandBuffer::WriteTimestamp(
    const grfx::Query*  pQuery,
    grfx::PipelineStage pipelineStage,
    uint32_t            queryIndex)
{
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");
    vkCmdWriteTimestamp(
        mCommandBuffer,
        ToVkPipelineStage(pipelineStage),
        ToApi(pQuery)->GetVkQueryPool(),
        queryIndex);
}

void CommandBuffer::ResolveQueryData(
    grfx::Query* pQuery,
    uint32_t     startIndex,
    uint32_t     numQueries)
{
    PPX_ASSERT_MSG((startIndex + numQueries) <= pQuery->GetCount(), "invalid query index/number");
    const VkQueryResultFlags flags = VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT;
    vkCmdCopyQueryPoolResults(mCommandBuffer, ToApi(pQuery)->GetVkQueryPool(), startIndex, numQueries, ToApi(pQuery)->GetReadBackBuffer(), 0, ToApi(pQuery)->GetQueryTypeSize(), flags);
}

// -------------------------------------------------------------------------------------------------
// CommandPool
// -------------------------------------------------------------------------------------------------
Result CommandPool::CreateApiObjects(const grfx::CommandPoolCreateInfo* pCreateInfo)
{
    VkCommandPoolCreateInfo vkci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    vkci.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkci.queueFamilyIndex        = ToApi(pCreateInfo->pQueue)->GetQueueFamilyIndex();

    VkResult vkres = vk::CreateCommandPool(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mCommandPool);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateCommandPool failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void CommandPool::DestroyApiObjects()
{
    if (mCommandPool) {
        vkDestroyCommandPool(
            ToApi(GetDevice())->GetVkDevice(),
            mCommandPool,
            nullptr);

        mCommandPool.Reset();
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
