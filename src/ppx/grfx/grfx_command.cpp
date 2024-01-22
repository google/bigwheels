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

#include "ppx/grfx/grfx_command.h"
#include "ppx/grfx/grfx_buffer.h"
#include "ppx/grfx/grfx_queue.h"
#include "ppx/grfx/grfx_draw_pass.h"
#include "ppx/grfx/grfx_fullscreen_quad.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_mesh.h"
#include "ppx/grfx/grfx_render_pass.h"
#include "ppx/grfx/grfx_texture.h"

namespace ppx {
namespace grfx {

CommandType CommandPool::GetCommandType() const
{
    return mCreateInfo.pQueue->GetCommandType();
}

bool CommandBuffer::HasActiveRenderPass() const
{
    return !IsNull(mCurrentRenderPass) || mDynamicRenderPassActive;
}

void CommandBuffer::BeginRenderPass(const grfx::RenderPassBeginInfo* pBeginInfo)
{
    if (HasActiveRenderPass()) {
        PPX_ASSERT_MSG(false, "cannot nest render passes");
    }

    if (pBeginInfo->pRenderPass->HasLoadOpClear()) {
        uint32_t rtvCount   = pBeginInfo->pRenderPass->GetRenderTargetCount();
        uint32_t clearCount = pBeginInfo->RTVClearCount;
        if (clearCount < rtvCount) {
            PPX_ASSERT_MSG(false, "clear count cannot less than RTV count");
        }
    }

    BeginRenderPassImpl(pBeginInfo);
    mCurrentRenderPass = pBeginInfo->pRenderPass;
}

void CommandBuffer::EndRenderPass()
{
    if (IsNull(mCurrentRenderPass)) {
        PPX_ASSERT_MSG(false, "no render pass to end");
    }
    PPX_ASSERT_MSG(!mDynamicRenderPassActive, "Dynamic render pass active, use EndRendering instead");

    EndRenderPassImpl();
    mCurrentRenderPass = nullptr;
}

void CommandBuffer::BeginRendering(const grfx::RenderingInfo* pRenderingInfo)
{
    PPX_ASSERT_NULL_ARG(pRenderingInfo);
    PPX_ASSERT_MSG(!HasActiveRenderPass(), "cannot nest render passes");

    BeginRenderingImpl(pRenderingInfo);
    mDynamicRenderPassActive = true;
    mRenderArea = pRenderingInfo->renderArea;
    mRenderTargetView = pRenderingInfo->pRenderTargetViews;
    mRenderTargetCount = pRenderingInfo->renderTargetCount;
    mDepthStencilView = pRenderingInfo->pDepthStencilView;
}

void CommandBuffer::EndRendering()
{
    PPX_ASSERT_MSG(mDynamicRenderPassActive, "no render pass to end")
    PPX_ASSERT_MSG(IsNull(mCurrentRenderPass), "Non-dynamic render pass active, use EndRendering instead");

    EndRenderingImpl();
    mDynamicRenderPassActive = false;
}

void CommandBuffer::BeginRenderPass(const grfx::RenderPass* pRenderPass)
{
    PPX_ASSERT_NULL_ARG(pRenderPass);

    grfx::RenderPassBeginInfo beginInfo = {};
    beginInfo.pRenderPass               = pRenderPass;
    beginInfo.renderArea                = pRenderPass->GetRenderArea();

    beginInfo.RTVClearCount = pRenderPass->GetRenderTargetCount();
    for (uint32_t i = 0; i < beginInfo.RTVClearCount; ++i) {
        grfx::ImagePtr rtvImage     = pRenderPass->GetRenderTargetImage(i);
        beginInfo.RTVClearValues[i] = rtvImage->GetRTVClearValue();
    }

    grfx::ImagePtr dsvImage = pRenderPass->GetDepthStencilImage();
    if (dsvImage) {
        beginInfo.DSVClearValue = dsvImage->GetDSVClearValue();
    }

    BeginRenderPass(&beginInfo);
}

void CommandBuffer::BeginRenderPass(
    const grfx::DrawPass*           pDrawPass,
    const grfx::DrawPassClearFlags& clearFlags)
{
    PPX_ASSERT_NULL_ARG(pDrawPass);

    grfx::RenderPassBeginInfo beginInfo = {};
    pDrawPass->PrepareRenderPassBeginInfo(clearFlags, &beginInfo);

    BeginRenderPass(&beginInfo);
}

void CommandBuffer::TransitionImageLayout(
    grfx::RenderPass*   pRenderPass,
    grfx::ResourceState renderTargetBeforeState,
    grfx::ResourceState renderTargetAfterState,
    grfx::ResourceState depthStencilTargetBeforeState,
    grfx::ResourceState depthStencilTargetAfterState)
{
    PPX_ASSERT_NULL_ARG(pRenderPass);

    const uint32_t n = pRenderPass->GetRenderTargetCount();
    for (uint32_t i = 0; i < n; ++i) {
        grfx::ImagePtr renderTarget;
        Result         ppxres = pRenderPass->GetRenderTargetImage(i, &renderTarget);
        PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "failed getting render pass render target");

        TransitionImageLayout(
            renderTarget,
            PPX_ALL_SUBRESOURCES,
            renderTargetBeforeState,
            renderTargetAfterState);
    }

