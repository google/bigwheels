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

#ifndef ppx_grfx_dx12_pipeline_h
#define ppx_grfx_dx12_pipeline_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_pipeline.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class ComputePipeline
    : public grfx::ComputePipeline
{
public:
    ComputePipeline() {}
    virtual ~ComputePipeline() {}

    D3D12PipelineStatePtr GetDxPipeline() const { return mPipeline; }

protected:
    virtual Result CreateApiObjects(const grfx::ComputePipelineCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12PipelineStatePtr mPipeline;
};

// -------------------------------------------------------------------------------------------------

class GraphicsPipeline
    : public grfx::GraphicsPipeline
{
public:
    GraphicsPipeline() {}
    virtual ~GraphicsPipeline() {}

    D3D12PipelineStatePtr  GetDxPipeline() const { return mPipeline; }
    D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return mPrimitiveTopology; }

protected:
    virtual Result CreateApiObjects(const grfx::GraphicsPipelineCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    void InitializeShaderStages(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        D3D12_GRAPHICS_PIPELINE_STATE_DESC&     desc);

    void InitializeBlendState(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        D3D12_BLEND_DESC&                       desc);

    void InitializeRasterizerState(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        D3D12_RASTERIZER_DESC&                  desc);

    void InitializeDepthStencilState(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        D3D12_DEPTH_STENCIL_DESC&               desc);

    void InitializeInputLayout(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        std::vector<D3D12_INPUT_ELEMENT_DESC>&  inputElements,
        D3D12_INPUT_LAYOUT_DESC&                desc);

    void InitializeOutput(
        const grfx::GraphicsPipelineCreateInfo* pCreateInfo,
        D3D12_GRAPHICS_PIPELINE_STATE_DESC&     desc);

private:
    D3D12PipelineStatePtr  mPipeline;
    D3D_PRIMITIVE_TOPOLOGY mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

// -------------------------------------------------------------------------------------------------

class PipelineInterface
    : public grfx::PipelineInterface
{
public:
    PipelineInterface() {}
    virtual ~PipelineInterface() {}

    D3D12RootSignaturePtr GetDxRootSignature() const { return mRootSignature; }
    uint32_t              GetParameterIndexCount() const { return CountU32(mParameterIndices); }
    UINT                  FindParameterIndex(uint32_t set, uint32_t binding) const;

protected:
    virtual Result CreateApiObjects(const grfx::PipelineInterfaceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D12RootSignaturePtr mRootSignature;

    struct ParameterIndex
    {
        uint32_t set     = PPX_VALUE_IGNORED;
        uint32_t binding = PPX_VALUE_IGNORED;
        uint32_t index   = PPX_VALUE_IGNORED;
    };
    std::vector<ParameterIndex> mParameterIndices;
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_pipeline_h
