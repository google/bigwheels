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

#include "ppx/grfx/grfx_swapchain.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_render_pass.h"
#include "ppx/grfx/grfx_instance.h"

namespace ppx {
namespace grfx {

Result Swapchain::Create(const grfx::SwapchainCreateInfo* pCreateInfo)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo->pQueue);
    if (IsNull(pCreateInfo->pQueue)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    Result ppxres = grfx::DeviceObject<grfx::SwapchainCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Update the stored create info's image count since the actual
    // number of images might be different (hopefully more) than
    // what was originally requested.
    if (!IsHeadless()) {
        mCreateInfo.imageCount = CountU32(mColorImages);
    }
    if (mCreateInfo.imageCount != pCreateInfo->imageCount) {
        PPX_LOG_INFO("Swapchain actual image count is different from what was requested\n"
                     << "   actual    : " << mCreateInfo.imageCount << "\n"
                     << "   requested : " << pCreateInfo->imageCount);
    }

    //
    // NOTE: mCreateInfo will be used from this point on.
    //

    // Create color images if needed. This is only needed if we're creating
    // a headless swapchain.
    if (mColorImages.empty()) {
        for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
            grfx::ImageCreateInfo rtCreateInfo = ImageCreateInfo::RenderTarget2D(mCreateInfo.width, mCreateInfo.height, mCreateInfo.colorFormat);
            rtCreateInfo.ownership             = grfx::OWNERSHIP_RESTRICTED;
            rtCreateInfo.RTVClearValue         = {0.0f, 0.0f, 0.0f, 0.0f};
            rtCreateInfo.initialState          = grfx::RESOURCE_STATE_PRESENT;
            rtCreateInfo.usageFlags =
                grfx::IMAGE_USAGE_COLOR_ATTACHMENT |
                grfx::IMAGE_USAGE_TRANSFER_SRC |
                grfx::IMAGE_USAGE_TRANSFER_DST |
                grfx::IMAGE_USAGE_SAMPLED;

            grfx::ImagePtr renderTarget;
            ppxres = GetDevice()->CreateImage(&rtCreateInfo, &renderTarget);
            if (Failed(ppxres)) {
                return ppxres;
            }

            mColorImages.push_back(renderTarget);
        }
    }

    // Create depth images if needed. This is usually needed for both normal swapchains
    // and headless swapchains, but not needed for XR swapchains which create their own
    // depth images.
    //
    ppxres = CreateDepthImages();
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = CreateRenderPasses();
    if (Failed(ppxres)) {
        return ppxres;
    }

    if (IsHeadless()) {
        // Set mCurrentImageIndex to (imageCount - 1) so that the first
        // AcquireNextImage call acquires the first image at index 0.
        mCurrentImageIndex = mCreateInfo.imageCount - 1;

        // Create command buffers to signal and wait semaphores at
        // AcquireNextImage and Present calls.
        for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
            grfx::CommandBufferPtr commandBuffer = nullptr;
            mCreateInfo.pQueue->CreateCommandBuffer(&commandBuffer, 0, 0);
            mHeadlessCommandBuffers.push_back(commandBuffer);
        }
    }

    PPX_LOG_INFO("Swapchain created");
    PPX_LOG_INFO("   "
                 << "resolution  : " << mCreateInfo.width << "x" << mCreateInfo.height);
    PPX_LOG_INFO("   "
                 << "image count : " << mCreateInfo.imageCount);

    return ppx::SUCCESS;
}

void Swapchain::Destroy()
{
    DestroyRenderPasses();

    DestroyDepthImages();

    DestroyColorImages();

#if defined(PPX_BUILD_XR)
    if (mXrColorSwapchain != XR_NULL_HANDLE) {
        xrDestroySwapchain(mXrColorSwapchain);
    }
    if (mXrDepthSwapchain != XR_NULL_HANDLE) {
        xrDestroySwapchain(mXrDepthSwapchain);
    }
#endif

    for (auto& elem : mHeadlessCommandBuffers) {
        if (elem) {
            mCreateInfo.pQueue->DestroyCommandBuffer(elem);
        }
    }
    mHeadlessCommandBuffers.clear();

    grfx::DeviceObject<grfx::SwapchainCreateInfo>::Destroy();
}

void Swapchain::DestroyColorImages()
{
    for (auto& elem : mColorImages) {
        if (elem) {
            GetDevice()->DestroyImage(elem);
        }
    }
    mColorImages.clear();
}

