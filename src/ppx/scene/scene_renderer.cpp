#include "ppx/scene/scene_renderer.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_swapchain.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// RendererOutput
// -------------------------------------------------------------------------------------------------
RenderOutput::RenderOutput(
    scene::Renderer* pRenderer)
    : mRenderer(pRenderer)
{
}

RenderOutput::~RenderOutput()
{
}

// -------------------------------------------------------------------------------------------------
// RendererOutputToImage
// -------------------------------------------------------------------------------------------------
RenderOutputToImage::RenderOutputToImage(
    scene::Renderer* pRenderer,
    grfx::Image*     pInitialImage)
    : scene::RenderOutput(pRenderer),
      mImage(pInitialImage)
{
}

RenderOutputToImage::~RenderOutputToImage()
{
}

ppx::Result RenderOutputToImage::Create(
    scene::Renderer*             pRenderer,
    grfx::Image*                 pInitialImage,
    scene::RenderOutputToImage** ppRendererOutput)
{
    if (IsNull(pRenderer) || IsNull(ppRendererOutput)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    auto pObject = new scene::RenderOutputToImage(pRenderer, pInitialImage);
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    *ppRendererOutput = pObject;

    return ppx::SUCCESS;
}

void RenderOutputToImage::Destroy(scene::RenderOutputToImage* pRendererOutput)
{
    if (IsNull(pRendererOutput)) {
        return;
    }

    delete pRendererOutput;
}

ppx::Result RenderOutputToImage::GetRenderTargetImage(
    grfx::Image**    ppImage,
    grfx::Semaphore* pImageReadySemaphore)
{
    (void)pImageReadySemaphore;

    if (IsNull(ppImage)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    *ppImage = mImage;

    return ppx::SUCCESS;
}

void RenderOutputToImage::SetImage(grfx::Image* pImage)
{
    mImage = pImage;
}

// -------------------------------------------------------------------------------------------------
// RendererOutputToSwapchain
// -------------------------------------------------------------------------------------------------
RenderOutputToSwapchain::RenderOutputToSwapchain(
    scene::Renderer* pRenderer,
    grfx::Swapchain* pInitialSwapchain)
    : scene::RenderOutput(pRenderer),
      mSwapchain(pInitialSwapchain)
{
}

RenderOutputToSwapchain::~RenderOutputToSwapchain()
{
}

ppx::Result RenderOutputToSwapchain::CreateObject()
{
    // Create fence for image acquisition
    {
        grfx::FenceCreateInfo createInfo = {};

        auto ppxres = GetRenderer()->GetDevice()->CreateFence(
            &createInfo,
            &mFence);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

void RenderOutputToSwapchain::DestroyObject()
{
    if (mFence) {
        GetRenderer()->GetDevice()->DestroyFence(mFence);
        mFence.Reset();
    }
}

ppx::Result RenderOutputToSwapchain::Create(
    scene::Renderer*                 pRenderer,
    grfx::Swapchain*                 pInitialSwapchain,
    scene::RenderOutputToSwapchain** ppRendererOutput)
{
    if (IsNull(pRenderer) || IsNull(ppRendererOutput)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    auto pRendererOutput = new scene::RenderOutputToSwapchain(
        pRenderer,
        pInitialSwapchain);
    if (IsNull(pRendererOutput)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    auto ppxres = pRendererOutput->CreateObject();
    if (Failed(ppxres)) {
        delete pRendererOutput;
        return ppxres;
    }

    *ppRendererOutput = pRendererOutput;

    return ppx::SUCCESS;
}

void RenderOutputToSwapchain::Destroy(scene::RenderOutputToSwapchain* pRendererOutput)
{
    if (IsNull(pRendererOutput)) {
        return;
    }

    pRendererOutput->DestroyObject();

    delete pRendererOutput;
}

ppx::Result RenderOutputToSwapchain::GetRenderTargetImage(
    grfx::Image**    ppImage,
    grfx::Semaphore* pImageReadySemaphore)
{
    if (IsNull(ppImage)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    auto ppxres = mSwapchain->AcquireNextImage(
        UINT64_MAX,
        pImageReadySemaphore,
        mFence,
        &mImageIndex);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = mFence->WaitAndReset();
    if (Failed(ppxres)) {
        return ppxres;
    }

    *ppImage = mSwapchain->GetColorImage(mImageIndex).Get();

    return ppx::SUCCESS;
}

void RenderOutputToSwapchain::SetSwapchain(grfx::Swapchain* pSwapchain)
{
    mSwapchain = pSwapchain;
}

// -------------------------------------------------------------------------------------------------
// Renderer
// -------------------------------------------------------------------------------------------------
Renderer::Renderer(
    grfx::Device* pDevice,
    uint32_t      numInFlightFrames)
    : mDevice(pDevice),
      mNumInFlightFrames(numInFlightFrames)
{
}

Renderer::~Renderer()
{
}

ppx::Result Renderer::GetRenderOutputRenderPass(
    grfx::Image*       pImage,
    grfx::RenderPass** ppRenderPass)
{
    if (IsNull(pImage) || IsNull(ppRenderPass)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    grfx::RenderPassPtr renderPass = nullptr;

    // Find a render pass for pImage, if there isn't one create it.
    auto it = mOutputRenderPasses.find(pImage);
    if (it != mOutputRenderPasses.end()) {
        renderPass = (*it).second;
    }
    else {
        auto ppxres = CreateOutputRenderPass(
            pImage,
            &renderPass);
        if (Failed(ppxres)) {
            return ppxres;
        }

        mOutputRenderPasses[pImage] = renderPass;
    }

    *ppRenderPass = renderPass.Get();

    return ppx::SUCCESS;
}

void Renderer::SetScene(scene::Scene* pScene)
{
    mScene = pScene;
}

ppx::Result Renderer::Render(
    scene::RenderOutput* pOutput,
    grfx::Semaphore*     pRenderCompleteSemaphore)
{
    if (IsNull(pOutput)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    auto ppxres = this->RenderInternal(
        pOutput,
        pRenderCompleteSemaphore);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result Renderer::CreateOutputRenderPass(
    grfx::Image*       pImage,
    grfx::RenderPass** ppRenderPass)
{
    if (IsNull(pImage) || IsNull(ppRenderPass)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    grfx::RenderPassCreateInfo3 createInfo = {};
    createInfo.width                       = pImage->GetWidth();
    createInfo.height                      = pImage->GetHeight();
    createInfo.renderTargetCount           = 1;
    createInfo.pRenderTargetImages[0]      = pImage;
    createInfo.renderTargetClearValues[0]  = grfx::RenderTargetClearValue{0, 0, 0, 0};
    createInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_LOAD;
    createInfo.renderTargetStoreOps[0]     = grfx::ATTACHMENT_STORE_OP_STORE;

    grfx::RenderPassPtr renderPass = nullptr;
    auto                ppxres     = GetDevice()->CreateRenderPass(&createInfo, &renderPass);
    if (Failed(ppxres)) {
        return ppxres;
    }

    *ppRenderPass = renderPass.Get();

    return ppx::SUCCESS;
}

} // namespace scene
} // namespace ppx
