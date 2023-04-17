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

#ifndef ppx_render_target_h
#define ppx_render_target_h

#include "ppx/grfx/grfx_config.h"

#include <functional>
#include <memory>

namespace ppx {

//! @class RenderTarget
//!
//! The RenderTarget interface should match Swapchain.
//! It could be backed by a actual swapchain or an off-screen buffer.
//!

class RenderTarget
{
public:
    RenderTarget() = default;
    virtual ~RenderTarget() {}

    RenderTarget(const RenderTarget&)            = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

public:
    virtual uint32_t     GetImageCount() const  = 0;
    virtual grfx::Format GetColorFormat() const = 0;
    virtual grfx::Format GetDepthFormat() const = 0;

    virtual Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const = 0;
    virtual Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const = 0;

    // Full image width/height, might be larger than the render area
    // In Swapchain, it's Get{Width,Height} but in that case the viewport and image size are the same.
    virtual uint32_t GetImageWidth() const  = 0;
    virtual uint32_t GetImageHeight() const = 0;

    virtual Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const = 0;

    virtual Result AcquireNextImage(
        uint64_t         timeout,    // Nanoseconds
        grfx::Semaphore* pSemaphore, // Wait sempahore
        grfx::Fence*     pFence,     // Wait fence
        uint32_t*        pImageIndex) = 0;

    virtual Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) = 0;

    virtual grfx::Rect GetRenderArea() const; // Returns ScissorRect.
    grfx::Viewport     GetViewport(float minDepth = 0.0, float maxDepth = 1.0) const;
    float              GetAspect() const;

    grfx::ImagePtr GetColorImage(uint32_t imageIndex) const;
    grfx::ImagePtr GetDepthImage(uint32_t imageIndex) const;

    grfx::RenderPassPtr GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;

    virtual grfx::Device* GetDevice() const = 0;

protected:
    virtual Result OnUpdate() { return ppx::SUCCESS; }
};

class SwapchainRenderTarget : public RenderTarget
{
public:
    static std::unique_ptr<SwapchainRenderTarget> Create(grfx::Swapchain* swapchain);

    Result ResizeSwapchain(uint32_t w, uint32_t h);
    Result ReplaceSwapchain(grfx::Swapchain*);
    bool   NeedUpdate();
    void   SetNeedUpdate();

public:
    uint32_t     GetImageCount() const final;
    grfx::Format GetColorFormat() const final;
    grfx::Format GetDepthFormat() const final;

    uint32_t GetImageWidth() const final;
    uint32_t GetImageHeight() const final;

    Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const final;

    Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const final;

    Result AcquireNextImage(
        uint64_t         timeout,
        grfx::Semaphore* pSemaphore,
        grfx::Fence*     pFence,
        uint32_t*        pImageIndex) final;

    Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) final;

    grfx::Device* GetDevice() const final;

private:
    grfx::Swapchain* mSwapchain  = nullptr;
    bool             mNeedUpdate = false;

    SwapchainRenderTarget(grfx::Swapchain* swapchain);
};

class RenderTargetPresentCommon
{
public:
    Result Init(grfx::Queue* queue, uint32_t imageCount);

    Result Present(
        RenderTarget*                             realTarget,
        uint32_t                                  imageIndex,
        uint32_t                                  waitSemaphoreCount,
        const grfx::Semaphore* const*             ppWaitSemaphores,
        std::function<void(grfx::CommandBuffer*)> recordCommands);

private:
    grfx::Queue* mQueue = nullptr;
    grfx::Queue* GetQueue() const { return mQueue; }

    std::vector<grfx::CommandBufferPtr> mCommandBuffers;
    std::vector<grfx::SemaphorePtr>     mSemaphores;
};

class IndirectRenderTarget : public RenderTarget
{
public:
    struct CreateInfo
    {
        RenderTarget* next        = nullptr;
        grfx::Queue*  pQueue      = nullptr;
        uint32_t      width       = 0;
        uint32_t      height      = 0;
        grfx::Format  colorFormat = grfx::FORMAT_UNDEFINED;
        grfx::Format  depthFormat = grfx::FORMAT_UNDEFINED;
        uint32_t      imageCount  = 0;
    };
    static std::unique_ptr<IndirectRenderTarget> Create(const CreateInfo& createInfo);

