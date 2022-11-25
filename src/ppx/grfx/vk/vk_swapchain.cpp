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

#include "ppx/grfx/vk/vk_swapchain.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_instance.h"
#include "ppx/grfx/vk/vk_queue.h"
#include "ppx/grfx/vk/vk_sync.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// Surface
// -------------------------------------------------------------------------------------------------
Result Surface::CreateApiObjects(const grfx::SurfaceCreateInfo* pCreateInfo)
{
#if defined(PPX_GGP)
    VkStreamDescriptorSurfaceCreateInfoGGP vkci = {VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP};
    vkci.streamDescriptor                       = GgpStreamDescriptorConstants::kGgpPrimaryStreamDescriptor;

    PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP = reinterpret_cast<PFN_vkCreateStreamDescriptorSurfaceGGP>(
        vkGetInstanceProcAddr(ToApi(GetInstance())->GetVkInstance(), "vkCreateStreamDescriptorSurfaceGGP"));
    PPX_ASSERT_MSG(vkCreateStreamDescriptorSurfaceGGP != nullptr, "Error getting function vkCreateStreamDescriptorSurfaceGGP");

    VkResult vkres = vkCreateStreamDescriptorSurfaceGGP(
        ToApi(GetInstance())->GetVkInstance(),
        &vkci,
        nullptr,
        &mSurface);
#elif defined(PPX_LINUX_XCB)
    VkXcbSurfaceCreateInfoKHR vkci = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    vkci.connection                = pCreateInfo->connection;
    vkci.window                    = pCreateInfo->window;

    VkResult vkres = vkCreateXcbSurfaceKHR(
        ToApi(GetInstance())->GetVkInstance(),
        &vkci,
        nullptr,
        &mSurface);
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#elif defined(PPX_MSW)
    VkWin32SurfaceCreateInfoKHR vkci = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    vkci.hinstance                   = pCreateInfo->hinstance;
    vkci.hwnd                        = pCreateInfo->hwnd;

    VkResult vkres = vkCreateWin32SurfaceKHR(
        ToApi(GetInstance())->GetVkInstance(),
        &vkci,
        nullptr,
        &mSurface);
#endif

    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreate<Platform>Surface failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    // Gpu
    vk::Gpu* pGpu = ToApi(pCreateInfo->pGpu);

    // Surface capabilities
    vkres = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pGpu->GetVkGpu(), mSurface, &mCapabilities);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_INFO("Vulkan swapchain surface info");
    PPX_LOG_INFO("   "
                 << "minImageCount : " << mCapabilities.minImageCount);
    PPX_LOG_INFO("   "
                 << "maxImageCount : " << mCapabilities.maxImageCount);

    // Surface formats
    {
        uint32_t count = 0;

        vkres = vkGetPhysicalDeviceSurfaceFormatsKHR(pGpu->GetVkGpu(), mSurface, &count, nullptr);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceFormatsKHR(0) failed: " << ToString(vkres));
            DestroyApiObjects();
            return ppx::ERROR_API_FAILURE;
        }

        if (count > 0) {
            mSurfaceFormats.resize(count);

            vkres = vkGetPhysicalDeviceSurfaceFormatsKHR(pGpu->GetVkGpu(), mSurface, &count, mSurfaceFormats.data());
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceFormatsKHR(1) failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }
    }

    // Presentable queue families
    {
        uint32_t count = pGpu->GetQueueFamilyCount();
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < count; ++queueFamilyIndex) {
            VkBool32 supported = VK_FALSE;

            vkres = vkGetPhysicalDeviceSurfaceSupportKHR(pGpu->GetVkGpu(), queueFamilyIndex, mSurface, &supported);
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceSupportKHR failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
            if (supported == VK_TRUE) {
                mPresentableQueueFamilies.push_back(queueFamilyIndex);
            }
        }

        if (mPresentableQueueFamilies.empty()) {
            PPX_ASSERT_MSG(false, "no presentable queue family found");
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Present modes
    {
        uint32_t count = 0;

        vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(pGpu->GetVkGpu(), mSurface, &count, nullptr);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfacePresentModesKHR(0) failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }

        if (count > 0) {
            mPresentModes.resize(count);

            vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(pGpu->GetVkGpu(), mSurface, &count, mPresentModes.data());
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkGetPhysicalDeviceSurfacePresentModesKHR(1) failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }
    }

    return ppx::SUCCESS;
}

