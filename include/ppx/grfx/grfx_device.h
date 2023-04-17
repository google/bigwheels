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

#ifndef ppx_grfx_device_h
#define ppx_grfx_device_h

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_buffer.h"
#include "ppx/grfx/grfx_command.h"
#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_draw_pass.h"
#include "ppx/grfx/grfx_fullscreen_quad.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_mesh.h"
#include "ppx/grfx/grfx_pipeline.h"
#include "ppx/grfx/grfx_queue.h"
#include "ppx/grfx/grfx_query.h"
#include "ppx/grfx/grfx_render_pass.h"
#include "ppx/grfx/grfx_shader.h"
#include "ppx/grfx/grfx_swapchain.h"
#include "ppx/grfx/grfx_sync.h"
#include "ppx/grfx/grfx_text_draw.h"
#include "ppx/grfx/grfx_texture.h"

namespace ppx {
namespace grfx {

//! @struct DeviceCreateInfo
//!
//!
struct DeviceCreateInfo
{
    grfx::Gpu*               pGpu                  = nullptr;
    uint32_t                 graphicsQueueCount    = 0;
    uint32_t                 computeQueueCount     = 0;
    uint32_t                 transferQueueCount    = 0;
    std::vector<std::string> vulkanExtensions      = {};      // [OPTIONAL] Additional device extensions
    const void*              pVulkanDeviceFeatures = nullptr; // [OPTIONAL] Pointer to custom VkPhysicalDeviceFeatures
#if defined(PPX_BUILD_XR)
    XrComponent* pXrComponent = nullptr;
#endif
};

//! @class Device
//!
//!
class Device
    : public grfx::InstanceObject<grfx::DeviceCreateInfo>
{
public:
    Device() {}
    virtual ~Device() {}

    bool isDebugEnabled() const;

    grfx::Api GetApi() const;

    grfx::GpuPtr GetGpu() const { return mCreateInfo.pGpu; }

    const char*    GetDeviceName() const;
    grfx::VendorId GetDeviceVendorId() const;

    Result CreateBuffer(const grfx::BufferCreateInfo* pCreateInfo, grfx::Buffer** ppBuffer);
    void   DestroyBuffer(const grfx::Buffer* pBuffer);

    Result CreateCommandPool(const grfx::CommandPoolCreateInfo* pCreateInfo, grfx::CommandPool** ppCommandPool);
    void   DestroyCommandPool(const grfx::CommandPool* pCommandPool);

    Result CreateComputePipeline(const grfx::ComputePipelineCreateInfo* pCreateInfo, grfx::ComputePipeline** ppComputePipeline);
    void   DestroyComputePipeline(const grfx::ComputePipeline* pComputePipeline);

    Result CreateDepthStencilView(const grfx::DepthStencilViewCreateInfo* pCreateInfo, grfx::DepthStencilView** ppDepthStencilView);
    void   DestroyDepthStencilView(const grfx::DepthStencilView* pDepthStencilView);

    Result CreateDescriptorPool(const grfx::DescriptorPoolCreateInfo* pCreateInfo, grfx::DescriptorPool** ppDescriptorPool);
    void   DestroyDescriptorPool(const grfx::DescriptorPool* pDescriptorPool);

    Result CreateDescriptorSetLayout(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo, grfx::DescriptorSetLayout** ppDescriptorSetLayout);
    void   DestroyDescriptorSetLayout(const grfx::DescriptorSetLayout* pDescriptorSetLayout);

    Result CreateDrawPass(const grfx::DrawPassCreateInfo* pCreateInfo, grfx::DrawPass** ppDrawPass);
    Result CreateDrawPass(const grfx::DrawPassCreateInfo2* pCreateInfo, grfx::DrawPass** ppDrawPass);
    Result CreateDrawPass(const grfx::DrawPassCreateInfo3* pCreateInfo, grfx::DrawPass** ppDrawPass);
    void   DestroyDrawPass(const grfx::DrawPass* pDrawPass);

    Result CreateFence(const grfx::FenceCreateInfo* pCreateInfo, grfx::Fence** ppFence);
    void   DestroyFence(const grfx::Fence* pFence);

    Result CreateFullscreenQuad(const grfx::FullscreenQuadCreateInfo* pCreateInfo, grfx::FullscreenQuad** ppFullscreenQuad);
    void   DestroyFullscreenQuad(const grfx::FullscreenQuad* pFullscreenQuad);

    Result CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline);
    Result CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo2* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline);
    void   DestroyGraphicsPipeline(const grfx::GraphicsPipeline* pGraphicsPipeline);