    void UpdateRenderArea(grfx::Rect renderArea);
    void UpdateViewport(uint32_t width, uint32_t height)
    {
        UpdateRenderArea({0, 0, width, height});
    }

public:
    uint32_t     GetImageCount() const final { return mCreateInfo.imageCount; }
    grfx::Format GetColorFormat() const final { return mCreateInfo.colorFormat; }
    grfx::Format GetDepthFormat() const final { return mCreateInfo.depthFormat; }

    uint32_t GetImageWidth() const final { return mCreateInfo.width; }
    uint32_t GetImageHeight() const final { return mCreateInfo.height; }

    Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const final;
    Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const final;

    grfx::Rect GetRenderArea() const final { return mRenderArea; }

    grfx::Device* GetDevice() const final;

    Result AcquireNextImage(
        uint64_t         timeout,    // Nanoseconds
        grfx::Semaphore* pSemaphore, // Wait sempahore
        grfx::Fence*     pFence,     // Wait fence
        uint32_t*        pImageIndex) final;

    Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) final;

private:
    IndirectRenderTarget(const CreateInfo& createInfo);

    Result Init();

    CreateInfo mCreateInfo = {};
    grfx::Rect mRenderArea = {};

    std::vector<grfx::ImagePtr> mDepthImages;
    std::vector<grfx::ImagePtr> mColorImages;

    RenderTargetPresentCommon mPresent;

    RenderTarget* Next() { return mCreateInfo.next; }
    grfx::Queue*  GetQueue() const { return mCreateInfo.pQueue; }

    void RecordCommands(grfx::CommandBuffer* commandBuffer, uint32_t imageIndex);
};

// Wrap a existing RenderTarget, and modify some of its behaivour.
class RenderTargetWrap : public RenderTarget
{
public:
    RenderTargetWrap(RenderTarget* impl)
        : mImpl(impl) {}

    uint32_t     GetImageCount() const override { return mImpl->GetImageCount(); }
    grfx::Format GetColorFormat() const override { return mImpl->GetColorFormat(); }
    grfx::Format GetDepthFormat() const override { return mImpl->GetDepthFormat(); }

    Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const override { return mImpl->GetColorImage(imageIndex, ppImage); }
    Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const override { return mImpl->GetDepthImage(imageIndex, ppImage); }

    // Full image width/height, might be larger than the render area
    uint32_t GetImageWidth() const override { return mImpl->GetImageWidth(); }
    uint32_t GetImageHeight() const override { return mImpl->GetImageHeight(); }

    using RenderTarget::GetRenderPass;
    Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const override
    {
        return mImpl->GetRenderPass(imageIndex, loadOp, ppRenderPass);
    }

    Result AcquireNextImage(
        uint64_t         timeout,    // Nanoseconds
        grfx::Semaphore* pSemaphore, // Wait sempahore
        grfx::Fence*     pFence,     // Wait fence
        uint32_t*        pImageIndex) override
    {
        return mImpl->AcquireNextImage(timeout, pSemaphore, pFence, pImageIndex);
    }

    Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) override
    {
        return mImpl->Present(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
    }

    grfx::Rect GetRenderArea() const override { return mImpl->GetRenderArea(); }

    grfx::Device* GetDevice() const final { return mImpl->GetDevice(); }

protected:
    RenderTarget* mImpl;
};

// Draw ImGui after actual presentation.
class RenderTargetPresentHook : public RenderTargetWrap
{
public:
    static std::unique_ptr<RenderTargetPresentHook> Create(grfx::Queue* queue, RenderTarget* backing, std::function<void(grfx::CommandBuffer*)> f);

    Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) final;

private:
    RenderTargetPresentHook(RenderTarget* backing, std::function<void(grfx::CommandBuffer*)> f);
    Result Init(grfx::Queue* queue);

    std::function<void(grfx::CommandBuffer*)> mOnPresent;
    RenderTargetPresentCommon                 mPresent;

    void RecordCommands(grfx::CommandBuffer* commandBuffer, uint32_t imageIndex);
};

} // namespace ppx

#endif // ppx_render_target_h