Result Swapchain::CreateDepthImages()
{
#if defined(PPX_BUILD_XR)
    PPX_ASSERT_MSG(false, "Swapchain::CreateDepthImages() is meant for non-XR applications, was it called by mistake?");
#endif

    if ((mCreateInfo.depthFormat != grfx::FORMAT_UNDEFINED) && mDepthImages.empty()) {
        for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
            grfx::ImageCreateInfo dpCreateInfo = ImageCreateInfo::DepthStencilTarget(mCreateInfo.width, mCreateInfo.height, mCreateInfo.depthFormat);
            dpCreateInfo.ownership             = grfx::OWNERSHIP_RESTRICTED;
            dpCreateInfo.DSVClearValue         = {1.0f, 0xFF};

            grfx::ImagePtr depthStencilTarget;
            auto           ppxres = GetDevice()->CreateImage(&dpCreateInfo, &depthStencilTarget);
            if (Failed(ppxres)) {
                return ppxres;
            }

            mDepthImages.push_back(depthStencilTarget);
        }
    }

    return ppx::SUCCESS;
}

void Swapchain::DestroyDepthImages()
{
#if defined(PPX_BUILD_XR)
    PPX_ASSERT_MSG(false, "Swapchain::DestroyDepthImages() is meant for non-XR applications, was it called by mistake?");
#endif

    for (auto& elem : mDepthImages) {
        if (elem) {
            GetDevice()->DestroyImage(elem);
        }
    }
    mDepthImages.clear();
}

Result Swapchain::CreateRenderPasses()
{
    uint32_t imageCount = CountU32(mColorImages);
    PPX_ASSERT_MSG((imageCount > 0), "No color images found for swapchain renderpasses");

    // Create render passes with grfx::ATTACHMENT_LOAD_OP_CLEAR for render target.
    for (size_t i = 0; i < imageCount; ++i) {
        grfx::RenderPassCreateInfo3 rpCreateInfo = {};
        rpCreateInfo.width                       = mCreateInfo.width;
        rpCreateInfo.height                      = mCreateInfo.height;
        rpCreateInfo.renderTargetCount           = 1;
        rpCreateInfo.pRenderTargetImages[0]      = mColorImages[i];
        rpCreateInfo.pDepthStencilImage          = mDepthImages.empty() ? nullptr : mDepthImages[i];
        rpCreateInfo.renderTargetClearValues[0]  = {{0.0f, 0.0f, 0.0f, 0.0f}};
        rpCreateInfo.depthStencilClearValue      = {1.0f, 0xFF};
        rpCreateInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.depthLoadOp                 = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.ownership                   = grfx::OWNERSHIP_RESTRICTED;

        grfx::RenderPassPtr renderPass;
        auto                ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Swapchain::CreateRenderPass(CLEAR) failed");
            return ppxres;
        }

        mClearRenderPasses.push_back(renderPass);
    }

    // Create render passes with grfx::ATTACHMENT_LOAD_OP_LOAD for render target.
    for (size_t i = 0; i < imageCount; ++i) {
        grfx::RenderPassCreateInfo3 rpCreateInfo = {};
        rpCreateInfo.width                       = mCreateInfo.width;
        rpCreateInfo.height                      = mCreateInfo.height;
        rpCreateInfo.renderTargetCount           = 1;
        rpCreateInfo.pRenderTargetImages[0]      = mColorImages[i];
        rpCreateInfo.pDepthStencilImage          = mDepthImages.empty() ? nullptr : mDepthImages[i];
        rpCreateInfo.renderTargetClearValues[0]  = {{0.0f, 0.0f, 0.0f, 0.0f}};
        rpCreateInfo.depthStencilClearValue      = {1.0f, 0xFF};
        rpCreateInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_LOAD;
        rpCreateInfo.depthLoadOp                 = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        rpCreateInfo.ownership                   = grfx::OWNERSHIP_RESTRICTED;

        grfx::RenderPassPtr renderPass;
        auto                ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Swapchain::CreateRenderPass(LOAD) failed");
            return ppxres;
        }

        mLoadRenderPasses.push_back(renderPass);
    }

    return ppx::SUCCESS;
}

void Swapchain::DestroyRenderPasses()
{
#if defined(PPX_BUILD_XR)
    PPX_ASSERT_MSG(false, "Swapchain::DestroyRenderPasses() is meant for non-XR applications, was it called by mistake?");
#endif

    for (auto& elem : mClearRenderPasses) {
        if (elem) {
            GetDevice()->DestroyRenderPass(elem);
        }
    }
    mClearRenderPasses.clear();

    for (auto& elem : mLoadRenderPasses) {
        if (elem) {
            GetDevice()->DestroyRenderPass(elem);
        }
    }
    mLoadRenderPasses.clear();
}

bool Swapchain::IsHeadless() const
{
#if defined(PPX_BUILD_XR)
    return mCreateInfo.pXrComponent == nullptr && mCreateInfo.pSurface == nullptr;
#else
    return mCreateInfo.pSurface == nullptr;
#endif
}

Result Swapchain::GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    if (!IsIndexInRange(imageIndex, mColorImages)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppImage = mColorImages[imageIndex];
    return ppx::SUCCESS;
}

