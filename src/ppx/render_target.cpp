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

#include "ppx/render_target.h"

#include "ppx/config.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_enums.h"
#include "ppx/grfx/grfx_render_pass.h"
#include "ppx/grfx/grfx_instance.h"
#include "ppx/grfx/grfx_swapchain.h"
#include "ppx/grfx/grfx_queue.h"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// Implement RenderPass for RenderTarget

namespace {

class RenderTargetRenderPassImpl
{
public:
    Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const;
    Result UpdateRenderPass(RenderTarget* renderTarget);

private:
    std::vector<grfx::RenderPassPtr> mClearRenderPasses;
    std::vector<grfx::RenderPassPtr> mLoadRenderPasses;
};


template <typename T>
class WithRenderPass : public T
{
public:
    using T::GetRenderPass;
    using T::T;
    Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const final
    {
        return mRenderPass.GetRenderPass(imageIndex, loadOp, ppRenderPass);
    }

protected:
    Result OnUpdate() override
    {
        Result ppxres = mRenderPass.UpdateRenderPass(this);
        if (Failed(ppxres)) {
            return ppxres;
        }
        return T::OnUpdate();
    }

private:
    RenderTargetRenderPassImpl mRenderPass;
};

Result RenderTargetRenderPassImpl::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const
{
    if (!IsIndexInRange(imageIndex, mClearRenderPasses)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    if (loadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
        *ppRenderPass = mClearRenderPasses[imageIndex];
    }
    else {
        *ppRenderPass = mLoadRenderPasses[imageIndex];
    }
    return ppx::SUCCESS;
}

Result RenderTargetRenderPassImpl::UpdateRenderPass(RenderTarget* renderTarget)
{
    for (auto rp : mClearRenderPasses) {
        renderTarget->GetDevice()->DestroyRenderPass(rp);
    }
    mClearRenderPasses.clear();
    for (auto rp : mLoadRenderPasses) {
        renderTarget->GetDevice()->DestroyRenderPass(rp);
    }
    mLoadRenderPasses.clear();

    bool hasDepthImage = renderTarget->GetDepthFormat() != grfx::FORMAT_UNDEFINED;

    // Create render passes with grfx::ATTACHMENT_LOAD_OP_CLEAR for render target.
    for (uint32_t i = 0; i < renderTarget->GetImageCount(); i++) {
        grfx::RenderPassCreateInfo3 rpCreateInfo = {};
        rpCreateInfo.width                       = renderTarget->GetImageWidth();
        rpCreateInfo.height                      = renderTarget->GetImageHeight();
        rpCreateInfo.renderTargetCount           = 1;
        rpCreateInfo.pRenderTargetImages[0]      = renderTarget->GetColorImage(i);
        rpCreateInfo.pDepthStencilImage          = hasDepthImage ? renderTarget->GetDepthImage(i) : nullptr;
        rpCreateInfo.renderTargetClearValues[0]  = {{0.0f, 0.0f, 0.0f, 0.0f}};
        rpCreateInfo.depthStencilClearValue      = {1.0f, 0xFF};
        rpCreateInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.depthLoadOp                 = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.ownership                   = grfx::OWNERSHIP_RESTRICTED;

        grfx::RenderPassPtr renderPass;

        Result ppxres = renderTarget->GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "RenderTargetRenderPassImpl::UpdateRenderPass(CLEAR) failed");
            return ppxres;
        }

        mClearRenderPasses.push_back(renderPass);
    }

