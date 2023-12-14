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

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci)
{
    return CreateModifiedRenderPassCreateInfo()->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo2& vkci)
{
    return CreateModifiedRenderPassCreateInfo()->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode, const VkRenderPassCreateInfo& vkci)
{
    return CreateModifiedRenderPassCreateInfo(device, mode)->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode, const VkRenderPassCreateInfo2& vkci)
{
    return CreateModifiedRenderPassCreateInfo(device, mode)->Initialize(vkci).Get();
}

std::shared_ptr<ShadingRatePattern::ModifiedRenderPassCreateInfo> ShadingRatePattern::CreateModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode)
{
    switch (mode) {
        case SHADING_RATE_FDM:
            return std::make_shared<ShadingRatePattern::FDMModifiedRenderPassCreateInfo>();
        case SHADING_RATE_VRS:
            return std::make_shared<ShadingRatePattern::VRSModifiedRenderPassCreateInfo>(device->GetShadingRateCapabilities());
        default:
            return nullptr;
    }
}

ShadingRatePattern::ModifiedRenderPassCreateInfo& ShadingRatePattern::ModifiedRenderPassCreateInfo::Initialize(const VkRenderPassCreateInfo& vkci)
{
    LoadVkRenderPassCreateInfo(vkci);
    UpdateRenderPassForShadingRateImplementation();
    return *this;
}

ShadingRatePattern::ModifiedRenderPassCreateInfo& ShadingRatePattern::ModifiedRenderPassCreateInfo::Initialize(const VkRenderPassCreateInfo2& vkci)
{
    LoadVkRenderPassCreateInfo2(vkci);
    UpdateRenderPassForShadingRateImplementation();
    return *this;
}

namespace {

// Converts VkAttachmentDescription to VkAttachmentDescription2
VkAttachmentDescription2 ToVkAttachmentDescription2(const VkAttachmentDescription& attachment)
{
    VkAttachmentDescription2 attachment2 = {VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2};
    attachment2.flags                    = attachment.flags;
    attachment2.format                   = attachment.format;
    attachment2.samples                  = attachment.samples;
    attachment2.loadOp                   = attachment.loadOp;
    attachment2.storeOp                  = attachment.storeOp;
    attachment2.stencilLoadOp            = attachment.stencilLoadOp;
    attachment2.stencilStoreOp           = attachment.stencilStoreOp;
    attachment2.initialLayout            = attachment.initialLayout;
    attachment2.finalLayout              = attachment.finalLayout;
    return attachment2;
}

// Converts VkAttachmentReference to VkAttachmentReference2
VkAttachmentReference2 ToVkAttachmentReference2(const VkAttachmentReference& ref)
{
    VkAttachmentReference2 ref2 = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
    ref2.attachment             = ref.attachment;
    ref2.layout                 = ref.layout;
    return ref2;
}

// Converts VkSubpassDependency to VkSubpassDependency2
VkSubpassDependency2 ToVkSubpassDependency2(const VkSubpassDependency& dependency)
{
    VkSubpassDependency2 dependency2 = {VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2};
    dependency2.srcSubpass           = dependency.srcSubpass;
    dependency2.dstSubpass           = dependency.dstSubpass;
    dependency2.srcStageMask         = dependency.srcStageMask;
    dependency2.dstStageMask         = dependency.dstStageMask;
    dependency2.srcAccessMask        = dependency.srcAccessMask;
    dependency2.dstAccessMask        = dependency.dstAccessMask;
    dependency2.dependencyFlags      = dependency.dependencyFlags;
    return dependency2;
}

// Copy an array to a vector and initialize an output count and data pointer.
template <typename T>
void CopyArrayWithVectorStorage(uint32_t inCount, const T* inData, std::vector<T>& vec, uint32_t& outCount, T const*& outData)
{
    vec.clear();
    if (inCount == 0) {
        outCount = 0;
        outData  = nullptr;
    }
    vec.resize(inCount);
    std::copy_n(inData, inCount, vec.begin());
    outCount = CountU32(vec);
    outData  = DataPtr(vec);
}

// Convert the elements in an array, store in a vector, and initialize an output count and data pointer.
template <typename T1, typename T2, typename Conv>
void ConvertArrayWithVectorStorage(uint32_t inCount, const T1* inData, std::vector<T2>& vec, uint32_t& outCount, T2 const*& outData, Conv&& conv)
{
    vec.clear();
    if (inCount == 0) {
        outCount = 0;
        outData  = nullptr;
    }
    vec.resize(inCount);
    for (uint32_t i = 0; i < inCount; ++i) {
        vec[i] = conv(inData[i]);
    }
    outCount = CountU32(vec);
    outData  = DataPtr(vec);
}

} // namespace

