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
            imageCreateInfo.format                                        = FORMAT_R8_UINT;
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
    imageCreateInfo.depth  = 1;

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

std::unique_ptr<ShadingRatePattern::RenderPassModifier> ShadingRatePattern::CreateRenderPassModifier(vk::Device* device, ShadingRateMode mode)
{
    switch (mode) {
        case SHADING_RATE_FDM:
            return std::make_unique<ShadingRatePattern::FDMRenderPassModifier>();
        case SHADING_RATE_VRS:
            return std::make_unique<ShadingRatePattern::VRSRenderPassModifier>(device->GetShadingRateCapabilities());
        default:
            return nullptr;
    }
}

VkAttachmentReference2 ToVkAttachmentReference2(const VkAttachmentReference& ref)
{
    VkAttachmentReference2 ref2 = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
    ref2.attachment             = ref.attachment;
    ref2.layout                 = ref.layout;
    return ref2;
}

std::vector<VkAttachmentReference2> ToVkAttachmentReference2(uint32_t referenceCount, const VkAttachmentReference* pReferences)
{
    std::vector<VkAttachmentReference2> references2(referenceCount);
    for (uint32_t i = 0; i < referenceCount; ++i) {
        references2[i] = ToVkAttachmentReference2(pReferences[i]);
    }
    return references2;
}

void ShadingRatePattern::RenderPassModifier::Initialize(const VkRenderPassCreateInfo& vkci)
{
    auto& vkci2 = mVkRenderPassCreateInfo2;
    vkci2.pNext = vkci.pNext;
    vkci2.flags = vkci.flags;

    mAttachments.clear();
    mAttachments.reserve(vkci.attachmentCount + 1);
    mAttachments.resize(vkci.attachmentCount);
    for (uint32_t i = 0; i < vkci.attachmentCount; ++i) {
        const VkAttachmentDescription& attachment  = vkci.pAttachments[i];
        VkAttachmentDescription2&      attachment2 = mAttachments[i];
        attachment2.sType                          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachment2.flags                          = attachment.flags;
        attachment2.format                         = attachment.format;
        attachment2.samples                        = attachment.samples;
        attachment2.loadOp                         = attachment.loadOp;
        attachment2.storeOp                        = attachment.storeOp;
        attachment2.stencilLoadOp                  = attachment.stencilLoadOp;
        attachment2.stencilStoreOp                 = attachment.stencilStoreOp;
        attachment2.initialLayout                  = attachment.initialLayout;
        attachment2.finalLayout                    = attachment.finalLayout;
    }
    vkci2.attachmentCount = CountU32(mAttachments);
    vkci2.pAttachments    = DataPtr(mAttachments);

    mSubpasses.clear();
    mSubpasses.resize(vkci.subpassCount);
    mSubpassAttachments.resize(vkci.subpassCount);
    for (uint32_t i = 0; i < vkci.subpassCount; ++i) {
        const VkSubpassDescription& subpass  = vkci.pSubpasses[i];
        VkSubpassDescription2&      subpass2 = mSubpasses[i];
        subpass2.sType                       = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
        subpass2.flags                       = subpass.flags;
        subpass2.pipelineBindPoint           = subpass.pipelineBindPoint;

        auto& inputAttachments        = mSubpassAttachments[i].inputAttachments;
        inputAttachments              = ToVkAttachmentReference2(subpass.inputAttachmentCount, subpass.pInputAttachments);
        subpass2.inputAttachmentCount = CountU32(inputAttachments);
        subpass2.pInputAttachments    = DataPtr(inputAttachments);

        auto& colorAttachments        = mSubpassAttachments[i].colorAttachments;
        colorAttachments              = ToVkAttachmentReference2(subpass.colorAttachmentCount, subpass.pColorAttachments);
        subpass2.colorAttachmentCount = CountU32(colorAttachments);
        subpass2.pColorAttachments    = DataPtr(colorAttachments);

        if (!IsNull(subpass.pResolveAttachments)) {
            auto& resolveAttachments     = mSubpassAttachments[i].resolveAttachments;
            resolveAttachments           = ToVkAttachmentReference2(subpass.colorAttachmentCount, subpass.pResolveAttachments);
            subpass2.pResolveAttachments = DataPtr(resolveAttachments);
        }

        if (!IsNull(subpass.pDepthStencilAttachment)) {
            auto& depthStencilAttachment     = mSubpassAttachments[i].depthStencilAttachment;
            depthStencilAttachment           = ToVkAttachmentReference2(*subpass.pDepthStencilAttachment);
            subpass2.pDepthStencilAttachment = &depthStencilAttachment;
        }

        auto& preserveAttachments = mSubpassAttachments[i].preserveAttachments;
        preserveAttachments.resize(subpass.preserveAttachmentCount);
        std::copy_n(subpass.pPreserveAttachments, subpass.preserveAttachmentCount, preserveAttachments.begin());
        subpass2.preserveAttachmentCount = CountU32(preserveAttachments);
        subpass2.pPreserveAttachments    = DataPtr(preserveAttachments);
    }
    vkci2.subpassCount = CountU32(mSubpasses);
    vkci2.pSubpasses   = DataPtr(mSubpasses);

    mDependencies.clear();
    mDependencies.resize(vkci.dependencyCount);
    for (uint32_t i = 0; i < vkci.dependencyCount; ++i) {
        const VkSubpassDependency& dependency  = vkci.pDependencies[i];
        VkSubpassDependency2&      dependency2 = mDependencies[i];
        dependency2.sType                      = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        dependency2.srcSubpass                 = dependency.srcSubpass;
        dependency2.dstSubpass                 = dependency.dstSubpass;
        dependency2.srcStageMask               = dependency.srcStageMask;
        dependency2.dstStageMask               = dependency.dstStageMask;
        dependency2.srcAccessMask              = dependency.srcAccessMask;
        dependency2.dstAccessMask              = dependency.dstAccessMask;
        dependency2.dependencyFlags            = dependency.dependencyFlags;
    }
    vkci2.dependencyCount = CountU32(mDependencies);
    vkci2.pDependencies   = DataPtr(mDependencies);

    InitializeImpl();
}

