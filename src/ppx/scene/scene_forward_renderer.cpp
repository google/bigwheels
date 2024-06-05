#include "ppx/scene/scene_forward_renderer.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// ForwardRenderer
// -------------------------------------------------------------------------------------------------
ForwardRenderer::ForwardRenderer(
    grfx::Device* pDevice,
    uint32_t      numInFlightFrames)
    : scene::Renderer(pDevice, numInFlightFrames)
{
}

ForwardRenderer::~ForwardRenderer()
{
    this->DestroyObjects();
}

ppx::Result ForwardRenderer::CreateObjects()
{
    for (uint32_t i = 0; i < GetNumInFlightFrames(); ++i) {
        Frame frame = {};

        PPX_CHECKED_CALL(GetDevice()->GetGraphicsQueue()->CreateCommandBuffer(&frame.renderOutputCmd));

        /*
                grfx::SemaphoreCreateInfo semaCreateInfo = {};
                PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

                grfx::FenceCreateInfo fenceCreateInfo = {true}; // Create signaled
                PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));
        */
        mFrames.push_back(frame);
    }

    return ppx::SUCCESS;
}

void ForwardRenderer::DestroyObjects()
{
    for (auto& frame : mFrames) {
        GetDevice()->GetGraphicsQueue()->DestroyCommandBuffer(frame.renderOutputCmd);
        // GetDevice()->DestroySemaphore(frame.imageAcquiredSemaphore);
        // GetDevice()->DestroyFence(frame.renderCompleteFence);
    }
}

ppx::Result ForwardRenderer::Create(grfx::Device* pDevice, uint32_t numInFlightFrames, scene::Renderer** ppRenderer)
{
    if (IsNull(ppRenderer)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    scene::ForwardRenderer* pRenderer = new scene::ForwardRenderer(pDevice, numInFlightFrames);
    if (IsNull(pRenderer)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    auto ppxres = pRenderer->CreateObjects();
    if (Failed(ppxres)) {
        delete pRenderer;
        return ppxres;
    }

    *ppRenderer = pRenderer;

    return ppx::SUCCESS;
}

ppx::Result ForwardRenderer::RenderToOutput(
    ForwardRenderer::Frame& frame,
    scene::RenderOutput*    pOutput,
    grfx::Semaphore*        pRenderCompleteSemaphore)
{
    // Get output render target image
    grfx::Image* pOutputImage = nullptr;
    //
    // auto ppxres = pOutput->GetRenderTargetImage(&pOutputImage, frame.imageAcquiredSemaphore.Get());
    // if (Failed(ppxres)) {
    //    return ppxres;
    //}

    // Get render pass for render output render target image
    grfx::RenderPass* pOutputRenderPass = nullptr;
    //
    auto ppxres = GetRenderOutputRenderPass(
        pOutputImage,
        &pOutputRenderPass);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Output render command buffer
    {
        auto& cmd = frame.renderOutputCmd;

        // Begin command buffer
        ppxres = cmd->Begin();
        if (Failed(ppxres)) {
            return ppxres;
        }

        // Transition swapchain image if needed
        if (pOutput->IsSwapchain()) {
            cmd->TransitionImageLayout(
                pOutputImage,
                PPX_ALL_SUBRESOURCES,
                grfx::RESOURCE_STATE_PRESENT,
                grfx::RESOURCE_STATE_RENDER_TARGET);
        }

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = pOutputRenderPass;
        beginInfo.renderArea                = pOutputRenderPass->GetRenderArea();

        cmd->BeginRenderPass(&beginInfo);
        {
            cmd->ClearRenderTarget(pOutputImage, {1, 0, 0, 1});
        }
        cmd->EndRenderPass();

        // Transition swapchain image if needed
        if (pOutput->IsSwapchain()) {
            cmd->TransitionImageLayout(
                pOutputImage,
                PPX_ALL_SUBRESOURCES,
                grfx::RESOURCE_STATE_RENDER_TARGET,
                grfx::RESOURCE_STATE_PRESENT);
        }

        // End command buffer
        ppxres = cmd->End();
        if (Failed(ppxres)) {
            return ppxres;
        }

        // Submit output render command buffer
        {
            grfx::SubmitInfo submitInfo   = {};
            submitInfo.commandBufferCount = 1;
            submitInfo.ppCommandBuffers   = &cmd;
            // submitInfo.waitSemaphoreCount   = 1;
            // submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
            // submitInfo.signalSemaphoreCount = 1;
            // submitInfo.ppSignalSemaphores   = &pRenderCompleteSemaphore;
            // submitInfo.pFence               = frame.renderCompleteFence;

            PPX_CHECKED_CALL(GetDevice()->GetGraphicsQueue()->Submit(&submitInfo));
        }
    }

    return ppx::SUCCESS;
}

ppx::Result ForwardRenderer::RenderInternal(
    scene::RenderOutput* pOutput,
    grfx::Semaphore*     pRenderCompleteSemaphore)
{
    auto& frame = mFrames[mCurrentFrameIndex];

    // Wait for and reset render complete fence
    // PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Render to output
    auto ppxres = RenderToOutput(
        frame,
        pOutput,
        pRenderCompleteSemaphore);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

} // namespace scene
} // namespace ppx