    Result CreateImage(const grfx::ImageCreateInfo* pCreateInfo, grfx::Image** ppImage);
    void   DestroyImage(const grfx::Image* pImage);

    Result CreateMesh(const grfx::MeshCreateInfo* pCreateInfo, grfx::Mesh** ppMesh);
    void   DestroyMesh(const grfx::Mesh* pMesh);

    Result CreatePipelineInterface(const grfx::PipelineInterfaceCreateInfo* pCreateInfo, grfx::PipelineInterface** ppPipelineInterface);
    void   DestroyPipelineInterface(const grfx::PipelineInterface* pPipelineInterface);

    Result CreateQuery(const grfx::QueryCreateInfo* pCreateInfo, grfx::Query** ppQuery);
    void   DestroyQuery(const grfx::Query* pQuery);

    Result CreateRenderPass(const grfx::RenderPassCreateInfo* pCreateInfo, grfx::RenderPass** ppRenderPass);
    Result CreateRenderPass(const grfx::RenderPassCreateInfo2* pCreateInfo, grfx::RenderPass** ppRenderPass);
    Result CreateRenderPass(const grfx::RenderPassCreateInfo3* pCreateInfo, grfx::RenderPass** ppRenderPass);
    void   DestroyRenderPass(const grfx::RenderPass* pRenderPass);

    Result CreateRenderTargetView(const grfx::RenderTargetViewCreateInfo* pCreateInfo, grfx::RenderTargetView** ppRenderTargetView);
    void   DestroyRenderTargetView(const grfx::RenderTargetView* pRenderTargetView);

    Result CreateSampledImageView(const grfx::SampledImageViewCreateInfo* pCreateInfo, grfx::SampledImageView** ppSampledImageView);
    void   DestroySampledImageView(const grfx::SampledImageView* pSampledImageView);

    Result CreateSampler(const grfx::SamplerCreateInfo* pCreateInfo, grfx::Sampler** ppSampler);
    void   DestroySampler(const grfx::Sampler* pSampler);

    Result CreateSemaphore(const grfx::SemaphoreCreateInfo* pCreateInfo, grfx::Semaphore** ppSemaphore);
    void   DestroySemaphore(const grfx::Semaphore* pSemaphore);

    Result CreateShaderModule(const grfx::ShaderModuleCreateInfo* pCreateInfo, grfx::ShaderModule** ppShaderModule);
    void   DestroyShaderModule(const grfx::ShaderModule* pShaderModule);

    Result CreateStorageImageView(const grfx::StorageImageViewCreateInfo* pCreateInfo, grfx::StorageImageView** ppStorageImageView);
    void   DestroyStorageImageView(const grfx::StorageImageView* pStorageImageView);

    Result CreateSwapchain(const grfx::SwapchainCreateInfo* pCreateInfo, grfx::Swapchain** ppSwapchain);
    void   DestroySwapchain(const grfx::Swapchain* pSwapchain);

    Result CreateTextDraw(const grfx::TextDrawCreateInfo* pCreateInfo, grfx::TextDraw** ppTextDraw);
    void   DestroyTextDraw(const grfx::TextDraw* pTextDraw);