Result Swapchain::GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const
{
    if (!IsIndexInRange(imageIndex, mDepthImages)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppImage = mDepthImages[imageIndex];
    return ppx::SUCCESS;
}

Result Swapchain::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const
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

grfx::ImagePtr Swapchain::GetColorImage(uint32_t imageIndex) const
{
    grfx::ImagePtr object;
    GetColorImage(imageIndex, &object);
    return object;
}

grfx::ImagePtr Swapchain::GetDepthImage(uint32_t imageIndex) const
{
    grfx::ImagePtr object;
    GetDepthImage(imageIndex, &object);
    return object;
}

grfx::RenderPassPtr Swapchain::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp) const
{
    grfx::RenderPassPtr object;
    GetRenderPass(imageIndex, loadOp, &object);
    return object;
}

Result Swapchain::AcquireNextImage(
    uint64_t         timeout,    // Nanoseconds
    grfx::Semaphore* pSemaphore, // Wait sempahore
    grfx::Fence*     pFence,     // Wait fence
    uint32_t*        pImageIndex)
{
#if defined(PPX_BUILD_XR)
    if (mCreateInfo.pXrComponent != nullptr) {
        PPX_ASSERT_MSG(mXrColorSwapchain != XR_NULL_HANDLE, "Invalid color xrSwapchain handle!");
        PPX_ASSERT_MSG(pSemaphore == nullptr, "Should not use semaphore when XR is enabled!");
        PPX_ASSERT_MSG(pFence == nullptr, "Should not use fence when XR is enabled!");
        XrSwapchainImageAcquireInfo acquire_info = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        CHECK_XR_CALL(xrAcquireSwapchainImage(mXrColorSwapchain, &acquire_info, pImageIndex));

        XrSwapchainImageWaitInfo wait_info = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wait_info.timeout                  = XR_INFINITE_DURATION;
        CHECK_XR_CALL(xrWaitSwapchainImage(mXrColorSwapchain, &wait_info));

        if (mXrDepthSwapchain != XR_NULL_HANDLE) {
            uint32_t                    colorImageIndex = *pImageIndex;
            XrSwapchainImageAcquireInfo acquire_info    = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            CHECK_XR_CALL(xrAcquireSwapchainImage(mXrDepthSwapchain, &acquire_info, pImageIndex));

            XrSwapchainImageWaitInfo wait_info = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wait_info.timeout                  = XR_INFINITE_DURATION;
            CHECK_XR_CALL(xrWaitSwapchainImage(mXrDepthSwapchain, &wait_info));

            PPX_ASSERT_MSG(colorImageIndex == *pImageIndex, "Color and depth swapchain image indices are different");
        }
        return ppx::SUCCESS;
    }
#endif

    if (IsHeadless()) {
        return AcquireNextImageHeadless(timeout, pSemaphore, pFence, pImageIndex);
    }

    return AcquireNextImageInternal(timeout, pSemaphore, pFence, pImageIndex);
}

Result Swapchain::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    if (IsHeadless()) {
        return PresentHeadless(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
    }

    return PresentInternal(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
}

Result Swapchain::AcquireNextImageHeadless(uint64_t timeout, grfx::Semaphore* pSemaphore, grfx::Fence* pFence, uint32_t* pImageIndex)
{
    *pImageIndex       = (mCurrentImageIndex + 1u) % CountU32(mColorImages);
    mCurrentImageIndex = *pImageIndex;

    grfx::CommandBufferPtr commandBuffer = mHeadlessCommandBuffers[mCurrentImageIndex];

    commandBuffer->Begin();
    commandBuffer->End();

    grfx::SubmitInfo sInfo     = {};
    sInfo.ppCommandBuffers     = &commandBuffer;
    sInfo.commandBufferCount   = 1;
    sInfo.pFence               = pFence;
    sInfo.ppSignalSemaphores   = &pSemaphore;
    sInfo.signalSemaphoreCount = 1;
    mCreateInfo.pQueue->Submit(&sInfo);

    return ppx::SUCCESS;
}

Result Swapchain::PresentHeadless(uint32_t imageIndex, uint32_t waitSemaphoreCount, const grfx::Semaphore* const* ppWaitSemaphores)
{
    grfx::CommandBufferPtr commandBuffer = mHeadlessCommandBuffers[mCurrentImageIndex];

    commandBuffer->Begin();
    commandBuffer->End();

    grfx::SubmitInfo sInfo   = {};
    sInfo.ppCommandBuffers   = &commandBuffer;
    sInfo.commandBufferCount = 1;
    sInfo.ppWaitSemaphores   = ppWaitSemaphores;
    sInfo.waitSemaphoreCount = waitSemaphoreCount;
    mCreateInfo.pQueue->Submit(&sInfo);

    return ppx::SUCCESS;
}

} // namespace grfx
} // namespace ppx
