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

#include "ppx/grfx/dx12/dx12_swapchain.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_image.h"
#include "ppx/grfx/dx12/dx12_instance.h"
#include "ppx/grfx/dx12/dx12_queue.h"
#include "ppx/grfx/dx12/dx12_sync.h"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// Surface
// -------------------------------------------------------------------------------------------------
Result Surface::CreateApiObjects(const grfx::SurfaceCreateInfo* pCreateInfo)
{
    if (pCreateInfo->hwnd == nullptr) {
        PPX_ASSERT_MSG(false, "window handle is null");
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // D3D12 doesn't have a surface the only thing to do is copy the window handle
    mWindowHandle = pCreateInfo->hwnd;

    return ppx::SUCCESS;
}

void Surface::DestroyApiObjects()
{
}

// -------------------------------------------------------------------------------------------------
// Swapchain
// -------------------------------------------------------------------------------------------------
Result Swapchain::CreateApiObjects(const grfx::SwapchainCreateInfo* pCreateInfo)
{
    if (IsHeadless()) {
        return ppx::SUCCESS;
    }

    std::vector<ID3D12Resource*> colorImages;

#if defined(PPX_BUILD_XR)
    std::vector<ID3D12Resource*> depthImages;

    const bool isXREnabled = (mCreateInfo.pXrComponent != nullptr);
    if (isXREnabled) {
        const XrComponent& xrComponent = *mCreateInfo.pXrComponent;

        PPX_ASSERT_MSG(xrComponent.GetColorFormat() == pCreateInfo->colorFormat, "XR color format differs from requested swapchain format");

        XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.arraySize             = 1;
        info.mipCount              = 1;
        info.faceCount             = 1;
        info.format                = dx::ToDxgiFormat(xrComponent.GetColorFormat());
        info.width                 = xrComponent.GetWidth();
        info.height                = xrComponent.GetHeight();
        info.sampleCount           = xrComponent.GetSampleCount();
        info.usageFlags            = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        CHECK_XR_CALL(xrCreateSwapchain(xrComponent.GetSession(), &info, &mXrColorSwapchain));

        // Find out how many textures were generated for the swapchain.
        uint32_t imageCount = 0;
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrColorSwapchain, 0, &imageCount, nullptr));
        std::vector<XrSwapchainImageD3D12KHR> surfaceImages;
        surfaceImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrColorSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)surfaceImages.data()));
        for (uint32_t i = 0; i < imageCount; i++) {
            colorImages.push_back(surfaceImages[i].texture);
        }

        if (xrComponent.GetDepthFormat() != grfx::FORMAT_UNDEFINED && xrComponent.UsesDepthSwapchains()) {
            PPX_ASSERT_MSG(xrComponent.GetDepthFormat() == pCreateInfo->depthFormat, "XR depth format differs from requested swapchain format");

            XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
            info.arraySize             = 1;
            info.mipCount              = 1;
            info.faceCount             = 1;
            info.format                = dx::ToDxgiFormat(xrComponent.GetDepthFormat());
            info.width                 = xrComponent.GetWidth();
            info.height                = xrComponent.GetHeight();
            info.sampleCount           = xrComponent.GetSampleCount();
            info.usageFlags            = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            CHECK_XR_CALL(xrCreateSwapchain(xrComponent.GetSession(), &info, &mXrDepthSwapchain));

            imageCount = 0;
            CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrDepthSwapchain, 0, &imageCount, nullptr));
            std::vector<XrSwapchainImageD3D12KHR> swapchainDepthImages;
            swapchainDepthImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
            CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrDepthSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainDepthImages.data()));
            for (uint32_t i = 0; i < imageCount; i++) {
                depthImages.push_back(swapchainDepthImages[i].texture);
            }

            PPX_ASSERT_MSG(depthImages.size() == colorImages.size(), "XR depth and color swapchains have different number of images");
        }
    }
    else
