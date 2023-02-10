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

#include "ppx/grfx/dx11/dx11_command_list.h"

#include <algorithm>

namespace ppx::grfx::dx11 {

class ContextBoundState
{
public:
    ContextBoundState() {}
    ~ContextBoundState() {}

    UINT VSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.VS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds VS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.VS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT HSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.HS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds HS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.HS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT DSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.DS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds DS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.DS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT GSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.GS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds GS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.GS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT PSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.PS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds PS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.PS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT CSGetBoundSRVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.CS.maxSlotSRV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds CS SRV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.CS.SRVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    UINT CSGetBoundUAVSlots(const ID3D11Resource* pResource, UINT* pSlot) const
    {
        UINT slotCount = 0;
        if (!IsNull(pResource)) {
            UINT n = mBoundState.CS.maxSlotUAV;
            PPX_ASSERT_MSG(n < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, "max slot exceeds CS UAV slot count");

            for (UINT i = 0; i <= n; ++i) {
                if (mBoundState.CS.UAVs[i].Get() == pResource) {
                    pSlot[slotCount] = i;
                    slotCount += 1;
                }
            }
        }
        return slotCount;
    }

    void VSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for VS SRV");
            return;
        }
        mBoundState.VS.SRVs[slot].Reset();
        mBoundState.VS.SRVs[slot] = resource;
        mBoundState.VS.maxSlotSRV = std::max<UINT>(mBoundState.VS.maxSlotSRV, slot);
    }

    void HSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for HS SRV");
            return;
        }
        mBoundState.HS.SRVs[slot].Reset();
        mBoundState.HS.SRVs[slot] = resource;
        mBoundState.HS.maxSlotSRV = std::max<UINT>(mBoundState.HS.maxSlotSRV, slot);
    }

    void DSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for DS SRV");
            return;
        }
        mBoundState.DS.SRVs[slot].Reset();
        mBoundState.DS.SRVs[slot] = resource;
        mBoundState.DS.maxSlotSRV = std::max<UINT>(mBoundState.DS.maxSlotSRV, slot);
    }

    void GSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for GS SRV");
            return;
        }
        mBoundState.GS.SRVs[slot].Reset();
        mBoundState.GS.SRVs[slot] = resource;
        mBoundState.GS.maxSlotSRV = std::max<UINT>(mBoundState.GS.maxSlotSRV, slot);
    }

    void PSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for PS SRV");
            return;
        }
        mBoundState.PS.SRVs[slot].Reset();
        mBoundState.PS.SRVs[slot] = resource;
        mBoundState.PS.maxSlotSRV = std::max<UINT>(mBoundState.PS.maxSlotSRV, slot);
    }

    void CSSetBoundSRVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for CS SRV");
            return;
        }
        mBoundState.CS.SRVs[slot].Reset();
        mBoundState.CS.SRVs[slot] = resource;
        mBoundState.CS.maxSlotSRV = std::max<UINT>(mBoundState.CS.maxSlotSRV, slot);
    }

    void CSSetBoundUAVSlot(UINT slot, const ComPtr<ID3D11Resource>& resource)
    {
        if (slot >= D3D11_1_UAV_SLOT_COUNT) {
            PPX_ASSERT_MSG(false, "invalid slot (" << slot << " for CS UAV");
            return;
        }
        mBoundState.CS.UAVs[slot].Reset();
        mBoundState.CS.UAVs[slot] = resource;
        mBoundState.CS.maxSlotUAV = std::max<UINT>(mBoundState.CS.maxSlotUAV, slot);
    }

private:
    struct ComputeShaderBoundState
    {
        UINT                   maxSlotSRV = 0;
        UINT                   maxSlotUAV = 0;
        ComPtr<ID3D11Resource> SRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
        ComPtr<ID3D11Resource> UAVs[D3D11_1_UAV_SLOT_COUNT];
    };

    struct GraphicsShaderBoundState
    {
        UINT                   maxSlotSRV = 0;
        ComPtr<ID3D11Resource> SRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    };

    struct BoundState
    {
        GraphicsShaderBoundState VS = {};
        GraphicsShaderBoundState HS = {};
        GraphicsShaderBoundState DS = {};
        GraphicsShaderBoundState GS = {};
        GraphicsShaderBoundState PS = {};
        ComputeShaderBoundState  CS = {};
    };

    BoundState mBoundState = {};
};

static ContextBoundState sContextBoundState;

// -------------------------------------------------------------------------------------------------
// CommandList
// -------------------------------------------------------------------------------------------------

CommandList::CommandList()
{
    mActions.reserve(32);
}

CommandList::~CommandList()
{
}

Action& CommandList::NewAction(Cmd cmd)
{
    uint32_t id = CountU32(mActions);
    mActions.emplace_back(Action{id, cmd});
    Action& action = mActions.back();
    return action;
}

void CommandList::Reset()
{
    mComputeSlotState.Reset();
    mGraphicsSlotState.Reset();
    mIndexBufferState.Reset();
    mVertexBufferState.Reset();
    mScissorState.Reset();
    mViewportState.Reset();
    mRTVDSVState.Reset();
    mPipelineState.Reset();

    mActions.clear();
}

static void UpdateConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers,
    ConstantBufferSlots& Slots)
{
    for (UINT i = 0; i < NumBuffers; ++i) {
        UINT          slot   = i + StartSlot;
        ID3D11Buffer* buffer = IsNull(ppConstantBuffers) ? nullptr : ppConstantBuffers[i];
        Slots.Buffers[slot]  = buffer;
    }

    UINT Index = Slots.NumBindings;
    PPX_ASSERT_MSG((Index < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT), "Index (" << Index << ") exceed D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT (" << D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT << ")");

    Slots.Bindings[Index].StartSlot = StartSlot;
    Slots.Bindings[Index].NumSlots  = NumBuffers;
    Slots.NumBindings += 1;
}

