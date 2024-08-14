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

#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_pipeline.h"
#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_enums.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// BlendAttachmentState
// -------------------------------------------------------------------------------------------------
grfx::BlendAttachmentState BlendAttachmentState::BlendModeAdditive()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = true;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags::RGBA();

    return state;
}

grfx::BlendAttachmentState BlendAttachmentState::BlendModeAlpha()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = true;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags::RGBA();

    return state;
}

grfx::BlendAttachmentState BlendAttachmentState::BlendModeOver()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = true;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_SRC_ALPHA;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags::RGBA();

    return state;
}

grfx::BlendAttachmentState BlendAttachmentState::BlendModeUnder()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = true;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_DST_ALPHA;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_ZERO;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags::RGBA();

    return state;
}

grfx::BlendAttachmentState BlendAttachmentState::BlendModePremultAlpha()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = true;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags::RGBA();

    return state;
}

grfx::BlendAttachmentState BlendAttachmentState::BlendModeDisableOutput()
{
    grfx::BlendAttachmentState state = {};
    state.blendEnable                = false;
    state.srcColorBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.dstColorBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.colorBlendOp               = grfx::BLEND_OP_ADD;
    state.srcAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.dstAlphaBlendFactor        = grfx::BLEND_FACTOR_ONE;
    state.alphaBlendOp               = grfx::BLEND_OP_ADD;
    state.colorWriteMask             = grfx::ColorComponentFlags(0);

    return state;
}

namespace internal {

// -------------------------------------------------------------------------------------------------
// internal
// -------------------------------------------------------------------------------------------------
void FillOutGraphicsPipelineCreateInfo(
    const grfx::GraphicsPipelineCreateInfo2* pSrcCreateInfo,
    grfx::GraphicsPipelineCreateInfo*        pDstCreateInfo)
{
    // Set to default values
    *pDstCreateInfo = {};

    pDstCreateInfo->dynamicRenderPass = pSrcCreateInfo->dynamicRenderPass;

    // Shaders
    pDstCreateInfo->VS = pSrcCreateInfo->VS;
    pDstCreateInfo->PS = pSrcCreateInfo->PS;

    // Vertex input
    {
        pDstCreateInfo->vertexInputState.bindingCount = pSrcCreateInfo->vertexInputState.bindingCount;
        for (uint32_t i = 0; i < pDstCreateInfo->vertexInputState.bindingCount; ++i) {
            pDstCreateInfo->vertexInputState.bindings[i] = pSrcCreateInfo->vertexInputState.bindings[i];
        }
    }

    // Input assembly
    {
        pDstCreateInfo->inputAssemblyState.topology = pSrcCreateInfo->topology;
    }

    // Raster
    {
        pDstCreateInfo->rasterState.polygonMode = pSrcCreateInfo->polygonMode;
        pDstCreateInfo->rasterState.cullMode    = pSrcCreateInfo->cullMode;
        pDstCreateInfo->rasterState.frontFace   = pSrcCreateInfo->frontFace;
    }

    // Depth/stencil
    {
        pDstCreateInfo->depthStencilState.depthTestEnable       = pSrcCreateInfo->depthReadEnable;
        pDstCreateInfo->depthStencilState.depthWriteEnable      = pSrcCreateInfo->depthWriteEnable;
        pDstCreateInfo->depthStencilState.depthCompareOp        = pSrcCreateInfo->depthCompareOp;
        pDstCreateInfo->depthStencilState.depthBoundsTestEnable = false;
        pDstCreateInfo->depthStencilState.minDepthBounds        = 0.0f;
        pDstCreateInfo->depthStencilState.maxDepthBounds        = 1.0f;
        pDstCreateInfo->depthStencilState.stencilTestEnable     = false;
        pDstCreateInfo->depthStencilState.front                 = {};
        pDstCreateInfo->depthStencilState.back                  = {};
    }

    // Color blend
    {
        pDstCreateInfo->colorBlendState.blendAttachmentCount = pSrcCreateInfo->outputState.renderTargetCount;
        for (uint32_t i = 0; i < pDstCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
            switch (pSrcCreateInfo->blendModes[i]) {
                default: break;

                case grfx::BLEND_MODE_ADDITIVE: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModeAdditive();
                } break;

                case grfx::BLEND_MODE_ALPHA: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModeAlpha();
                } break;

                case grfx::BLEND_MODE_OVER: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModeOver();
                } break;

                case grfx::BLEND_MODE_UNDER: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModeUnder();
                } break;