    if (pRenderPass->HasDepthStencil()) {
        grfx::ImagePtr depthStencil;
        Result         ppxres = pRenderPass->GetDepthStencilImage(&depthStencil);
        PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "failed getting render pass depth/stencil");

        TransitionImageLayout(
            depthStencil,
            PPX_ALL_SUBRESOURCES,
            depthStencilTargetBeforeState,
            depthStencilTargetAfterState);
    }
}

void CommandBuffer::TransitionImageLayout(
    const grfx::Texture* pTexture,
    uint32_t             mipLevel,
    uint32_t             mipLevelCount,
    uint32_t             arrayLayer,
    uint32_t             arrayLayerCount,
    grfx::ResourceState  beforeState,
    grfx::ResourceState  afterState,
    const grfx::Queue*   pSrcQueue,
    const grfx::Queue*   pDstQueue)
{
    TransitionImageLayout(
        pTexture->GetImage(),
        mipLevel,
        mipLevelCount,
        arrayLayer,
        arrayLayerCount,
        beforeState,
        afterState,
        pSrcQueue,
        pDstQueue);
}

void CommandBuffer::TransitionImageLayout(
    grfx::DrawPass*     pDrawPass,
    grfx::ResourceState renderTargetBeforeState,
    grfx::ResourceState renderTargetAfterState,
    grfx::ResourceState depthStencilTargetBeforeState,
    grfx::ResourceState depthStencilTargetAfterState)
{
    PPX_ASSERT_NULL_ARG(pDrawPass);

    const uint32_t n = pDrawPass->GetRenderTargetCount();
    for (uint32_t i = 0; i < n; ++i) {
        grfx::TexturePtr renderTarget;
        Result           ppxres = pDrawPass->GetRenderTargetTexture(i, &renderTarget);
        PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "failed getting draw pass render target");

        TransitionImageLayout(
            renderTarget->GetImage(),
            PPX_ALL_SUBRESOURCES,
            renderTargetBeforeState,
            renderTargetAfterState);
    }

    if (pDrawPass->HasDepthStencil()) {
        grfx::TexturePtr depthSencil;
        Result           ppxres = pDrawPass->GetDepthStencilTexture(&depthSencil);
        PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "failed getting draw pass depth/stencil");

        TransitionImageLayout(
            depthSencil->GetImage(),
            PPX_ALL_SUBRESOURCES,
            depthStencilTargetBeforeState,
            depthStencilTargetAfterState);
    }
}

void CommandBuffer::SetViewports(const grfx::Viewport& viewport)
{
    SetViewports(1, &viewport);
}

void CommandBuffer::SetScissors(const grfx::Rect& scissor)
{
    SetScissors(1, &scissor);
}

