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

#include "ppx/grfx/dx11/dx11_swapchain.h"
#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_instance.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// Surface
// -------------------------------------------------------------------------------------------------
Result Surface::CreateApiObjects(const grfx::SurfaceCreateInfo* pCreateInfo)
{
    if (pCreateInfo->hwnd == nullptr) {
        PPX_ASSERT_MSG(false, "window handle is null");
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // D3D11 doesn't have a surface the only thing to do is copy the window handle
    mWindowHandle = pCreateInfo->hwnd;

    return ppx::SUCCESS;
}

void Surface::DestroyApiObjects()
{
}

uint32_t Surface::GetMinImageWidth() const
{
    return 0;
}

uint32_t Surface::GetMinImageHeight() const
{
    return 0;
}

uint32_t Surface::GetMinImageCount() const
{
    return 1;
}

uint32_t Surface::GetMaxImageWidth() const
{
    return 65536;
}

uint32_t Surface::GetMaxImageHeight() const
{
    return 65536;
}

uint32_t Surface::GetMaxImageCount() const
{
    return DXGI_MAX_SWAP_CHAIN_BUFFERS;
}

// -------------------------------------------------------------------------------------------------
// Swapchain
// -------------------------------------------------------------------------------------------------
Result Swapchain::CreateApiObjects(const grfx::SwapchainCreateInfo* pCreateInfo)
{
    std::vector<ID3D11Resource*> images;

#if defined(PPX_BUILD_XR)
    const bool isXREnabled = (mCreateInfo.pXrComponent != nullptr);
    if (isXREnabled) {
        const XrComponent& xrComponent = *mCreateInfo.pXrComponent;

        XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.arraySize             = 1;
        info.mipCount              = 1;
        info.faceCount             = 1;
        info.format                = dx::ToDxgiFormat(xrComponent.GetColorFormat());
        info.width                 = xrComponent.GetWidth();
        info.height                = xrComponent.GetHeight();
        info.sampleCount           = xrComponent.GetSampleCount();
        info.usageFlags            = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        CHECK_XR_CALL(xrCreateSwapchain(xrComponent.GetSession(), &info, &mXrSwapchain));

        // Find out how many textures were generated for the swapchain.
        uint32_t imageCount = 0;
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrSwapchain, 0, &imageCount, nullptr));
        std::vector<XrSwapchainImageD3D11KHR> surfaceImages;
        surfaceImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)surfaceImages.data()));
        for (uint32_t i = 0; i < imageCount; i++) {
            images.push_back(surfaceImages[i].texture);
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

        // Present mode behavior
        UINT flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        // Set flag
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
                    flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                } break;
            }

            if (flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) {
                BOOL    allowTearing = FALSE;
                HRESULT hr           = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
                if (!SUCCEEDED(hr)) {
                    return ppx::ERROR_API_FAILURE;
                }

                if (allowTearing == FALSE) {
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
        dxDesc.Format                = dx::ToDxgiFormat(pCreateInfo->colorFormat);
        dxDesc.Stereo                = FALSE;
        dxDesc.SampleDesc.Count      = 1;
        dxDesc.SampleDesc.Quality    = 0;
        dxDesc.BufferUsage           = bufferUsage;
        dxDesc.BufferCount           = static_cast<UINT>(pCreateInfo->imageCount);
        dxDesc.Scaling               = DXGI_SCALING_NONE;
        dxDesc.SwapEffect            = swapEffect;
        dxDesc.AlphaMode             = DXGI_ALPHA_MODE_IGNORE;
        dxDesc.Flags                 = flags;

        ComPtr<IDXGISwapChain1> dxgiSwapChain;
        HRESULT                 hr = factory->CreateSwapChainForHwnd(
            ToApi(GetDevice())->GetDxDevice(),               // pDevice
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

        hr = dxgiSwapChain.As(&mSwapchain);
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

        // Store the texture resources created by the swapchain.
        // Query the image count again in case the underlying swapchain
        // modified the number of images created.
        {
            DXGI_SWAP_CHAIN_DESC dxDesc = {};
            HRESULT              hr     = mSwapchain->GetDesc(&dxDesc);
            if (FAILED(hr)) {
                PPX_ASSERT_MSG(false, "IDXGISwapChain::GetDesc failed");
                return ppx::ERROR_API_FAILURE;
            }

            // In Direct3D 11, applications could call GetBuffer(0, ...) only once. Every call to
            // Present() implicitly changes the resource identity of the returned interface.
            // See: https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements
            D3D11Texture2DPtr surface;
            hr = mSwapchain->GetBuffer(0, __uuidof(typename D3D11Texture2DPtr::InterfaceType), &surface);
            if (FAILED(hr)) {
                return ppx::ERROR_API_FAILURE;
            }

            for (UINT i = 0; i < dxDesc.BufferCount; ++i) {
                images.push_back(surface.Get());
            }
        }
    }

    // Create images
    {
        for (size_t i = 0; i < images.size(); ++i) {
            grfx::ImageCreateInfo imageCreateInfo           = {};
            imageCreateInfo.type                            = grfx::IMAGE_TYPE_2D;
            imageCreateInfo.width                           = pCreateInfo->width;
            imageCreateInfo.height                          = pCreateInfo->height;
            imageCreateInfo.depth                           = 1;
            imageCreateInfo.format                          = pCreateInfo->colorFormat;
            imageCreateInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
            imageCreateInfo.mipLevelCount                   = 1;
            imageCreateInfo.arrayLayerCount                 = 1;
            imageCreateInfo.usageFlags.bits.transferSrc     = true;
            imageCreateInfo.usageFlags.bits.transferDst     = true;
            imageCreateInfo.usageFlags.bits.sampled         = true;
            imageCreateInfo.usageFlags.bits.storage         = true;
            imageCreateInfo.usageFlags.bits.colorAttachment = true;
            imageCreateInfo.pApiObject                      = images[i];

            grfx::ImagePtr image;
            Result         ppxres = GetDevice()->CreateImage(&imageCreateInfo, &image);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "image create failed");
                return ppxres;
            }

            mColorImages.push_back(image);
        }
    }

    return ppx::SUCCESS;
}