    // Create render passes with grfx::ATTACHMENT_LOAD_OP_LOAD for render target.
    for (uint32_t i = 0; i < renderTarget->GetImageCount(); i++) {
        grfx::RenderPassCreateInfo3 rpCreateInfo = {};
        rpCreateInfo.width                       = renderTarget->GetImageWidth();
        rpCreateInfo.height                      = renderTarget->GetImageHeight();
        rpCreateInfo.renderTargetCount           = 1;
        rpCreateInfo.pRenderTargetImages[0]      = renderTarget->GetColorImage(i);
        rpCreateInfo.pDepthStencilImage          = hasDepthImage ? renderTarget->GetDepthImage(i) : nullptr;
        rpCreateInfo.renderTargetClearValues[0]  = {{0.0f, 0.0f, 0.0f, 0.0f}};
        rpCreateInfo.depthStencilClearValue      = {1.0f, 0xFF};
        rpCreateInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_LOAD;
        rpCreateInfo.depthLoadOp                 = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.ownership                   = grfx::OWNERSHIP_RESTRICTED;

        grfx::RenderPassPtr renderPass;

        Result ppxres = renderTarget->GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "RenderTargetRenderPassImpl::UpdateRenderPass(LOAD) failed");
            return ppxres;
        }

        mLoadRenderPasses.push_back(renderPass);
    }

    return ppx::SUCCESS;
}

} // namespace

// -------------------------------------------------------------------------------------------------
// RenderTarget

grfx::Rect RenderTarget::GetRenderArea() const
{
    return grfx::Rect{0, 0, GetImageWidth(), GetImageHeight()};
}

grfx::ImagePtr RenderTarget::GetColorImage(uint32_t imageIndex) const
{
    grfx::ImagePtr object;
    GetColorImage(imageIndex, &object);
    return object;
}

grfx::ImagePtr RenderTarget::GetDepthImage(uint32_t imageIndex) const
{
    grfx::ImagePtr object;
    GetDepthImage(imageIndex, &object);
    return object;
}

float RenderTarget::GetAspect() const
{
    grfx::Rect rect = GetRenderArea();
    return static_cast<float>(rect.width) / rect.height;
}

grfx::Viewport RenderTarget::GetViewport(float minDepth, float maxDepth) const
{
    grfx::Rect rect = GetRenderArea();
    return grfx::Viewport{
        static_cast<float>(rect.x),
        static_cast<float>(rect.y),
        static_cast<float>(rect.width),
        static_cast<float>(rect.height),
        minDepth,
        maxDepth,
    };
}

grfx::RenderPassPtr RenderTarget::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp) const
{
    grfx::RenderPassPtr object;
    GetRenderPass(imageIndex, loadOp, &object);
    return object;
}

// -------------------------------------------------------------------------------------------------
// SwapchainRenderTarget

std::unique_ptr<SwapchainRenderTarget> SwapchainRenderTarget::Create(grfx::Swapchain* swapchain)
{
    std::unique_ptr<SwapchainRenderTarget> renderTarget(new WithRenderPass<SwapchainRenderTarget>(swapchain));
    PPX_CHECKED_CALL(renderTarget->OnUpdate());
    return renderTarget;
}

SwapchainRenderTarget::SwapchainRenderTarget(grfx::Swapchain* swapchain)
    : mSwapchain(swapchain)
{
}

Result SwapchainRenderTarget::ResizeSwapchain(uint32_t w, uint32_t h)
{
    Result ppxres = mSwapchain->Resize(w, h);
    if (ppxres == ppx::SUCCESS) {
        mNeedUpdate = false;
        return OnUpdate();
    }
    return ppxres;
}

Result SwapchainRenderTarget::ReplaceSwapchain(grfx::Swapchain* swapchain)
{
    mSwapchain  = swapchain;
    mNeedUpdate = false;
    return OnUpdate();
}

bool SwapchainRenderTarget::NeedUpdate()
{
    return mNeedUpdate;
}

void SwapchainRenderTarget::SetNeedUpdate()
{
    mNeedUpdate = true;
}

uint32_t SwapchainRenderTarget::GetImageCount() const
{
    return mSwapchain->GetImageCount();
}

grfx::Format SwapchainRenderTarget::GetColorFormat() const
{
    return mSwapchain->GetColorFormat();
}

grfx::Format SwapchainRenderTarget::GetDepthFormat() const
{
    return mSwapchain->GetDepthFormat();
}

uint32_t SwapchainRenderTarget::GetImageWidth() const
{
    return mSwapchain->GetWidth();
}

uint32_t SwapchainRenderTarget::GetImageHeight() const
{
    return mSwapchain->GetHeight();
}

