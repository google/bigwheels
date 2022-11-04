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

#ifndef dx11_command_list_hpp
#define dx11_command_list_hpp

#include "ppx/grfx/dx11/dx11_config.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// Enums
// -------------------------------------------------------------------------------------------------

enum Cmd
{
    CMD_UNDEFINED = 0,
    CMD_CLEAR_DSV,
    CMD_CLEAR_RTV,
    CMD_NULLIFY,
    CMD_DISPATCH,
    CMD_DRAW,
    CMD_DRAW_INDEXED,
    CMD_COPY_BUFFER_TO_BUFFER,
    CMD_COPY_BUFFER_TO_IMAGE,
    CMD_COPY_IMAGE_TO_BUFFER,
    CMD_COPY_IMAGE_TO_IMAGE,
    CMD_BEGIN_QUERY,
    CMD_END_QUERY,
    CMD_WRITE_TIMESTAMP,
    CMD_IMGUI_RENDER,
};

enum NullifyStage
{
    NULLIFY_STAGE_UNDEFINED = 0,
    NULLIFY_STAGE_VS        = 1,
    NULLIFY_STAGE_HS        = 2,
    NULLIFY_STAGE_DS        = 3,
    NULLIFY_STAGE_GS        = 4,
    NULLIFY_STAGE_PS        = 5,
    NULLIFY_STAGE_CS        = 6,
};

enum NullifyType
{
    NULLIFY_TYPE_UNDEFINED = 0,
    NULLIFY_TYPE_SRV       = 1,
    NULLIFY_TYPE_UAV       = 2,
};

// -------------------------------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------------------------------

struct SlotBindings
{
    UINT StartSlot;
    UINT NumSlots;
};

struct ConstantBufferSlots
{
    ID3D11Buffer* Buffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    UINT          NumBindings = 0;
    SlotBindings  Bindings[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];