static void UpdateShaderResourceViews(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews,
    ShaderResourceViewSlots&         Slots)
{
    for (UINT i = 0; i < NumViews; ++i) {
        UINT                      slot = i + StartSlot;
        ID3D11ShaderResourceView* view = ppShaderResourceViews[i];
        Slots.Views[slot]              = view;

        ComPtr<ID3D11Resource> resource;
        view->GetResource(&resource);
        Slots.Resources[slot] = resource;
    }

    UINT Index = Slots.NumBindings;
    PPX_ASSERT_MSG((Index < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT), "Index (" << Index << ") exceed D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT (" << D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT << ")");

    Slots.Bindings[Index].StartSlot = StartSlot;
    Slots.Bindings[Index].NumSlots  = NumViews;
    Slots.NumBindings += 1;
}

static void UpdateSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers,
    SamplerSlots&              Slots)
{
    for (UINT i = 0; i < NumSamplers; ++i) {
        UINT                slot    = i + StartSlot;
        ID3D11SamplerState* sampler = IsNull(ppSamplers) ? nullptr : ppSamplers[i];
        Slots.Samplers[slot]        = sampler;
    }

    UINT Index = Slots.NumBindings;
    PPX_ASSERT_MSG((Index < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT), "Index (" << Index << ") exceed D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT (" << D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT << ")");

    Slots.Bindings[Index].StartSlot = StartSlot;
    Slots.Bindings[Index].NumSlots  = NumSamplers;
    Slots.NumBindings += 1;
}

static void UpdateUnorderedAccessViews(
    UINT                              StartSlot,
    UINT                              NumViews,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    UnorderedAccessViewSlots&         Slots)
{
    for (UINT i = 0; i < NumViews; ++i) {
        UINT                       slot = i + StartSlot;
        ID3D11UnorderedAccessView* view = ppUnorderedAccessViews[i];
        Slots.Views[slot]               = view;

        ComPtr<ID3D11Resource> resource;
        view->GetResource(&resource);
        Slots.Resources[slot] = resource;
    }

    UINT Index = Slots.NumBindings;
    PPX_ASSERT_MSG((Index < D3D11_1_UAV_SLOT_COUNT), "Index (" << Index << ") exceed D3D11_1_UAV_SLOT_COUNT (" << D3D11_1_UAV_SLOT_COUNT << ")");

    Slots.Bindings[Index].StartSlot = StartSlot;
    Slots.Bindings[Index].NumSlots  = NumViews;
    Slots.NumBindings += 1;
}

void CommandList::CSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    ComputeSlotState* pState = mComputeSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->CS.ConstantBuffers);
}

void CommandList::CSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    ComputeSlotState* pState = mComputeSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->CS.ShaderResourceViews);
}

void CommandList::CSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    ComputeSlotState* pState = mComputeSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->CS.Samplers);
}

void CommandList::CSSetUnorderedAccess(
    UINT                              StartSlot,
    UINT                              NumViews,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews)
{
    ComputeSlotState* pState = mComputeSlotState.GetCurrent();
    UpdateUnorderedAccessViews(StartSlot, NumViews, ppUnorderedAccessViews, pState->CS.UnorderedAccessViews);
}

void CommandList::DSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->DS.ConstantBuffers);
}

void CommandList::DSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->DS.ShaderResourceViews);
}

void CommandList::DSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->DS.Samplers);
}

void CommandList::GSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->GS.ConstantBuffers);
}

void CommandList::GSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->GS.ShaderResourceViews);
}

void CommandList::GSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->GS.Samplers);
}

void CommandList::HSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->HS.ConstantBuffers);
}

void CommandList::HSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->HS.ShaderResourceViews);
}

void CommandList::HSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->HS.Samplers);
}

void CommandList::PSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->PS.ConstantBuffers);
}

void CommandList::PSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->PS.ShaderResourceViews);
}

void CommandList::PSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->PS.Samplers);
}

void CommandList::VSSetConstantBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers, pState->VS.ConstantBuffers);
}

void CommandList::VSSetShaderResources(
    UINT                             StartSlot,
    UINT                             NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateShaderResourceViews(StartSlot, NumViews, ppShaderResourceViews, pState->VS.ShaderResourceViews);
}

void CommandList::VSSetSamplers(
    UINT                       StartSlot,
    UINT                       NumSamplers,
    ID3D11SamplerState* const* ppSamplers)
{
    GraphicsSlotState* pState = mGraphicsSlotState.GetCurrent();
    UpdateSamplers(StartSlot, NumSamplers, ppSamplers, pState->VS.Samplers);
}

void CommandList::IASetIndexBuffer(
    ID3D11Buffer* pIndexBuffer,
    DXGI_FORMAT   Format,
    UINT          Offset)
{
    IndexBufferState* pState = mIndexBufferState.GetCurrent();

    pState->pIndexBuffer = pIndexBuffer;
    pState->Format       = Format;
    pState->Offset       = Offset;
}

void CommandList::IASetVertexBuffers(
    UINT                 StartSlot,
    UINT                 NumBuffers,
    ID3D11Buffer* const* ppVertexBuffers,
    const UINT*          pStrides,
    const UINT*          pOffsets)
{
    VertexBufferState* pState = mVertexBufferState.GetCurrent();

    pState->StartSlot  = StartSlot;
    pState->NumBuffers = NumBuffers;
    for (UINT i = 0; i < NumBuffers; ++i) {
        pState->ppVertexBuffers[i] = ppVertexBuffers[i];
        pState->pStrides[i]        = pStrides[i];
        pState->pOffsets[i]        = pOffsets[i];
    }
}

void CommandList::RSSetScissorRects(
    UINT              NumRects,
    const D3D11_RECT* pRects)
{
    ScissorState* pState = mScissorState.GetCurrent();

    pState->NumRects = NumRects;
    for (UINT i = 0; i < NumRects; ++i) {
        memcpy(&pState->pRects[i], &pRects[i], sizeof(D3D11_RECT));
    }
}

void CommandList::RSSetViewports(
    UINT                  NumViewports,
    const D3D11_VIEWPORT* pViewports)
{
    ViewportState* pState = mViewportState.GetCurrent();

    pState->NumViewports = NumViewports;
    for (UINT i = 0; i < NumViewports; ++i) {
        memcpy(&pState->pViewports[i], &pViewports[i], sizeof(D3D11_VIEWPORT));
    }
}