Result SwapchainRenderTarget::GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    return mSwapchain->GetColorImage(imageIndex, ppImage);
}

Result SwapchainRenderTarget::GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    return mSwapchain->GetDepthImage(imageIndex, ppImage);
}

Result SwapchainRenderTarget::AcquireNextImage(
    uint64_t         timeout,
    grfx::Semaphore* pSemaphore,
    grfx::Fence*     pFence,
    uint32_t*        pImageIndex)
{
    Result ppxres = mSwapchain->AcquireNextImage(timeout, pSemaphore, pFence, pImageIndex);
    if (ppxres == ppx::ERROR_OUT_OF_DATE) {
        mNeedUpdate = true;
        return ppxres;
    }
    if (ppxres == ppx::ERROR_SUBOPTIMAL) {
        mNeedUpdate = true;
        return ppx::SUCCESS;
    }
    return ppxres;
}

Result SwapchainRenderTarget::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    Result ppxres = mSwapchain->Present(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
    if (ppxres == ppx::ERROR_OUT_OF_DATE) {
        mNeedUpdate = true;
        return ppxres;
    }
    if (ppxres == ppx::ERROR_SUBOPTIMAL) {
        mNeedUpdate = true;
        return ppx::SUCCESS;
    }
    return ppxres;
}

grfx::Device* SwapchainRenderTarget::GetDevice() const
{
    return mSwapchain->GetDevice();
}

// -------------------------------------------------------------------------------------------------
// RenderTargetPresentCommon
Result RenderTargetPresentCommon::Init(grfx::Queue* queue, uint32_t imageCount)
{
    mQueue = queue;

    for (uint32_t i = 0; i < imageCount; ++i) {
        grfx::CommandBufferPtr commandBuffer = nullptr;
        mQueue->CreateCommandBuffer(&commandBuffer, 0, 0);
        mCommandBuffers.push_back(commandBuffer);
    }
    grfx::Device* device = mQueue->GetDevice();
    for (uint32_t i = 0; i < imageCount; i++) {
        grfx::SemaphoreCreateInfo info = {};
        grfx::SemaphorePtr        pSemaphore;
        PPX_CHECKED_CALL(device->CreateSemaphore(&info, &pSemaphore));
        mSemaphores.push_back(pSemaphore);
    }
    return ppx::SUCCESS;
}

Result RenderTargetPresentCommon::Present(
    RenderTarget*                             realTarget,
    uint32_t                                  imageIndex,
    uint32_t                                  waitSemaphoreCount,
    const grfx::Semaphore* const*             ppWaitSemaphores,
    std::function<void(grfx::CommandBuffer*)> recordCommandss)
{
    grfx::CommandBufferPtr commandBuffer = mCommandBuffers[imageIndex];

    commandBuffer->Begin();
    recordCommandss(commandBuffer);
    commandBuffer->End();

    grfx::Semaphore* pSignalSemaphore = mSemaphores[imageIndex].Get();

    grfx::SubmitInfo sInfo     = {};
    sInfo.ppCommandBuffers     = &commandBuffer;
    sInfo.commandBufferCount   = 1;
    sInfo.ppWaitSemaphores     = ppWaitSemaphores;
    sInfo.waitSemaphoreCount   = waitSemaphoreCount;
    sInfo.ppSignalSemaphores   = &pSignalSemaphore;
    sInfo.signalSemaphoreCount = 1;
    mQueue->Submit(&sInfo);

    return realTarget->Present(imageIndex, 1, &pSignalSemaphore);
}

// -------------------------------------------------------------------------------------------------
// IndirectRenderTarget

std::unique_ptr<IndirectRenderTarget> IndirectRenderTarget::Create(const CreateInfo& createInfo)
{
    std::unique_ptr<IndirectRenderTarget> renderTarget(new WithRenderPass<IndirectRenderTarget>(createInfo));
    PPX_CHECKED_CALL(renderTarget->Init());
    return renderTarget;
}

