// Copyright 2023 Google LLC
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

#ifndef ppx_scene_renderer_h
#define ppx_scene_renderer_h

#include "ppx/scene/scene_config.h"

namespace ppx {
namespace scene {

struct GraphicsPipeline
{
    std::string                  idString            = "";
    grfx::DescriptorSetLayoutPtr descriptorSetLayout = nullptr;
    grfx::PipelineInterfacePtr   pipelineInterface   = nullptr;
    grfx::GraphicsPipelinePtr    pipeline            = nullptr;
};

// -------------------------------------------------------------------------------------------------

struct ComputePipeline
{
    std::string                  idString            = "";
    grfx::DescriptorSetLayoutPtr descriptorSetLayout = nullptr;
    grfx::PipelineInterfacePtr   pipelineInterface   = nullptr;
    grfx::GraphicsPipelinePtr    pipeline            = nullptr;
};

// -------------------------------------------------------------------------------------------------

struct RenderTargetAttachment
{
    grfx::ImagePtr               image            = nullptr;
    grfx::RenderTargetViewPtr    renderTargetView = nullptr;
    grfx::SampledImageViewPtr    sampledImageView = nullptr;
    grfx::RenderTargetClearValue clearValue       = {};
};

// -------------------------------------------------------------------------------------------------

struct DepthStencilAttachment
{
    grfx::ImagePtr               image            = nullptr;
    grfx::DepthStencilViewPtr    dephtStencilView = nullptr;
    grfx::SampledImageViewPtr    sampledImageView = nullptr;
    grfx::DepthStencilClearValue clearValue       = {};
};

// -------------------------------------------------------------------------------------------------

struct RenderPass
{
    std::string                                name                    = "";
    std::vector<scene::RenderTargetAttachment> renderTargetAttachments = {};
    scene::DepthStencilAttachment              depthStencilAttachment  = {};
    grfx::RenderPassPtr                        renderPass              = nullptr;
};

// -------------------------------------------------------------------------------------------------

struct ComputePass
{
    const scene::GraphicsPipeline*       pPipeline      = nullptr;
    std::vector<grfx::BufferPtr>         inputBuffers   = {};
    std::vector<grfx::SampledImageView*> inputTextures  = {};
    std::vector<grfx::BufferPtr>         outputBuffers  = {};
    std::vector<grfx::StorageImageView*> outputTextures = {};
};

// -------------------------------------------------------------------------------------------------

class RenderOutput
{
protected:
    RenderOutput(
        scene::Renderer* pRenderer);

public:
    virtual ~RenderOutput();

    scene::Renderer* GetRenderer() const { return mRenderer; }

    virtual ppx::Result GetRenderTargetImage(
        grfx::Image**    ppImage,
        grfx::Semaphore* pImageReadySemaphore) = 0;

    virtual bool IsSwapchain() const { return false; }

private:
    scene::Renderer* mRenderer = nullptr;
};

// -------------------------------------------------------------------------------------------------

class RenderOutputToImage
    : public scene::RenderOutput
{
protected:
    RenderOutputToImage(
        scene::Renderer* pRenderer,
        grfx::Image*     pInitialImage = nullptr);

public:
    virtual ~RenderOutputToImage();

    static ppx::Result Create(
        scene::Renderer*             pRenderer,
        grfx::Image*                 pInitialImage, // Can be NULL
        scene::RenderOutputToImage** ppRendererOutput);

    static void Destroy(scene::RenderOutputToImage* pRendererOutput);

    virtual ppx::Result GetRenderTargetImage(
        grfx::Image**    ppImage,
        grfx::Semaphore* pImageReadySemaphore) override;

    void SetImage(grfx::Image* pImage);

private:
    grfx::Image* mImage = nullptr;
};

// -------------------------------------------------------------------------------------------------

class RenderOutputToSwapchain
    : public scene::RenderOutput
{
protected:
    RenderOutputToSwapchain(
        scene::Renderer* pRenderer,
        grfx::Swapchain* pInitialSwapchain);

public:
    virtual ~RenderOutputToSwapchain();

    static ppx::Result Create(
        scene::Renderer*                 pRenderer,
        grfx::Swapchain*                 pInitialSwapchain, // Can be NULL
        scene::RenderOutputToSwapchain** ppRendererOutput);

    static void Destroy(scene::RenderOutputToSwapchain* pRendererOutput);

    virtual ppx::Result GetRenderTargetImage(
        grfx::Image**    ppImage,
        grfx::Semaphore* pImageReadySemaphore) override;

    virtual bool IsSwapchain() const override { return true; }

    void SetSwapchain(grfx::Swapchain* pSwapchain);

private:
    ppx::Result CreateObject();
    void        DestroyObject();

private:
    grfx::Swapchain* mSwapchain  = nullptr;
    grfx::FencePtr   mFence      = nullptr;
    uint32_t         mImageIndex = 0;
};

// -------------------------------------------------------------------------------------------------

class Renderer
{
protected:
    Renderer(
        grfx::Device* pDevice,
        uint32_t      numInFlightFrames);

public:
    virtual ~Renderer();

    grfx::Device* GetDevice() const { return mDevice; }

    uint32_t      GetNumInFlightFrames() const { return mNumInFlightFrames; }
    scene::Scene* GetScene() const { return mScene; }

    void SetScene(scene::Scene* pScene);

    ppx::Result Render(
        scene::RenderOutput* pOutput,
        grfx::Semaphore*     pRenderCompleteSemaphore);

protected:
    ppx::Result GetRenderOutputRenderPass(
        grfx::Image*       pImage,
        grfx::RenderPass** ppRenderPass);

private:
    virtual ppx::Result RenderInternal(
        scene::RenderOutput* pOutput,
        grfx::Semaphore*     pRenderCompleteSemaphore) = 0;

    // Create render pass with 1 render target using pImage.
    // OVerride this method in derived renderers to customize output render passes.
    //
    virtual ppx::Result CreateOutputRenderPass(
        grfx::Image*       pImage,
        grfx::RenderPass** ppRenderPass);

protected:
    grfx::Device*                                         mDevice             = nullptr;
    uint32_t                                              mNumInFlightFrames  = 0;
    uint32_t                                              mNumFramesRendered  = 0;
    uint32_t                                              mCurrentFrameIndex  = 0;
    uint2                                                 mRenderResolution   = uint2(0, 0);
    std::vector<scene::GraphicsPipeline>                  mGraphicsPipelines  = {};
    std::vector<scene::GraphicsPipeline>                  mComputePipelines   = {};
    std::unordered_map<grfx::Image*, grfx::RenderPassPtr> mOutputRenderPasses = {};
    bool                                                  mEnableDepthPrePass = false;
    scene::Scene*                                         mScene              = nullptr;
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_renderer_h
