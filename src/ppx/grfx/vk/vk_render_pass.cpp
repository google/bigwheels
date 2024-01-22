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

#include "ppx/grfx/vk/vk_render_pass.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_image.h"
#include "ppx/grfx/vk/vk_shading_rate.h"
#include "ppx/grfx/vk/vk_util.h"

#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

namespace ppx {
namespace grfx {
namespace vk {

Result RenderPass::CreateRenderPass(const grfx::internal::RenderPassCreateInfo* pCreateInfo)
{
    bool hasMultiView   = ToApi(GetDevice())->HasMultiView() ? true : false;
    bool hasDepthSencil = mDepthStencilView ? true : false;

    uint32_t      depthStencilAttachment = -1;
    size_t        rtvCount               = CountU32(mRenderTargetViews);
    VkImageLayout depthStencillayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Determine layout for depth/stencil
    {
        // These variables are not used for anything meaningful
        // in ToVkBarrierDst so they can be all zeroes.
        //
        VkPhysicalDeviceFeatures features   = {};
        VkPipelineStageFlags     stageMask  = 0;
        VkAccessFlags            accessMask = 0;

        Result ppxres = ToVkBarrierDst(pCreateInfo->depthStencilState, grfx::CommandType::COMMAND_TYPE_GRAPHICS, features, stageMask, accessMask, depthStencillayout);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed to determine layout for depth stencil state");
            return ppxres;
        }
    }

    // Attachment descriptions
    std::vector<VkAttachmentDescription> attachmentDescs;
    {
        for (uint32_t i = 0; i < rtvCount; ++i) {
            grfx::RenderTargetViewPtr rtv = mRenderTargetViews[i];

            VkAttachmentDescription desc = {};
            desc.flags                   = 0;
            desc.format                  = ToVkFormat(rtv->GetFormat());
            desc.samples                 = ToVkSampleCount(rtv->GetSampleCount());
            desc.loadOp                  = ToVkAttachmentLoadOp(rtv->GetLoadOp());
            desc.storeOp                 = ToVkAttachmentStoreOp(rtv->GetStoreOp());
            desc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            desc.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentDescs.push_back(desc);
        }

        if (hasDepthSencil) {
            grfx::DepthStencilViewPtr dsv = mDepthStencilView;

            VkAttachmentDescription desc = {};
            desc.flags                   = 0;
            desc.format                  = ToVkFormat(dsv->GetFormat());
            desc.samples                 = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp                  = ToVkAttachmentLoadOp(dsv->GetDepthLoadOp());
            desc.storeOp                 = ToVkAttachmentStoreOp(dsv->GetDepthStoreOp());
            desc.stencilLoadOp           = ToVkAttachmentLoadOp(dsv->GetStencilLoadOp());
            desc.stencilStoreOp          = ToVkAttachmentStoreOp(dsv->GetStencilStoreOp());
            desc.initialLayout           = depthStencillayout;
            desc.finalLayout             = depthStencillayout;

            depthStencilAttachment = attachmentDescs.size();
            attachmentDescs.push_back(desc);
        }
    }

    std::vector<VkAttachmentReference> colorRefs;
    {
        for (uint32_t i = 0; i < rtvCount; ++i) {
            VkAttachmentReference ref = {};
            ref.attachment            = i;
            ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs.push_back(ref);
        }
    }

    VkAttachmentReference depthStencilRef = {};
    if (hasDepthSencil) {
        depthStencilRef.attachment = depthStencilAttachment;
        depthStencilRef.layout     = depthStencillayout;
    }

    VkSubpassDescription subpassDescription    = {};
    subpassDescription.flags                   = 0;
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.colorAttachmentCount    = CountU32(colorRefs);
    subpassDescription.pColorAttachments       = DataPtr(colorRefs);
    subpassDescription.pResolveAttachments     = nullptr;
    subpassDescription.pDepthStencilAttachment = hasDepthSencil ? &depthStencilRef : nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;

    VkSubpassDependency subpassDependencies = {};
    subpassDependencies.srcSubpass          = VK_SUBPASS_EXTERNAL;
    subpassDependencies.dstSubpass          = 0;
    subpassDependencies.srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies.dstStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies.srcAccessMask       = 0;
    subpassDependencies.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependencies.dependencyFlags     = 0;

    VkRenderPassCreateInfo vkci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    vkci.flags                  = 0;
    vkci.attachmentCount        = CountU32(attachmentDescs);
    vkci.pAttachments           = DataPtr(attachmentDescs);
    vkci.subpassCount           = 1;
    vkci.pSubpasses             = &subpassDescription;
    vkci.dependencyCount        = 1;
    vkci.pDependencies          = &subpassDependencies;

