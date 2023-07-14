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

#ifndef ppx_grfx_command_buffer_h
#define ppx_grfx_command_buffer_h

// *** Graphics API Note ***
//
// In the cosmos of game engines, there's more than one way to build
// command buffers and track various bits that company that.
//
// Smaller engines and graphics demo may favor command buffer reuse or
// at least reusing the same resources in a similar order per frame.
//
// Larger engines may have have an entire job system where available
// command buffers are use for the next immediate task. There may
// or may not be any affinity for command buffers and tasks.
//
// We're going to favor the second case - command buffers do not
// have affinity for tasks. This means that for D3D12 we'll copy
// descriptors from the set's CPU heaps to the command buffer's
// GPU visible heaps when the indGraphicsDescriptorSets or
// BindComputeDescriptorSets is called. This may not be the most
// efficient way to do this but it does give us the flexiblity
// to shape D3D12 to look like Vulkan.
//

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct BufferToBufferCopyInfo
//!
//!
struct BufferToBufferCopyInfo
{
    uint64_t size = 0;

    struct srcBuffer
    {
        uint64_t offset = 0;
    } srcBuffer;

    struct
    {
        uint32_t offset = 0;
    } dstBuffer;
};

//! @struct BufferToImageCopyInfo
//!
//!
struct BufferToImageCopyInfo
{
    struct
    {
        uint32_t imageWidth      = 0; // [pixels]
        uint32_t imageHeight     = 0; // [pixels]
        uint32_t imageRowStride  = 0; // [bytes]
        uint64_t footprintOffset = 0; // [bytes]
        uint32_t footprintWidth  = 0; // [pixels]
        uint32_t footprintHeight = 0; // [pixels]
        uint32_t footprintDepth  = 0; // [pixels]
    } srcBuffer;

    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
        uint32_t x               = 0; // [pixels]
        uint32_t y               = 0; // [pixels]
        uint32_t z               = 0; // [pixels]
        uint32_t width           = 0; // [pixels]
        uint32_t height          = 0; // [pixels]
        uint32_t depth           = 0; // [pixels]
    } dstImage;
};

//! @struct ImageToBufferCopyInfo
//!
//!
struct ImageToBufferCopyInfo
{
    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } srcImage;

    struct
    {
        uint32_t x = 0; // [pixels]
        uint32_t y = 0; // [pixels]
        uint32_t z = 0; // [pixels]
    } extent;
};

//! @struct ImageToBufferOutputPitch
//!
//!
struct ImageToBufferOutputPitch
{
    uint32_t rowPitch = 0;
};

//! @struct ImageToImageCopyInfo
//!
//!
struct ImageToImageCopyInfo
{
    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } srcImage;

    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } dstImage;

    struct
    {
        uint32_t x = 0; // [pixels]
        uint32_t y = 0; // [pixels]
        uint32_t z = 0; // [pixels]
    } extent;
};

// -------------------------------------------------------------------------------------------------

struct RenderPassBeginInfo
{
    //
    // The value of RTVClearCount cannot be less than the number
    // of RTVs in pRenderPass.
    //
    const grfx::RenderPass*      pRenderPass                            = nullptr;
    grfx::Rect                   renderArea                             = {};
    uint32_t                     RTVClearCount                          = 0;
    grfx::RenderTargetClearValue RTVClearValues[PPX_MAX_RENDER_TARGETS] = {0.0f, 0.0f, 0.0f, 0.0f};
    grfx::DepthStencilClearValue DSVClearValue                          = {1.0f, 0xFF};
};

// -------------------------------------------------------------------------------------------------

//! @struct CommandPoolCreateInfo
//!
//!
struct CommandPoolCreateInfo
{
    const grfx::Queue* pQueue = nullptr;
};

//! @class CommandPool
//!
//!
class CommandPool
    : public grfx::DeviceObject<grfx::CommandPoolCreateInfo>
{
public:
    CommandPool() {}
    virtual ~CommandPool() {}

    grfx::CommandType GetCommandType() const;
};

// -------------------------------------------------------------------------------------------------

