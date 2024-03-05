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

#include "ppx/grfx/vk/vk_pipeline.h"
#include "ppx/grfx/vk/vk_descriptor.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_render_pass.h"
#include "ppx/grfx/vk/vk_shader.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// ComputePipeline
// -------------------------------------------------------------------------------------------------
Result ComputePipeline::CreateApiObjects(const grfx::ComputePipelineCreateInfo* pCreateInfo)
{
    VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    ssci.flags                           = 0;
    ssci.pSpecializationInfo             = nullptr;
    ssci.pName                           = pCreateInfo->CS.entryPoint.c_str();
    ssci.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
    ssci.module                          = ToApi(pCreateInfo->CS.pModule)->GetVkShaderModule();

    VkComputePipelineCreateInfo vkci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    vkci.flags                       = 0;
    vkci.stage                       = ssci;
    vkci.layout                      = ToApi(pCreateInfo->pPipelineInterface)->GetVkPipelineLayout();
    vkci.basePipelineHandle          = VK_NULL_HANDLE;
    vkci.basePipelineIndex           = 0;

    VkResult vkres = vkCreateComputePipelines(
        ToApi(GetDevice())->GetVkDevice(),
        VK_NULL_HANDLE,
        1,
        &vkci,
        nullptr,
        &mPipeline);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateComputePipelines failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void ComputePipeline::DestroyApiObjects()
{
    if (mPipeline) {
        vkDestroyPipeline(ToApi(GetDevice())->GetVkDevice(), mPipeline, nullptr);
        mPipeline.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// GraphicsPipeline
// -------------------------------------------------------------------------------------------------
Result GraphicsPipeline::InitializeShaderStages(
    const grfx::GraphicsPipelineCreateInfo*       pCreateInfo,
    std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    VkGraphicsPipelineCreateInfo&                 vkCreateInfo)
{
    // VS
    if (!IsNull(pCreateInfo->VS.pModule)) {
        const vk::ShaderModule* pModule = ToApi(pCreateInfo->VS.pModule);

        VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ssci.flags                           = 0;
        ssci.pSpecializationInfo             = nullptr;
        ssci.pName                           = pCreateInfo->VS.entryPoint.c_str();
        ssci.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        ssci.module                          = pModule->GetVkShaderModule();
        shaderStages.push_back(ssci);
    }

    // HS
    if (!IsNull(pCreateInfo->HS.pModule)) {
        const vk::ShaderModule* pModule = ToApi(pCreateInfo->HS.pModule);

        VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ssci.flags                           = 0;
        ssci.pSpecializationInfo             = nullptr;
        ssci.pName                           = pCreateInfo->HS.entryPoint.c_str();
        ssci.stage                           = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        ssci.module                          = pModule->GetVkShaderModule();
        shaderStages.push_back(ssci);
    }

    // DS
    if (!IsNull(pCreateInfo->DS.pModule)) {
        const vk::ShaderModule* pModule = ToApi(pCreateInfo->DS.pModule);

        VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ssci.flags                           = 0;
        ssci.pSpecializationInfo             = nullptr;
        ssci.pName                           = pCreateInfo->DS.entryPoint.c_str();
        ssci.stage                           = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        ssci.module                          = pModule->GetVkShaderModule();
        shaderStages.push_back(ssci);
    }

    // GS
    if (!IsNull(pCreateInfo->GS.pModule)) {
        const vk::ShaderModule* pModule = ToApi(pCreateInfo->GS.pModule);

        VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ssci.flags                           = 0;
        ssci.pSpecializationInfo             = nullptr;
        ssci.pName                           = pCreateInfo->GS.entryPoint.c_str();
        ssci.stage                           = VK_SHADER_STAGE_GEOMETRY_BIT;
        ssci.module                          = pModule->GetVkShaderModule();
        shaderStages.push_back(ssci);
    }

    // PS
    if (!IsNull(pCreateInfo->PS.pModule)) {
        const vk::ShaderModule* pModule = ToApi(pCreateInfo->PS.pModule);

        VkPipelineShaderStageCreateInfo ssci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        ssci.flags                           = 0;
        ssci.pSpecializationInfo             = nullptr;
        ssci.pName                           = pCreateInfo->PS.entryPoint.c_str();
        ssci.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        ssci.module                          = pModule->GetVkShaderModule();
        shaderStages.push_back(ssci);
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeVertexInput(
    const grfx::GraphicsPipelineCreateInfo*         pCreateInfo,
    std::vector<VkVertexInputAttributeDescription>& vkAttributes,
    std::vector<VkVertexInputBindingDescription>&   vkBindings,
    VkPipelineVertexInputStateCreateInfo&           stateCreateInfo)
{
    // Fill out Vulkan attributes and bindings
    for (uint32_t bindingIndex = 0; bindingIndex < pCreateInfo->vertexInputState.bindingCount; ++bindingIndex) {
        const grfx::VertexBinding& binding = pCreateInfo->vertexInputState.bindings[bindingIndex];
        // Iterate each attribute in the binding
        const uint32_t attributeCount = binding.GetAttributeCount();
        for (uint32_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
            // This should be safe since there's no modifications to the index
            const grfx::VertexAttribute* pAttribute = nullptr;
            binding.GetAttribute(attributeIndex, &pAttribute);

            VkVertexInputAttributeDescription vkAttribute = {};
            vkAttribute.location                          = pAttribute->location;
            vkAttribute.binding                           = pAttribute->binding;
            vkAttribute.format                            = ToVkFormat(pAttribute->format);
            vkAttribute.offset                            = pAttribute->offset;
            vkAttributes.push_back(vkAttribute);
        }

        VkVertexInputBindingDescription vkBinding = {};
        vkBinding.binding                         = binding.GetBinding();
        vkBinding.stride                          = binding.GetStride();
        vkBinding.inputRate                       = ToVkVertexInputRate(binding.GetInputRate());
        vkBindings.push_back(vkBinding);
    }

    stateCreateInfo.flags                           = 0;
    stateCreateInfo.vertexBindingDescriptionCount   = CountU32(vkBindings);
    stateCreateInfo.pVertexBindingDescriptions      = DataPtr(vkBindings);
    stateCreateInfo.vertexAttributeDescriptionCount = CountU32(vkAttributes);
    stateCreateInfo.pVertexAttributeDescriptions    = DataPtr(vkAttributes);

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeInputAssembly(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    VkPipelineInputAssemblyStateCreateInfo& stateCreateInfo)
{
    stateCreateInfo.flags                  = 0;
    stateCreateInfo.topology               = ToVkPrimitiveTopology(pCreateInfo->inputAssemblyState.topology);
    stateCreateInfo.primitiveRestartEnable = pCreateInfo->inputAssemblyState.primitiveRestartEnable ? VK_TRUE : VK_FALSE;

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeTessellation(
    const grfx::GraphicsPipelineCreateInfo*               pCreateInfo,
    VkPipelineTessellationDomainOriginStateCreateInfoKHR& domainOriginStateCreateInfo,
    VkPipelineTessellationStateCreateInfo&                stateCreateInfo)
{
    domainOriginStateCreateInfo.domainOrigin = ToVkTessellationDomainOrigin(pCreateInfo->tessellationState.domainOrigin);

    stateCreateInfo.flags              = 0;
    stateCreateInfo.patchControlPoints = pCreateInfo->tessellationState.patchControlPoints;

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeViewports(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    VkPipelineViewportStateCreateInfo&      stateCreateInfo)
{
    stateCreateInfo.flags         = 0;
    stateCreateInfo.viewportCount = 1;
    stateCreateInfo.pViewports    = nullptr;
    stateCreateInfo.scissorCount  = 1;
    stateCreateInfo.pScissors     = nullptr;

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeRasterization(
    const grfx::GraphicsPipelineCreateInfo*             pCreateInfo,
    VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthClipStateCreateInfo,
    VkPipelineRasterizationStateCreateInfo&             stateCreateInfo)
{
    stateCreateInfo.flags                   = 0;
    stateCreateInfo.depthClampEnable        = pCreateInfo->rasterState.depthClampEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.rasterizerDiscardEnable = pCreateInfo->rasterState.rasterizeDiscardEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.polygonMode             = ToVkPolygonMode(pCreateInfo->rasterState.polygonMode);
    stateCreateInfo.cullMode                = ToVkCullMode(pCreateInfo->rasterState.cullMode);
    stateCreateInfo.frontFace               = ToVkFrontFace(pCreateInfo->rasterState.frontFace);
    stateCreateInfo.depthBiasEnable         = pCreateInfo->rasterState.depthBiasEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.depthBiasConstantFactor = pCreateInfo->rasterState.depthBiasConstantFactor;
    stateCreateInfo.depthBiasClamp          = pCreateInfo->rasterState.depthBiasClamp;
    stateCreateInfo.depthBiasSlopeFactor    = pCreateInfo->rasterState.depthBiasSlopeFactor;
    stateCreateInfo.lineWidth               = 1.0f;

    // Handle depth clip enable
    if (ToApi(GetDevice())->HasDepthClipEnabled()) {
        depthClipStateCreateInfo.flags           = 0;
        depthClipStateCreateInfo.depthClipEnable = pCreateInfo->rasterState.depthClipEnable;
        // Set pNext
        stateCreateInfo.pNext = &depthClipStateCreateInfo;
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeMultisample(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    VkPipelineMultisampleStateCreateInfo&   stateCreateInfo)
{
    stateCreateInfo.flags                 = 0;
    stateCreateInfo.rasterizationSamples  = ToVkSampleCount(pCreateInfo->rasterState.rasterizationSamples);
    stateCreateInfo.sampleShadingEnable   = VK_FALSE;
    stateCreateInfo.minSampleShading      = 0.0f;
    stateCreateInfo.pSampleMask           = nullptr;
    stateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    stateCreateInfo.alphaToOneEnable      = VK_FALSE;

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeDepthStencil(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    VkPipelineDepthStencilStateCreateInfo&  stateCreateInfo)
{
    stateCreateInfo.flags                 = 0;
    stateCreateInfo.depthTestEnable       = pCreateInfo->depthStencilState.depthTestEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.depthWriteEnable      = pCreateInfo->depthStencilState.depthWriteEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.depthCompareOp        = ToVkCompareOp(pCreateInfo->depthStencilState.depthCompareOp);
    stateCreateInfo.depthBoundsTestEnable = pCreateInfo->depthStencilState.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.stencilTestEnable     = pCreateInfo->depthStencilState.stencilTestEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.front.failOp          = ToVkStencilOp(pCreateInfo->depthStencilState.front.failOp);
    stateCreateInfo.front.passOp          = ToVkStencilOp(pCreateInfo->depthStencilState.front.passOp);
    stateCreateInfo.front.depthFailOp     = ToVkStencilOp(pCreateInfo->depthStencilState.front.depthFailOp);
    stateCreateInfo.front.compareOp       = ToVkCompareOp(pCreateInfo->depthStencilState.front.compareOp);
    stateCreateInfo.front.compareMask     = pCreateInfo->depthStencilState.front.compareMask;
    stateCreateInfo.front.writeMask       = pCreateInfo->depthStencilState.front.writeMask;
    stateCreateInfo.front.reference       = pCreateInfo->depthStencilState.front.reference;
    stateCreateInfo.back.failOp           = ToVkStencilOp(pCreateInfo->depthStencilState.back.failOp);
    stateCreateInfo.back.passOp           = ToVkStencilOp(pCreateInfo->depthStencilState.back.passOp);
    stateCreateInfo.back.depthFailOp      = ToVkStencilOp(pCreateInfo->depthStencilState.back.depthFailOp);
    stateCreateInfo.back.compareOp        = ToVkCompareOp(pCreateInfo->depthStencilState.back.compareOp);
    stateCreateInfo.back.compareMask      = pCreateInfo->depthStencilState.back.compareMask;
    stateCreateInfo.back.writeMask        = pCreateInfo->depthStencilState.back.writeMask;
    stateCreateInfo.back.reference        = pCreateInfo->depthStencilState.back.reference;
    stateCreateInfo.minDepthBounds        = pCreateInfo->depthStencilState.minDepthBounds;
    stateCreateInfo.maxDepthBounds        = pCreateInfo->depthStencilState.maxDepthBounds;

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeColorBlend(
    const grfx::GraphicsPipelineCreateInfo*           pCreateInfo,
    std::vector<VkPipelineColorBlendAttachmentState>& vkAttachments,
    VkPipelineColorBlendStateCreateInfo&              stateCreateInfo)
{
    // auto& attachemnts = m_create_info.color_blend_attachment_states.GetStates();
    //// Warn if colorWriteMask is zero
    //{
    //    uint32_t count = CountU32(attachemnts);
    //    for (uint32_t i = 0; i < count; ++i) {
    //        auto& attachment = attachemnts[i];
    //        if (attachment.colorWriteMask == 0) {
    //            std::string name = "<UNAMED>";
    //            VKEX_LOG_RAW("\n*** VKEX WARNING: Graphics Pipeline Warning! ***");
    //            VKEX_LOG_WARN("Function : " << __FUNCTION__);
    //            VKEX_LOG_WARN("Mesage   : Color blend attachment state " << i << " has colorWriteMask=0x0, is this what you want?");
    //            VKEX_LOG_RAW("");
    //        }
    //    }
    //}

    for (uint32_t i = 0; i < pCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
        const grfx::BlendAttachmentState& attachment = pCreateInfo->colorBlendState.blendAttachments[i];

        VkPipelineColorBlendAttachmentState vkAttachment = {};
        vkAttachment.blendEnable                         = attachment.blendEnable ? VK_TRUE : VK_FALSE;
        vkAttachment.srcColorBlendFactor                 = ToVkBlendFactor(attachment.srcColorBlendFactor);
        vkAttachment.dstColorBlendFactor                 = ToVkBlendFactor(attachment.dstColorBlendFactor);
        vkAttachment.colorBlendOp                        = ToVkBlendOp(attachment.colorBlendOp);
        vkAttachment.srcAlphaBlendFactor                 = ToVkBlendFactor(attachment.srcAlphaBlendFactor);
        vkAttachment.dstAlphaBlendFactor                 = ToVkBlendFactor(attachment.dstAlphaBlendFactor);
        vkAttachment.alphaBlendOp                        = ToVkBlendOp(attachment.alphaBlendOp);
        vkAttachment.colorWriteMask                      = ToVkColorComponentFlags(attachment.colorWriteMask);

        vkAttachments.push_back(vkAttachment);
    }

    stateCreateInfo.flags             = 0;
    stateCreateInfo.logicOpEnable     = pCreateInfo->colorBlendState.logicOpEnable ? VK_TRUE : VK_FALSE;
    stateCreateInfo.logicOp           = ToVkLogicOp(pCreateInfo->colorBlendState.logicOp);
    stateCreateInfo.attachmentCount   = CountU32(vkAttachments);
    stateCreateInfo.pAttachments      = DataPtr(vkAttachments);
    stateCreateInfo.blendConstants[0] = pCreateInfo->colorBlendState.blendConstants[0];
    stateCreateInfo.blendConstants[1] = pCreateInfo->colorBlendState.blendConstants[1];
    stateCreateInfo.blendConstants[2] = pCreateInfo->colorBlendState.blendConstants[2];
    stateCreateInfo.blendConstants[3] = pCreateInfo->colorBlendState.blendConstants[3];

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeDynamicState(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    std::vector<VkDynamicState>&            dynamicStates,
    VkPipelineDynamicStateCreateInfo&       stateCreateInfo)
{
    // NOTE: Since D3D12 doesn't have line width other than 1.0, dynamic
    //       line width is not supported.
    //
    dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    // dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);

#if defined(PPX_VK_EXTENDED_DYNAMIC_STATE)
    if (ToApi(GetDevice())->IsExtendedDynamicStateAvailable()) {
        // Provided by VK_EXT_extended_dynamic_state
        dynamicStates.push_back(VK_DYNAMIC_STATE_CULL_MODE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_FRONT_FACE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_OP_EXT);
    }
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

    stateCreateInfo.flags             = 0;
    stateCreateInfo.dynamicStateCount = CountU32(dynamicStates);
    stateCreateInfo.pDynamicStates    = DataPtr(dynamicStates);

    return ppx::SUCCESS;
}

Result GraphicsPipeline::CreateApiObjects(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    VkGraphicsPipelineCreateInfo vkci = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    Result                                       ppxres = InitializeShaderStages(pCreateInfo, shaderStages, vkci);

    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkVertexInputBindingDescription>   vertexBindings;
    VkPipelineVertexInputStateCreateInfo           vertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    //
    ppxres = InitializeVertexInput(pCreateInfo, vertexAttributes, vertexBindings, vertexInputState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    //
    ppxres = InitializeInputAssembly(pCreateInfo, inputAssemblyState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkPipelineTessellationDomainOriginStateCreateInfoKHR domainOriginStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR};
    VkPipelineTessellationStateCreateInfo                tessellationState           = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    //
    ppxres = InitializeTessellation(pCreateInfo, domainOriginStateCreateInfo, tessellationState);
    if (Failed(ppxres)) {
        return ppxres;
    }
    tessellationState.pNext = (pCreateInfo->tessellationState.patchControlPoints > 0) ? &domainOriginStateCreateInfo : nullptr;

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    //
    ppxres = InitializeViewports(pCreateInfo, viewportState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT};
    VkPipelineRasterizationStateCreateInfo             rasterizationState       = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    //
    ppxres = InitializeRasterization(pCreateInfo, depthClipStateCreateInfo, rasterizationState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkPipelineMultisampleStateCreateInfo multisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    //
    ppxres = InitializeMultisample(pCreateInfo, multisampleState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    //
    ppxres = InitializeDepthStencil(pCreateInfo, depthStencilState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    std::vector<VkPipelineColorBlendAttachmentState> attachments;
    VkPipelineColorBlendStateCreateInfo              colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    //
    ppxres = InitializeColorBlend(pCreateInfo, attachments, colorBlendState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    std::vector<VkDynamicState>      dynamicStatesArray;
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    //
    ppxres = InitializeDynamicState(pCreateInfo, dynamicStatesArray, dynamicState);
    if (Failed(ppxres)) {
        return ppxres;
    }

    VkRenderPassPtr       renderPass = VK_NULL_HANDLE;
    std::vector<VkFormat> renderTargetFormats;
    for (uint32_t i = 0; i < pCreateInfo->outputState.renderTargetCount; ++i) {
        renderTargetFormats.push_back(ToVkFormat(pCreateInfo->outputState.renderTargetFormats[i]));
    }
    VkFormat depthStencilFormat = ToVkFormat(pCreateInfo->outputState.depthStencilFormat);

#if defined(VK_KHR_dynamic_rendering)
    VkPipelineRenderingCreateInfo renderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

    if (pCreateInfo->dynamicRenderPass) {
        renderingCreateInfo.viewMask                = 0;
        renderingCreateInfo.colorAttachmentCount    = CountU32(renderTargetFormats);
        renderingCreateInfo.pColorAttachmentFormats = DataPtr(renderTargetFormats);
        renderingCreateInfo.depthAttachmentFormat   = depthStencilFormat;
        if (GetFormatDescription(pCreateInfo->outputState.depthStencilFormat)->aspect & FORMAT_ASPECT_STENCIL) {
            renderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;
        }

        vkci.pNext = &renderingCreateInfo;
    }
    else
#endif
    {
        // Create temporary render pass
        //

        VkResult vkres = vk::CreateTransientRenderPass(
            ToApi(GetDevice()),
            CountU32(renderTargetFormats),
            DataPtr(renderTargetFormats),
            depthStencilFormat,
            ToVkSampleCount(pCreateInfo->rasterState.rasterizationSamples),
            &renderPass,
            pCreateInfo->shadingRateMode);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vk::CreateTransientRenderPass failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Fill in pointers nad remaining values
    //
    vkci.flags               = 0;
    vkci.stageCount          = CountU32(shaderStages);
    vkci.pStages             = DataPtr(shaderStages);
    vkci.pVertexInputState   = &vertexInputState;
    vkci.pInputAssemblyState = &inputAssemblyState;
    vkci.pTessellationState  = &tessellationState;
    vkci.pViewportState      = &viewportState;
    vkci.pRasterizationState = &rasterizationState;
    vkci.pMultisampleState   = &multisampleState;
    vkci.pDepthStencilState  = &depthStencilState;
    vkci.pColorBlendState    = &colorBlendState;
    vkci.pDynamicState       = &dynamicState;
    vkci.layout              = ToApi(pCreateInfo->pPipelineInterface)->GetVkPipelineLayout();
    vkci.renderPass          = renderPass;
    vkci.subpass             = 0; // One subpass to rule them all
    vkci.basePipelineHandle  = VK_NULL_HANDLE;
    vkci.basePipelineIndex   = -1;

    // [VRS] set pipeline shading rate
    VkPipelineFragmentShadingRateStateCreateInfoKHR shadingRate = {VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR};
    if (pCreateInfo->shadingRateMode == SHADING_RATE_VRS) {
        shadingRate.fragmentSize   = VkExtent2D{1, 1};
        shadingRate.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
        shadingRate.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
        InsertPNext(vkci, shadingRate);
    }

    VkResult vkres = vkCreateGraphicsPipelines(
        ToApi(GetDevice())->GetVkDevice(),
        VK_NULL_HANDLE,
        1,
        &vkci,
        nullptr,
        &mPipeline);
    // Destroy transient render pass
    if (renderPass) {
        vkDestroyRenderPass(
            ToApi(GetDevice())->GetVkDevice(),
            renderPass,
            nullptr);
    }
    // Process result
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateGraphicsPipelines failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void GraphicsPipeline::DestroyApiObjects()
{
    if (mPipeline) {
        vkDestroyPipeline(ToApi(GetDevice())->GetVkDevice(), mPipeline, nullptr);
        mPipeline.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// PipelineInterface
// -------------------------------------------------------------------------------------------------
Result PipelineInterface::CreateApiObjects(const grfx::PipelineInterfaceCreateInfo* pCreateInfo)
{
    VkDescriptorSetLayout setLayouts[PPX_MAX_BOUND_DESCRIPTOR_SETS] = {VK_NULL_HANDLE};
    for (uint32_t i = 0; i < pCreateInfo->setCount; ++i) {
        setLayouts[i] = ToApi(pCreateInfo->sets[i].pLayout)->GetVkDescriptorSetLayout();
    }

    bool                hasPushConstants   = (pCreateInfo->pushConstants.count > 0);
    VkPushConstantRange pushConstantsRange = {};
    if (hasPushConstants) {
        const uint32_t sizeInBytes = pCreateInfo->pushConstants.count * sizeof(uint32_t);

        // Double check device limits
        auto& limits = ToApi(GetDevice()->GetGpu())->GetLimits();
        if (sizeInBytes > limits.maxPushConstantsSize) {
            PPX_ASSERT_MSG(false, "push constants size in bytes (" << sizeInBytes << ") exceeds VkPhysicalDeviceLimits::maxPushConstantsSize (" << limits.maxPushConstantsSize << ")");
            return ppx::ERROR_LIMIT_EXCEEDED;
        }

        // Save stage flags for use in command buffer
        mPushConstantShaderStageFlags = ToVkShaderStageFlags(pCreateInfo->pushConstants.shaderVisiblity);

        // Fill out range
        pushConstantsRange.stageFlags = mPushConstantShaderStageFlags;
        pushConstantsRange.offset     = 0;
        pushConstantsRange.size       = sizeInBytes;
    }

    VkPipelineLayoutCreateInfo vkci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    vkci.flags                      = 0;
    vkci.setLayoutCount             = pCreateInfo->setCount;
    vkci.pSetLayouts                = setLayouts;
    vkci.pushConstantRangeCount     = hasPushConstants ? 1 : 0;
    vkci.pPushConstantRanges        = hasPushConstants ? &pushConstantsRange : nullptr;

    VkResult vkres = vkCreatePipelineLayout(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mPipelineLayout);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreatePipelineLayout failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void PipelineInterface::DestroyApiObjects()
{
    if (mPipelineLayout) {
        vkDestroyPipelineLayout(ToApi(GetDevice())->GetVkDevice(), mPipelineLayout, nullptr);
        mPipelineLayout.Reset();
    }
}

} // namespace vk
} // namespace grfx
} // namespace ppx
