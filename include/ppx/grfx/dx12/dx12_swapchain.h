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

#ifndef ppx_grfx_dx12_swapchain_h
#define ppx_grfx_dx12_swapchain_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_swapchain.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class Surface
    : public grfx::Surface
{
public:
    Surface() {}
    virtual ~Surface() {}

    HWND GetWindowHandle() const { return mWindowHandle; }

    virtual uint32_t GetMinImageWidth() const override { return 0; }
    virtual uint32_t GetMinImageHeight() const override { return 0; }
    virtual uint32_t GetMinImageCount() const override { return 1; }
    virtual uint32_t GetMaxImageWidth() const override { return 65536; }
    virtual uint32_t GetMaxImageHeight() const override { return 65536; }
    virtual uint32_t GetMaxImageCount() const override { return DXGI_MAX_SWAP_CHAIN_BUFFERS; }

protected:
    virtual Result CreateApiObjects(const grfx::SurfaceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    HWND mWindowHandle = nullptr;
};

// -------------------------------------------------------------------------------------------------

class Swapchain
    : public grfx::Swapchain
{
public:
    Swapchain() {}
    virtual ~Swapchain() {}

    virtual Result PresentInternal(
        uint32_t                      imageIndex,
        uint32_t                      waitSemaphoreCount,
        const grfx::Semaphore* const* ppWaitSemaphores) override;

protected:
    virtual Result CreateApiObjects(const grfx::SwapchainCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

    virtual Result AcquireNextImageInternal(
        uint64_t         timeout,
        grfx::Semaphore* pSemaphore,
        grfx::Fence*     pFence,
        uint32_t*        pImageIndex) override;

    virtual Result ResizeInternal(uint32_t width, uint32_t height) override;

private:
    DXGISwapChainPtr     mSwapchain;
    HANDLE               mFrameLatencyWaitableObject = nullptr;
    D3D12CommandQueuePtr mQueue;
    UINT                 mSwapchainFlags;

    //
    // Store sync internval so we can control its behavior based
    // on which present mode the client requested.
    //
    // See:
    //   https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
    //
    UINT mSyncInterval   = 1;
    BOOL mTearingEnabled = FALSE;

    Result CreateImages(std::vector<ID3D12Resource*> colorImages, std::vector<ID3D12Resource*> depthImages);
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_swapchain_h