void CommandList::OMSetRenderTargets(
    UINT                           NumViews,
    ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView*        pDepthStencilView)
{
    if (NumViews > D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) {
        PPX_ASSERT_MSG(false, "NumViews (" << NumViews << ") exceeds D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT (" << D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT << ")");
    }

    NumViews = std::min<UINT>(NumViews, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

    // Update state
    RTVDSVState* pState = mRTVDSVState.GetCurrent();
    {
        // Number of views
        pState->NumViews = NumViews;
        // Copy RTVs
        size_t size = NumViews * sizeof(ID3D11RenderTargetView*);
        std::memcpy(pState->ppRenderTargetViews, ppRenderTargetViews, size);
        // Copy DSV
        pState->pDepthStencilView = pDepthStencilView;
    }
}

void CommandList::SetPipelineState(const PipelineState* pPipelinestate)
{
    PipelineState* pState = mPipelineState.GetCurrent();

    size_t size = sizeof(PipelineState);
    std::memcpy(pState, pPipelinestate, size);
}

void CommandList::ClearDepthStencilView(
    ID3D11DepthStencilView* pDepthStencilView,
    UINT                    ClearFlags,
    FLOAT                   Depth,
    UINT8                   Stencil)
{
    Action& action = NewAction(CMD_CLEAR_DSV);

    action.args.clearDSV.rtvDsvStateIndex  = mRTVDSVState.Commit();
    action.args.clearDSV.pDepthStencilView = pDepthStencilView;
    action.args.clearDSV.ClearFlags        = ClearFlags;
    action.args.clearDSV.Depth             = Depth;
    action.args.clearDSV.Stencil           = Stencil;
}

void CommandList::ClearRenderTargetView(
    ID3D11RenderTargetView* pRenderTargetView,
    const FLOAT             ColorRGBA[4])
{
    Action& action = NewAction(CMD_CLEAR_RTV);

    action.args.clearRTV.rtvDsvStateIndex  = mRTVDSVState.Commit();
    action.args.clearRTV.pRenderTargetView = pRenderTargetView;
    action.args.clearRTV.ColorRGBA[0]      = ColorRGBA[0];
    action.args.clearRTV.ColorRGBA[1]      = ColorRGBA[1];
    action.args.clearRTV.ColorRGBA[2]      = ColorRGBA[2];
    action.args.clearRTV.ColorRGBA[3]      = ColorRGBA[3];
}

void CommandList::Nullify(
    ID3D11Resource* pResource,
    NullifyType     Type)
{
    Action& action = NewAction(CMD_NULLIFY);

    action.args.nullify.pResource = pResource;
    action.args.nullify.Type      = Type;
}

void CommandList::Dispatch(
    UINT ThreadGroupCountX,
    UINT ThreadGroupCountY,
    UINT ThreadGroupCountZ)
{
    Action& action = NewAction(CMD_DISPATCH);

    action.args.dispatch.computeSlotStateIndex = mComputeSlotState.Commit();
    action.args.dispatch.pipelineStateIndex    = mPipelineState.Commit();

    action.args.dispatch.ThreadGroupCountX = ThreadGroupCountX;
    action.args.dispatch.ThreadGroupCountY = ThreadGroupCountY;
    action.args.dispatch.ThreadGroupCountZ = ThreadGroupCountZ;
}

void CommandList::DrawInstanced(
    UINT VertexCountPerInstance,
    UINT InstanceCount,
    UINT StartVertexLocation,
    UINT StartInstanceLocation)
{
    Action& action = NewAction(CMD_DRAW);

    action.args.draw.graphicsSlotStateIndex = mGraphicsSlotState.Commit();
    action.args.draw.vertexBufferStateIndex = mVertexBufferState.Commit();
    action.args.draw.scissorStateIndex      = mScissorState.Commit();
    action.args.draw.viewportStateIndex     = mViewportState.Commit();
    action.args.draw.rtvDsvStateIndex       = mRTVDSVState.Commit();
    action.args.draw.pipelineStateIndex     = mPipelineState.Commit();

    action.args.draw.VertexCountPerInstance = VertexCountPerInstance;
    action.args.draw.InstanceCount          = InstanceCount;
    action.args.draw.StartVertexLocation    = StartVertexLocation;
    action.args.draw.StartInstanceLocation  = StartInstanceLocation;
}

void CommandList::DrawIndexedInstanced(
    UINT IndexCountPerInstance,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT  BaseVertexLocation,
    UINT StartInstanceLocation)
{
    Action& action = NewAction(CMD_DRAW_INDEXED);

    action.args.drawIndexed.graphicsSlotStateIndex = mGraphicsSlotState.Commit();
    action.args.drawIndexed.indexBufferStateIndex  = mIndexBufferState.Commit();
    action.args.drawIndexed.vertexBufferStateIndex = mVertexBufferState.Commit();
    action.args.drawIndexed.scissorStateIndex      = mScissorState.Commit();
    action.args.drawIndexed.viewportStateIndex     = mViewportState.Commit();
    action.args.drawIndexed.rtvDsvStateIndex       = mRTVDSVState.Commit();
    action.args.drawIndexed.pipelineStateIndex     = mPipelineState.Commit();

    action.args.drawIndexed.IndexCountPerInstance = IndexCountPerInstance;
    action.args.drawIndexed.InstanceCount         = InstanceCount;
    action.args.drawIndexed.StartIndexLocation    = StartIndexLocation;
    action.args.drawIndexed.BaseVertexLocation    = BaseVertexLocation;
    action.args.drawIndexed.StartInstanceLocation = StartInstanceLocation;
}

void CommandList::CopyBufferToBuffer(const args::CopyBufferToBuffer* pCopyArgs)
{
    Action& action = NewAction(CMD_COPY_BUFFER_TO_BUFFER);

    std::memcpy(&action.args.copyBufferToBuffer, pCopyArgs, sizeof(args::CopyBufferToBuffer));
}

void CommandList::CopyBufferToImage(const args::CopyBufferToImage* pCopyArgs)
{
    Action& action = NewAction(CMD_COPY_BUFFER_TO_IMAGE);

    std::memcpy(&action.args.copyBufferToImage, pCopyArgs, sizeof(args::CopyBufferToImage));
}

void CommandList::CopyImageToBuffer(const args::CopyImageToBuffer* pCopyArgs)
{
    Action& action = NewAction(CMD_COPY_IMAGE_TO_BUFFER);

    std::memcpy(&action.args.copyImageToBuffer, pCopyArgs, sizeof(args::CopyImageToBuffer));
}

void CommandList::CopyImageToImage(const args::CopyImageToImage* pCopyArgs)
{
    Action& action = NewAction(CMD_COPY_IMAGE_TO_IMAGE);

    std::memcpy(&action.args.copyImageToImage, pCopyArgs, sizeof(args::CopyImageToImage));
}

void CommandList::BeginQuery(const args::BeginQuery* pBeginQuery)
{
    Action& action = NewAction(CMD_BEGIN_QUERY);

    std::memcpy(&action.args.beginQuery, pBeginQuery, sizeof(args::BeginQuery));
}

void CommandList::EndQuery(const args::EndQuery* pEndQuery)
{
    Action& action = NewAction(CMD_END_QUERY);

    std::memcpy(&action.args.endQuery, pEndQuery, sizeof(args::EndQuery));
}

void CommandList::WriteTimestamp(const args::WriteTimestamp* pWriteTimestamp)
{
    Action& action = NewAction(CMD_WRITE_TIMESTAMP);

    std::memcpy(&action.args.writeTimestamp, pWriteTimestamp, sizeof(args::WriteTimestamp));
}

void CommandList::ImGuiRender(void (*pFn)(void))
{
    Action& action = NewAction(CMD_IMGUI_RENDER);

    action.args.imGuiRender.pRenderFn = pFn;
}

static bool ExecuteIndexChanged(uint32_t& execIndex, const uint32_t stateIndex)
{
    bool changed = (execIndex != stateIndex);
    if (changed) {
        execIndex = stateIndex;
    }
    return changed;
}

static void ExecuteSetConstantBufferSlots(
    D3D11_SHADER_VERSION_TYPE                      shaderType, // For debugging
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    void (D3D11DeviceContextPtr::InterfaceType::*SetConstantBuffersFn)(UINT, UINT, ID3D11Buffer* const*),
    const ConstantBufferSlots& slots)
{
    for (UINT i = 0; i < slots.NumBindings; ++i) {
        const SlotBindings&  binding   = slots.Bindings[i];
        ID3D11Buffer* const* ppBuffers = &slots.Buffers[binding.StartSlot];
        (pDeviceContext->*SetConstantBuffersFn)(binding.StartSlot, binding.NumSlots, ppBuffers);
    }
}

static void ExecuteSetShaderResourceViewSlots(
    D3D11_SHADER_VERSION_TYPE                      shaderType, // For debugging
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    void (D3D11DeviceContextPtr::InterfaceType::*SetShaderResourceViewsFn)(UINT, UINT, ID3D11ShaderResourceView* const*),
    void (ContextBoundState::*SetBoundSRVSlotFn)(UINT, const ComPtr<ID3D11Resource>&),
    const ShaderResourceViewSlots& slots)
{
    ContextBoundState* pContextBoundState = &sContextBoundState;
    for (UINT i = 0; i < slots.NumBindings; ++i) {
        const SlotBindings&              binding               = slots.Bindings[i];
        ID3D11ShaderResourceView* const* ppShaderResourceViews = &slots.Views[binding.StartSlot];
        (pDeviceContext->*SetShaderResourceViewsFn)(binding.StartSlot, binding.NumSlots, ppShaderResourceViews);
        for (UINT j = 0; j < binding.NumSlots; ++j) {
            UINT slot = binding.StartSlot + j;
            (pContextBoundState->*SetBoundSRVSlotFn)(slot, slots.Resources[slot]);
        }
    }
}

static void ExecuteSetSamplerSlots(
    D3D11_SHADER_VERSION_TYPE                      shaderType, // For debugging
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    void (D3D11DeviceContextPtr::InterfaceType::*SetSamplersFn)(UINT, UINT, ID3D11SamplerState* const*),
    const SamplerSlots& slots)
{
    for (UINT i = 0; i < slots.NumBindings; ++i) {
        const SlotBindings&        binding    = slots.Bindings[i];
        ID3D11SamplerState* const* ppSamplers = &slots.Samplers[binding.StartSlot];
        (pDeviceContext->*SetSamplersFn)(binding.StartSlot, binding.NumSlots, ppSamplers);
    }
}

static void ExecuteSetUnorderedAccessViewSlots(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    void (D3D11DeviceContextPtr::InterfaceType::*SetUnorderedAccessViewsFn)(UINT, UINT, ID3D11UnorderedAccessView* const*, const UINT*),
    void (ContextBoundState::*SetBoundUAVSlotFn)(UINT, const ComPtr<ID3D11Resource>&),
    const UnorderedAccessViewSlots& slots)
{
    ContextBoundState* pContextBoundState = &sContextBoundState;
    for (UINT i = 0; i < slots.NumBindings; ++i) {
        const SlotBindings&               binding                = slots.Bindings[i];
        ID3D11UnorderedAccessView* const* ppUnorderedAccessViews = &slots.Views[binding.StartSlot];
        (pDeviceContext->*SetUnorderedAccessViewsFn)(binding.StartSlot, binding.NumSlots, ppUnorderedAccessViews, nullptr);
        for (UINT j = 0; j < binding.NumSlots; ++j) {
            UINT slot = binding.StartSlot + j;
            (pContextBoundState->*SetBoundUAVSlotFn)(slot, slots.Resources[slot]);
        }
    }
}

static void ExecuteSetComputeSlotState(
    ExecutionState&         execState,
    const ComputeSlotState& state)
{
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_COMPUTE_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::CSSetConstantBuffers,
        state.CS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_COMPUTE_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::CSSetShaderResources,
        &ContextBoundState::CSSetBoundSRVSlot,
        state.CS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_COMPUTE_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::CSSetSamplers,
        state.CS.Samplers);
    ExecuteSetUnorderedAccessViewSlots(
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::CSSetUnorderedAccessViews,
        &ContextBoundState::CSSetBoundUAVSlot,
        state.CS.UnorderedAccessViews);
}