    Result CreateTexture(const grfx::TextureCreateInfo* pCreateInfo, grfx::Texture** ppTexture);
    void   DestroyTexture(const grfx::Texture* pTexture);

    Result CreateTextureFont(const grfx::TextureFontCreateInfo* pCreateInfo, grfx::TextureFont** ppTextureFont);
    void   DestroyTextureFont(const grfx::TextureFont* pTextureFont);

    // See comment section for grfx::internal::CommandBufferCreateInfo for
    // details about 'resourceDescriptorCount' and 'samplerDescriptorCount'.
    //
    Result AllocateCommandBuffer(
        const grfx::CommandPool* pPool,
        grfx::CommandBuffer**    ppCommandBuffer,
        uint32_t                 resourceDescriptorCount = PPX_DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
        uint32_t                 samplerDescriptorCount  = PPX_DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
    void FreeCommandBuffer(const grfx::CommandBuffer* pCommandBuffer);

    Result AllocateDescriptorSet(grfx::DescriptorPool* pPool, const grfx::DescriptorSetLayout* pLayout, grfx::DescriptorSet** ppSet);
    void   FreeDescriptorSet(const grfx::DescriptorSet* pSet);

    uint32_t       GetGraphicsQueueCount() const;
    Result         GetGraphicsQueue(uint32_t index, grfx::Queue** ppQueue) const;
    grfx::QueuePtr GetGraphicsQueue(uint32_t index = 0) const;

    uint32_t       GetComputeQueueCount() const;
    Result         GetComputeQueue(uint32_t index, grfx::Queue** ppQueue) const;
    grfx::QueuePtr GetComputeQueue(uint32_t index = 0) const;

    uint32_t       GetTransferQueueCount() const;
    Result         GetTransferQueue(uint32_t index, grfx::Queue** ppQueue) const;
    grfx::QueuePtr GetTransferQueue(uint32_t index = 0) const;

    grfx::QueuePtr GetAnyAvailableQueue() const;

    virtual Result WaitIdle() = 0;

    virtual bool PipelineStatsAvailable() const    = 0;
    virtual bool DynamicRenderingSupported() const = 0;
    virtual bool IndependentBlendingSupported() const = 0;
    virtual bool FragmentStoresAndAtomicsSupported() const = 0;

protected:
    virtual Result Create(const grfx::DeviceCreateInfo* pCreateInfo) override;
    virtual void   Destroy() override;
    friend class grfx::Instance;

    virtual Result AllocateObject(grfx::Buffer** ppObject)              = 0;
    virtual Result AllocateObject(grfx::CommandBuffer** ppObject)       = 0;
    virtual Result AllocateObject(grfx::CommandPool** ppObject)         = 0;
    virtual Result AllocateObject(grfx::ComputePipeline** ppObject)     = 0;
    virtual Result AllocateObject(grfx::DepthStencilView** ppObject)    = 0;
    virtual Result AllocateObject(grfx::DescriptorPool** ppObject)      = 0;
    virtual Result AllocateObject(grfx::DescriptorSet** ppObject)       = 0;
    virtual Result AllocateObject(grfx::DescriptorSetLayout** ppObject) = 0;
    virtual Result AllocateObject(grfx::Fence** ppObject)               = 0;
    virtual Result AllocateObject(grfx::GraphicsPipeline** ppObject)    = 0;
    virtual Result AllocateObject(grfx::Image** ppObject)               = 0;
    virtual Result AllocateObject(grfx::PipelineInterface** ppObject)   = 0;
    virtual Result AllocateObject(grfx::Queue** ppObject)               = 0;
    virtual Result AllocateObject(grfx::Query** ppObject)               = 0;
    virtual Result AllocateObject(grfx::RenderPass** ppObject)          = 0;
    virtual Result AllocateObject(grfx::RenderTargetView** ppObject)    = 0;
    virtual Result AllocateObject(grfx::SampledImageView** ppObject)    = 0;
    virtual Result AllocateObject(grfx::Sampler** ppObject)             = 0;
    virtual Result AllocateObject(grfx::Semaphore** ppObject)           = 0;
    virtual Result AllocateObject(grfx::ShaderModule** ppObject)        = 0;
    virtual Result AllocateObject(grfx::ShaderProgram** ppObject)       = 0;
    virtual Result AllocateObject(grfx::StorageImageView** ppObject)    = 0;
    virtual Result AllocateObject(grfx::Swapchain** ppObject)           = 0;

    virtual Result AllocateObject(grfx::DrawPass** ppObject);
    virtual Result AllocateObject(grfx::FullscreenQuad** ppObject);
    virtual Result AllocateObject(grfx::Mesh** ppObject);
    virtual Result AllocateObject(grfx::TextDraw** ppObject);
    virtual Result AllocateObject(grfx::Texture** ppObject);
    virtual Result AllocateObject(grfx::TextureFont** ppObject);

    template <
        typename ObjectT,
        typename CreateInfoT,
        typename ContainerT = std::vector<ObjPtr<ObjectT>>>
    Result CreateObject(const CreateInfoT* pCreateInfo, ContainerT& container, ObjectT** ppObject);

    template <
        typename ObjectT,
        typename ContainerT = std::vector<ObjPtr<ObjectT>>>
    void DestroyObject(ContainerT& container, const ObjectT* pObject);

    template <typename ObjectT>
    void DestroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

    Result CreateGraphicsQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);
    Result CreateComputeQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);
    Result CreateTransferQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);