                case grfx::BLEND_MODE_PREMULT_ALPHA: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModePremultAlpha();
                } break;
                case grfx::BLEND_MODE_DISABLE_OUTPUT: {
                    pDstCreateInfo->colorBlendState.blendAttachments[i] = grfx::BlendAttachmentState::BlendModeDisableOutput();
                }
            }
        }
    }

    // Output
    {
        pDstCreateInfo->outputState.renderTargetCount = pSrcCreateInfo->outputState.renderTargetCount;
        for (uint32_t i = 0; i < pDstCreateInfo->outputState.renderTargetCount; ++i) {
            pDstCreateInfo->outputState.renderTargetFormats[i] = pSrcCreateInfo->outputState.renderTargetFormats[i];
        }

        pDstCreateInfo->outputState.depthStencilFormat = pSrcCreateInfo->outputState.depthStencilFormat;
    }

    // Shading rate mode
    pDstCreateInfo->shadingRateMode = pSrcCreateInfo->shadingRateMode;

    // Pipeline internface
    pDstCreateInfo->pPipelineInterface = pSrcCreateInfo->pPipelineInterface;

    // MultiView details
    pDstCreateInfo->multiViewState = pSrcCreateInfo->multiViewState;
}

} // namespace internal

// -------------------------------------------------------------------------------------------------
// ComputePipeline
// -------------------------------------------------------------------------------------------------
Result ComputePipeline::Create(const grfx::ComputePipelineCreateInfo* pCreateInfo)
{
    if (IsNull(pCreateInfo->pPipelineInterface)) {
        PPX_ASSERT_MSG(false, "pipeline interface is null (compute pipeline)");
        return ppx::ERROR_GRFX_OPERATION_NOT_PERMITTED;
    }

    Result ppxres = grfx::DeviceObject<grfx::ComputePipelineCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// GraphicsPipeline
// -------------------------------------------------------------------------------------------------
Result GraphicsPipeline::Create(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    //// Checked binding range
    // for (uint32_t i = 0; i < pCreateInfo->vertexInputState.attributeCount; ++i) {
    //     const grfx::VertexAttribute& attribute = pCreateInfo->vertexInputState.attributes[i];
    //     if (attribute.binding >= PPX_MAX_VERTEX_BINDINGS) {
    //         PPX_ASSERT_MSG(false, "binding exceeds PPX_MAX_VERTEX_ATTRIBUTES");
    //         return ppx::ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED;
    //     }
    //     if (attribute.format == grfx::FORMAT_UNDEFINED) {
    //         PPX_ASSERT_MSG(false, "vertex attribute format is undefined");
    //         return ppx::ERROR_GRFX_VERTEX_ATTRIBUTE_FROMAT_UNDEFINED;
    //     }
    // }
    //
    //// Build input bindings
    //{
    //    // Collect attributes into bindings
    //    for (uint32_t i = 0; i < pCreateInfo->vertexInputState.attributeCount; ++i) {
    //        const grfx::VertexAttribute& attribute = pCreateInfo->vertexInputState.attributes[i];
    //
    //        auto it = std::find_if(
    //            std::begin(mInputBindings),
    //            std::end(mInputBindings),
    //            [attribute](const VertexInputBinding& elem) -> bool {
    //            bool isSame = attribute.binding == elem.binding;
    //            return isSame; });
    //        if (it != std::end(mInputBindings)) {
    //            it->attributes.push_back(attribute);
    //        }
    //        else {
    //            VertexInputBinding set = {attribute.binding};
    //            mInputBindings.push_back(set);
    //            mInputBindings.back().attributes.push_back(attribute);
    //        }
    //    }
    //
    //    // Calculate offsets and stride
    //    for (auto& elem : mInputBindings) {
    //        elem.CalculateOffsetsAndStride();
    //    }
    //
    //    // Check classifactions
    //    for (auto& elem : mInputBindings) {
    //        uint32_t inputRateVertexCount   = 0;
    //        uint32_t inputRateInstanceCount = 0;
    //        for (auto& attr : elem.attributes) {
    //            inputRateVertexCount += (attr.inputRate == grfx::VERTEX_INPUT_RATE_VERTEX) ? 1 : 0;
    //            inputRateInstanceCount += (attr.inputRate == grfx::VERETX_INPUT_RATE_INSTANCE) ? 1 : 0;
    //        }
    //        // Cannot mix input rates
    //        if ((inputRateInstanceCount > 0) && (inputRateVertexCount > 0)) {
    //            PPX_ASSERT_MSG(false, "cannot mix vertex input rates in same binding");
    //            return ppx::ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES;
    //        }
    //    }
    //}

    if (IsNull(pCreateInfo->pPipelineInterface)) {
        PPX_ASSERT_MSG(false, "pipeline interface is null (graphics pipeline)");
        return ppx::ERROR_GRFX_OPERATION_NOT_PERMITTED;
    }

    if (pCreateInfo->dynamicRenderPass && !GetDevice()->DynamicRenderingSupported()) {
        PPX_ASSERT_MSG(false, "Cannot create a pipeline with dynamic render pass, dynamic rendering is not supported.");
        return ppx::ERROR_GRFX_OPERATION_NOT_PERMITTED;
    }

    Result ppxres = grfx::DeviceObject<grfx::GraphicsPipelineCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// PipelineInterface
// -------------------------------------------------------------------------------------------------
Result PipelineInterface::Create(const grfx::PipelineInterfaceCreateInfo* pCreateInfo)
{
    if (pCreateInfo->setCount > PPX_MAX_BOUND_DESCRIPTOR_SETS) {
        PPX_ASSERT_MSG(false, "set count exceeds PPX_MAX_BOUND_DESCRIPTOR_SETS");
        return ppx::ERROR_LIMIT_EXCEEDED;
    }

    // If we have more than one set...we need to do some checks
    if (pCreateInfo->setCount > 0) {
        // Paranoid clear
        mSetNumbers.clear();
        // Copy set numbers
        std::vector<uint32_t> sortedSetNumbers;
        for (uint32_t i = 0; i < pCreateInfo->setCount; ++i) {
            uint32_t set = pCreateInfo->sets[i].set;
            sortedSetNumbers.push_back(set); // Sortable array
            mSetNumbers.push_back(set);      // Preserves declared ordering
        }
        // Sort set numbers
        std::sort(std::begin(sortedSetNumbers), std::end(sortedSetNumbers));
        // Check for uniqueness
        for (size_t i = 1; i < sortedSetNumbers.size(); ++i) {
            uint32_t setB = sortedSetNumbers[i];
            uint32_t setA = sortedSetNumbers[i - 1];
            uint32_t diff = setB - setA;
            if (diff == 0) {
                PPX_ASSERT_MSG(false, "set numbers are not unique");
                return ppx::ERROR_GRFX_NON_UNIQUE_SET;
            }
        }
        // Check for consecutive ness
        //
        // Assume consecutive
        mHasConsecutiveSetNumbers = true;
        for (size_t i = 1; i < mSetNumbers.size(); ++i) {
            int32_t setB = static_cast<int32_t>(sortedSetNumbers[i]);
            int32_t setA = static_cast<int32_t>(sortedSetNumbers[i - 1]);
            int32_t diff = setB - setA;
            if (diff != 1) {
                mHasConsecutiveSetNumbers = false;
                break;
            }
        }
    }

    // Check limit and make sure the push constants binding/set doesn't collide
    // with an existing binding in the set layouts.
    //
    if (pCreateInfo->pushConstants.count > 0) {
        if (pCreateInfo->pushConstants.count > PPX_MAX_PUSH_CONSTANTS) {
            PPX_ASSERT_MSG(false, "push constants count (" << pCreateInfo->pushConstants.count << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");
            return ppx::ERROR_LIMIT_EXCEEDED;
        }

        if (pCreateInfo->pushConstants.binding == PPX_VALUE_IGNORED) {
            PPX_ASSERT_MSG(false, "push constants binding number is invalid");
            return ppx::ERROR_GRFX_INVALID_BINDING_NUMBER;
        }
        if (pCreateInfo->pushConstants.set == PPX_VALUE_IGNORED) {
            PPX_ASSERT_MSG(false, "push constants set number is invalid");
            return ppx::ERROR_GRFX_INVALID_SET_NUMBER;
        }

        for (uint32_t i = 0; i < pCreateInfo->setCount; ++i) {
            auto& set = pCreateInfo->sets[i];
            // Skip if set number doesn't match
            if (set.set != pCreateInfo->pushConstants.set) {
                continue;
            }
            // See if the layout has a binding that's the same as the push constants binding
            const uint32_t pushConstantsBinding = pCreateInfo->pushConstants.binding;
            auto&          bindings             = set.pLayout->GetBindings();
            auto           it                   = std::find_if(
                bindings.begin(),
                bindings.end(),
                [pushConstantsBinding](const grfx::DescriptorBinding& elem) -> bool {
                    bool match = (elem.binding == pushConstantsBinding);
                    return match; });
            // Error out if a match is found
            if (it != bindings.end()) {
                PPX_ASSERT_MSG(false, "push constants binding and set overlaps with a binding in set " << set.set);
                return ppx::ERROR_GRFX_NON_UNIQUE_BINDING;
            }
        }
    }

    Result ppxres = grfx::DeviceObject<grfx::PipelineInterfaceCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

const grfx::DescriptorSetLayout* PipelineInterface::GetSetLayout(uint32_t setNumber) const
{
    const grfx::DescriptorSetLayout* pLayout = nullptr;
    for (uint32_t i = 0; i < mCreateInfo.setCount; ++i) {
        if (mCreateInfo.sets[i].set == setNumber) {
            pLayout = mCreateInfo.sets[i].pLayout;
            break;
        }
    }
    return pLayout;
}

} // namespace grfx
} // namespace ppx
