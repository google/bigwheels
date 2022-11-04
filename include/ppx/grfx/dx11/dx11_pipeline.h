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

#ifndef ppx_grfx_dx11_pipeline_h
#define ppx_grfx_dx11_pipeline_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_pipeline.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class ComputePipeline
    : public grfx::ComputePipeline
{
public:
    ComputePipeline() {}
    virtual ~ComputePipeline() {}

    typename D3D11ComputeShaderPtr::InterfaceType* GetCS() const { return mCS.Get(); };

protected:
    virtual Result CreateApiObjects(const grfx::ComputePipelineCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11ComputeShaderPtr mCS;
};

// -------------------------------------------------------------------------------------------------

class GraphicsPipeline
    : public grfx::GraphicsPipeline
{
public:
    GraphicsPipeline() {}
    virtual ~GraphicsPipeline() {}

    typename D3D11VertexShaderPtr::InterfaceType*      GetVS() const { return mVS.Get(); };
    typename D3D11HullShaderPtr::InterfaceType*        GetHS() const { return mHS.Get(); };
    typename D3D11DomainShaderPtr::InterfaceType*      GetDS() const { return mDS.Get(); };
    typename D3D11GeometryShaderPtr::InterfaceType*    GetGS() const { return mGS.Get(); };
    typename D3D11PixelShaderPtr::InterfaceType*       GetPS() const { return mPS.Get(); };
    typename D3D11InputLayoutPtr::InterfaceType*       GetInputLayout() const { return mInputLayout.Get(); };
    D3D11_PRIMITIVE_TOPOLOGY                           GetPrimitiveTopology() const { return mPrimitiveTopology; }
    typename D3D11RasterizerStatePtr::InterfaceType*   GetRasterizerState() const { return mRasterizerState.Get(); };
    typename D3D11DepthStencilStatePtr::InterfaceType* GetDepthStencilState() const { return mDepthStencilState.Get(); };
    typename D3D11BlendStatePtr::InterfaceType*        GetBlendState() const { return mBlendState.Get(); };
    const FLOAT*                                       GetBlendFactors() const { return mBlendFactors; }
    UINT                                               GetSampleMask() const { return mSampleMask; }

protected:
    virtual Result CreateApiObjects(const grfx::GraphicsPipelineCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result InitializeShaders(const grfx::GraphicsPipelineCreateInfo* pCreateInfo);
    Result InitializeInputLayout(const grfx::GraphicsPipelineCreateInfo* pCreateInfo);
    Result InitializeRasterizerState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo);
    Result InitializeDepthStencilState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo);
    Result InitializeBlendState(const grfx::GraphicsPipelineCreateInfo* pCreateInfo);

private:
    D3D11VertexShaderPtr      mVS;
    D3D11HullShaderPtr        mHS;
    D3D11DomainShaderPtr      mDS;
    D3D11GeometryShaderPtr    mGS;
    D3D11PixelShaderPtr       mPS;
    D3D11InputLayoutPtr       mInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY  mPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D11RasterizerStatePtr   mRasterizerState;
    D3D11DepthStencilStatePtr mDepthStencilState;
    D3D11BlendStatePtr        mBlendState;
    FLOAT                     mBlendFactors[4];
    UINT                      mSampleMask = UINT_MAX; // @TODO: Figure out the right way to handle this
};

// -------------------------------------------------------------------------------------------------

class PipelineInterface
    : public grfx::PipelineInterface
{
public:
    PipelineInterface() {}
    virtual ~PipelineInterface() {}

protected:
    virtual Result CreateApiObjects(const grfx::PipelineInterfaceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_pipeline_h