    void NewCommitInit()
    {
        NumBindings = 0;
        std::memset(&Bindings, 0, sizeof(SlotBindings) * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
    }
};

struct ShaderResourceViewSlots
{
    ID3D11ShaderResourceView* Views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ComPtr<ID3D11Resource>    Resources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    UINT                      NumBindings = 0;
    SlotBindings              Bindings[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

    void NewCommitInit()
    {
        NumBindings = 0;
        std::memset(&Bindings, 0, sizeof(SlotBindings) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
    }
};

struct SamplerSlots
{
    ID3D11SamplerState* Samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    UINT                NumBindings = 0;
    SlotBindings        Bindings[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

    void NewCommitInit()
    {
        NumBindings = 0;
        std::memset(&Bindings, 0, sizeof(SlotBindings) * D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
    }
};

struct UnorderedAccessViewSlots
{
    ID3D11UnorderedAccessView* Views[D3D11_1_UAV_SLOT_COUNT];
    ComPtr<ID3D11Resource>     Resources[D3D11_1_UAV_SLOT_COUNT];
    UINT                       NumBindings = 0;
    SlotBindings               Bindings[D3D11_1_UAV_SLOT_COUNT];

    void NewCommitInit()
    {
        NumBindings = 0;
        std::memset(&Bindings, 0, sizeof(SlotBindings) * D3D11_1_UAV_SLOT_COUNT);
    }
};

struct ComputeShaderSlots
{
    ConstantBufferSlots      ConstantBuffers;
    ShaderResourceViewSlots  ShaderResourceViews;
    SamplerSlots             Samplers;
    UnorderedAccessViewSlots UnorderedAccessViews;

    void NewCommitInit()
    {
        ConstantBuffers.NewCommitInit();
        ShaderResourceViews.NewCommitInit();
        Samplers.NewCommitInit();
        UnorderedAccessViews.NewCommitInit();
    }
};

struct GraphicsShaderSlot
{
    ConstantBufferSlots     ConstantBuffers;
    ShaderResourceViewSlots ShaderResourceViews;
    SamplerSlots            Samplers;

    void NewCommitInit()
    {
        ConstantBuffers.NewCommitInit();
        ShaderResourceViews.NewCommitInit();
        Samplers.NewCommitInit();
    }
};

// -------------------------------------------------------------------------------------------------
// States
// -------------------------------------------------------------------------------------------------

struct IndexBufferState
{
    ID3D11Buffer* pIndexBuffer;
    DXGI_FORMAT   Format;
    UINT          Offset;

    void Reset()
    {
        pIndexBuffer = nullptr;
        Format       = DXGI_FORMAT_UNKNOWN;
        Offset       = 0;
    }

    void NewCommitInit() {}
};

struct VertexBufferState
{
    UINT          StartSlot;
    UINT          NumBuffers;
    ID3D11Buffer* ppVertexBuffers[PPX_MAX_VERTEX_BINDINGS];
    UINT          pStrides[PPX_MAX_VERTEX_BINDINGS];
    UINT          pOffsets[PPX_MAX_VERTEX_BINDINGS];

    void Reset()
    {
        StartSlot  = 0;
        NumBuffers = 0;
        for (UINT i = 0; i < PPX_MAX_VERTEX_BINDINGS; ++i) {
            ppVertexBuffers[i] = nullptr;
            pStrides[i]        = 0;
            pOffsets[i]        = 0;
        }
    }

    void NewCommitInit() {}
};

struct ComputeSlotState
{
    ComputeShaderSlots CS;

    void Reset()
    {
        std::memset(reinterpret_cast<void*>(&CS), 0, sizeof(ComputeShaderSlots));
    }

    void NewCommitInit()
    {
        CS.NewCommitInit();
    }
};

struct GraphicsSlotState
{
    GraphicsShaderSlot VS;
    GraphicsShaderSlot HS;
    GraphicsShaderSlot DS;
    GraphicsShaderSlot GS;
    GraphicsShaderSlot PS;

    void Reset()
    {
        std::memset(reinterpret_cast<void*>(&VS), 0, sizeof(GraphicsShaderSlot));
        std::memset(reinterpret_cast<void*>(&HS), 0, sizeof(GraphicsShaderSlot));
        std::memset(reinterpret_cast<void*>(&DS), 0, sizeof(GraphicsShaderSlot));
        std::memset(reinterpret_cast<void*>(&GS), 0, sizeof(GraphicsShaderSlot));
        std::memset(reinterpret_cast<void*>(&PS), 0, sizeof(GraphicsShaderSlot));
    }

    void NewCommitInit()
    {
        VS.NewCommitInit();
        HS.NewCommitInit();
        DS.NewCommitInit();
        GS.NewCommitInit();
        PS.NewCommitInit();
    }
};

struct ScissorState
{
    UINT       NumRects = 0;
    D3D11_RECT pRects[PPX_MAX_SCISSORS];

    void Reset()
    {
        NumRects = 0;
        std::memset(pRects, 0, PPX_MAX_SCISSORS * sizeof(D3D11_RECT));
    }

    void NewCommitInit() {}
};

struct ViewportState
{
    UINT           NumViewports;
    D3D11_VIEWPORT pViewports[PPX_MAX_VIEWPORTS];

    void Reset()
    {
        NumViewports = 0;
        std::memset(pViewports, 0, PPX_MAX_VIEWPORTS * sizeof(D3D11_VIEWPORT));
    }

    void NewCommitInit() {}
};

struct RTVDSVState
{
    UINT                    NumViews;
    ID3D11RenderTargetView* ppRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* pDepthStencilView;

    void Reset()
    {
        NumViews          = 0;
        pDepthStencilView = nullptr;

        size_t size = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT * sizeof(ID3D11RenderTargetView*);
        std::memset(ppRenderTargetViews, 0, size);
    }

    void NewCommitInit() {}
};

struct PipelineState
{
    ID3D11VertexShader*      VS;
    ID3D11HullShader*        HS;
    ID3D11DomainShader*      DS;
    ID3D11GeometryShader*    GS;
    ID3D11PixelShader*       PS;
    ID3D11ComputeShader*     CS;
    ID3D11InputLayout*       InputLayout;
    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
    ID3D11RasterizerState2*  RasterizerState;
    ID3D11DepthStencilState* DepthStencilState;
    ID3D11BlendState*        BlendState;
    FLOAT                    BlendFactors[4];
    UINT                     SampleMask;

    void Reset()
    {
        VS                = nullptr;
        HS                = nullptr;
        DS                = nullptr;
        GS                = nullptr;
        PS                = nullptr;
        CS                = nullptr;
        InputLayout       = nullptr;
        PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        RasterizerState   = nullptr;
        DepthStencilState = nullptr;
        BlendState        = nullptr;
    }

    void NewCommitInit() {}
};

struct ExecutionState
{
    typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext = nullptr;

    uint32_t computeSlotStateIndex  = dx11::kInvalidStateIndex;
    uint32_t graphicsSlotStateIndex = dx11::kInvalidStateIndex;
    uint32_t indexBufferStateIndex  = dx11::kInvalidStateIndex;
    uint32_t vertexBufferStateIndex = dx11::kInvalidStateIndex;
    uint32_t scissorStateIndex      = dx11::kInvalidStateIndex;
    uint32_t viewportStateIndex     = dx11::kInvalidStateIndex;
    uint32_t rtvDsvStateIndex       = dx11::kInvalidStateIndex;
    uint32_t pipelineStateIndex     = dx11::kInvalidStateIndex;
};

template <typename DataT>
class StateStackT
{
public:
    StateStackT()
    {
        mStack.reserve(32);
        Reset();
    }

    void Reset()
    {
        mDirty = false;
        mStack.resize(1);
        mCurrent = &mStack.back();
        mCurrent->Reset();
        mCommitedIndex = 0;
    }

    DataT* GetCurrent()
    {
        //
        // Assume any calls to this function will write data
        //
        mDirty = true;
        return mCurrent;
    }

    const DataT* GetCurrent() const
    {
        return mCurrent;
    }

    const DataT& At(uint32_t index) const
    {
        return mStack[index];
    }

    uint32_t Commit()
    {
        if (mDirty) {
            mCommitedIndex = static_cast<uint32_t>(mStack.size() - 1);
            mStack.emplace_back(mStack.back());
            mCurrent = &mStack.back();
            mCurrent->NewCommitInit();
            mDirty = false;
        }
        return mCommitedIndex;
    }

private:
    bool               mDirty         = false;
    std::vector<DataT> mStack         = {};
    DataT*             mCurrent       = nullptr;
    uint32_t           mCommitedIndex = 0;
};

// -------------------------------------------------------------------------------------------------
// Args
// -------------------------------------------------------------------------------------------------

namespace args {

struct ClearDSV
{
    uint32_t rtvDsvStateIndex;

    ID3D11DepthStencilView* pDepthStencilView;
    UINT                    ClearFlags;
    FLOAT                   Depth;
    UINT8                   Stencil;
};

struct ClearRTV
{
    uint32_t rtvDsvStateIndex;

    ID3D11RenderTargetView* pRenderTargetView;
    FLOAT                   ColorRGBA[4];
};

struct Nullify
{
    ID3D11Resource* pResource = nullptr;
    NullifyStage    Stage     = NULLIFY_STAGE_UNDEFINED;
    NullifyType     Type      = NULLIFY_TYPE_UNDEFINED;
};

struct Dispatch
{
    uint32_t computeSlotStateIndex = dx11::kInvalidStateIndex;
    uint32_t pipelineStateIndex    = dx11::kInvalidStateIndex;

    UINT ThreadGroupCountX;
    UINT ThreadGroupCountY;
    UINT ThreadGroupCountZ;
};

struct Draw
{
    uint32_t graphicsSlotStateIndex = dx11::kInvalidStateIndex;
    uint32_t vertexBufferStateIndex = dx11::kInvalidStateIndex;
    uint32_t scissorStateIndex      = dx11::kInvalidStateIndex;
    uint32_t viewportStateIndex     = dx11::kInvalidStateIndex;
    uint32_t rtvDsvStateIndex       = dx11::kInvalidStateIndex;
    uint32_t pipelineStateIndex     = dx11::kInvalidStateIndex;

    UINT VertexCountPerInstance;
    UINT InstanceCount;
    UINT StartVertexLocation;
    UINT StartInstanceLocation;
};

struct DrawIndexed
{
    uint32_t computeSlotStateIndex  = dx11::kInvalidStateIndex;
    uint32_t graphicsSlotStateIndex = dx11::kInvalidStateIndex;
    uint32_t indexBufferStateIndex  = dx11::kInvalidStateIndex;
    uint32_t vertexBufferStateIndex = dx11::kInvalidStateIndex;
    uint32_t scissorStateIndex      = dx11::kInvalidStateIndex;
    uint32_t viewportStateIndex     = dx11::kInvalidStateIndex;
    uint32_t rtvDsvStateIndex       = dx11::kInvalidStateIndex;
    uint32_t pipelineStateIndex     = dx11::kInvalidStateIndex;

    UINT IndexCountPerInstance;
    UINT InstanceCount;
    UINT StartIndexLocation;
    INT  BaseVertexLocation;
    UINT StartInstanceLocation;
};

struct CopyBufferToBuffer
{
    uint32_t        size            = 0;
    uint32_t        srcBufferOffset = 0;
    uint32_t        dstBufferOffset = 0;
    ID3D11Resource* pSrcResource    = nullptr;
    ID3D11Resource* pDstResource    = nullptr;
};

struct CopyBufferToImage
{
    struct
    {
        uint32_t imageWidth      = 0; // [pixels]
        uint32_t imageHeight     = 0; // [pixels]
        uint32_t imageRowStride  = 0; // [bytes]
        uint64_t footprintOffset = 0; // [bytes]
        uint32_t footprintWidth  = 0; // [pixels]
        uint32_t footprintHeight = 0; // [pixels]
        uint32_t footprintDepth  = 0; // [pixels]
    } srcBuffer;

    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
        uint32_t x               = 0; // [pixels]
        uint32_t y               = 0; // [pixels]
        uint32_t z               = 0; // [pixels]
        uint32_t width           = 0; // [pixels]
        uint32_t height          = 0; // [pixels]
        uint32_t depth           = 0; // [pixels]
    } dstImage;

    D3D11_MAP       mapType      = InvalidValue<D3D11_MAP>();
    bool            isCube       = false;
    uint32_t        mipSpan      = 0;
    ID3D11Resource* pSrcResource = nullptr;
    ID3D11Resource* pDstResource = nullptr;
};

struct CopyImageToBuffer
{
    struct
    {
        uint32_t mipLevel   = 0;
        uint32_t arrayLayer = 0; // Must be 0 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } srcImage;

    struct
    {
        uint32_t x = 0; // [pixels]
        uint32_t y = 0; // [pixels]
        uint32_t z = 0; // [pixels]
    } extent;

    bool     isDepthStencilCopy = false;
    uint32_t srcMipLevels       = 0;
    uint32_t srcBytesPerTexel   = 0;
    union
    {
        D3D11_TEXTURE1D_DESC texture1D;
        D3D11_TEXTURE2D_DESC texture2D;
        D3D11_TEXTURE3D_DESC texture3D;
    } srcTextureDesc;
    D3D11_RESOURCE_DIMENSION srcTextureDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    ID3D11Resource*          pSrcResource        = nullptr;
    D3D11_BUFFER_DESC        dstBufferDesc;
    ID3D11Resource*          pDstResource = nullptr;
};

struct CopyImageToImage
{
    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } srcImage;

    struct
    {
        uint32_t mipLevel        = 0;
        uint32_t arrayLayer      = 0; // Must be 0 for 3D images
        uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
        struct
        {
            uint32_t x = 0; // [pixels]
            uint32_t y = 0; // [pixels]
            uint32_t z = 0; // [pixels]
        } offset;
    } dstImage;

    struct
    {
        uint32_t x = 0; // [pixels]
        uint32_t y = 0; // [pixels]
        uint32_t z = 0; // [pixels]
    } extent;

    bool                     isDepthStencilCopy  = false;
    uint32_t                 srcMipLevels        = 0;
    uint32_t                 dstMipLevels        = 0;
    D3D11_RESOURCE_DIMENSION srcTextureDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    ID3D11Resource*          pSrcResource        = nullptr;
    ID3D11Resource*          pDstResource        = nullptr;
};

struct BeginQuery
{
    ID3D11Query* pQuery;
};

struct EndQuery
{
    ID3D11Query* pQuery;
};

struct WriteTimestamp
{
    ID3D11Query* pQuery;
};

struct ImGuiRender
{
    void (*pRenderFn)(void) = nullptr;
};

} // namespace args

// -------------------------------------------------------------------------------------------------
// Command List
// -------------------------------------------------------------------------------------------------

struct Action
{
    uint32_t id  = dx11::kInvalidStateIndex;
    Cmd      cmd = CMD_UNDEFINED;

    struct
    {
        union
        {
            args::ClearDSV           clearDSV;
            args::ClearRTV           clearRTV;
            args::Nullify            nullify;
            args::Dispatch           dispatch;
            args::Draw               draw;
            args::DrawIndexed        drawIndexed;
            args::CopyBufferToBuffer copyBufferToBuffer;
            args::CopyBufferToImage  copyBufferToImage;
            args::CopyImageToBuffer  copyImageToBuffer;
            args::CopyImageToImage   copyImageToImage;
            args::BeginQuery         beginQuery;
            args::EndQuery           endQuery;
            args::WriteTimestamp     writeTimestamp;
            args::ImGuiRender        imGuiRender;
        };
    } args;
};

class CommandList
{
public:
    CommandList();
    ~CommandList();

    void Reset();

    void CSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void CSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void CSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void CSSetUnorderedAccess(
        UINT                              StartSlot,
        UINT                              NumViews,
        ID3D11UnorderedAccessView* const* ppUnorderedAccessViews);

    void DSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void DSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void DSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void GSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void GSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void GSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void HSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void HSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void HSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void PSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void PSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void PSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void VSSetConstantBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    void VSSetShaderResources(
        UINT                             StartSlot,
        UINT                             NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    void VSSetSamplers(
        UINT                       StartSlot,
        UINT                       NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    void IASetIndexBuffer(
        ID3D11Buffer* pIndexBuffer,
        DXGI_FORMAT   Format,
        UINT          Offset);

    void IASetVertexBuffers(
        UINT                 StartSlot,
        UINT                 NumBuffers,
        ID3D11Buffer* const* ppVertexBuffers,
        const UINT*          pStrides,
        const UINT*          pOffsets);

    void RSSetScissorRects(
        UINT              NumRects,
        const D3D11_RECT* pRects);

    void RSSetViewports(
        UINT                  NumViewports,
        const D3D11_VIEWPORT* pViewports);

    void OMSetRenderTargets(
        UINT                           NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView*        pDepthStencilView);

    void SetPipelineState(const PipelineState* pPipelinestate);

    void ClearDepthStencilView(
        ID3D11DepthStencilView* pDepthStencilView,
        UINT                    ClearFlags,
        FLOAT                   Depth,
        UINT8                   Stencil);

    void ClearRenderTargetView(
        ID3D11RenderTargetView* pRenderTargetView,
        const FLOAT             ColorRGBA[4]);

    void Nullify(
        ID3D11Resource* pResource,
        NullifyType     Type);

    void Dispatch(
        UINT ThreadGroupCountX,
        UINT ThreadGroupCountY,
        UINT ThreadGroupCountZ);

    void DrawInstanced(
        UINT VertexCountPerInstance,
        UINT InstanceCount,
        UINT StartVertexLocation,
        UINT StartInstanceLocation);

    void DrawIndexedInstanced(
        UINT IndexCountPerInstance,
        UINT InstanceCount,
        UINT StartIndexLocation,
        INT  BaseVertexLocation,
        UINT StartInstanceLocation);

    void CopyBufferToBuffer(const args::CopyBufferToBuffer* pCopyArgs);

    void CopyBufferToImage(const args::CopyBufferToImage* pCopyArgs);

    void CopyImageToBuffer(const args::CopyImageToBuffer* pCopyArgs);

    void CopyImageToImage(const args::CopyImageToImage* pCopyArgs);

    void BeginQuery(const args::BeginQuery* pBeginQuery);
    void EndQuery(const args::EndQuery* pEndQuery);
    void WriteTimestamp(const args::WriteTimestamp* pWriteTimestamp);

    void ImGuiRender(void (*pFn)(void));

    void Execute(typename D3D11DeviceContextPtr::InterfaceType* pDeviceContext) const;

private:
    Action& NewAction(Cmd cmd);

    void ExecuteClearDSV(ExecutionState& execState, const Action& action) const;
    void ExecuteClearRTV(ExecutionState& execState, const Action& action) const;
    void ExecuteDispatch(ExecutionState& execState, const Action& action) const;
    void ExecuteDraw(ExecutionState& execState, const Action& action) const;
    void ExecuteDrawIndexed(ExecutionState& execState, const Action& action) const;

private:
    StateStackT<ComputeSlotState>  mComputeSlotState;
    StateStackT<GraphicsSlotState> mGraphicsSlotState;
    StateStackT<IndexBufferState>  mIndexBufferState;
    StateStackT<VertexBufferState> mVertexBufferState;
    StateStackT<ScissorState>      mScissorState;
    StateStackT<ViewportState>     mViewportState;
    StateStackT<RTVDSVState>       mRTVDSVState;
    StateStackT<PipelineState>     mPipelineState;
    std::vector<Action>            mActions;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif //  dx11_command_list_hpp