protected:
    grfx::InstancePtr                         mInstance;
    std::vector<grfx::BufferPtr>              mBuffers;
    std::vector<grfx::CommandBufferPtr>       mCommandBuffers;
    std::vector<grfx::CommandPoolPtr>         mCommandPools;
    std::vector<grfx::ComputePipelinePtr>     mComputePipelines;
    std::vector<grfx::DepthStencilViewPtr>    mDepthStencilViews;
    std::vector<grfx::DescriptorPoolPtr>      mDescriptorPools;
    std::vector<grfx::DescriptorSetPtr>       mDescriptorSets;
    std::vector<grfx::DescriptorSetLayoutPtr> mDescriptorSetLayouts;
    std::vector<grfx::DrawPassPtr>            mDrawPasses;
    std::vector<grfx::FencePtr>               mFences;
    std::vector<grfx::FullscreenQuadPtr>      mFullscreenQuads;
    std::vector<grfx::GraphicsPipelinePtr>    mGraphicsPipelines;
    std::vector<grfx::ImagePtr>               mImages;
    std::vector<grfx::MeshPtr>                mMeshes;
    std::vector<grfx::PipelineInterfacePtr>   mPipelineInterfaces;
    std::vector<grfx::QueryPtr>               mQuerys;
    std::vector<grfx::RenderPassPtr>          mRenderPasses;
    std::vector<grfx::RenderTargetViewPtr>    mRenderTargetViews;
    std::vector<grfx::SampledImageViewPtr>    mSampledImageViews;
    std::vector<grfx::SamplerPtr>             mSamplers;
    std::vector<grfx::SemaphorePtr>           mSemaphores;
    std::vector<grfx::ShaderModulePtr>        mShaderModules;
    std::vector<grfx::ShaderProgramPtr>       mShaderPrograms;
    std::vector<grfx::StorageImageViewPtr>    mStorageImageViews;
    std::vector<grfx::SwapchainPtr>           mSwapchains;
    std::vector<grfx::TextDrawPtr>            mTextDraws;
    std::vector<grfx::TexturePtr>             mTextures;
    std::vector<grfx::TextureFontPtr>         mTextureFonts;
    std::vector<grfx::QueuePtr>               mGraphicsQueues;
    std::vector<grfx::QueuePtr>               mComputeQueues;
    std::vector<grfx::QueuePtr>               mTransferQueues;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_device_h