static void ExecuteSetGraphicsSlotState(
    ExecutionState&          execState,
    const GraphicsSlotState& state)
{
    // VS
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_VERTEX_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::VSSetConstantBuffers,
        state.VS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_VERTEX_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::VSSetShaderResources,
        &ContextBoundState::VSSetBoundSRVSlot,
        state.VS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_VERTEX_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::VSSetSamplers,
        state.VS.Samplers);

    // HS
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_HULL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::HSSetConstantBuffers,
        state.HS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_HULL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::HSSetShaderResources,
        &ContextBoundState::HSSetBoundSRVSlot,
        state.HS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_HULL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::HSSetSamplers,
        state.HS.Samplers);

    // DS
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_DOMAIN_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::DSSetConstantBuffers,
        state.DS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_DOMAIN_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::DSSetShaderResources,
        &ContextBoundState::DSSetBoundSRVSlot,
        state.DS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_DOMAIN_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::DSSetSamplers,
        state.DS.Samplers);

    // GS
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_GEOMETRY_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::GSSetConstantBuffers,
        state.GS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_GEOMETRY_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::GSSetShaderResources,
        &ContextBoundState::GSSetBoundSRVSlot,
        state.GS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_GEOMETRY_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::GSSetSamplers,
        state.GS.Samplers);

    // PS
    ExecuteSetConstantBufferSlots(
        D3D11_SHVER_PIXEL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::PSSetConstantBuffers,
        state.PS.ConstantBuffers);
    ExecuteSetShaderResourceViewSlots(
        D3D11_SHVER_PIXEL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::PSSetShaderResources,
        &ContextBoundState::PSSetBoundSRVSlot,
        state.PS.ShaderResourceViews);
    ExecuteSetSamplerSlots(
        D3D11_SHVER_PIXEL_SHADER,
        execState.pDeviceContext,
        &D3D11DeviceContextPtr::InterfaceType::PSSetSamplers,
        state.PS.Samplers);
}

