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
    const grfx::BufferToImageCopyInfo& pCopyInfo,
    grfx::Buffer*                      pSrcBuffer,
    grfx::Image*                       pDstImage)
{
    return CopyBufferToImage({pCopyInfo}, pSrcBuffer, pDstImage);
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