namespace internal {

//! @struct CommandBufferCreateInfo
//!
//! For D3D12 every command buffer will have two GPU visible descriptor heaps:
//!   - one for CBVSRVUAV descriptors
//!   - one for Sampler descriptors
//!
//! Both of heaps are set when the command buffer begins.
//!
//! Each time that BindGraphicsDescriptorSets or BindComputeDescriptorSets
//! is called, the contents of each descriptor set's CBVSRVAUV and Sampler heaps
//! will be copied into the command buffer's respective heap.
//!
//! The offsets from used in the copies will be saved and used to set the
//! root descriptor tables.
//!
//! 'resourceDescriptorCount' and 'samplerDescriptorCount' tells the
//! D3D12 command buffer how large CBVSRVUAV and Sampler heaps should be.
//!
//! 'samplerDescriptorCount' cannot exceed PPX_MAX_SAMPLER_DESCRIPTORS.
//!
//! Vulkan does not use 'samplerDescriptorCount' or 'samplerDescriptorCount'.
//!
struct CommandBufferCreateInfo
{
    const grfx::CommandPool* pPool                   = nullptr;
    uint32_t                 resourceDescriptorCount = PPX_DEFAULT_RESOURCE_DESCRIPTOR_COUNT;
    uint32_t                 samplerDescriptorCount  = PPX_DEFAULT_SAMPLE_DESCRIPTOR_COUNT;
};

} // namespace internal