void Surface::DestroyApiObjects()
{
    if (mSurface) {
        vkDestroySurfaceKHR(
            ToApi(GetInstance())->GetVkInstance(),
            mSurface,
            nullptr);
        mSurface.Reset();
    }
}

uint32_t Surface::GetMinImageWidth() const
{
    return mCapabilities.minImageExtent.width;
}

uint32_t Surface::GetMinImageHeight() const
{
    return mCapabilities.minImageExtent.height;
}

uint32_t Surface::GetMinImageCount() const
{
    return mCapabilities.minImageCount;
}

uint32_t Surface::GetMaxImageWidth() const
{
    return mCapabilities.maxImageExtent.width;
}

uint32_t Surface::GetMaxImageHeight() const
{
    return mCapabilities.maxImageExtent.height;
}

uint32_t Surface::GetMaxImageCount() const
{
    return mCapabilities.maxImageCount;
}

// -------------------------------------------------------------------------------------------------
// Swapchain
// -------------------------------------------------------------------------------------------------
Result Swapchain::CreateApiObjects(const grfx::SwapchainCreateInfo* pCreateInfo)
{
    uint32_t             imageCount = 0;
    std::vector<VkImage> images;
    VkResult             vkres;

#if defined(PPX_BUILD_XR)
    const bool isXREnabled = (mCreateInfo.pXrComponent != nullptr);
    if (isXREnabled) {
        const XrComponent& xrComponent = *mCreateInfo.pXrComponent;

        XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.arraySize             = 1;
        info.mipCount              = 1;
        info.faceCount             = 1;
        info.format                = ToVkFormat(xrComponent.GetColorFormat());
        info.width                 = xrComponent.GetWidth();
        info.height                = xrComponent.GetHeight();
        info.sampleCount           = xrComponent.GetSampleCount();
        info.usageFlags            = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        CHECK_XR_CALL(xrCreateSwapchain(xrComponent.GetSession(), &info, &mXrColorSwapchain));

        // Find out how many textures were generated for the swapchain
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrColorSwapchain, 0, &imageCount, nullptr));
        images.resize(imageCount);
        std::vector<XrSwapchainImageVulkanKHR> surfaceImages;
        surfaceImages.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
        CHECK_XR_CALL(xrEnumerateSwapchainImages(mXrColorSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)surfaceImages.data()));
        for (uint32_t i = 0; i < imageCount; i++) {
            images[i] = surfaceImages[i].image;
        }
    }
    else