static void ExecuteIASetIndexBuffer(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const IndexBufferState&                        state)
{
    pDeviceContext->IASetIndexBuffer(
        state.pIndexBuffer,
        state.Format,
        state.Offset);
}

static void ExecuteIASetVertexBuffers(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const VertexBufferState&                       state)
{
    pDeviceContext->IASetVertexBuffers(
        state.StartSlot,
        state.NumBuffers,
        state.ppVertexBuffers,
        state.pStrides,
        state.pOffsets);
}

static void ExecuteRSSetScissorRects(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const ScissorState&                            state)
{
    pDeviceContext->RSSetScissorRects(
        state.NumRects,
        state.pRects);
}

static void ExecuteRSSetViewports(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const ViewportState&                           state)
{
    pDeviceContext->RSSetViewports(
        state.NumViewports,
        state.pViewports);
}

static void ExecuteOMSetRenderTargets(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const RTVDSVState&                             state)
{
    pDeviceContext->OMSetRenderTargets(
        state.NumViews,
        state.ppRenderTargetViews,
        state.pDepthStencilView);
}

static void ExecuteSetComputePipelines(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const PipelineState&                           state)
{
    pDeviceContext->CSSetShader(state.CS, nullptr, 0);
}

static void ExecuteSetGraphicsPipelines(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const PipelineState&                           state)
{
    // clang-format off
    pDeviceContext->VSSetShader(state.VS, nullptr, 0);
    if (!IsNull(state.HS)) pDeviceContext->HSSetShader(state.HS, nullptr, 0);
    if (!IsNull(state.DS)) pDeviceContext->DSSetShader(state.DS, nullptr, 0);
    if (!IsNull(state.GS)) pDeviceContext->GSSetShader(state.GS, nullptr, 0);
    if (!IsNull(state.PS)) pDeviceContext->PSSetShader(state.PS, nullptr, 0);
    // clang-format on

    pDeviceContext->IASetInputLayout(state.InputLayout);
    pDeviceContext->IASetPrimitiveTopology(state.PrimitiveTopology);
    pDeviceContext->RSSetState(state.RasterizerState);
    pDeviceContext->OMSetBlendState(state.BlendState, state.BlendFactors, state.SampleMask);

    // @TODO: Figure out how to properly determine the StencilRef value
    //
    const UINT stencilRef = 1;
    pDeviceContext->OMSetDepthStencilState(state.DepthStencilState, stencilRef);
}

static void ExecuteNullify(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::Nullify&                           args)
{
    ID3D11ShaderResourceView*  nullSRV[1] = {nullptr};
    ID3D11UnorderedAccessView* nullUAV[1] = {nullptr};

    UINT         count = 0;
    UINT         slots[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    const size_t size = sizeof(UINT) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    if (args.Type == NULLIFY_TYPE_SRV) {
        // VS
        std::memset(slots, 0, size);
        count = sContextBoundState.VSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->VSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.VSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }

        // HS
        std::memset(slots, 0, size);
        count = sContextBoundState.HSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->HSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.HSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }

        // DS
        std::memset(slots, 0, size);
        count = sContextBoundState.DSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->DSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.DSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }

        // GS
        std::memset(slots, 0, size);
        count = sContextBoundState.GSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->GSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.GSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }

        // PS
        std::memset(slots, 0, size);
        count = sContextBoundState.PSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->PSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.PSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }

        // CS
        std::memset(slots, 0, size);
        count = sContextBoundState.CSGetBoundSRVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->CSSetShaderResources(slot, 1, nullSRV);
            sContextBoundState.CSSetBoundSRVSlot(slot, ComPtr<ID3D11Resource>());
        }
    }
    else if (args.Type == NULLIFY_TYPE_UAV) {
        std::memset(slots, 0, size);
        count = sContextBoundState.CSGetBoundUAVSlots(args.pResource, slots);
        for (UINT i = 0; i < count; ++i) {
            UINT slot = slots[i];
            pDeviceContext->CSSetUnorderedAccessViews(slot, 1, nullUAV, nullptr);
            sContextBoundState.CSSetBoundUAVSlot(slot, ComPtr<ID3D11Resource>());
        }
    }
}