void ShadingRatePattern::ModifiedRenderPassCreateInfo::LoadVkRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci)
{
    auto& vkci2 = mVkRenderPassCreateInfo2;
    vkci2.pNext = vkci.pNext;
    vkci2.flags = vkci.flags;

    ConvertArrayWithVectorStorage(vkci.attachmentCount, vkci.pAttachments, mAttachments, vkci2.attachmentCount, vkci2.pAttachments, &ToVkAttachmentDescription2);
    ConvertArrayWithVectorStorage(vkci.subpassCount, vkci.pSubpasses, mSubpasses, vkci2.subpassCount, vkci2.pSubpasses, [this](const VkSubpassDescription& subpass) {
        VkSubpassDescription2 subpass2 = {VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2};
        subpass2.flags                 = subpass.flags;
        subpass2.pipelineBindPoint     = subpass.pipelineBindPoint;

        auto& subpassAttachments = mSubpassAttachments.emplace_back();
        ConvertArrayWithVectorStorage(subpass.inputAttachmentCount, subpass.pInputAttachments, subpassAttachments.inputAttachments, subpass2.inputAttachmentCount, subpass2.pInputAttachments, &ToVkAttachmentReference2);
        ConvertArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pColorAttachments, subpassAttachments.colorAttachments, subpass2.colorAttachmentCount, subpass2.pColorAttachments, &ToVkAttachmentReference2);
        if (!IsNull(subpass.pResolveAttachments)) {
            uint32_t resolveAttachmentCount = 0;
            ConvertArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pResolveAttachments, subpassAttachments.resolveAttachments, resolveAttachmentCount, subpass2.pResolveAttachments, &ToVkAttachmentReference2);
        }
        if (!IsNull(subpass.pDepthStencilAttachment)) {
            auto& depthStencilAttachment     = subpassAttachments.depthStencilAttachment;
            depthStencilAttachment           = ToVkAttachmentReference2(*subpass.pDepthStencilAttachment);
            subpass2.pDepthStencilAttachment = &depthStencilAttachment;
        }
        CopyArrayWithVectorStorage(subpass.preserveAttachmentCount, subpass.pPreserveAttachments, subpassAttachments.preserveAttachments, subpass2.preserveAttachmentCount, subpass2.pPreserveAttachments);
        return subpass2;
    });
    ConvertArrayWithVectorStorage(vkci.dependencyCount, vkci.pDependencies, mDependencies, vkci2.dependencyCount, vkci2.pDependencies, &ToVkSubpassDependency2);
}

void ShadingRatePattern::ModifiedRenderPassCreateInfo::LoadVkRenderPassCreateInfo2(const VkRenderPassCreateInfo2& vkci)
{
    VkRenderPassCreateInfo2& vkci2 = mVkRenderPassCreateInfo2;
    vkci2                          = vkci;

    CopyArrayWithVectorStorage(vkci.attachmentCount, vkci.pAttachments, mAttachments, vkci2.attachmentCount, vkci2.pAttachments);
    ConvertArrayWithVectorStorage(vkci.subpassCount, vkci.pSubpasses, mSubpasses, vkci2.subpassCount, vkci2.pSubpasses, [this](const VkSubpassDescription2& subpass) {
        VkSubpassDescription2 subpass2 = subpass;

        auto& subpassAttachments = mSubpassAttachments.emplace_back();
        CopyArrayWithVectorStorage(subpass.inputAttachmentCount, subpass.pInputAttachments, subpassAttachments.inputAttachments, subpass2.inputAttachmentCount, subpass2.pInputAttachments);
        CopyArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pColorAttachments, subpassAttachments.colorAttachments, subpass2.colorAttachmentCount, subpass2.pColorAttachments);
        if (!IsNull(subpass.pResolveAttachments)) {
            uint32_t resolveAttachmentCount = 0;
            CopyArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pResolveAttachments, subpassAttachments.resolveAttachments, resolveAttachmentCount, subpass2.pResolveAttachments);
        }
        if (!IsNull(subpass.pDepthStencilAttachment)) {
            auto& depthStencilAttachment     = subpassAttachments.depthStencilAttachment;
            depthStencilAttachment           = *subpass.pDepthStencilAttachment;
            subpass2.pDepthStencilAttachment = &depthStencilAttachment;
        }
        CopyArrayWithVectorStorage(subpass.preserveAttachmentCount, subpass.pPreserveAttachments, subpassAttachments.preserveAttachments, subpass2.preserveAttachmentCount, subpass2.pPreserveAttachments);
        return subpass2;
    });
    CopyArrayWithVectorStorage(vkci.dependencyCount, vkci.pDependencies, mDependencies, vkci2.dependencyCount, vkci2.pDependencies);
}

void ShadingRatePattern::FDMModifiedRenderPassCreateInfo::UpdateRenderPassForShadingRateImplementation()
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

void ShadingRatePattern::VRSModifiedRenderPassCreateInfo::UpdateRenderPassForShadingRateImplementation()
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
