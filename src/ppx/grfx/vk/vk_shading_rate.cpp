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

#include "ppx/grfx/vk/vk_shading_rate.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_image.h"
#include "ppx/grfx/grfx_device.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

Result ShadingRatePattern::CreateApiObjects(const ShadingRatePatternCreateInfo* pCreateInfo)
{
    mShadingRateMode         = pCreateInfo->shadingRateMode;
    const auto& capabilities = GetDevice()->GetShadingRateCapabilities();

    grfx::ImageCreateInfo imageCreateInfo       = {};
    imageCreateInfo.usageFlags.bits.transferDst = true;
    imageCreateInfo.ownership                   = OWNERSHIP_EXCLUSIVE;
    Extent2D minTexelSize, maxTexelSize;
    switch (pCreateInfo->shadingRateMode) {
        case SHADING_RATE_FDM: {
            minTexelSize                                       = capabilities.fdm.minTexelSize;
            maxTexelSize                                       = capabilities.fdm.maxTexelSize;
            imageCreateInfo.format                             = FORMAT_R8G8_UNORM;
            imageCreateInfo.usageFlags.bits.fragmentDensityMap = true;
            imageCreateInfo.initialState                       = RESOURCE_STATE_FRAGMENT_DENSITY_MAP_ATTACHMENT;
        } break;
        case SHADING_RATE_VRS: {
            minTexelSize                                                  = capabilities.vrs.minTexelSize;
            maxTexelSize                                                  = capabilities.vrs.maxTexelSize;
            imageCreateInfo.format                                        = FORMAT_R8_UNORM;
            imageCreateInfo.usageFlags.bits.fragmentShadingRateAttachment = true;
            imageCreateInfo.initialState                                  = RESOURCE_STATE_FRAGMENT_SHADING_RATE_ATTACHMENT;
        } break;
        default:
            PPX_ASSERT_MSG(false, "Cannot create ShadingRatePattern for ShadingRateMode " << pCreateInfo->shadingRateMode);
            return ppx::ERROR_FAILED;
    }

    if (pCreateInfo->texelSize.width == 0 && pCreateInfo->texelSize.height == 0) {
        mTexelSize = minTexelSize;
    }
    else {
        mTexelSize = pCreateInfo->texelSize;
    }

    PPX_ASSERT_MSG(
        mTexelSize.width >= minTexelSize.width,
        "Texel width (" << mTexelSize.width << ") must be >= the minimum texel width from capabilities (" << minTexelSize.width << ")");
    PPX_ASSERT_MSG(
        mTexelSize.height >= minTexelSize.height,
        "Texel height (" << mTexelSize.height << ") must be >= the minimum texel height from capabilities (" << minTexelSize.height << ")");
    PPX_ASSERT_MSG(
        mTexelSize.width <= maxTexelSize.width,
        "Texel width (" << mTexelSize.width << ") must be <= the maximum texel width from capabilities (" << maxTexelSize.width << ")");
    PPX_ASSERT_MSG(
        mTexelSize.height <= maxTexelSize.height,
        "Texel height (" << mTexelSize.height << ") must be <= the maximum texel height from capabilities (" << maxTexelSize.height << ")");

    imageCreateInfo.width  = (pCreateInfo->framebufferSize.width + mTexelSize.width - 1) / mTexelSize.width;
    imageCreateInfo.height = (pCreateInfo->framebufferSize.height + mTexelSize.height - 1) / mTexelSize.height;

    PPX_CHECKED_CALL(GetDevice()->CreateImage(&imageCreateInfo, &mAttachmentImage));

    VkImageViewCreateInfo vkci           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vkci.flags                           = 0;
    vkci.image                           = ToApi(mAttachmentImage)->GetVkImage();
    vkci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    vkci.format                          = ToVkFormat(imageCreateInfo.format);
    vkci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vkci.subresourceRange.baseMipLevel   = 0;
    vkci.subresourceRange.levelCount     = 1;
    vkci.subresourceRange.baseArrayLayer = 0;
    vkci.subresourceRange.layerCount     = 1;

    VkResult vkres = vk::CreateImageView(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mAttachmentView);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateImageView(ShadingRatePatternView) failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void ShadingRatePattern::DestroyApiObjects()
{
    if (mAttachmentView) {
        vkDestroyImageView(
            ToApi(GetDevice())->GetVkDevice(),
            mAttachmentView,
            nullptr);
        mAttachmentView.Reset();
    }
    if (mAttachmentImage) {
        GetDevice()->DestroyImage(mAttachmentImage);
        mAttachmentImage.Reset();
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