#endif
    {
        // Surface capabilities check
        {
            bool                            isImageCountValid = false;
            const VkSurfaceCapabilitiesKHR& caps              = ToApi(pCreateInfo->pSurface)->GetCapabilities();
            if (caps.maxImageCount > 0) {
                bool isInBoundsMin = (pCreateInfo->imageCount >= caps.minImageCount);
                bool isInBoundsMax = (pCreateInfo->imageCount <= caps.maxImageCount);
                isImageCountValid  = isInBoundsMin && isInBoundsMax;
            }
            else {
                isImageCountValid = (pCreateInfo->imageCount >= caps.minImageCount);
            }
            if (!isImageCountValid) {
                PPX_ASSERT_MSG(false, "Invalid swapchain image count");
                return ppx::ERROR_INVALID_CREATE_ARGUMENT;
            }

            // if (
            // if ((pCreateInfo->imageCount < caps.minImageCount) || (pCreateInfo->imageCount > caps.maxImageCount)) {
            //      PPX_ASSERT_MSG(false, "Invalid swapchain image count");
            //      return ppx::ERROR_INVALID_CREATE_ARGUMENT;
            //}
        }

        // Surface format
        VkSurfaceFormatKHR surfaceFormat = {};
        {
            VkFormat format = ToVkFormat(pCreateInfo->colorFormat);
            if (format == VK_FORMAT_UNDEFINED) {
                PPX_ASSERT_MSG(false, "Invalid swapchain format");
                return ppx::ERROR_INVALID_CREATE_ARGUMENT;
            }

            const std::vector<VkSurfaceFormatKHR>& surfaceFormats = ToApi(pCreateInfo->pSurface)->GetSurfaceFormats();
            auto                                   it             = std::find_if(
                std::begin(surfaceFormats),
                std::end(surfaceFormats),
                [format](const VkSurfaceFormatKHR& elem) -> bool {
                bool isMatch = (elem.format == format);
                return isMatch; });

            if (it == std::end(surfaceFormats)) {
                PPX_ASSERT_MSG(false, "Unsupported swapchain format");
                return ppx::ERROR_INVALID_CREATE_ARGUMENT;
            }

            surfaceFormat = *it;
        }

        // Present mode
        VkPresentModeKHR presentMode = ToVkPresentMode(pCreateInfo->presentMode);
        if (presentMode == ppx::InvalidValue<VkPresentModeKHR>()) {
            PPX_ASSERT_MSG(false, "Invalid swapchain present mode");
            return ppx::ERROR_INVALID_CREATE_ARGUMENT;
        }
        // Fall back if present mode isn't supported
        {
            uint32_t count = 0;
            VkResult vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(
                ToApi(GetDevice()->GetGpu())->GetVkGpu(),
                ToApi(pCreateInfo->pSurface)->GetVkSurface(),
                &count,
                nullptr);
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }

            std::vector<VkPresentModeKHR> presentModes(count);
            vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(
                ToApi(GetDevice()->GetGpu())->GetVkGpu(),
                ToApi(pCreateInfo->pSurface)->GetVkSurface(),
                &count,
                presentModes.data());
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }

            bool supported = false;
            for (uint32_t i = 0; i < count; ++i) {
                if (presentMode == presentModes[i]) {
                    supported = true;
                    break;
                }
            }
            if (!supported) {
                PPX_LOG_WARN("Switching Vulkan present mode to VK_PRESENT_MODE_FIFO_KHR because " << ToString(presentMode) << " is not supported");
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
        }

        // Image usage
        //
        // NOTE: D3D12 support for DXGI_USAGE_UNORDERED_ACCESS is pretty spotty
        //       so we'll leave out VK_IMAGE_USAGE_STORAGE_BIT for now to
        //       keep the swwapchains between D3D12 and Vulkan as equivalent as
        //       possible.
        //
        VkImageUsageFlags usageFlags =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Create swapchain
        VkSwapchainCreateInfoKHR vkci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        vkci.pNext                    = nullptr;
        vkci.flags                    = 0;
        vkci.surface                  = ToApi(pCreateInfo->pSurface)->GetVkSurface();
        vkci.minImageCount            = pCreateInfo->imageCount;
        vkci.imageFormat              = surfaceFormat.format;
        vkci.imageColorSpace          = surfaceFormat.colorSpace;
        vkci.imageExtent              = {pCreateInfo->width, pCreateInfo->height};
        vkci.imageArrayLayers         = 1;
        vkci.imageUsage               = usageFlags;
        vkci.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
        vkci.queueFamilyIndexCount    = 0;
        vkci.pQueueFamilyIndices      = nullptr;
        vkci.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        vkci.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vkci.presentMode              = presentMode;
        vkci.clipped                  = VK_FALSE;
        vkci.oldSwapchain             = VK_NULL_HANDLE;

        vkres = vkCreateSwapchainKHR(
            ToApi(GetDevice())->GetVkDevice(),
            &vkci,
            nullptr,
            &mSwapchain);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }

        vkres = vkGetSwapchainImagesKHR(ToApi(GetDevice())->GetVkDevice(), mSwapchain, &imageCount, nullptr);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkGetSwapchainImagesKHR(0) failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_INFO("Vulkan swapchain image count: " << imageCount);

        if (imageCount > 0) {
            images.resize(imageCount);
            vkres = vkGetSwapchainImagesKHR(ToApi(GetDevice())->GetVkDevice(), mSwapchain, &imageCount, images.data());
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vkGetSwapchainImagesKHR(1) failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }
    }

    // Transition images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    {
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#if defined(PPX_BUILD_XR)
        // We do not present for XR render targets
        if (isXREnabled) {
            newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
#endif
        vk::Queue* pQueue = ToApi(pCreateInfo->pQueue);
        for (uint32_t i = 0; i < imageCount; ++i) {
            vkres = pQueue->TransitionImageLayout(
                images[i],                          // image
                VK_IMAGE_ASPECT_COLOR_BIT,          // aspectMask
                0,                                  // baseMipLevel
                1,                                  // levelCount
                0,                                  // baseArrayLayer
                1,                                  // layerCount
                VK_IMAGE_LAYOUT_UNDEFINED,          // oldLayout
                newLayout,                          // newLayout
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT); // newPipelineStage)
            if (vkres != VK_SUCCESS) {
                PPX_ASSERT_MSG(false, "vk::Queue::TransitionImageLayout failed: " << ToString(vkres));
                return ppx::ERROR_API_FAILURE;
            }
        }
    }

    // Create images
    for (uint32_t i = 0; i < imageCount; ++i) {
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

    // Save queue for presentation
    mQueue = ToApi(pCreateInfo->pQueue)->GetVkQueue();

    // Success
    return ppx::SUCCESS;
}