static void ExecuteCopyBufferToBuffer(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::CopyBufferToBuffer&                args)
{
    D3D11_BOX srcBox = {};
    srcBox.left      = static_cast<UINT>(args.srcBufferOffset);
    srcBox.right     = static_cast<UINT>(args.srcBufferOffset + args.size);
    srcBox.top       = 0;
    srcBox.bottom    = 1;
    srcBox.front     = 0;
    srcBox.back      = 1;

    pDeviceContext->CopySubresourceRegion(
        args.pDstResource,
        0,
        static_cast<UINT>(args.dstBufferOffset),
        0,
        0,
        args.pSrcResource,
        0,
        &srcBox);
}

static void ExecuteCopyBufferToImage(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::CopyBufferToImage&                 args)
{
    D3D11_MAPPED_SUBRESOURCE mappedSubres = {};
    HRESULT                  hr           = pDeviceContext->Map(args.pSrcResource, 0, args.mapType, 0, &mappedSubres);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "could not map src buffer memory");
    }

    const char* pMappedAddress = static_cast<const char*>(mappedSubres.pData);
    for (uint32_t i = 0; i < args.dstImage.arrayLayerCount; ++i) {
        uint32_t arrayLayer       = args.dstImage.arrayLayer + i;
        UINT     subresourceIndex = static_cast<UINT>((args.dstImage.arrayLayer * args.mipSpan) + args.dstImage.mipLevel);

        D3D11_BOX dstBox = {};
        dstBox.left      = args.dstImage.x;
        dstBox.top       = args.dstImage.y;
        dstBox.front     = args.dstImage.z;
        dstBox.right     = args.dstImage.x + args.srcBuffer.footprintWidth;
        dstBox.bottom    = args.dstImage.y + args.srcBuffer.footprintHeight;
        dstBox.back      = args.dstImage.z + args.dstImage.depth;

        const char* pSrcData = pMappedAddress + args.srcBuffer.footprintOffset;

        UINT srcRowPitch   = static_cast<UINT>(args.srcBuffer.imageRowStride);
        UINT srcDepthPitch = static_cast<UINT>(args.srcBuffer.imageRowStride * args.srcBuffer.imageHeight);

        D3D11_COPY_FLAGS copyFlags = D3D11_COPY_DISCARD;
        if (args.isCube) {
            copyFlags = D3D11_COPY_NO_OVERWRITE;
        }

        pDeviceContext->UpdateSubresource1(
            args.pDstResource,
            subresourceIndex,
            &dstBox,
            pSrcData,
            srcRowPitch,
            srcDepthPitch,
            copyFlags);
    }

    pDeviceContext->Unmap(args.pSrcResource, 0);
}

static void ExecuteCopyImageToBuffer(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::CopyImageToBuffer&                 args)
{
    // In D3D11, we can't simply perform a GPU copy between an image and
    // a buffer. We must first map the image on the CPU, copy tightly-packed
    // texels to a staging CPU buffer, and finally copy it to the destination
    // buffer.
    // However, if the source image or the destination buffer are not
    // CPU-mappable, we must create staging resources.
    // To avoid considering all cases, we always employ the same strategy:
    //     1) Create a CPU-mappable staging image.
    //     2) Create a CPU-mappable staging buffer.
    //     3) Copy source image into staging image.
    //     4) Copy staging image into staging buffer.
    //     5) Copy staging buffer into destination buffer.

    ID3D11Device* device;
    pDeviceContext->GetDevice(&device);

    // 1) Create a CPU-mappable staging image.
    ID3D11Resource* stagingSrcResource = nullptr;
    if (args.srcTextureDimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D) {
        ID3D11Texture1D*     stagingTexture;
        D3D11_TEXTURE1D_DESC desc = args.srcTextureDesc.texture1D;
        desc.Width                = args.extent.x;
        desc.BindFlags            = 0;
        desc.Usage                = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
        device->CreateTexture1D(&desc, nullptr, &stagingTexture);
        stagingSrcResource = stagingTexture;
    }
    else if (args.srcTextureDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
        ID3D11Texture2D*     stagingTexture;
        D3D11_TEXTURE2D_DESC desc = args.srcTextureDesc.texture2D;
        desc.Width                = args.extent.x;
        desc.Height               = args.extent.y;
        desc.BindFlags            = 0;
        desc.Usage                = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
        HRESULT hr                = device->CreateTexture2D(&desc, nullptr, &stagingTexture);
        stagingSrcResource        = stagingTexture;
    }
    else {
        ID3D11Texture3D*     stagingTexture;
        D3D11_TEXTURE3D_DESC desc = args.srcTextureDesc.texture3D;
        desc.Width                = args.extent.x;
        desc.Height               = args.extent.y;
        desc.Depth                = args.extent.z;
        desc.BindFlags            = 0;
        desc.Usage                = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
        device->CreateTexture3D(&desc, nullptr, &stagingTexture);
        stagingSrcResource = stagingTexture;
    }
    PPX_ASSERT_MSG(stagingSrcResource != nullptr, "Failed to create staging image for image-to-buffer copy");

    // 2) Create a CPU-mappable staging buffer.
    ID3D11Resource* stagingDstResource = nullptr;
    {
        ID3D11Buffer*     stagingBuffer;
        D3D11_BUFFER_DESC desc = args.dstBufferDesc;
        desc.BindFlags         = 0;
        desc.Usage             = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&desc, nullptr, &stagingBuffer);
        stagingDstResource = stagingBuffer;
    }
    PPX_ASSERT_MSG(stagingDstResource != nullptr, "Failed to create staging buffer for image-to-buffer copy");

    // 3) Copy source image into staging image.
    UINT srcSubresourceIndex = ToSubresourceIndex(args.srcImage.mipLevel, args.srcImage.arrayLayer, args.srcMipLevels);

    // Depth-stencil textures can only be copied in full.
    if (args.isDepthStencilCopy) {
        pDeviceContext->CopySubresourceRegion(stagingSrcResource, 0, 0, 0, 0, args.pSrcResource, srcSubresourceIndex, nullptr);
    }
    else {
        D3D11_BOX srcBox = {0, 0, 0, 1, 1, 1};
        srcBox.left      = args.srcImage.offset.x;
        srcBox.right     = args.srcImage.offset.x + args.extent.x;
        if (args.srcTextureDimension != D3D11_RESOURCE_DIMENSION_TEXTURE1D) { // Can only be set for 2D and 3D textures.
            srcBox.top    = args.srcImage.offset.y;
            srcBox.bottom = args.srcImage.offset.y + args.extent.y;
        }
        if (args.srcTextureDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D) { // Can only be set for 3D textures.
            srcBox.front = args.srcImage.offset.z;
            srcBox.back  = args.srcImage.offset.z + args.extent.z;
        }
        pDeviceContext->CopySubresourceRegion(
            stagingSrcResource,
            0,
            0,
            0,
            0,
            args.pSrcResource,
            srcSubresourceIndex,
            &srcBox);
    }

    // 4) Copy staging image into staging buffer.
    D3D11_MAPPED_SUBRESOURCE mappedSrc = {};
    HRESULT                  hr        = pDeviceContext->Map(stagingSrcResource, srcSubresourceIndex, D3D11_MAP_READ, 0, &mappedSrc);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "could not map staging source image memory");
    }

    D3D11_MAPPED_SUBRESOURCE mappedDst = {};
    hr                                 = pDeviceContext->Map(stagingDstResource, 0, D3D11_MAP_WRITE, 0, &mappedDst);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "could not map staging destination buffer memory");
    }

    // Tigthly pack the texels.
    size_t bytesPerRow = static_cast<size_t>(args.srcBytesPerTexel) * args.extent.x;

    char* dst = (char*)mappedDst.pData;
    for (size_t d = 0; d < std::max(1u, args.extent.z); ++d) {
        char* src = ((char*)mappedSrc.pData) + d * mappedSrc.DepthPitch;
        for (size_t y = 0; y < std::max(1u, args.extent.y); ++y) {
            memcpy(dst, src, bytesPerRow);
            src += mappedSrc.RowPitch;
            dst += bytesPerRow;
        }
    }

    pDeviceContext->Unmap(stagingSrcResource, srcSubresourceIndex);
    pDeviceContext->Unmap(stagingDstResource, 0);

    // 5) Copy staging buffer into destination buffer.
    pDeviceContext->CopyResource(args.pDstResource, stagingDstResource);

    // Release staging resources.
    stagingSrcResource->Release();
    stagingDstResource->Release();
}