IndirectRenderTarget::IndirectRenderTarget(const CreateInfo& createInfo)
    : mCreateInfo(createInfo), mRenderArea(0, 0, createInfo.width, createInfo.height)
{
}

grfx::Device* IndirectRenderTarget::GetDevice() const
{
    return mCreateInfo.pQueue->GetDevice();
}

Result IndirectRenderTarget::Init()
{
    {
        Result ppxres = mPresent.Init(mCreateInfo.pQueue, mCreateInfo.imageCount);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    // Create color images if needed. This is only needed if we're creating
    // a headless swapchain.
    if (mColorImages.empty()) {
        for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
            grfx::ImageCreateInfo rtCreateInfo = grfx::ImageCreateInfo::RenderTarget2D(mCreateInfo.width, mCreateInfo.height, mCreateInfo.colorFormat);
            rtCreateInfo.ownership             = grfx::OWNERSHIP_RESTRICTED;
            rtCreateInfo.RTVClearValue         = {0.0f, 0.0f, 0.0f, 0.0f};
            rtCreateInfo.initialState          = grfx::RESOURCE_STATE_PRESENT;
            rtCreateInfo.usageFlags =
                grfx::IMAGE_USAGE_COLOR_ATTACHMENT |
                grfx::IMAGE_USAGE_TRANSFER_SRC |
                grfx::IMAGE_USAGE_TRANSFER_DST |
                grfx::IMAGE_USAGE_SAMPLED;

            grfx::ImagePtr renderTargetOLD;

            Result ppxres = GetQueue()->GetDevice()->CreateImage(&rtCreateInfo, &renderTargetOLD);
            if (Failed(ppxres)) {
                return ppxres;
            }

            mColorImages.push_back(renderTargetOLD);
        }
    }

    // Create depth images if needed. This is usually needed for both normal swapchains
    // and headless swapchains, but not needed for XR swapchains which create their own
    // depth images.
    if (mCreateInfo.depthFormat != grfx::FORMAT_UNDEFINED && mDepthImages.empty()) {
        for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
            grfx::ImageCreateInfo dpCreateInfo = grfx::ImageCreateInfo::DepthStencilTarget(mCreateInfo.width, mCreateInfo.height, mCreateInfo.depthFormat);
            dpCreateInfo.ownership             = grfx::OWNERSHIP_RESTRICTED;
            dpCreateInfo.DSVClearValue         = {1.0f, 0xFF};

            grfx::ImagePtr depthStencilTarget;

            Result ppxres = GetQueue()->GetDevice()->CreateImage(&dpCreateInfo, &depthStencilTarget);
            if (Failed(ppxres)) {
                return ppxres;
            }

            mDepthImages.push_back(depthStencilTarget);
        }
    }

    return OnUpdate();
}

Result IndirectRenderTarget::GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    if (!IsIndexInRange(imageIndex, mColorImages)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppImage = mColorImages[imageIndex];
    return ppx::SUCCESS;
}

Result IndirectRenderTarget::GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    if (!IsIndexInRange(imageIndex, mDepthImages)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppImage = mDepthImages[imageIndex];
    return ppx::SUCCESS;
}

void IndirectRenderTarget::UpdateRenderArea(grfx::Rect renderArea)
{
    if (renderArea.x < 0 || renderArea.y < 0 ||
        renderArea.width == 0 || renderArea.height == 0 ||
        renderArea.x + renderArea.width > mCreateInfo.width ||
        renderArea.y + renderArea.height > mCreateInfo.height) {
        return;
    }
    mRenderArea = renderArea;
}

Result IndirectRenderTarget::AcquireNextImage(
    uint64_t         timeout,    // Nanoseconds
    grfx::Semaphore* pSemaphore, // Wait sempahore
    grfx::Fence*     pFence,     // Wait fence
    uint32_t*        pImageIndex)
{
    return Next()->AcquireNextImage(timeout, pSemaphore, pFence, pImageIndex);
}