    if (hasMultiView && pCreateInfo->multiViewMask > 0) {
        VkRenderPassMultiviewCreateInfo multiviewInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO};
        multiviewInfo.subpassCount                    = 1;
        multiviewInfo.pViewMasks                      = &pCreateInfo->multiViewMask;
        multiviewInfo.correlationMaskCount            = 1;
        multiviewInfo.pCorrelationMasks               = &pCreateInfo->multiCorrelationMask;

        vkci.pNext = &multiviewInfo;
    }

    if (!IsNull(pCreateInfo->pShadingRatePattern)) {
        auto     modifiedCreateInfo = ToApi(pCreateInfo->pShadingRatePattern)->GetModifiedRenderPassCreateInfo(vkci);
        VkResult vkres              = vk::CreateRenderPass(
            ToApi(GetDevice())->GetVkDevice(),
            modifiedCreateInfo.get(),
            nullptr,
            &mRenderPass);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkCreateRenderPass failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }
    else {
        VkResult vkres = vk::CreateRenderPass(
            ToApi(GetDevice())->GetVkDevice(),
            &vkci,
            nullptr,
            &mRenderPass);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkCreateRenderPass failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

Result RenderPass::CreateFramebuffer(const grfx::internal::RenderPassCreateInfo* pCreateInfo)
{
    bool hasDepthSencil = mDepthStencilView ? true : false;

    size_t rtvCount = CountU32(mRenderTargetViews);

    std::vector<VkImageView> attachments;
    for (uint32_t i = 0; i < rtvCount; ++i) {
        grfx::RenderTargetViewPtr rtv = mRenderTargetViews[i];
        attachments.push_back(ToApi(rtv.Get())->GetVkImageView());
    }

    if (hasDepthSencil) {
        grfx::DepthStencilViewPtr dsv = mDepthStencilView;
        attachments.push_back(ToApi(dsv.Get())->GetVkImageView());
    }

    if (!IsNull(pCreateInfo->pShadingRatePattern)) {
        if (pCreateInfo->pShadingRatePattern->GetShadingRateMode() == grfx::SHADING_RATE_FDM) {
            if (rtvCount > 0) {
                // Check that all or none of the render targets and depth-stencil attachments are subsampled.
                bool subsampled = mRenderTargetViews[0]->GetImage()->GetCreateFlags().bits.subsampledFormat;
                if (!subsampled) {
                    PPX_ASSERT_MSG(GetDevice()->GetShadingRateCapabilities().fdm.supportsNonSubsampledImages, "Non-subsampled render target images with FDM shading rate are not supported.");
                }

                // This device does not support non-subsampled image attachments with FDM shading rate.
                // Check that all the attachments are subsampled.
                for (uint32_t i = 0; i < rtvCount; ++i) {
                    grfx::RenderTargetViewPtr rtv = mRenderTargetViews[i];
                    if (subsampled) {
                        PPX_ASSERT_MSG(rtv->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render target image 0 is subsampled, but render target " << i << " is not subsampled.");
                    }
                    else {
                        PPX_ASSERT_MSG(!rtv->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render target image 0 is not subsampled, but render target " << i << " is subsampled.");
                    }
                }
                if (hasDepthSencil) {
                    if (subsampled) {
                        PPX_ASSERT_MSG(mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render targets are subsampled, but depth-stencil image is not subsampled.");
                    }
                    else {
                        PPX_ASSERT_MSG(!mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render targets are subsampled, but depth-stencil image is not subsampled.");
                    }
                }
            }
            else if (hasDepthSencil && !mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat) {
                // No render targets, only depth/stencil which is not subsampled.
                PPX_ASSERT_MSG(GetDevice()->GetShadingRateCapabilities().fdm.supportsNonSubsampledImages, "Non-subsampled depth-stencil image with FDM shading rate are not supported.");
            }
        }
        attachments.push_back(ToApi(pCreateInfo->pShadingRatePattern)->GetAttachmentImageView());
    }
    VkFramebufferCreateInfo vkci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    vkci.flags                   = 0;
    vkci.renderPass              = mRenderPass;
    vkci.attachmentCount         = CountU32(attachments);
    vkci.pAttachments            = DataPtr(attachments);
    vkci.width                   = pCreateInfo->width;
    vkci.height                  = pCreateInfo->height;
    vkci.layers                  = 1;

    VkResult vkres = vkCreateFramebuffer(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mFramebuffer);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateFramebuffer failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result RenderPass::CreateApiObjects(const grfx::internal::RenderPassCreateInfo* pCreateInfo)
{
    Result ppxres = CreateRenderPass(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = CreateFramebuffer(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void RenderPass::DestroyApiObjects()
{
    if (mFramebuffer) {
        vkDestroyFramebuffer(
            ToApi(GetDevice())->GetVkDevice(),
            mFramebuffer,
            nullptr);
        mFramebuffer.Reset();
    }

    if (mRenderPass) {
        vkDestroyRenderPass(
            ToApi(GetDevice())->GetVkDevice(),
            mRenderPass,
            nullptr);
        mRenderPass.Reset();
    }
}

// -------------------------------------------------------------------------------------------------

VkResult CreateTransientRenderPass(
    vk::Device*           device,
    uint32_t              renderTargetCount,
    const VkFormat*       pRenderTargetFormats,
    VkFormat              depthStencilFormat,
    VkSampleCountFlagBits sampleCount,
    uint32_t              viewMask,
    uint32_t              correlationMask,
    VkRenderPass*         pRenderPass,
    grfx::ShadingRateMode shadingRateMode)
{
    bool hasDepthSencil = (depthStencilFormat != VK_FORMAT_UNDEFINED);

    uint32_t depthStencilAttachment = -1;

    std::vector<VkAttachmentDescription> attachmentDescs;
    {
        for (uint32_t i = 0; i < renderTargetCount; ++i) {
            VkAttachmentDescription desc = {};
            desc.flags                   = 0;
            desc.format                  = pRenderTargetFormats[i];
            desc.samples                 = sampleCount;
            desc.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDescs.push_back(desc);
        }

        if (hasDepthSencil) {
            VkAttachmentDescription desc = {};
            desc.flags                   = 0;
            desc.format                  = depthStencilFormat;
            desc.samples                 = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthStencilAttachment       = attachmentDescs.size();
            attachmentDescs.push_back(desc);
        }
    }

    std::vector<VkAttachmentReference> colorRefs;
    {
        for (uint32_t i = 0; i < renderTargetCount; ++i) {
            VkAttachmentReference ref = {};
            ref.attachment            = i;
            ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs.push_back(ref);
        }
    }

    VkAttachmentReference depthStencilRef = {};
    if (hasDepthSencil) {
        depthStencilRef.attachment = depthStencilAttachment;
        depthStencilRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDescription    = {};
    subpassDescription.flags                   = 0;
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount    = 0;
    subpassDescription.pInputAttachments       = nullptr;
    subpassDescription.colorAttachmentCount    = CountU32(colorRefs);
    subpassDescription.pColorAttachments       = DataPtr(colorRefs);
    subpassDescription.pResolveAttachments     = nullptr;
    subpassDescription.pDepthStencilAttachment = hasDepthSencil ? &depthStencilRef : nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;

    VkSubpassDependency subpassDependencies = {};
    subpassDependencies.srcSubpass          = VK_SUBPASS_EXTERNAL;
    subpassDependencies.dstSubpass          = 0;
    subpassDependencies.srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies.dstStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies.srcAccessMask       = 0;
    subpassDependencies.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependencies.dependencyFlags     = 0;

    VkRenderPassCreateInfo vkci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    vkci.flags                  = 0;
    vkci.attachmentCount        = CountU32(attachmentDescs);
    vkci.pAttachments           = DataPtr(attachmentDescs);
    vkci.subpassCount           = 1;
    vkci.pSubpasses             = &subpassDescription;
    vkci.dependencyCount        = 1;
    vkci.pDependencies          = &subpassDependencies;
    // Callers responsibiltiy to only set viewmask if it is required
    if (viewMask > 0) {
        VkRenderPassMultiviewCreateInfo multiviewInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO};
        multiviewInfo.subpassCount                    = 1;
        multiviewInfo.pViewMasks                      = &viewMask;
        multiviewInfo.correlationMaskCount            = 1;
        multiviewInfo.pCorrelationMasks               = &correlationMask;

        vkci.pNext = &multiviewInfo;
    }

    if (shadingRateMode != SHADING_RATE_NONE) {
        auto     modifiedCreateInfo = vk::ShadingRatePattern::GetModifiedRenderPassCreateInfo(device, shadingRateMode, vkci);
        VkResult vkres              = vk::CreateRenderPass(
            device->GetVkDevice(),
            modifiedCreateInfo.get(),
            nullptr,
            pRenderPass);
        if (vkres != VK_SUCCESS) {
            return vkres;
        }
    }
    else {
        VkResult vkres = vk::CreateRenderPass(
            device->GetVkDevice(),
            &vkci,
            nullptr,
            pRenderPass);
        if (vkres != VK_SUCCESS) {
            return vkres;
        }
    }

    return VK_SUCCESS;
}

} // namespace vk
} // namespace grfx
} // namespace ppx