void CommandBuffer::BindIndexBuffer(const grfx::Buffer* pBuffer, grfx::IndexType indexType, uint64_t offset)
{
    PPX_ASSERT_NULL_ARG(pBuffer);

    grfx::IndexBufferView view = {};
    view.pBuffer               = pBuffer;
    view.indexType             = indexType;
    view.offset                = offset;

    BindIndexBuffer(&view);
}

void CommandBuffer::BindIndexBuffer(const grfx::Mesh* pMesh, uint64_t offset)
{
    PPX_ASSERT_NULL_ARG(pMesh);

    BindIndexBuffer(pMesh->GetIndexBuffer(), pMesh->GetIndexType(), offset);
}

void CommandBuffer::BindVertexBuffers(
    uint32_t                   bufferCount,
    const grfx::Buffer* const* buffers,
    const uint32_t*            pStrides,
    const uint64_t*            pOffsets)
{
    PPX_ASSERT_NULL_ARG(buffers);
    PPX_ASSERT_NULL_ARG(pStrides);
    PPX_ASSERT_MSG(bufferCount < PPX_MAX_VERTEX_BINDINGS, "bufferCount exceeds PPX_MAX_VERTEX_ATTRIBUTES");

    grfx::VertexBufferView views[PPX_MAX_VERTEX_BINDINGS] = {};
    for (uint32_t i = 0; i < bufferCount; ++i) {
        views[i].pBuffer = buffers[i];
        views[i].stride  = pStrides[i];
        if (!IsNull(pOffsets)) {
            views[i].offset = pOffsets[i];
        }
    }

    BindVertexBuffers(bufferCount, views);
}

void CommandBuffer::BindVertexBuffers(const grfx::Mesh* pMesh, const uint64_t* pOffsets)
{
    PPX_ASSERT_NULL_ARG(pMesh);

    const grfx::Buffer* buffers[PPX_MAX_VERTEX_BINDINGS] = {nullptr};
    uint32_t            strides[PPX_MAX_VERTEX_BINDINGS] = {0};

    uint32_t bufferCount = pMesh->GetVertexBufferCount();
    for (uint32_t i = 0; i < bufferCount; ++i) {
        buffers[i] = pMesh->GetVertexBuffer(i);
        strides[i] = pMesh->GetVertexBufferDescription(i)->stride;
    }

    BindVertexBuffers(bufferCount, buffers, strides, pOffsets);
}

void CommandBuffer::Draw(const grfx::FullscreenQuad* pQuad, uint32_t setCount, const grfx::DescriptorSet* const* ppSets)
{
    BindGraphicsDescriptorSets(pQuad->GetPipelineInterface(), setCount, ppSets);
    BindGraphicsPipeline(pQuad->GetPipeline());
    Draw(3, 1);
}

void CommandBuffer::PushGraphicsUniformBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,          // pipelineBindPoint
        pInterface,                           // pInterface
        grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        binding,                              // binding
        set,                                  // set
        bufferOffset,                         // bufferOffset
        pBuffer,                              // pBuffer
        nullptr,                              // pSampledImageView
        nullptr,                              // pStorageImageView
        nullptr);                             // pSampler
}

void CommandBuffer::PushGraphicsStructuredBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,                // pipelineBindPoint
        pInterface,                                 // pInterface
        grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, // descriptorType
        binding,                                    // binding
        set,                                        // set
        bufferOffset,                               // bufferOffset
        pBuffer,                                    // pBuffer
        nullptr,                                    // pSampledImageView
        nullptr,                                    // pStorageImageView
        nullptr);                                   // pSampler
}

void CommandBuffer::PushGraphicsStorageBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,                // pipelineBindPoint
        pInterface,                                 // pInterface
        grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, // descriptorType
        binding,                                    // binding
        set,                                        // set
        bufferOffset,                               // bufferOffset
        pBuffer,                                    // pBuffer
        nullptr,                                    // pSampledImageView
        nullptr,                                    // pStorageImageView
        nullptr);                                   // pSampler
}