Result IndirectRenderTarget::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    return mPresent.Present(
        mCreateInfo.next,
        imageIndex,
        waitSemaphoreCount,
        ppWaitSemaphores,
        [this, imageIndex](grfx::CommandBuffer* commandBuffer) {
            RecordCommands(commandBuffer, imageIndex);
        });
}

void IndirectRenderTarget::RecordCommands(grfx::CommandBuffer* commandBuffer, uint32_t imageIndex)
{
    grfx::ImageToImageCopyInfo imcopy = {};
    {
        grfx::Rect src           = GetRenderArea();
        grfx::Rect dst           = Next()->GetRenderArea();
        imcopy.srcImage.offset.x = src.x;
        imcopy.srcImage.offset.y = src.y;
        imcopy.dstImage.offset.x = dst.x;
        imcopy.dstImage.offset.y = dst.y;
        if (src.width > dst.width) {
            imcopy.srcImage.offset.x += (src.width - dst.width) / 2;
        }
        else {
            imcopy.dstImage.offset.x += (dst.width - src.width) / 2;
        }
        if (src.height > dst.height) {
            imcopy.srcImage.offset.y += (src.height - dst.height) / 2;
        }
        else {
            imcopy.dstImage.offset.y += (dst.height - src.height) / 2;
        }
        imcopy.extent.x = std::min(src.width, dst.width);
        imcopy.extent.y = std::min(src.height, dst.height);
    }

    commandBuffer->TransitionImageLayout(Next()->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
    {
        // Clear screen.
        grfx::RenderPassPtr renderPass = Next()->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_CLEAR);

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0.5, 0.5, 0.5, 0}};
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        commandBuffer->BeginRenderPass(&beginInfo);
        commandBuffer->EndRenderPass();
    }
    commandBuffer->TransitionImageLayout(Next()->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_COPY_DST);
    {
        // Copy rendered image.
        grfx::Image* srcImage = RenderTarget::GetColorImage(imageIndex);
        grfx::Image* dstImage = Next()->GetColorImage(imageIndex);
        // Note(tianc): this should be a image blit instead of copy.
        commandBuffer->CopyImageToImage(&imcopy, srcImage, dstImage);
    }
    commandBuffer->TransitionImageLayout(Next()->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_PRESENT);
}

// -------------------------------------------------------------------------------------------------
// RenderTargetPresentHook

std::unique_ptr<RenderTargetPresentHook> RenderTargetPresentHook::Create(grfx::Queue* queue, RenderTarget* backing, std::function<void(grfx::CommandBuffer*)> f)
{
    std::unique_ptr<RenderTargetPresentHook> renderTarget(new RenderTargetPresentHook(backing, f));
    PPX_CHECKED_CALL(renderTarget->Init(queue));
    return renderTarget;
}

RenderTargetPresentHook::RenderTargetPresentHook(RenderTarget* backing, std::function<void(grfx::CommandBuffer*)> f)
    : RenderTargetWrap(backing), mOnPresent(f)
{
}

Result RenderTargetPresentHook::Init(grfx::Queue* queue)
{
    return mPresent.Init(queue, GetImageCount());
}

Result RenderTargetPresentHook::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    return mPresent.Present(
        mImpl,
        imageIndex,
        waitSemaphoreCount,
        ppWaitSemaphores,
        [this, imageIndex](grfx::CommandBuffer* commandBuffer) {
            return RecordCommands(commandBuffer, imageIndex);
        });
}

void RenderTargetPresentHook::RecordCommands(grfx::CommandBuffer* commandBuffer, uint32_t imageIndex)
{
    commandBuffer->TransitionImageLayout(mImpl->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
    {
        grfx::RenderPassPtr renderPass = mImpl->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_LOAD);

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0.5, 0.5, 0.5, 0}};
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        commandBuffer->BeginRenderPass(&beginInfo);
        if (mOnPresent) {
            commandBuffer->SetViewports(mImpl->GetViewport());
            commandBuffer->SetScissors(mImpl->GetRenderArea());
            mOnPresent(commandBuffer);
        }
        commandBuffer->EndRenderPass();
    }
    commandBuffer->TransitionImageLayout(mImpl->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
}

} // namespace ppx