//! @class CommandBuffer
//!
//!
class CommandBuffer
    : public grfx::DeviceObject<grfx::internal::CommandBufferCreateInfo>
{
public:
    CommandBuffer() {}
    virtual ~CommandBuffer() {}

    virtual Result Begin() = 0;
    virtual Result End()   = 0;

    void BeginRenderPass(const grfx::RenderPassBeginInfo* pBeginInfo);
    void EndRenderPass();

    grfx::CommandType GetCommandType() { return mCreateInfo.pPool->GetCommandType(); }

    //! @fn TransitionImageLayout
    //!
    //! Vulkan requires a queue ownership transfer if a resource
    //! is used by queues in different queue families:
    //!  - Use \b pSrcQueue to specify a queue in the source queue family
    //!  - Use \b pDstQueue to specify a queue in the destination queue family
    //!  - If \b pSrcQueue and \b pDstQueue belong to the same queue family
    //!    then the queue ownership transfer won't happen.
    //!
    //! D3D12 ignores both \b pSrcQueue and \b pDstQueue since they're not
    //! relevant.
    //!
    virtual void TransitionImageLayout(
        const grfx::Image*  pImage,
        uint32_t            mipLevel,
        uint32_t            mipLevelCount,
        uint32_t            arrayLayer,
        uint32_t            arrayLayerCount,
        grfx::ResourceState beforeState,
        grfx::ResourceState afterState,
        const grfx::Queue*  pSrcQueue = nullptr,
        const grfx::Queue*  pDstQueue = nullptr) = 0;

    //
    // See comment at function \b TransitionImageLayout for details
    // on queue ownership transfer.
    //
    virtual void BufferResourceBarrier(
        const grfx::Buffer* pBuffer,
        grfx::ResourceState beforeState,
        grfx::ResourceState afterState,
        const grfx::Queue*  pSrcQueue = nullptr,
        const grfx::Queue*  pDstQueue = nullptr) = 0;

    virtual void SetViewports(
        uint32_t              viewportCount,
        const grfx::Viewport* pViewports) = 0;

    virtual void SetScissors(
        uint32_t          scissorCount,
        const grfx::Rect* pScissors) = 0;

    virtual void BindGraphicsDescriptorSets(
        const grfx::PipelineInterface*    pInterface,
        uint32_t                          setCount,
        const grfx::DescriptorSet* const* ppSets) = 0;

    //
    // Parameters count and dstOffset are measured in DWORDs (uint32_t) aka 32-bit values.
    // To set the first 4 32-bit values, use count = 4, dstOffset = 0.
    // To set the 16 DWORDs starting at offset 8, use count=16, dstOffset = 8.
    //
    // VK: pValues is subjected to Vulkan packing rules. BigWheels compiles HLSL
    //     shaders with -fvk-use-dx-layout on. This makes the packing rules match
    //     that of D3D12. However, if a shader is compiled without that flag or
    //     with a different compiler or source language. The contents pointed to
    //     by pValues must respect the packing rules in effect.
    //
    virtual void PushGraphicsConstants(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       count,
        const void*                    pValues,
        uint32_t                       dstOffset = 0) = 0;

    virtual void PushGraphicsUniformBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushGraphicsStructuredBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushGraphicsStorageBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushGraphicsSampledImage(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::SampledImageView*  pView);

    virtual void PushGraphicsStorageImage(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::StorageImageView*  pView);

    virtual void PushGraphicsSampler(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::Sampler*           pSampler);

    virtual void BindGraphicsPipeline(const grfx::GraphicsPipeline* pPipeline) = 0;

    virtual void BindComputeDescriptorSets(
        const grfx::PipelineInterface*    pInterface,
        uint32_t                          setCount,
        const grfx::DescriptorSet* const* ppSets) = 0;

    // See comments at SetGraphicsPushConstants for explanation about count, pValues and dstOffset.
    virtual void PushComputeConstants(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       count,
        const void*                    pValues,
        uint32_t                       dstOffset = 0) = 0;

    virtual void PushComputeUniformBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushComputeStructuredBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushComputeStorageBuffer(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer);

    virtual void PushComputeSampledImage(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::SampledImageView*  pView);

    virtual void PushComputeStorageImage(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::StorageImageView*  pView);

    virtual void PushComputeSampler(
        const grfx::PipelineInterface* pInterface,
        uint32_t                       binding,
        uint32_t                       set,
        const grfx::Sampler*           pSampler);

    virtual void BindComputePipeline(const grfx::ComputePipeline* pPipeline) = 0;

    virtual void BindIndexBuffer(const grfx::IndexBufferView* pView) = 0;

    virtual void BindVertexBuffers(
        uint32_t                      viewCount,
        const grfx::VertexBufferView* pViews) = 0;

    virtual void Draw(
        uint32_t vertexCount,
        uint32_t instanceCount = 1,
        uint32_t firstVertex   = 0,
        uint32_t firstInstance = 0) = 0;

    virtual void DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount = 1,
        uint32_t firstIndex    = 0,
        int32_t  vertexOffset  = 0,
        uint32_t firstInstance = 0) = 0;

    virtual void Dispatch(
        uint32_t groupCountX,
        uint32_t groupCountY,
        uint32_t groupCountZ) = 0;

    virtual void CopyBufferToBuffer(
        const grfx::BufferToBufferCopyInfo* pCopyInfo,
        grfx::Buffer*                       pSrcBuffer,
        grfx::Buffer*                       pDstBuffer) = 0;

    virtual void CopyBufferToImage(
        const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
        grfx::Buffer*                                   pSrcBuffer,
        grfx::Image*                                    pDstImage) = 0;

    virtual void CopyBufferToImage(
        const grfx::BufferToImageCopyInfo* pCopyInfo,
        grfx::Buffer*                      pSrcBuffer,
        grfx::Image*                       pDstImage) = 0;

    //! @brief Copies an image to a buffer.
    //! @param pCopyInfo The specifications of the image region to copy.
    //! @param pSrcImage The source image.
    //! @param pDstBuffer The destination buffer.
    //! @return The image row pitch as written to the destination buffer.
    virtual grfx::ImageToBufferOutputPitch CopyImageToBuffer(
        const grfx::ImageToBufferCopyInfo* pCopyInfo,
        grfx::Image*                       pSrcImage,
        grfx::Buffer*                      pDstBuffer) = 0;

    virtual void CopyImageToImage(
        const grfx::ImageToImageCopyInfo* pCopyInfo,
        grfx::Image*                      pSrcImage,
        grfx::Image*                      pDstImage) = 0;

    virtual void BeginQuery(
        const grfx::Query* pQuery,
        uint32_t           queryIndex) = 0;

    virtual void EndQuery(
        const grfx::Query* pQuery,
        uint32_t           queryIndex) = 0;

    virtual void WriteTimestamp(
        const grfx::Query*  pQuery,
        grfx::PipelineStage pipelineStage,
        uint32_t            queryIndex) = 0;

    virtual void ResolveQueryData(
        grfx::Query* pQuery,
        uint32_t     startIndex,
        uint32_t     numQueries) = 0;

    // ---------------------------------------------------------------------------------------------
    // Convenience functions
    // ---------------------------------------------------------------------------------------------
    void BeginRenderPass(const grfx::RenderPass* pRenderPass);

    void BeginRenderPass(
        const grfx::DrawPass*           pDrawPass,
        const grfx::DrawPassClearFlags& clearFlags = grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

    virtual void TransitionImageLayout(
        const grfx::Texture* pTexture,
        uint32_t             mipLevel,
        uint32_t             mipLevelCount,
        uint32_t             arrayLayer,
        uint32_t             arrayLayerCount,
        grfx::ResourceState  beforeState,
        grfx::ResourceState  afterState,
        const grfx::Queue*   pSrcQueue = nullptr,
        const grfx::Queue*   pDstQueue = nullptr);

    void TransitionImageLayout(
        grfx::RenderPass*   pRenderPass,
        grfx::ResourceState renderTargetBeforeState,
        grfx::ResourceState renderTargetAfterState,
        grfx::ResourceState depthStencilTargetBeforeState,
        grfx::ResourceState depthStencilTargetAfterState);

    void TransitionImageLayout(
        grfx::DrawPass*     pDrawPass,
        grfx::ResourceState renderTargetBeforeState,
        grfx::ResourceState renderTargetAfterState,
        grfx::ResourceState depthStencilTargetBeforeState,
        grfx::ResourceState depthStencilTargetAfterState);

    void SetViewports(const grfx::Viewport& viewport);

    void SetScissors(const grfx::Rect& scissor);

    void BindIndexBuffer(const grfx::Buffer* pBuffer, grfx::IndexType indexType, uint64_t offset = 0);

    void BindIndexBuffer(const grfx::Mesh* pMesh, uint64_t offset = 0);

    void BindVertexBuffers(
        uint32_t                   bufferCount,
        const grfx::Buffer* const* buffers,
        const uint32_t*            pStrides,
        const uint64_t*            pOffsets = nullptr);

    void BindVertexBuffers(const grfx::Mesh* pMesh, const uint64_t* pOffsets = nullptr);

    //
    // NOTE: If you're running into an issue where VS2019 is incorrectly
    //       resolving call to this function to the Draw(vertexCount, ...)
    //       function above - it might be that the last parameter isn't
    //       explicitly the double pointer type. Possibly due to some casting.
    //
    //       For example this might give VS2019 some grief:
    //          grfx::DescriptorSetPtr set;
    //          Draw(quad, 1, set);
    //
    //       Use this instead:
    //          grfx::DescriptorSetPtr set;
    //          Draw(quad, 1, &set);
    //
    void Draw(const grfx::FullscreenQuad* pQuad, uint32_t setCount, const grfx::DescriptorSet* const* ppSets);

private:
    virtual void BeginRenderPassImpl(const grfx::RenderPassBeginInfo* pBeginInfo) = 0;
    virtual void EndRenderPassImpl()                                              = 0;

    virtual void PushDescriptorImpl(
        grfx::CommandType              pipelineBindPoint,
        const grfx::PipelineInterface* pInterface,
        grfx::DescriptorType           descriptorType,
        uint32_t                       binding,
        uint32_t                       set,
        uint32_t                       bufferOffset,
        const grfx::Buffer*            pBuffer,
        const grfx::SampledImageView*  pSampledImageView,
        const grfx::StorageImageView*  pStorageImageView,
        const grfx::Sampler*           pSampler) = 0;

    const grfx::RenderPass* mCurrentRenderPass = nullptr;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_command_buffer_h