#endif
    {
        DXGIFactoryPtr factory = ToApi(GetDevice()->GetInstance())->GetDxFactory();

        // For performance we'll use a flip-model swapchain. This limits formats
        // to the list below.
        bool        formatSupported = false;
        DXGI_FORMAT dxgiFormat      = dx::ToDxgiFormat(pCreateInfo->colorFormat);
        switch (dxgiFormat) {
            default: break;
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                formatSupported = true;
                break;
        }

        if (!formatSupported) {
            PPX_ASSERT_MSG(false, "unsupported swapchain format");
            return ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT;
        }

        // Make swapchain waitable
        mFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Set swapchain flags. We enable tearing if supported.
        {
            switch (pCreateInfo->presentMode) {
                default: {
                    return ppx::ERROR_GRFX_UNSUPPORTED_PRESENT_MODE;
                } break;

                case grfx::PRESENT_MODE_FIFO: {
                    mSyncInterval = 1;
                } break;
                case grfx::PRESENT_MODE_MAILBOX: {
                    mSyncInterval = 0;
                } break;
                case grfx::PRESENT_MODE_IMMEDIATE: {
                    mSyncInterval = 0;
                    mFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                } break;
            }

            if (mFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) {
                HRESULT hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &mTearingEnabled, sizeof(mTearingEnabled));
                if (!SUCCEEDED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }

                if (mTearingEnabled == FALSE) {
                    return ppx::ERROR_GRFX_UNSUPPORTED_PRESENT_MODE;
                }
            }
        }

        // DXGI_USAGE_UNORDERED_ACCESS is usually not supported so we'll lave it out for now.
        //
        DXGI_USAGE       bufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        DXGI_SWAP_EFFECT swapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        DXGI_SWAP_CHAIN_DESC1 dxDesc = {};
        dxDesc.Width                 = static_cast<UINT>(pCreateInfo->width);
        dxDesc.Height                = static_cast<UINT>(pCreateInfo->height);
        dxDesc.Format                = dxgiFormat;
        dxDesc.Stereo                = FALSE;
        dxDesc.SampleDesc.Count      = 1;
        dxDesc.SampleDesc.Quality    = 0;
        dxDesc.BufferUsage           = bufferUsage;
        dxDesc.BufferCount           = static_cast<UINT>(pCreateInfo->imageCount);
        dxDesc.Scaling               = DXGI_SCALING_NONE;
        dxDesc.SwapEffect            = swapEffect;
        dxDesc.AlphaMode             = DXGI_ALPHA_MODE_IGNORE;
        dxDesc.Flags                 = mFlags;

        D3D12CommandQueuePtr::InterfaceType* pCmdQueue = ToApi(pCreateInfo->pQueue)->GetDxQueue();
        CComPtr<IDXGISwapChain1>             dxgiSwapChain;
        HRESULT                              hr = factory->CreateSwapChainForHwnd(
            ToApi(pCreateInfo->pQueue)->GetDxQueue(),        // pDevice
            ToApi(pCreateInfo->pSurface)->GetWindowHandle(), // hWnd
            &dxDesc,                                         // pDesc
            nullptr,                                         // pFullscreenDesc
            nullptr,                                         // pRestrictToOutput
            &dxgiSwapChain);                                 // ppSwapChain
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "IDXGIFactory2::CreateSwapChainForHwnd failed");
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(DXGISwapChain, dxgiSwapChain.Get());

        hr = dxgiSwapChain.QueryInterface(&mSwapchain);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "failed casting to IDXGISwapChain");
            return ppx::ERROR_API_FAILURE;
        }

        mFrameLatencyWaitableObject = mSwapchain->GetFrameLatencyWaitableObject();
        if (IsNull(mFrameLatencyWaitableObject)) {
            PPX_ASSERT_MSG(false, "IDXGISwapChain2::GetFrameLatencyWaitableObject failed");
            return ppx::ERROR_API_FAILURE;
        }

        hr = mSwapchain->SetMaximumFrameLatency(1);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "IDXGISwapChain2::SetMaximumFrameLatency failed");
            return ppx::ERROR_API_FAILURE;
        }

        // Retrieve swapchain buffer pointers
        {
            // Query the buffers count in case the underlying swapchain modified the number of buffers created.
            DXGI_SWAP_CHAIN_DESC dxDesc = {};
            HRESULT              hr     = mSwapchain->GetDesc(&dxDesc);
            if (FAILED(hr)) {
                PPX_ASSERT_MSG(false, "IDXGISwapChain::GetDesc failed");
                return ppx::ERROR_API_FAILURE;
            }

            // Grab the texture resources created by the swapchain.
            for (UINT i = 0; i < dxDesc.BufferCount; ++i) {
                D3D12ResourcePtr resource;
                HRESULT          hr = mSwapchain->GetBuffer(i, IID_PPV_ARGS(&resource));
                if (FAILED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }
                colorImages.push_back(resource);
            }
        }
    }

    // Create color images
    {
        std::vector<void*> apiObjects;
        for (const auto& colorImage : colorImages) {
            apiObjects.push_back(colorImage);
        }
        auto ppxres = CreateColorImagesFromApiObjects(apiObjects);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

#if defined(PPX_BUILD_XR)
    if (!depthImages.empty()) {
        std::vector<void*> apiObjects;
        for (const auto& depthImage : depthImages) {
            apiObjects.push_back(depthImage);
        }
        CreateDepthImagesFromApiObjects(apiObjects);
    }
#endif

    // Save queue for later use
    mQueue = ToApi(pCreateInfo->pQueue)->GetDxQueue();

    return ppx::SUCCESS;
}

