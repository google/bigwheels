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

#include "ppx/grfx/dx12/dx12_pipeline.h"
#include "ppx/grfx/dx12/dx12_descriptor.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_shader.h"
#include "ppx/grfx/dx12/dx12_util.h"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// ComputePipeline
// -------------------------------------------------------------------------------------------------
Result ComputePipeline::CreateApiObjects(const grfx::ComputePipelineCreateInfo* pCreateInfo)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature                    = ToApi(pCreateInfo->pPipelineInterface)->GetDxRootSignature().Get();
    desc.CS.pShaderBytecode                = ToApi(pCreateInfo->CS.pModule)->GetCode();
    desc.CS.BytecodeLength                 = ToApi(pCreateInfo->CS.pModule)->GetSize();
    desc.NodeMask                          = 0;
    desc.CachedPSO                         = {};
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPipeline));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateComputePipelineState failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12PipelineState(Compute), mPipeline.Get());

    return ppx::SUCCESS;
}

void ComputePipeline::DestroyApiObjects()
{
    if (mPipeline) {
        mPipeline.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// GraphicsPipeline
// -------------------------------------------------------------------------------------------------
void GraphicsPipeline::InitializeShaderStages(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    D3D12_GRAPHICS_PIPELINE_STATE_DESC&     desc)
{
    // VS
    if (!IsNull(pCreateInfo->VS.pModule)) {
        desc.VS.pShaderBytecode = ToApi(pCreateInfo->VS.pModule)->GetCode();
        desc.VS.BytecodeLength  = ToApi(pCreateInfo->VS.pModule)->GetSize();
    }

    // HS
    if (!IsNull(pCreateInfo->HS.pModule)) {
        desc.HS.pShaderBytecode = ToApi(pCreateInfo->HS.pModule)->GetCode();
        desc.HS.BytecodeLength  = ToApi(pCreateInfo->HS.pModule)->GetSize();
    }

    // DS
    if (!IsNull(pCreateInfo->DS.pModule)) {
        desc.DS.pShaderBytecode = ToApi(pCreateInfo->DS.pModule)->GetCode();
        desc.DS.BytecodeLength  = ToApi(pCreateInfo->DS.pModule)->GetSize();
    }

    // GS
    if (!IsNull(pCreateInfo->GS.pModule)) {
        desc.GS.pShaderBytecode = ToApi(pCreateInfo->GS.pModule)->GetCode();
        desc.GS.BytecodeLength  = ToApi(pCreateInfo->GS.pModule)->GetSize();
    }

    // PS
    if (!IsNull(pCreateInfo->PS.pModule)) {
        desc.PS.pShaderBytecode = ToApi(pCreateInfo->PS.pModule)->GetCode();
        desc.PS.BytecodeLength  = ToApi(pCreateInfo->PS.pModule)->GetSize();
    }
}

void GraphicsPipeline::InitializeBlendState(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    D3D12_BLEND_DESC&                       desc)
{
    desc.AlphaToCoverageEnable  = static_cast<BOOL>(pCreateInfo->multisampleState.alphaToCoverageEnable);
    desc.IndependentBlendEnable = (pCreateInfo->colorBlendState.blendAttachmentCount > 0) ? TRUE : FALSE;

    PPX_ASSERT_MSG(pCreateInfo->colorBlendState.blendAttachmentCount < PPX_MAX_RENDER_TARGETS, "blendAttachmentCount exceeds PPX_MAX_RENDER_TARGETS");
    for (uint32_t i = 0; i < pCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
        const grfx::BlendAttachmentState& ppxBlend = pCreateInfo->colorBlendState.blendAttachments[i];
        D3D12_RENDER_TARGET_BLEND_DESC&   d3dBlend = desc.RenderTarget[i];

        d3dBlend.BlendEnable           = static_cast<BOOL>(ppxBlend.blendEnable);
        d3dBlend.LogicOpEnable         = static_cast<BOOL>(pCreateInfo->colorBlendState.logicOpEnable);
        d3dBlend.SrcBlend              = ToD3D12Blend(ppxBlend.srcColorBlendFactor);
        d3dBlend.DestBlend             = ToD3D12Blend(ppxBlend.dstColorBlendFactor);
        d3dBlend.BlendOp               = ToD3D12BlendOp(ppxBlend.colorBlendOp);
        d3dBlend.SrcBlendAlpha         = ToD3D12Blend(ppxBlend.srcAlphaBlendFactor);
        d3dBlend.DestBlendAlpha        = ToD3D12Blend(ppxBlend.dstAlphaBlendFactor);
        d3dBlend.BlendOpAlpha          = ToD3D12BlendOp(ppxBlend.alphaBlendOp);
        d3dBlend.LogicOp               = ToD3D12LogicOp(pCreateInfo->colorBlendState.logicOp);
        d3dBlend.RenderTargetWriteMask = ToD3D12WriteMask(ppxBlend.colorWriteMask);
    }
}

void GraphicsPipeline::InitializeRasterizerState(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    D3D12_RASTERIZER_DESC&                  desc)
{
    desc.FillMode              = ToD3D12FillMode(pCreateInfo->rasterState.polygonMode);
    desc.CullMode              = ToD3D12CullMode(pCreateInfo->rasterState.cullMode);
    desc.FrontCounterClockwise = (pCreateInfo->rasterState.frontFace == grfx::FRONT_FACE_CCW) ? TRUE : FALSE;
    desc.DepthBias             = pCreateInfo->rasterState.depthBiasEnable ? static_cast<INT>(pCreateInfo->rasterState.depthBiasConstantFactor) : 0;
    desc.DepthBiasClamp        = pCreateInfo->rasterState.depthBiasEnable ? pCreateInfo->rasterState.depthBiasClamp : 0;
    desc.SlopeScaledDepthBias  = pCreateInfo->rasterState.depthBiasEnable ? pCreateInfo->rasterState.depthBiasSlopeFactor : 0;
    desc.DepthClipEnable       = static_cast<BOOL>(pCreateInfo->rasterState.depthClipEnable);
    desc.MultisampleEnable     = FALSE; // @TODO: Route this in
    desc.AntialiasedLineEnable = FALSE;
    desc.ForcedSampleCount     = 0;
    desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void GraphicsPipeline::InitializeDepthStencilState(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    D3D12_DEPTH_STENCIL_DESC&               desc)
{
    desc.DepthEnable                  = static_cast<BOOL>(pCreateInfo->depthStencilState.depthTestEnable);
    desc.DepthWriteMask               = pCreateInfo->depthStencilState.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc                    = ToD3D12ComparisonFunc(pCreateInfo->depthStencilState.depthCompareOp);
    desc.StencilEnable                = static_cast<BOOL>(pCreateInfo->depthStencilState.stencilTestEnable);
    desc.StencilReadMask              = D3D12_DEFAULT_STENCIL_READ_MASK;  // @TODO: Figure out to set properly
    desc.StencilWriteMask             = D3D12_DEFAULT_STENCIL_WRITE_MASK; // @TODO: Figure out to set properly
    desc.FrontFace.StencilFailOp      = ToD3D12StencilOp(pCreateInfo->depthStencilState.front.failOp);
    desc.FrontFace.StencilDepthFailOp = ToD3D12StencilOp(pCreateInfo->depthStencilState.front.depthFailOp);
    desc.FrontFace.StencilPassOp      = ToD3D12StencilOp(pCreateInfo->depthStencilState.front.passOp);
    desc.FrontFace.StencilFunc        = ToD3D12ComparisonFunc(pCreateInfo->depthStencilState.front.compareOp);
    desc.BackFace.StencilFailOp       = ToD3D12StencilOp(pCreateInfo->depthStencilState.back.failOp);
    desc.BackFace.StencilDepthFailOp  = ToD3D12StencilOp(pCreateInfo->depthStencilState.back.depthFailOp);
    desc.BackFace.StencilPassOp       = ToD3D12StencilOp(pCreateInfo->depthStencilState.back.passOp);
    desc.BackFace.StencilFunc         = ToD3D12ComparisonFunc(pCreateInfo->depthStencilState.back.compareOp);
}

void GraphicsPipeline::InitializeInputLayout(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    std::vector<D3D12_INPUT_ELEMENT_DESC>&  inputElements,
    D3D12_INPUT_LAYOUT_DESC&                desc)
{
    for (uint32_t bindingIndex = 0; bindingIndex < pCreateInfo->vertexInputState.bindingCount; ++bindingIndex) {
        const grfx::VertexBinding& binding = pCreateInfo->vertexInputState.bindings[bindingIndex];
        // Iterate each attribute in the binding
        const uint32_t attributeCount = binding.GetAttributeCount();
        for (uint32_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
            // This should be safe since there's no modifications to the index
            const grfx::VertexAttribute* pAttribute = nullptr;
            binding.GetAttribute(attributeIndex, &pAttribute);

            D3D12_INPUT_ELEMENT_DESC element = {};
            element.SemanticName             = pAttribute->semanticName.c_str();
            element.SemanticIndex            = 0;
            element.Format                   = dx::ToDxgiFormat(pAttribute->format);
            element.InputSlot                = static_cast<UINT>(pAttribute->binding);
            element.AlignedByteOffset        = static_cast<UINT>(pAttribute->offset);
            element.InputSlotClass           = (pAttribute->inputRate == grfx::VERETX_INPUT_RATE_INSTANCE) ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            element.InstanceDataStepRate     = (pAttribute->inputRate == grfx::VERETX_INPUT_RATE_INSTANCE) ? 1 : 0;
            inputElements.push_back(element);
        }
    }

    desc.NumElements        = static_cast<UINT>(inputElements.size());
    desc.pInputElementDescs = inputElements.data();
}

void GraphicsPipeline::InitializeOutput(
    const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
    D3D12_GRAPHICS_PIPELINE_STATE_DESC&     desc)
{
    desc.NumRenderTargets = pCreateInfo->outputState.renderTargetCount;

    for (UINT i = 0; i < desc.NumRenderTargets; ++i) {
        desc.RTVFormats[i] = dx::ToDxgiFormat(pCreateInfo->outputState.renderTargetFormats[i]);
    }

    desc.DSVFormat = dx::ToDxgiFormat(pCreateInfo->outputState.depthStencilFormat);
}

Result GraphicsPipeline::CreateApiObjects(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature                     = ToApi(pCreateInfo->pPipelineInterface)->GetDxRootSignature().Get();

    InitializeShaderStages(pCreateInfo, desc);

    desc.StreamOutput = {};

    InitializeBlendState(pCreateInfo, desc.BlendState);

    // @TODO: Figure out the right way to handle this
    desc.SampleMask = UINT_MAX;

    InitializeRasterizerState(pCreateInfo, desc.RasterizerState);

    InitializeDepthStencilState(pCreateInfo, desc.DepthStencilState);

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    InitializeInputLayout(pCreateInfo, inputElements, desc.InputLayout);

    // @TODO: Figure out how to route this in so it plays nice with Vulkan
    desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    desc.PrimitiveTopologyType = ToD3D12PrimitiveTopology(pCreateInfo->inputAssemblyState.topology);

    InitializeOutput(pCreateInfo, desc);

    // @TODO: Add logic to handle quality
    desc.SampleDesc.Count   = static_cast<UINT>(pCreateInfo->rasterState.rasterizationSamples);
    desc.SampleDesc.Quality = 0;

    desc.NodeMask  = 0;
    desc.CachedPSO = {};
    desc.Flags     = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPipeline));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateGraphicsPipelineState failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12PipelineState(Graphics), mPipeline.Get());

    // clang-format off
    switch (pCreateInfo->inputAssemblyState.topology) {
        default: {
            PPX_ASSERT_MSG(false, "unknown primitive topolgy type");
            return ppx::ERROR_INVALID_CREATE_ARGUMENT;
        }
        break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST  : mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        case grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN   : break;
        case grfx::PRIMITIVE_TOPOLOGY_POINT_LIST     : mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_LIST      : mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case grfx::PRIMITIVE_TOPOLOGY_LINE_STRIP     : mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case grfx::PRIMITIVE_TOPOLOGY_PATCH_LIST     : break;
    }
    // clang-format on

    return ppx::SUCCESS;
}