void Swapchain::DestroyApiObjects()
{
    if (mSwapchain) {
        vkDestroySwapchainKHR(
            ToApi(GetDevice())->GetVkDevice(),
            mSwapchain,
            nullptr);
        mSwapchain.Reset();
    }
}

Result Swapchain::AcquireNextImageInternal(
    uint64_t         timeout,
    grfx::Semaphore* pSemaphore,
    grfx::Fence*     pFence,
    uint32_t*        pImageIndex)
{
    VkSemaphore semaphore = VK_NULL_HANDLE;
    if (!IsNull(pSemaphore)) {
        semaphore = ToApi(pSemaphore)->GetVkSemaphore();
    }

    VkFence fence = VK_NULL_HANDLE;
    if (!IsNull(pFence)) {
        fence = ToApi(pFence)->GetVkFence();
    }

    VkResult vkres = vkAcquireNextImageKHR(
        ToApi(GetDevice())->GetVkDevice(),
        mSwapchain,
        timeout,
        semaphore,
        fence,
        pImageIndex);

    // Handle failure cases
    if (vkres < VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkAcquireNextImageKHR failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }
    // Handle warning cases
    if (vkres > VK_SUCCESS) {
        PPX_LOG_WARN("vkAcquireNextImageKHR returned: " << ToString(vkres));
    }

    currentImageIndex = *pImageIndex;
    return ppx::SUCCESS;
}

Result Swapchain::Present(
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    std::vector<VkSemaphore> semaphores;
    for (uint32_t i = 0; i < waitSemaphoreCount; ++i) {
        VkSemaphore semaphore = ToApi(ppWaitSemaphores[i])->GetVkSemaphore();
        semaphores.push_back(semaphore);
    }

    VkPresentInfoKHR vkpi   = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    vkpi.waitSemaphoreCount = CountU32(semaphores);
    vkpi.pWaitSemaphores    = DataPtr(semaphores);
    vkpi.swapchainCount     = 1;
    vkpi.pSwapchains        = mSwapchain;
    vkpi.pImageIndices      = &imageIndex;
    vkpi.pResults           = nullptr;

    VkResult vkres = vk::QueuePresent(
        mQueue,
        &vkpi);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

} // namespace vk
} // namespace grfx
} // namespace ppx