void CommandBuffer::PushGraphicsSampledImage(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::SampledImageView*  pView)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,         // pipelineBindPoint
        pInterface,                          // pInterface
        grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, // descriptorType
        binding,                             // binding
        set,                                 // set
        0,                                   // bufferOffset
        nullptr,                             // pBuffer
        pView,                               // pSampledImageView
        nullptr,                             // pStorageImageView
        nullptr);                            // pSampler
}

void CommandBuffer::PushGraphicsStorageImage(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::StorageImageView*  pView)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,         // pipelineBindPoint
        pInterface,                          // pInterface
        grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, // descriptorType
        binding,                             // binding
        set,                                 // set
        0,                                   // bufferOffset
        nullptr,                             // pBuffer
        nullptr,                             // pSampledImageView
        pView,                               // pStorageImageView
        nullptr);                            // pSampler
}

void CommandBuffer::PushGraphicsSampler(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::Sampler*           pSampler)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_GRAPHICS,   // pipelineBindPoint
        pInterface,                    // pInterface
        grfx::DESCRIPTOR_TYPE_SAMPLER, // descriptorType
        binding,                       // binding
        set,                           // set
        0,                             // bufferOffset
        nullptr,                       // pBuffer
        nullptr,                       // pSampledImageView
        nullptr,                       // pStorageImageView
        pSampler);                     // pSampler
}

void CommandBuffer::PushComputeUniformBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,           // pipelineBindPoint
        pInterface,                           // pInterface
        grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        binding,                              // binding
        set,                                  // set
        bufferOffset,                         // bufferOffset
        pBuffer,                              // pBuffer
        nullptr,                              // pSampledImageView
        nullptr,                              // pStorageImageView
        nullptr);                             // pSampler
}

void CommandBuffer::PushComputeStructuredBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,                 // pipelineBindPoint
        pInterface,                                 // pInterface
        grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, // descriptorType
        binding,                                    // binding
        set,                                        // set
        bufferOffset,                               // bufferOffset
        pBuffer,                                    // pBuffer
        nullptr,                                    // pSampledImageView
        nullptr,                                    // pStorageImageView
        nullptr);                                   // pSampler
}

void CommandBuffer::PushComputeStorageBuffer(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,                 // pipelineBindPoint
        pInterface,                                 // pInterface
        grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, // descriptorType
        binding,                                    // binding
        set,                                        // set
        bufferOffset,                               // bufferOffset
        pBuffer,                                    // pBuffer
        nullptr,                                    // pSampledImageView
        nullptr,                                    // pStorageImageView
        nullptr);                                   // pSampler
}

void CommandBuffer::PushComputeSampledImage(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::SampledImageView*  pView)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,          // pipelineBindPoint
        pInterface,                          // pInterface
        grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, // descriptorType
        binding,                             // binding
        set,                                 // set
        0,                                   // bufferOffset
        nullptr,                             // pBuffer
        pView,                               // pSampledImageView
        nullptr,                             // pStorageImageView
        nullptr);                            // pSampler
}

void CommandBuffer::PushComputeStorageImage(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::StorageImageView*  pView)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,          // pipelineBindPoint
        pInterface,                          // pInterface
        grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, // descriptorType
        binding,                             // binding
        set,                                 // set
        0,                                   // bufferOffset
        nullptr,                             // pBuffer
        nullptr,                             // pSampledImageView
        pView,                               // pStorageImageView
        nullptr);                            // pSampler
}

void CommandBuffer::PushComputeSampler(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       binding,
    uint32_t                       set,
    const grfx::Sampler*           pSampler)
{
    PushDescriptorImpl(
        grfx::COMMAND_TYPE_COMPUTE,    // pipelineBindPoint
        pInterface,                    // pInterface
        grfx::DESCRIPTOR_TYPE_SAMPLER, // descriptorType
        binding,                       // binding
        set,                           // set
        0,                             // bufferOffset
        nullptr,                       // pBuffer
        nullptr,                       // pSampledImageView
        nullptr,                       // pStorageImageView
        pSampler);                     // pSampler
}

} // namespace grfx
} // namespace ppx
