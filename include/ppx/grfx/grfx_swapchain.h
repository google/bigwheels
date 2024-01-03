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

#ifndef ppx_grfx_swapchain_h
#define ppx_grfx_swapchain_h

#include <limits>

#include "ppx/grfx/grfx_config.h"
#if defined(PPX_BUILD_XR)
#include "ppx/xr_component.h"
#endif

// clang-format off
#if defined(PPX_LINUX_XCB)
#   include <xcb/xcb.h>
#elif defined(PPX_LINUX_XLIB)
#   include <X11/Xlib.h>
#elif defined(PPX_LINUX_WAYLAND)
#   include <wayland-client.h>
#elif defined(PPX_MSW)
#   include <Windows.h>
#elif defined(PPX_ANDROID)
#   include <android_native_app_glue.h>
#endif
// clang-format on

namespace ppx {
namespace grfx {

//! @struct SurfaceCreateInfo
//!
//!
struct SurfaceCreateInfo
{
    // clang-format off
    grfx::Gpu*            pGpu = nullptr;
#if defined(PPX_LINUX_WAYLAND)
    struct wl_display*    display;
    struct wl_surface*    surface;
#elif defined(PPX_LINUX_XCB)
    xcb_connection_t*     connection;
    xcb_window_t          window;
#elif defined(PPX_LINUX_XLIB)
    Display*              dpy;
    Window                window;
#elif defined(PPX_MSW)
    HINSTANCE             hinstance;
    HWND                  hwnd;
#elif defined(PPX_ANDROID)
    android_app*          androidAppContext;
#endif
    // clang-format on
};

//! @class Surface
//!
//!
class Surface
    : public grfx::InstanceObject<grfx::SurfaceCreateInfo>
{
public:
    Surface() {}
    virtual ~Surface() {}

    virtual uint32_t GetMinImageWidth() const  = 0;
    virtual uint32_t GetMinImageHeight() const = 0;
    virtual uint32_t GetMinImageCount() const  = 0;
    virtual uint32_t GetMaxImageWidth() const  = 0;
    virtual uint32_t GetMaxImageHeight() const = 0;
    virtual uint32_t GetMaxImageCount() const  = 0;

    static constexpr uint32_t kInvalidExtent = std::numeric_limits<uint32_t>::max();

    virtual uint32_t GetCurrentImageWidth() const { return kInvalidExtent; }
    virtual uint32_t GetCurrentImageHeight() const { return kInvalidExtent; }
};

// -------------------------------------------------------------------------------------------------

//! @struct SwapchainCreateInfo
//!
//! NOTE: The member \b imageCount is the minimum image count.
//!       On Vulkan, the actual number of images created by
//!       the swapchain may be greater than this value.
//!
struct SwapchainCreateInfo
{
    grfx::Queue*              pQueue              = nullptr;
    grfx::Surface*            pSurface            = nullptr;
    grfx::ShadingRatePattern* pShadingRatePattern = nullptr;
    uint32_t                  width               = 0;
    uint32_t                  height              = 0;
    grfx::Format              colorFormat         = grfx::FORMAT_UNDEFINED;
    grfx::Format              depthFormat         = grfx::FORMAT_UNDEFINED;
    uint32_t                  imageCount          = 0;
    grfx::PresentMode         presentMode         = grfx::PRESENT_MODE_IMMEDIATE;
#if defined(PPX_BUILD_XR)
    XrComponent* pXrComponent = nullptr;
#endif
};

//! @class Swapchain
//!
//!
class Swapchain
    : public grfx::DeviceObject<grfx::SwapchainCreateInfo>
{
public:
    Swapchain() {}
    virtual ~Swapchain() {}

    bool         IsHeadless() const;
    uint32_t     GetWidth() const { return mCreateInfo.width; }
    uint32_t     GetHeight() const { return mCreateInfo.height; }
    uint32_t     GetImageCount() const { return mCreateInfo.imageCount; }
    grfx::Format GetColorFormat() const { return mCreateInfo.colorFormat; }
    grfx::Format GetDepthFormat() const { return mCreateInfo.depthFormat; }

    Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const;
    Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const;
    Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const;
    Result GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderTargetView** ppView) const;
    Result GetDepthStencilView(uint32_t imageIndex, grfx::DepthStencilView** ppView) const;

    // Convenience functions - returns empty object if index is invalid
    grfx::ImagePtr            GetColorImage(uint32_t imageIndex) const;
    grfx::ImagePtr            GetDepthImage(uint32_t imageIndex) const;
    grfx::RenderPassPtr       GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;
    grfx::RenderTargetViewPtr GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;
    grfx::DepthStencilViewPtr GetDepthStencilView(uint32_t imageIndex) const;

    Result AcquireNextImage(
        uint64_t         timeout,    // Nanoseconds
        grfx::Semaphore* pSemaphore, // Wait sempahore
        grfx::Fence*     pFence,     // Wait fence
        uint32_t*        pImageIndex);

    Result Present(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores);

    uint32_t GetCurrentImageIndex() const { return mCurrentImageIndex; }

    // D3D12 only, will return ERROR_FAILED on Vulkan
    virtual Result Resize(uint32_t width, uint32_t height) = 0;

#if defined(PPX_BUILD_XR)
    bool ShouldSkipExternalSynchronization() const
    {
        return mCreateInfo.pXrComponent != nullptr;
    }

    XrSwapchain GetXrColorSwapchain() const
    {
        return mXrColorSwapchain;
    }
    XrSwapchain GetXrDepthSwapchain() const
    {
        return mXrDepthSwapchain;
    }
#endif
protected:
    virtual Result Create(const grfx::SwapchainCreateInfo* pCreateInfo) override;
    virtual void   Destroy() override;
    friend class grfx::Device;

    // Make these protected since D3D12's swapchain resize will need to call them
    void   DestroyColorImages();
    Result CreateDepthImages();
    void   DestroyDepthImages();
    Result CreateRenderPasses();
    void   DestroyRenderPasses();
    Result CreateRenderTargets();
    void   DestroyRenderTargets();

private:
    virtual Result AcquireNextImageInternal(
        uint64_t         timeout,    // Nanoseconds
        grfx::Semaphore* pSemaphore, // Wait sempahore
        grfx::Fence*     pFence,     // Wait fence
        uint32_t*        pImageIndex) = 0;

    virtual Result PresentInternal(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) = 0;

    Result AcquireNextImageHeadless(
        uint64_t         timeout,
        grfx::Semaphore* pSemaphore,
        grfx::Fence*     pFence,
        uint32_t*        pImageIndex);

    Result PresentHeadless(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores);

    std::vector<grfx::CommandBufferPtr> mHeadlessCommandBuffers;

protected:
    grfx::QueuePtr                         mQueue;
    std::vector<grfx::ImagePtr>            mDepthImages;
    std::vector<grfx::ImagePtr>            mColorImages;
    std::vector<grfx::RenderTargetViewPtr> mClearRenderTargets;
    std::vector<grfx::RenderTargetViewPtr> mLoadRenderTargets;
    std::vector<grfx::DepthStencilViewPtr> mDepthStencilViews;
    std::vector<grfx::RenderPassPtr>       mClearRenderPasses;
    std::vector<grfx::RenderPassPtr>       mLoadRenderPasses;

#if defined(PPX_BUILD_XR)
    XrSwapchain mXrColorSwapchain = XR_NULL_HANDLE;
    XrSwapchain mXrDepthSwapchain = XR_NULL_HANDLE;
#endif

    // Keeps track of the image index returned by the last AcquireNextImage call.
    uint32_t mCurrentImageIndex = 0;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_swapchain_h