static void ExecuteCopyImageToImage(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::CopyImageToImage&                  args)
{
    for (uint32_t l = 0; l < args.srcImage.arrayLayerCount; ++l) {
        UINT srcSubresourceIndex = ToSubresourceIndex(args.srcImage.mipLevel, args.srcImage.arrayLayer + l, args.srcMipLevels);
        UINT dstSubresourceIndex = ToSubresourceIndex(args.dstImage.mipLevel, args.dstImage.arrayLayer + l, args.dstMipLevels);

        // Depth-stencil textures can only be copied in full.
        if (args.isDepthStencilCopy) {
            pDeviceContext->CopySubresourceRegion(args.pDstResource, dstSubresourceIndex, 0, 0, 0, args.pSrcResource, srcSubresourceIndex, nullptr);
        }
        else {
            D3D11_BOX srcBox = {0, 0, 0, 1, 1, 1};
            srcBox.left      = args.srcImage.offset.x;
            srcBox.right     = args.srcImage.offset.x + args.extent.x;
            if (args.srcTextureDimension != D3D11_RESOURCE_DIMENSION_TEXTURE1D) { // Can only be set for 2D and 3D textures.
                srcBox.top    = args.srcImage.offset.y;
                srcBox.bottom = args.srcImage.offset.y + args.extent.y;
            }
            if (args.srcTextureDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D) { // Can only be set for 3D textures.
                srcBox.front = args.srcImage.offset.z;
                srcBox.back  = args.srcImage.offset.z + args.extent.z;
            }

            pDeviceContext->CopySubresourceRegion(args.pDstResource, dstSubresourceIndex, args.dstImage.offset.x, args.dstImage.offset.y, args.dstImage.offset.z, args.pSrcResource, srcSubresourceIndex, &srcBox);
        }
    }
}

static void ExecuteBeginQuery(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::BeginQuery&                        args)
{
    pDeviceContext->Begin(args.pQuery);
}

static void ExecuteEndQuery(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::EndQuery&                          args)
{
    pDeviceContext->End(args.pQuery);
}

static void ExecuteWriteTimestamp(
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext,
    const args::WriteTimestamp&                    args)
{
    pDeviceContext->End(args.pQuery);
}

void CommandList::ExecuteClearDSV(ExecutionState& execState, const Action& action) const
{
    const args::ClearDSV& args = action.args.clearDSV;

    if (ExecuteIndexChanged(execState.rtvDsvStateIndex, args.rtvDsvStateIndex)) {
        const RTVDSVState& state = mRTVDSVState.At(execState.rtvDsvStateIndex);
        ExecuteOMSetRenderTargets(execState.pDeviceContext, state);
    }

    execState.pDeviceContext->ClearDepthStencilView(
        args.pDepthStencilView,
        args.ClearFlags,
        args.Depth,
        args.Stencil);
}

void CommandList::ExecuteClearRTV(ExecutionState& execState, const Action& action) const
{
    const args::ClearRTV& args = action.args.clearRTV;

    if (ExecuteIndexChanged(execState.rtvDsvStateIndex, args.rtvDsvStateIndex)) {
        const RTVDSVState& state = mRTVDSVState.At(execState.rtvDsvStateIndex);
        ExecuteOMSetRenderTargets(execState.pDeviceContext, state);
    }

    execState.pDeviceContext->ClearRenderTargetView(
        args.pRenderTargetView,
        args.ColorRGBA);
}

void CommandList::ExecuteDispatch(ExecutionState& execState, const Action& action) const
{
    const args::Dispatch& args = action.args.dispatch;

    if (ExecuteIndexChanged(execState.computeSlotStateIndex, args.computeSlotStateIndex)) {
        const ComputeSlotState& state = mComputeSlotState.At(execState.computeSlotStateIndex);
        ExecuteSetComputeSlotState(execState, state);
    }
    if (ExecuteIndexChanged(execState.pipelineStateIndex, args.pipelineStateIndex)) {
        const PipelineState& state = mPipelineState.At(execState.pipelineStateIndex);
        ExecuteSetComputePipelines(execState.pDeviceContext, state);
    }

    execState.pDeviceContext->Dispatch(
        args.ThreadGroupCountX,
        args.ThreadGroupCountY,
        args.ThreadGroupCountZ);
}