void ShadingRatePattern::RenderPassModifier::Initialize(const VkRenderPassCreateInfo2& vkci)
{
    VkRenderPassCreateInfo2& vkci2 = mVkRenderPassCreateInfo2;
    vkci2                          = vkci;

    mAttachments.reserve(vkci.attachmentCount + 1);
    mAttachments.resize(vkci.attachmentCount);
    std::copy_n(vkci.pAttachments, vkci.attachmentCount, mAttachments.begin());
    vkci2.pAttachments = DataPtr(mAttachments);

    mSubpasses.resize(vkci.subpassCount);
    std::copy_n(vkci.pSubpasses, vkci.subpassCount, mSubpasses.begin());
    vkci2.pSubpasses = DataPtr(mSubpasses);

    mDependencies.resize(vkci.dependencyCount);
    std::copy_n(vkci.pDependencies, vkci.dependencyCount, mDependencies.begin());
    vkci2.pDependencies = DataPtr(mDependencies);

    InitializeImpl();
}

void ShadingRatePattern::FDMRenderPassModifier::InitializeImpl()
{
    VkRenderPassCreateInfo2& vkci = mVkRenderPassCreateInfo2;

    VkAttachmentDescription2 densityMapDesc = {VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2};
    densityMapDesc.flags                    = 0;
    densityMapDesc.format                   = VK_FORMAT_R8G8_UNORM;
    densityMapDesc.samples                  = VK_SAMPLE_COUNT_1_BIT;
    densityMapDesc.loadOp                   = VK_ATTACHMENT_LOAD_OP_LOAD;
    densityMapDesc.storeOp                  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    densityMapDesc.stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    densityMapDesc.stencilStoreOp           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    densityMapDesc.initialLayout            = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
    densityMapDesc.finalLayout              = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
    mAttachments.push_back(densityMapDesc);

    mFdmInfo.fragmentDensityMapAttachment.layout     = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
    mFdmInfo.fragmentDensityMapAttachment.attachment = mAttachments.size() - 1;

    InsertPNext(vkci, mFdmInfo);

    vkci.attachmentCount = CountU32(mAttachments);
    vkci.pAttachments    = DataPtr(mAttachments);
}

void ShadingRatePattern::VRSRenderPassModifier::InitializeImpl()
{
    VkRenderPassCreateInfo2& vkci = mVkRenderPassCreateInfo2;

    VkAttachmentDescription2 vrsAttachmentDesc = {VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2};
    vrsAttachmentDesc.format                   = VK_FORMAT_R8_UINT;
    vrsAttachmentDesc.samples                  = VK_SAMPLE_COUNT_1_BIT;
    vrsAttachmentDesc.loadOp                   = VK_ATTACHMENT_LOAD_OP_LOAD;
    vrsAttachmentDesc.storeOp                  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vrsAttachmentDesc.stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vrsAttachmentDesc.stencilStoreOp           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vrsAttachmentDesc.initialLayout            = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
    vrsAttachmentDesc.finalLayout              = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
    mAttachments.push_back(vrsAttachmentDesc);

    vkci.attachmentCount = CountU32(mAttachments);
    vkci.pAttachments    = DataPtr(mAttachments);

    mVrsAttachmentRef.attachment = mAttachments.size() - 1;
    mVrsAttachmentRef.layout     = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

    mVrsAttachmentInfo.pFragmentShadingRateAttachment = &mVrsAttachmentRef;
    mVrsAttachmentInfo.shadingRateAttachmentTexelSize = {
        mCapabilities.vrs.minTexelSize.width,
        mCapabilities.vrs.minTexelSize.height,
    };

    for (auto& subpass : mSubpasses) {
        InsertPNext(subpass, mVrsAttachmentInfo);
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
