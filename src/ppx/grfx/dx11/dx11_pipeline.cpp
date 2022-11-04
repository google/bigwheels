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

#include "ppx/grfx/dx11/dx11_pipeline.h"
#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_shader.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// ComputePipeline
// -------------------------------------------------------------------------------------------------
Result ComputePipeline::CreateApiObjects(const grfx::ComputePipelineCreateInfo* pCreateInfo)
{
    const grfx::ShaderStageInfo& CS = pCreateInfo->CS;

    if (IsNull(CS.pModule)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    typename D3D11DevicePtr::InterfaceType* pDevice = ToApi(GetDevice())->GetDxDevice();

    HRESULT hr = pDevice->CreateComputeShader(ToApi(CS.pModule)->GetCode(), ToApi(CS.pModule)->GetSize(), nullptr, &mCS);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    return ppx::SUCCESS;
}

void ComputePipeline::DestroyApiObjects()
{
    if (mCS) {
        mCS.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// GraphicsPipeline
// -------------------------------------------------------------------------------------------------
Result GraphicsPipeline::InitializeShaders(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    const grfx::ShaderStageInfo& VS = pCreateInfo->VS;
    const grfx::ShaderStageInfo& HS = pCreateInfo->HS;
    const grfx::ShaderStageInfo& DS = pCreateInfo->DS;
    const grfx::ShaderStageInfo& GS = pCreateInfo->GS;
    const grfx::ShaderStageInfo& PS = pCreateInfo->PS;

    bool hasVS = !IsNull(VS.pModule);
    bool hasHS = !IsNull(HS.pModule);
    bool hasDS = !IsNull(DS.pModule);
    bool hasGS = !IsNull(GS.pModule);
    bool hasPS = !IsNull(PS.pModule);

    if (!(hasVS || hasHS || hasDS || hasGS || hasPS)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    if (!IsNull(VS.pModule)) {
        HRESULT hr = device->CreateVertexShader(ToApi(VS.pModule)->GetCode(), ToApi(VS.pModule)->GetSize(), nullptr, &mVS);
        if (FAILED(hr)) {
            return ppx::ERROR_FAILED;
        }
    }

    if (!IsNull(HS.pModule)) {
        HRESULT hr = device->CreateHullShader(ToApi(HS.pModule)->GetCode(), ToApi(HS.pModule)->GetSize(), nullptr, &mHS);
        if (FAILED(hr)) {
            return ppx::ERROR_FAILED;
        }
    }

    if (!IsNull(DS.pModule)) {
        HRESULT hr = device->CreateDomainShader(ToApi(DS.pModule)->GetCode(), ToApi(DS.pModule)->GetSize(), nullptr, &mDS);
        if (FAILED(hr)) {
            return ppx::ERROR_FAILED;
        }
    }

    if (!IsNull(GS.pModule)) {
        HRESULT hr = device->CreateGeometryShader(ToApi(GS.pModule)->GetCode(), ToApi(GS.pModule)->GetSize(), nullptr, &mGS);
        if (FAILED(hr)) {
            return ppx::ERROR_FAILED;
        }
    }

    if (!IsNull(PS.pModule)) {
        HRESULT hr = device->CreatePixelShader(ToApi(PS.pModule)->GetCode(), ToApi(PS.pModule)->GetSize(), nullptr, &mPS);
        if (FAILED(hr)) {
            return ppx::ERROR_FAILED;
        }
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeInputLayout(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;

    for (uint32_t bindingIndex = 0; bindingIndex < pCreateInfo->vertexInputState.bindingCount; ++bindingIndex) {
        const grfx::VertexBinding& binding = pCreateInfo->vertexInputState.bindings[bindingIndex];
        // Iterate each attribute in the binding
        const uint32_t attributeCount = binding.GetAttributeCount();
        for (uint32_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
            // This should be safe since there's no modifications to the index
            const grfx::VertexAttribute* pAttribute = nullptr;
            binding.GetAttribute(attributeIndex, &pAttribute);

            D3D11_INPUT_ELEMENT_DESC element = {};
            element.SemanticName             = pAttribute->semanticName.c_str();
            element.SemanticIndex            = 0;
            element.Format                   = dx::ToDxgiFormat(pAttribute->format);
            element.InputSlot                = static_cast<UINT>(pAttribute->binding);
            element.AlignedByteOffset        = static_cast<UINT>(pAttribute->offset);
            element.InputSlotClass           = (pAttribute->inputRate == grfx::VERETX_INPUT_RATE_INSTANCE) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
            element.InstanceDataStepRate     = (pAttribute->inputRate == grfx::VERETX_INPUT_RATE_INSTANCE) ? 1 : 0;
            inputElements.push_back(element);
        }
    }

    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    const grfx::ShaderStageInfo& VS = pCreateInfo->VS;

    HRESULT hr = device->CreateInputLayout(
        DataPtr(inputElements),
        static_cast<UINT>(CountU32(inputElements)),
        ToApi(VS.pModule)->GetCode(),
        ToApi(VS.pModule)->GetSize(),
        &mInputLayout);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeRasterizerState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    D3D11_RASTERIZER_DESC2 desc = {};
    desc.FillMode               = ToD3D11FillMode(pCreateInfo->rasterState.polygonMode);
    desc.CullMode               = ToD3D11CullMode(pCreateInfo->rasterState.cullMode);
    desc.FrontCounterClockwise  = (pCreateInfo->rasterState.frontFace == grfx::FRONT_FACE_CCW) ? TRUE : FALSE;
    desc.DepthBias              = pCreateInfo->rasterState.depthBiasEnable ? static_cast<INT>(pCreateInfo->rasterState.depthBiasConstantFactor) : 0;
    desc.DepthBiasClamp         = pCreateInfo->rasterState.depthBiasEnable ? pCreateInfo->rasterState.depthBiasClamp : 0;
    desc.SlopeScaledDepthBias   = pCreateInfo->rasterState.depthBiasEnable ? pCreateInfo->rasterState.depthBiasSlopeFactor : 0;
    desc.DepthClipEnable        = static_cast<BOOL>(pCreateInfo->rasterState.depthClipEnable);
    desc.ScissorEnable          = TRUE;
    desc.MultisampleEnable      = FALSE; // @TODO: Route this in
    desc.AntialiasedLineEnable  = FALSE;
    desc.ForcedSampleCount      = 0;
    desc.ConservativeRaster     = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    HRESULT hr = device->CreateRasterizerState2(&desc, &mRasterizerState);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeDepthStencilState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    D3D11_DEPTH_STENCIL_DESC desc     = {};
    desc.DepthEnable                  = static_cast<BOOL>(pCreateInfo->depthStencilState.depthTestEnable);
    desc.DepthWriteMask               = pCreateInfo->depthStencilState.depthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc                    = ToD3D11ComparisonFunc(pCreateInfo->depthStencilState.depthCompareOp);
    desc.StencilEnable                = static_cast<BOOL>(pCreateInfo->depthStencilState.stencilTestEnable);
    desc.StencilReadMask              = D3D11_DEFAULT_STENCIL_READ_MASK;  // @TODO: Figure out to set properly
    desc.StencilWriteMask             = D3D11_DEFAULT_STENCIL_WRITE_MASK; // @TODO: Figure out to set properly
    desc.FrontFace.StencilFailOp      = ToD3D11StencilOp(pCreateInfo->depthStencilState.front.failOp);
    desc.FrontFace.StencilDepthFailOp = ToD3D11StencilOp(pCreateInfo->depthStencilState.front.depthFailOp);
    desc.FrontFace.StencilPassOp      = ToD3D11StencilOp(pCreateInfo->depthStencilState.front.passOp);
    desc.FrontFace.StencilFunc        = ToD3D11ComparisonFunc(pCreateInfo->depthStencilState.front.compareOp);
    desc.BackFace.StencilFailOp       = ToD3D11StencilOp(pCreateInfo->depthStencilState.back.failOp);
    desc.BackFace.StencilDepthFailOp  = ToD3D11StencilOp(pCreateInfo->depthStencilState.back.depthFailOp);
    desc.BackFace.StencilPassOp       = ToD3D11StencilOp(pCreateInfo->depthStencilState.back.passOp);
    desc.BackFace.StencilFunc         = ToD3D11ComparisonFunc(pCreateInfo->depthStencilState.back.compareOp);

    HRESULT hr = device->CreateDepthStencilState(&desc, &mDepthStencilState);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::InitializeBlendState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    D3D11_BLEND_DESC1 desc      = {};
    desc.AlphaToCoverageEnable  = static_cast<BOOL>(pCreateInfo->multisampleState.alphaToCoverageEnable);
    desc.IndependentBlendEnable = (pCreateInfo->colorBlendState.blendAttachmentCount > 0) ? TRUE : FALSE;

    PPX_ASSERT_MSG(pCreateInfo->colorBlendState.blendAttachmentCount < PPX_MAX_RENDER_TARGETS, "blendAttachmentCount exceeds PPX_MAX_RENDER_TARGETS");
    for (uint32_t i = 0; i < pCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
        const grfx::BlendAttachmentState& ppxBlend = pCreateInfo->colorBlendState.blendAttachments[i];
        D3D11_RENDER_TARGET_BLEND_DESC1&  d3dBlend = desc.RenderTarget[i];

        d3dBlend.BlendEnable           = static_cast<BOOL>(ppxBlend.blendEnable);
        d3dBlend.LogicOpEnable         = static_cast<BOOL>(pCreateInfo->colorBlendState.logicOpEnable);
        d3dBlend.SrcBlend              = ToD3D11Blend(ppxBlend.srcColorBlendFactor);
        d3dBlend.DestBlend             = ToD3D11Blend(ppxBlend.dstColorBlendFactor);
        d3dBlend.BlendOp               = ToD3D11BlendOp(ppxBlend.colorBlendOp);
        d3dBlend.SrcBlendAlpha         = ToD3D11Blend(ppxBlend.srcAlphaBlendFactor);
        d3dBlend.DestBlendAlpha        = ToD3D11Blend(ppxBlend.dstAlphaBlendFactor);
        d3dBlend.BlendOpAlpha          = ToD3D11BlendOp(ppxBlend.alphaBlendOp);
        d3dBlend.LogicOp               = static_cast<D3D11_LOGIC_OP>(ToD3D11LogicOp(pCreateInfo->colorBlendState.logicOp));
        d3dBlend.RenderTargetWriteMask = ToD3D11WriteMask(ppxBlend.colorWriteMask);
    }

    HRESULT hr = device->CreateBlendState1(&desc, &mBlendState);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    return ppx::SUCCESS;
}

Result GraphicsPipeline::CreateApiObjects(const grfx::GraphicsPipelineCreateInfo* pCreateInfo)
{
    Result ppxres = InitializeShaders(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    if (pCreateInfo->vertexInputState.bindingCount > 0) {
        ppxres = InitializeInputLayout(pCreateInfo);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    mPrimitiveTopology = ToD3D11PrimitiveTopology(pCreateInfo->inputAssemblyState.topology);

    ppxres = InitializeRasterizerState(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = InitializeDepthStencilState(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = InitializeBlendState(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    size_t size = 4 * sizeof(float);
    std::memcpy(mBlendFactors, pCreateInfo->colorBlendState.blendConstants, size);

    return ppx::SUCCESS;
}

void GraphicsPipeline::DestroyApiObjects()
{
    if (mVS) {
        mVS.Reset();
    }

    if (mHS) {
        mHS.Reset();
    }

    if (mDS) {
        mDS.Reset();
    }

    if (mGS) {
        mGS.Reset();
    }

    if (mPS) {
        mPS.Reset();
    }

    if (mInputLayout) {
        mInputLayout.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// PipelineInterface
// -------------------------------------------------------------------------------------------------
Result PipelineInterface::CreateApiObjects(const grfx::PipelineInterfaceCreateInfo* pCreateInfo)
{
    return ppx::SUCCESS;
}

void PipelineInterface::DestroyApiObjects()
{
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