void Swapchain::DestroyApiObjects()
{
    mFrameLatencyWaitableObject = nullptr;

    if (mSwapchain) {
        mSwapchain->Release();
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
    //// Wait on swapchain
    //{
    //    DWORD millis = UINT32_MAX;
    //    DWORD result = WaitForSingleObjectEx(mFrameLatencyWaitableObject, millis, TRUE);
    //    // Confirmed timeout
    //    if (result == WAIT_TIMEOUT) {
    //        return ppx::ERROR_WAIT_TIMED_OUT;
    //    }
    //    // Confirmed fail
    //    if (result == WAIT_FAILED) {
    //        return ppx::ERROR_WAIT_FAILED;
    //    }
    //    // General fail
    //    if (result != WAIT_OBJECT_0) {
    //        return ppx::ERROR_WAIT_FAILED;
    //    }
    //}
    //
    //// Get next buffer index
    //*pImageIndex = static_cast<uint32_t>(mSwapchain->GetCurrentBackBufferIndex());
    //
    //// Signal semaphore
    //if (!IsNull(pSemaphore)) {
    //    UINT64  value = ToApi(pSemaphore)->GetNextSignalValue();
    //    HRESULT hr    = mQueue->Signal(ToApi(pSemaphore)->GetDxFence(), value);
    //    if (FAILED(hr)) {
    //        return ppx::ERROR_API_FAILURE;
    //    }
    //}
    //
    //// Signal fence
    //if (!IsNull(pFence)) {
    //    UINT64  value = ToApi(pFence)->GetNextSignalValue();
    //    HRESULT hr    = mQueue->Signal(ToApi(pFence)->GetDxFence(), value);
    //    if (FAILED(hr)) {
    //        return ppx::ERROR_API_FAILURE;
    //    }
    //}
    //
    //return ppx::SUCCESS;

    *pImageIndex = static_cast<uint32_t>(mPresentCount % mCreateInfo.imageCount);

    currentImageIndex = *pImageIndex;
    return ppx::SUCCESS;
}

Result Swapchain::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    //for (uint32_t i = 0; i < waitSemaphoreCount; ++i) {
    //    ID3D12Fence* pDxFence = ToApi(ppWaitSemaphores[i])->GetDxFence();
    //    UINT64       value    = ToApi(ppWaitSemaphores[i])->GetWaitForValue();
    //    HRESULT      hr       = mQueue->Wait(pDxFence, value);
    //    if (FAILED(hr)) {
    //        PPX_ASSERT_MSG(false, "ID3D12CommandQueue::Wait failed");
    //        return ppx::ERROR_API_FAILURE;
    //    }
    //}

    UINT    flags = 0;
    HRESULT hr    = mSwapchain->Present(mSyncInterval, flags);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "IDXGISwapChain::Present failed");
        return ppx::ERROR_API_FAILURE;
    }

    ++mPresentCount;

    return ppx::SUCCESS;
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