void GraphicsPipeline::DestroyApiObjects()
{
    if (mPipeline) {
        mPipeline.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// PipelineInterface
// -------------------------------------------------------------------------------------------------
Result PipelineInterface::CreateApiObjects(const grfx::PipelineInterfaceCreateInfo* pCreateInfo)
{
    dx12::Device* pDevice = ToApi(GetDevice());

    // std::vector<std::unique_ptr<std::vector<D3D12_DESCRIPTOR_RANGE1>>> parameterRanges;
    std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>> parameterRanges;

    // Creates a parameter for each binding - this is naive way of
    // create paramters and their associated range. It favors flexibility.
    //
    // @TODO: Optimize
    //
    std::vector<D3D12_ROOT_PARAMETER1> parameters;
    for (uint32_t setIndex = 0; setIndex < pCreateInfo->setCount; ++setIndex) {
        uint32_t                                    set      = pCreateInfo->sets[setIndex].set;
        const dx12::DescriptorSetLayout*            pLayout  = ToApi(pCreateInfo->sets[setIndex].pLayout);
        const std::vector<grfx::DescriptorBinding>& bindings = pLayout->GetBindings();

        for (size_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
            const grfx::DescriptorBinding& binding = bindings[bindingIndex];

            // Allocate unique range
            std::unique_ptr<D3D12_DESCRIPTOR_RANGE1> range = std::make_unique<D3D12_DESCRIPTOR_RANGE1>();
            if (!range) {
                PPX_ASSERT_MSG(false, "allocation for descriptor range failed for set=" << set << ", binding=" << binding.binding);
                return ppx::ERROR_ALLOCATION_FAILED;
            }
            // Fill out range
            range->RangeType                         = ToD3D12RangeType(binding.type);
            range->NumDescriptors                    = static_cast<UINT>(binding.arrayCount);
            range->BaseShaderRegister                = static_cast<UINT>(binding.binding);
            range->RegisterSpace                     = static_cast<UINT>(set);
            range->Flags                             = (range->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV) ? D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE : D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            // Fill out parameter
            D3D12_ROOT_PARAMETER1 parameter               = {};
            parameter.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameter.DescriptorTable.NumDescriptorRanges = 1;
            parameter.DescriptorTable.pDescriptorRanges   = range.get();
            parameter.ShaderVisibility                    = ToD3D12ShaderVisibliity(binding.shaderVisiblity);
            // Store parameter
            parameters.push_back(parameter);
            // Store ranges
            parameterRanges.push_back(std::move(range));
            // Store parameter index
            ParameterIndex paramIndex = {};
            paramIndex.set            = set;
            paramIndex.binding        = binding.binding;
            paramIndex.index          = static_cast<UINT>(parameters.size() - 1);
            mParameterIndices.push_back(paramIndex);
        }
    }

    // Add root constants
    if (pCreateInfo->pushConstants.count > 0) {
        D3D12_ROOT_PARAMETER1 parameter    = {};
        parameter.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameter.Constants.ShaderRegister = static_cast<UINT>(pCreateInfo->pushConstants.binding);
        parameter.Constants.RegisterSpace  = static_cast<UINT>(pCreateInfo->pushConstants.set);
        parameter.Constants.Num32BitValues = static_cast<UINT>(pCreateInfo->pushConstants.count);
        parameter.ShaderVisibility         = ToD3D12ShaderVisibliity(pCreateInfo->pushConstants.shaderVisiblity);
        // Store parameter
        parameters.push_back(parameter);

        // Store a specific parameter index for root constants to get to it without a search.
        mRootConstantsParameterIndex = CountU32(parameters) - 1;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
    desc.Version                             = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.NumParameters              = static_cast<UINT>(parameters.size());
    desc.Desc_1_1.pParameters                = DataPtr(parameters);
    desc.Desc_1_1.NumStaticSamplers          = 0;
    desc.Desc_1_1.pStaticSamplers            = nullptr;
    desc.Desc_1_1.Flags                      = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    CComPtr<ID3DBlob> sigBlob   = nullptr;
    CComPtr<ID3DBlob> errorMsgs = nullptr;
    HRESULT           hr        = pDevice->SerializeVersionedRootSignature(
        &desc,
        &sigBlob,
        &errorMsgs);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "dx::Device::SerializeVersionedRootSignature failed");
        return ppx::ERROR_API_FAILURE;
    }

    UINT nodeMask = 0;
    hr            = pDevice->GetDxDevice()->CreateRootSignature(
        nodeMask,
        sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateRootSignature failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12RootSignature, mRootSignature.Get());

    return ppx::SUCCESS;
}

void PipelineInterface::DestroyApiObjects()
{
    if (mRootSignature) {
        mRootSignature.Reset();
    }
}

UINT PipelineInterface::FindParameterIndex(uint32_t set, uint32_t binding) const
{
    auto it = FindIf(
        mParameterIndices,
        [set, binding](const ParameterIndex& elem) -> bool { 
            bool isSameSet = elem.set == set;
            bool isSameBinding = elem.binding == binding;
            bool isSame = isSameSet && isSameBinding;
            return isSame; });
    if (it == std::end(mParameterIndices)) {
        return PPX_VALUE_IGNORED;
    }
    return static_cast<UINT>(it->index);
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