void CommandList::ExecuteDraw(ExecutionState& execState, const Action& action) const
{
    const args::Draw& args = action.args.draw;
    if (ExecuteIndexChanged(execState.graphicsSlotStateIndex, args.graphicsSlotStateIndex)) {
        const GraphicsSlotState& state = mGraphicsSlotState.At(execState.graphicsSlotStateIndex);
        ExecuteSetGraphicsSlotState(execState, state);
    }
    if (ExecuteIndexChanged(execState.vertexBufferStateIndex, args.vertexBufferStateIndex)) {
        const VertexBufferState& state = mVertexBufferState.At(execState.vertexBufferStateIndex);
        ExecuteIASetVertexBuffers(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.scissorStateIndex, args.scissorStateIndex)) {
        const ScissorState& state = mScissorState.At(execState.scissorStateIndex);
        ExecuteRSSetScissorRects(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.viewportStateIndex, args.viewportStateIndex)) {
        const ViewportState& state = mViewportState.At(execState.viewportStateIndex);
        ExecuteRSSetViewports(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.rtvDsvStateIndex, args.rtvDsvStateIndex)) {
        const RTVDSVState& state = mRTVDSVState.At(execState.rtvDsvStateIndex);
        ExecuteOMSetRenderTargets(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.pipelineStateIndex, args.pipelineStateIndex)) {
        const PipelineState& state = mPipelineState.At(execState.pipelineStateIndex);
        ExecuteSetGraphicsPipelines(execState.pDeviceContext, state);
    }

    execState.pDeviceContext->DrawInstanced(
        args.VertexCountPerInstance,
        args.InstanceCount,
        args.StartVertexLocation,
        args.StartInstanceLocation);
}

void CommandList::ExecuteDrawIndexed(ExecutionState& execState, const Action& action) const
{
    const args::DrawIndexed& args = action.args.drawIndexed;
    if (ExecuteIndexChanged(execState.graphicsSlotStateIndex, args.graphicsSlotStateIndex)) {
        const GraphicsSlotState& state = mGraphicsSlotState.At(execState.graphicsSlotStateIndex);
        ExecuteSetGraphicsSlotState(execState, state);
    }
    if (ExecuteIndexChanged(execState.indexBufferStateIndex, args.indexBufferStateIndex)) {
        const IndexBufferState& state = mIndexBufferState.At(execState.indexBufferStateIndex);
        ExecuteIASetIndexBuffer(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.vertexBufferStateIndex, args.vertexBufferStateIndex)) {
        const VertexBufferState& state = mVertexBufferState.At(execState.vertexBufferStateIndex);
        ExecuteIASetVertexBuffers(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.scissorStateIndex, args.scissorStateIndex)) {
        const ScissorState& state = mScissorState.At(execState.scissorStateIndex);
        ExecuteRSSetScissorRects(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.viewportStateIndex, args.viewportStateIndex)) {
        const ViewportState& state = mViewportState.At(execState.viewportStateIndex);
        ExecuteRSSetViewports(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.rtvDsvStateIndex, args.rtvDsvStateIndex)) {
        const RTVDSVState& state = mRTVDSVState.At(execState.rtvDsvStateIndex);
        ExecuteOMSetRenderTargets(execState.pDeviceContext, state);
    }
    if (ExecuteIndexChanged(execState.pipelineStateIndex, args.pipelineStateIndex)) {
        const PipelineState& state = mPipelineState.At(execState.pipelineStateIndex);
        ExecuteSetGraphicsPipelines(execState.pDeviceContext, state);
    }

    execState.pDeviceContext->DrawIndexedInstanced(
        args.IndexCountPerInstance,
        args.InstanceCount,
        args.StartIndexLocation,
        args.BaseVertexLocation,
        args.StartInstanceLocation);
}

void CommandList::Execute(typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext) const
{
    ExecutionState execState = {pDeviceContext};

    for (auto& action : mActions) {
        switch (action.cmd) {
            default: {
                PPX_ASSERT_MSG(false, "unrecognized command id: " << action.cmd);
            } break;

            case CMD_CLEAR_DSV: {
                ExecuteClearDSV(execState, action);
            } break;

            case CMD_CLEAR_RTV: {
                ExecuteClearRTV(execState, action);
            } break;

            case CMD_NULLIFY: {
                ExecuteNullify(pDeviceContext, action.args.nullify);
            } break;

            case CMD_DISPATCH: {
                ExecuteDispatch(execState, action);
            } break;

            case CMD_DRAW: {
                ExecuteDraw(execState, action);
            } break;

            case CMD_DRAW_INDEXED: {
                ExecuteDrawIndexed(execState, action);
            } break;

            case CMD_COPY_BUFFER_TO_BUFFER: {
                ExecuteCopyBufferToBuffer(pDeviceContext, action.args.copyBufferToBuffer);

            } break;

            case CMD_COPY_BUFFER_TO_IMAGE: {
                ExecuteCopyBufferToImage(pDeviceContext, action.args.copyBufferToImage);
            } break;

            case CMD_COPY_IMAGE_TO_BUFFER: {
                ExecuteCopyImageToBuffer(pDeviceContext, action.args.copyImageToBuffer);
            } break;

            case CMD_COPY_IMAGE_TO_IMAGE: {
                ExecuteCopyImageToImage(pDeviceContext, action.args.copyImageToImage);
            } break;

            case CMD_BEGIN_QUERY: {
                ExecuteBeginQuery(pDeviceContext, action.args.beginQuery);
            } break;

            case CMD_END_QUERY: {
                ExecuteEndQuery(pDeviceContext, action.args.endQuery);
            } break;

            case CMD_WRITE_TIMESTAMP: {
                ExecuteWriteTimestamp(pDeviceContext, action.args.writeTimestamp);
            } break;

            case CMD_IMGUI_RENDER: {
                void (*pRenderFn)(void) = action.args.imGuiRender.pRenderFn;
                pRenderFn();
            } break;
        }
    }
}

} // namespace ppx::grfx::dx11