void Swapchain::DestroyApiObjects()
{
    mFrameLatencyWaitableObject = nullptr;

    if (mSwapchain) {
        mSwapchain.Reset();
    }

    if (mQueue) {
        mQueue.Reset();
    }
}

Result Swapchain::AcquireNextImageInternal(
    uint64_t         timeout,
    grfx::Semaphore* pSemaphore,
    grfx::Fence*     pFence,
    uint32_t*        pImageIndex)
{
    // Wait on swapchain
    {
        DWORD millis = UINT32_MAX;
        DWORD result = WaitForSingleObjectEx(mFrameLatencyWaitableObject, millis, TRUE);

        // Confirmed timeout
        if (result == WAIT_TIMEOUT) {
            return ppx::ERROR_WAIT_TIMED_OUT;
        }
        // Confirmed fail
        if (result == WAIT_FAILED) {
            return ppx::ERROR_WAIT_FAILED;
        }
        // General fail
        if (result != WAIT_OBJECT_0) {
            return ppx::ERROR_WAIT_FAILED;
        }
    }

    // Get next buffer index
    *pImageIndex = static_cast<uint32_t>(mSwapchain->GetCurrentBackBufferIndex());

    // Signal semaphore
    if (!IsNull(pSemaphore)) {
        UINT64  value = ToApi(pSemaphore)->GetNextSignalValue();
        HRESULT hr    = mQueue->Signal(ToApi(pSemaphore)->GetDxFence(), value);
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Signal fence
    if (!IsNull(pFence)) {
        UINT64  value = ToApi(pFence)->GetNextSignalValue();
        HRESULT hr    = mQueue->Signal(ToApi(pFence)->GetDxFence(), value);
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }
    }

    mCurrentImageIndex = *pImageIndex;

    return ppx::SUCCESS;
}

Result Swapchain::PresentInternal(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    for (uint32_t i = 0; i < waitSemaphoreCount; ++i) {
        ID3D12Fence* pDxFence = ToApi(ppWaitSemaphores[i])->GetDxFence();
        UINT64       value    = ToApi(ppWaitSemaphores[i])->GetWaitForValue();
        HRESULT      hr       = mQueue->Wait(pDxFence, value);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12CommandQueue::Wait failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    UINT    flags = mTearingEnabled != FALSE ? DXGI_PRESENT_ALLOW_TEARING : 0;
    HRESULT hr    = mSwapchain->Present(mSyncInterval, flags);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "IDXGISwapChain::Present failed");
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result Swapchain::Resize(uint32_t width, uint32_t height)
{
    mCreateInfo.width  = width;
    mCreateInfo.height = height;

    // Destroy these to make sure there's no reference before resizing
    DestroyRenderPasses();
    DestroyRenderTargetViews();
    DestroyDepthImages();
    DestroyColorImages();

    // Resize buffers
    HRESULT hr = mSwapchain->ResizeBuffers(
        0,                         // 0 preserves the existing buffer count
        static_cast<UINT>(width),  // New width
        static_cast<UINT>(height), // New height
        DXGI_FORMAT_UNKNOWN,       // DXGI_FORMAT_UNKNOWN preserves the existing format of buffer
        mFlags);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    // Create color images
    {
        // Get buffer count
        DXGI_SWAP_CHAIN_DESC desc = {};
        HRESULT              hr   = mSwapchain->GetDesc(&desc);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "IDXGISwapChain::GetDesc failed");
            return ppx::ERROR_API_FAILURE;
        }

        // Grab the texture resources created by the swapchain.
        std::vector<void*> apiObjects;
        for (UINT i = 0; i < desc.BufferCount; ++i) {
            D3D12ResourcePtr resource;
            HRESULT          hr = mSwapchain->GetBuffer(i, IID_PPV_ARGS(&resource));
            if (FAILED(hr)) {
                return ppx::ERROR_API_FAILURE;
            }
            apiObjects.push_back(resource);
        }

        Result ppxres = CreateColorImagesFromApiObjects(apiObjects);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Create depth images
    Result ppxres = CreateDepthImages();
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Create render targets
    ppxres = CreateRenderTargetViews();
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Create render passes
    ppxres = CreateRenderPasses();
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
