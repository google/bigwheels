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

#include "ppx/grfx/dx11/dx11_command.h"
#include "ppx/grfx/dx11/dx11_buffer.h"
#include "ppx/grfx/dx11/dx11_descriptor.h"
#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_image.h"
#include "ppx/grfx/dx11/dx11_pipeline.h"
#include "ppx/grfx/dx11/dx11_query.h"
#include "ppx/grfx/dx11/dx11_render_pass.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// CommandBuffer
// -------------------------------------------------------------------------------------------------
Result CommandBuffer::CreateApiObjects(const grfx::internal::CommandBufferCreateInfo* pCreateInfo)
{
    return ppx::SUCCESS;
}

void CommandBuffer::DestroyApiObjects()
{
}

Result CommandBuffer::Begin()
{
    mCommandList.Reset();

    return ppx::SUCCESS;
}

Result CommandBuffer::End()
{
    return ppx::SUCCESS;
}

void CommandBuffer::BeginRenderPassImpl(const grfx::RenderPassBeginInfo* pBeginInfo)
{
    const RenderPass* pRenderPass = ToApi(pBeginInfo->pRenderPass);
    UINT              rtvCount    = static_cast<UINT>(pRenderPass->GetRenderTargetCount());
    UINT              dsvCount    = pRenderPass->HasDepthStencil() ? 1 : 0;

    ID3D11RenderTargetView* pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    for (UINT i = 0; i < rtvCount; ++i) {
        auto pApiRtv = ToApi(pBeginInfo->pRenderPass->GetRenderTargetView(i));
        pRTVs[i]     = pApiRtv->GetDxRenderTargetView();
    }

    ID3D11DepthStencilView* pDSV = nullptr;
    if (dsvCount > 0) {
        auto pApiDsv = ToApi(pBeginInfo->pRenderPass->GetDepthStencilView());
        pDSV         = pApiDsv->GetDxDepthStencilView();
    }

    mCommandList.OMSetRenderTargets(rtvCount, pRTVs, pDSV);

    for (UINT i = 0; i < pBeginInfo->RTVClearCount; ++i) {
        auto pApiRtv = ToApi(pBeginInfo->pRenderPass->GetRenderTargetView(i));
        if (pApiRtv->GetLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            mCommandList.ClearRenderTargetView(
                pRTVs[i],
                pBeginInfo->RTVClearValues[i].rgba);
        }
    }

    if (dsvCount > 0) {
        auto pApiDsv    = ToApi(pBeginInfo->pRenderPass->GetDepthStencilView());
        UINT clearFlags = 0;
        if (pApiDsv->GetDepthLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            clearFlags |= D3D11_CLEAR_DEPTH;
        }
        if (pApiDsv->GetStencilLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            clearFlags |= D3D11_CLEAR_STENCIL;
        }
        if (clearFlags != 0) {
            mCommandList.ClearDepthStencilView(
                pDSV,
                clearFlags,
                pBeginInfo->DSVClearValue.depth,
                static_cast<UINT8>(pBeginInfo->DSVClearValue.stencil));
        }
    }
}

void CommandBuffer::EndRenderPassImpl()
{
}

void CommandBuffer::TransitionImageLayout(
    const grfx::Image*  pImage,
    uint32_t            mipLevel,
    uint32_t            mipLevelCount,
    uint32_t            arrayLayer,
    uint32_t            arrayLayerCount,
    grfx::ResourceState beforeState,
    grfx::ResourceState afterState,
    const grfx::Queue*  pSrcQueue,
    const grfx::Queue*  pDstQueue)
{
    switch (beforeState) {
        default: break;

        case grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case grfx::RESOURCE_STATE_SHADER_RESOURCE: {
            mCommandList.Nullify(ToApi(pImage)->GetDxResource(), NULLIFY_TYPE_SRV);
        } break;

        case grfx::RESOURCE_STATE_GENERAL:
        case grfx::RESOURCE_STATE_UNORDERED_ACCESS: {
            mCommandList.Nullify(ToApi(pImage)->GetDxResource(), NULLIFY_TYPE_UAV);
        } break;
    }
}

void CommandBuffer::BufferResourceBarrier(
    const grfx::Buffer* pBuffer,
    grfx::ResourceState beforeState,
    grfx::ResourceState afterState,
    const grfx::Queue*  pSrcQueue,
    const grfx::Queue*  pDstQueue)
{
}

void CommandBuffer::SetViewports(
    uint32_t              viewportCount,
    const grfx::Viewport* pViewports)
{
    D3D11_VIEWPORT viewports[PPX_MAX_VIEWPORTS];

    viewportCount = std::min<uint32_t>(viewportCount, PPX_MAX_VIEWPORTS);
    for (uint32_t i = 0; i < viewportCount; ++i) {
        viewports[i].TopLeftX = pViewports[i].x;
        viewports[i].TopLeftY = pViewports[i].y;
        viewports[i].Width    = pViewports[i].width;
        viewports[i].Height   = pViewports[i].height;
        viewports[i].MinDepth = pViewports[i].minDepth;
        viewports[i].MaxDepth = pViewports[i].maxDepth;
    };

    mCommandList.RSSetViewports(
        static_cast<UINT>(viewportCount),
        viewports);
}

void CommandBuffer::SetScissors(
    uint32_t          scissorCount,
    const grfx::Rect* pScissors)
{
    D3D11_RECT rects[PPX_MAX_SCISSORS];

    scissorCount = std::min<uint32_t>(scissorCount, PPX_MAX_SCISSORS);
    for (uint32_t i = 0; i < scissorCount; ++i) {
        rects[i].left   = pScissors[i].x;
        rects[i].top    = pScissors[i].y;
        rects[i].right  = pScissors[i].x + pScissors[i].width;
        rects[i].bottom = pScissors[i].y + pScissors[i].height;
    };

    mCommandList.RSSetScissorRects(
        static_cast<UINT>(scissorCount),
        rects);
}

static bool IsVS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_VS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

static bool IsHS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_HS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

static bool IsDS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_DS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

static bool IsGS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_GS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

static bool IsPS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_PS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

static void SetSlots(
    dx11::CommandList* pCmdList,
    void (dx11::CommandList::*SetConstantBufferFn)(UINT, UINT, ID3D11Buffer* const*),
    void (dx11::CommandList::*SetShaderResourcesFn)(UINT, UINT, ID3D11ShaderResourceView* const*),
    void (dx11::CommandList::*SetSamplersFn)(UINT, UINT, ID3D11SamplerState* const*),
    void (dx11::CommandList::*SetUnorderedAccesssFn)(UINT, UINT, ID3D11UnorderedAccessView* const*),
    const dx11::DescriptorArray& descriptorArray)
{
    switch (descriptorArray.descriptorType) {
        default: {
            PPX_ASSERT_MSG(false, "unrecognized descriptor type");
        } break;

        case grfx::D3D_DESCRIPTOR_TYPE_CBV: {
            UINT                 StartSlot  = static_cast<UINT>(descriptorArray.binding);
            UINT                 NumBuffers = static_cast<UINT>(CountU32(descriptorArray.resources));
            ID3D11Buffer* const* ppBuffers  = reinterpret_cast<ID3D11Buffer* const*>(DataPtr(descriptorArray.resources));
            (pCmdList->*SetConstantBufferFn)(StartSlot, NumBuffers, ppBuffers);
        } break;

        case grfx::D3D_DESCRIPTOR_TYPE_SRV: {
            UINT                             StartSlot = static_cast<UINT>(descriptorArray.binding);
            UINT                             NumViews  = static_cast<UINT>(CountU32(descriptorArray.resources));
            ID3D11ShaderResourceView* const* ppViews   = reinterpret_cast<ID3D11ShaderResourceView* const*>(DataPtr(descriptorArray.resources));
            (pCmdList->*SetShaderResourcesFn)(StartSlot, NumViews, ppViews);
        } break;

        case grfx::D3D_DESCRIPTOR_TYPE_SAMPLER: {
            UINT                       StartSlot   = static_cast<UINT>(descriptorArray.binding);
            UINT                       NumSamplers = static_cast<UINT>(CountU32(descriptorArray.resources));
            ID3D11SamplerState* const* ppSamplers  = reinterpret_cast<ID3D11SamplerState* const*>(DataPtr(descriptorArray.resources));
            (pCmdList->*SetSamplersFn)(StartSlot, NumSamplers, ppSamplers);
        } break;

        case grfx::D3D_DESCRIPTOR_TYPE_UAV: {
            UINT                              StartSlot = static_cast<UINT>(descriptorArray.binding);
            UINT                              NumViews  = static_cast<UINT>(CountU32(descriptorArray.resources));
            ID3D11UnorderedAccessView* const* ppViews   = reinterpret_cast<ID3D11UnorderedAccessView* const*>(DataPtr(descriptorArray.resources));
            (pCmdList->*SetUnorderedAccesssFn)(StartSlot, NumViews, ppViews);
        } break;
    }
}

void CommandBuffer::BindGraphicsDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    setCount = std::min<uint32_t>(setCount, PPX_MAX_BOUND_DESCRIPTOR_SETS);

    for (uint32_t setIndex = 0; setIndex < setCount; ++setIndex) {
        const grfx::dx11::DescriptorSet* pApiSet          = dx11::ToApi(ppSets[setIndex]);
        const auto&                      descriptorArrays = pApiSet->GetDescriptorArrays();

        size_t daCount = descriptorArrays.size();
        for (size_t daIndex = 0; daIndex < daCount; ++daIndex) {
            const dx11::DescriptorArray& descriptorArray = descriptorArrays[daIndex];
            if (IsVS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::VSSetConstantBuffers,
                    &dx11::CommandList::VSSetShaderResources,
                    &dx11::CommandList::VSSetSamplers,
                    nullptr,
                    descriptorArray);
            }
            if (IsHS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::HSSetConstantBuffers,
                    &dx11::CommandList::HSSetShaderResources,
                    &dx11::CommandList::HSSetSamplers,
                    nullptr,
                    descriptorArray);
            }
            if (IsDS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::DSSetConstantBuffers,
                    &dx11::CommandList::DSSetShaderResources,
                    &dx11::CommandList::DSSetSamplers,
                    nullptr,
                    descriptorArray);
            }
            if (IsGS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::GSSetConstantBuffers,
                    &dx11::CommandList::GSSetShaderResources,
                    &dx11::CommandList::GSSetSamplers,
                    nullptr,
                    descriptorArray);
            }
            if (IsPS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::PSSetConstantBuffers,
                    &dx11::CommandList::PSSetShaderResources,
                    &dx11::CommandList::PSSetSamplers,
                    nullptr,
                    descriptorArray);
            }
        }
    }
}

void CommandBuffer::BindGraphicsPipeline(const grfx::GraphicsPipeline* pPipeline)
{
    const dx11::GraphicsPipeline* pApiPipeline = ToApi(pPipeline);

    dx11::PipelineState pipelineState = {};
    pipelineState.VS                  = pApiPipeline->GetVS();
    pipelineState.HS                  = pApiPipeline->GetHS();
    pipelineState.DS                  = pApiPipeline->GetDS();
    pipelineState.GS                  = pApiPipeline->GetGS();
    pipelineState.PS                  = pApiPipeline->GetPS();
    pipelineState.InputLayout         = pApiPipeline->GetInputLayout();
    pipelineState.PrimitiveTopology   = pApiPipeline->GetPrimitiveTopology();
    pipelineState.RasterizerState     = pApiPipeline->GetRasterizerState();
    pipelineState.DepthStencilState   = pApiPipeline->GetDepthStencilState();
    pipelineState.BlendState          = pApiPipeline->GetBlendState();
    pipelineState.SampleMask          = pApiPipeline->GetSampleMask();

    size_t size = 4 * sizeof(FLOAT);
    std::memcpy(pipelineState.BlendFactors, pApiPipeline->GetBlendFactors(), size);

    mCommandList.SetPipelineState(&pipelineState);
}

static bool IsCS(grfx::ShaderStageBits shaderVisbility)
{
    bool match = (shaderVisbility == grfx::SHADER_STAGE_CS) || (shaderVisbility == grfx::SHADER_STAGE_ALL_GRAPHICS) || (shaderVisbility == grfx::SHADER_STAGE_ALL);
    return match;
}

void CommandBuffer::BindComputeDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    setCount = std::min<uint32_t>(setCount, PPX_MAX_BOUND_DESCRIPTOR_SETS);

    for (uint32_t setIndex = 0; setIndex < setCount; ++setIndex) {
        const grfx::dx11::DescriptorSet* pApiSet          = dx11::ToApi(ppSets[setIndex]);
        const auto&                      descriptorArrays = pApiSet->GetDescriptorArrays();

        size_t daCount = descriptorArrays.size();
        for (size_t daIndex = 0; daIndex < daCount; ++daIndex) {
            const dx11::DescriptorArray& descriptorArray = descriptorArrays[daIndex];
            if (IsCS(descriptorArray.shaderVisibility)) {
                SetSlots(
                    &mCommandList,
                    &dx11::CommandList::CSSetConstantBuffers,
                    &dx11::CommandList::CSSetShaderResources,
                    &dx11::CommandList::CSSetSamplers,
                    &dx11::CommandList::CSSetUnorderedAccess,
                    descriptorArray);
            }
        }
    }
}

void CommandBuffer::BindComputePipeline(const grfx::ComputePipeline* pPipeline)
{
    const dx11::ComputePipeline* pApiPipeline = ToApi(pPipeline);

    dx11::PipelineState pipelineState = {};
    pipelineState.CS                  = pApiPipeline->GetCS();

    mCommandList.SetPipelineState(&pipelineState);
}

void CommandBuffer::BindIndexBuffer(const grfx::IndexBufferView* pView)
{
    mCommandList.IASetIndexBuffer(
        ToApi(pView->pBuffer)->GetDxBuffer(),
        ToD3D11IndexFormat(pView->indexType),
        static_cast<UINT>(pView->offset));
}

void CommandBuffer::BindVertexBuffers(
    uint32_t                      viewCount,
    const grfx::VertexBufferView* pViews)
{
    ID3D11Buffer* ppBuffers[PPX_MAX_VERTEX_BINDINGS];
    UINT          strides[PPX_MAX_VERTEX_BINDINGS];
    UINT          offsets[PPX_MAX_VERTEX_BINDINGS];

    viewCount = std::min<uint32_t>(viewCount, PPX_MAX_VERTEX_BINDINGS);
    for (uint32_t i = 0; i < viewCount; ++i) {
        ppBuffers[i] = ToApi(pViews[i].pBuffer)->GetDxBuffer();
        strides[i]   = pViews[i].stride;
        offsets[i]   = 0;
    }

    mCommandList.IASetVertexBuffers(
        0,
        static_cast<UINT>(viewCount),
        ppBuffers,
        strides,
        offsets);
}

void CommandBuffer::Draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    mCommandList.DrawInstanced(
        static_cast<UINT>(vertexCount),
        static_cast<UINT>(instanceCount),
        static_cast<UINT>(firstVertex),
        static_cast<UINT>(firstInstance));
}

void CommandBuffer::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t  vertexOffset,
    uint32_t firstInstance)
{
    mCommandList.DrawIndexedInstanced(
        static_cast<UINT>(indexCount),
        static_cast<UINT>(instanceCount),
        static_cast<UINT>(firstIndex),
        static_cast<UINT>(vertexOffset),
        static_cast<UINT>(firstInstance));
}

void CommandBuffer::Dispatch(
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ)
{
    mCommandList.Dispatch(
        static_cast<UINT>(groupCountX),
        static_cast<UINT>(groupCountY),
        static_cast<UINT>(groupCountZ));
}

void CommandBuffer::CopyBufferToBuffer(
    const grfx::BufferToBufferCopyInfo* pCopyInfo,
    grfx::Buffer*                       pSrcBuffer,
    grfx::Buffer*                       pDstBuffer)
{
    dx11::args::CopyBufferToBuffer copyArgs = {};
    copyArgs.size                           = static_cast<uint32_t>(pCopyInfo->size);
    copyArgs.srcBufferOffset                = static_cast<uint32_t>(pCopyInfo->srcBuffer.offset);
    copyArgs.dstBufferOffset                = static_cast<uint32_t>(pCopyInfo->dstBuffer.offset);
    copyArgs.pSrcResource                   = ToApi(pSrcBuffer)->GetDxBuffer();
    copyArgs.pDstResource                   = ToApi(pDstBuffer)->GetDxBuffer();

    mCommandList.CopyBufferToBuffer(&copyArgs);
}

void CommandBuffer::CopyBufferToImage(
    const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
    grfx::Buffer*                                   pSrcBuffer,
    grfx::Image*                                    pDstImage)
{
    for (auto& pCopyInfo : pCopyInfos) {
        CopyBufferToImage(pCopyInfo, pSrcBuffer, pDstImage);
    }
}

void CommandBuffer::CopyBufferToImage(
    const grfx::BufferToImageCopyInfo& pCopyInfo,
    grfx::Buffer*                      pSrcBuffer,
    grfx::Image*                       pDstImage)
{
    dx11::args::CopyBufferToImage copyArgs = {};

    copyArgs.srcBuffer.imageWidth      = pCopyInfo.srcBuffer.imageWidth;
    copyArgs.srcBuffer.imageHeight     = pCopyInfo.srcBuffer.imageHeight;
    copyArgs.srcBuffer.imageRowStride  = pCopyInfo.srcBuffer.imageRowStride;
    copyArgs.srcBuffer.footprintOffset = pCopyInfo.srcBuffer.footprintOffset;
    copyArgs.srcBuffer.footprintWidth  = pCopyInfo.srcBuffer.footprintWidth;
    copyArgs.srcBuffer.footprintHeight = pCopyInfo.srcBuffer.footprintHeight;
    copyArgs.srcBuffer.footprintDepth  = pCopyInfo.srcBuffer.footprintDepth;
    copyArgs.dstImage.mipLevel         = pCopyInfo.dstImage.mipLevel;
    copyArgs.dstImage.arrayLayer       = pCopyInfo.dstImage.arrayLayer;
    copyArgs.dstImage.arrayLayerCount  = pCopyInfo.dstImage.arrayLayerCount;
    copyArgs.dstImage.x                = pCopyInfo.dstImage.x;
    copyArgs.dstImage.y                = pCopyInfo.dstImage.y;
    copyArgs.dstImage.z                = pCopyInfo.dstImage.z;
    copyArgs.dstImage.width            = pCopyInfo.dstImage.width;
    copyArgs.dstImage.height           = pCopyInfo.dstImage.height;
    copyArgs.dstImage.depth            = pCopyInfo.dstImage.depth;
    copyArgs.mapType                   = ToApi(pSrcBuffer)->GetMapType();
    copyArgs.isCube                    = (pDstImage->GetType() == grfx::IMAGE_TYPE_CUBE);
    copyArgs.mipSpan                   = pDstImage->GetMipLevelCount();
    copyArgs.pSrcResource              = ToApi(pSrcBuffer)->GetDxBuffer();
    copyArgs.pDstResource              = ToApi(pDstImage)->GetDxResource();

    mCommandList.CopyBufferToImage(&copyArgs);
}

grfx::ImageToBufferOutputPitch CommandBuffer::CopyImageToBuffer(
    const grfx::ImageToBufferCopyInfo* pCopyInfo,
    grfx::Image*                       pSrcImage,
    grfx::Buffer*                      pDstBuffer)
{
    PPX_ASSERT_MSG(pCopyInfo->srcImage.arrayLayerCount == 1, "D3D11 does not support image-to-buffer copies of more than a layer at a time");

    const grfx::FormatDesc* srcDesc = grfx::GetFormatDescription(pSrcImage->GetFormat());

    dx11::args::CopyImageToBuffer copyArgs = {};
    copyArgs.srcImage.arrayLayer           = pCopyInfo->srcImage.arrayLayer;
    copyArgs.srcImage.mipLevel             = pCopyInfo->srcImage.mipLevel;
    copyArgs.srcImage.offset.x             = pCopyInfo->srcImage.offset.x;
    copyArgs.srcImage.offset.y             = pCopyInfo->srcImage.offset.y;
    copyArgs.srcImage.offset.z             = pCopyInfo->srcImage.offset.z;
    copyArgs.extent.x                      = pCopyInfo->extent.x;
    copyArgs.extent.y                      = pCopyInfo->extent.y;
    copyArgs.extent.z                      = pCopyInfo->extent.z;
    copyArgs.isDepthStencilCopy            = pSrcImage->GetUsageFlags().bits.depthStencilAttachment;
    copyArgs.srcMipLevels                  = pSrcImage->GetMipLevelCount();
    copyArgs.srcBytesPerTexel              = srcDesc->bytesPerTexel;
    copyArgs.srcTextureDimension           = ToD3D11TextureResourceDimension(pSrcImage->GetType());
    copyArgs.pSrcResource                  = ToApi(pSrcImage)->GetDxResource();
    copyArgs.pDstResource                  = ToApi(pDstBuffer)->GetDxBuffer();
    if (pSrcImage->GetType() == grfx::IMAGE_TYPE_1D) {
        ToApi(pSrcImage)->GetDxTexture1D()->GetDesc(&copyArgs.srcTextureDesc.texture1D);
    }
    if (pSrcImage->GetType() == grfx::IMAGE_TYPE_2D) {
        ToApi(pSrcImage)->GetDxTexture2D()->GetDesc(&copyArgs.srcTextureDesc.texture2D);
    }
    else {
        ToApi(pSrcImage)->GetDxTexture3D()->GetDesc(&copyArgs.srcTextureDesc.texture3D);
    }
    ToApi(pDstBuffer)->GetDxBuffer()->GetDesc(&copyArgs.dstBufferDesc);

    mCommandList.CopyImageToBuffer(&copyArgs);

    // We'll always tightly pack the texels.
    grfx::ImageToBufferOutputPitch outPitch;
    outPitch.rowPitch = srcDesc->bytesPerTexel * pCopyInfo->extent.x;
    return outPitch;
}

void CommandBuffer::CopyImageToImage(
    const grfx::ImageToImageCopyInfo* pCopyInfo,
    grfx::Image*                      pSrcImage,
    grfx::Image*                      pDstImage)
{
    bool isSourceDepthStencil = grfx::GetFormatDescription(pSrcImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    bool isDestDepthStencil   = grfx::GetFormatDescription(pDstImage->GetFormat())->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL;
    PPX_ASSERT_MSG(isSourceDepthStencil == isDestDepthStencil, "both images in an image copy must be depth-stencil if one is depth-stencil");

    dx11::args::CopyImageToImage copyArgs = {};
    copyArgs.srcImage.arrayLayer          = pCopyInfo->srcImage.arrayLayer;
    copyArgs.srcImage.arrayLayerCount     = pCopyInfo->srcImage.arrayLayerCount;
    copyArgs.srcImage.mipLevel            = pCopyInfo->srcImage.mipLevel;
    copyArgs.srcImage.offset.x            = pCopyInfo->srcImage.offset.x;
    copyArgs.srcImage.offset.y            = pCopyInfo->srcImage.offset.y;
    copyArgs.srcImage.offset.z            = pCopyInfo->srcImage.offset.z;
    copyArgs.dstImage.arrayLayer          = pCopyInfo->dstImage.arrayLayer;
    copyArgs.dstImage.arrayLayerCount     = pCopyInfo->dstImage.arrayLayerCount;
    copyArgs.dstImage.mipLevel            = pCopyInfo->dstImage.mipLevel;
    copyArgs.dstImage.offset.x            = pCopyInfo->dstImage.offset.x;
    copyArgs.dstImage.offset.y            = pCopyInfo->dstImage.offset.y;
    copyArgs.dstImage.offset.z            = pCopyInfo->dstImage.offset.z;
    copyArgs.extent.x                     = pCopyInfo->extent.x;
    copyArgs.extent.y                     = pCopyInfo->extent.y;
    copyArgs.extent.z                     = pCopyInfo->extent.z;
    copyArgs.isDepthStencilCopy           = isSourceDepthStencil;
    copyArgs.srcMipLevels                 = pSrcImage->GetMipLevelCount();
    copyArgs.dstMipLevels                 = pDstImage->GetMipLevelCount();
    copyArgs.srcTextureDimension          = ToD3D11TextureResourceDimension(pSrcImage->GetType());
    copyArgs.pSrcResource                 = ToApi(pSrcImage)->GetDxResource();
    copyArgs.pDstResource                 = ToApi(pDstImage)->GetDxResource();

    mCommandList.CopyImageToImage(&copyArgs);
}

void CommandBuffer::BeginQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    dx11::args::BeginQuery beginQueryArgs = {};

    beginQueryArgs.pQuery = ToApi(pQuery)->GetQuery(queryIndex);

    mCommandList.BeginQuery(&beginQueryArgs);
}

void CommandBuffer::EndQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    dx11::args::EndQuery endQueryArgs = {};

    endQueryArgs.pQuery = ToApi(pQuery)->GetQuery(queryIndex);

    mCommandList.EndQuery(&endQueryArgs);
}

void CommandBuffer::WriteTimestamp(
    const grfx::Query*  pQuery,
    grfx::PipelineStage pipelineStage,
    uint32_t            queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    dx11::args::WriteTimestamp writeTimestampArgs = {};

    writeTimestampArgs.pQuery = ToApi(pQuery)->GetQuery(queryIndex);

    mCommandList.WriteTimestamp(&writeTimestampArgs);
}

void CommandBuffer::ResolveQueryData(
    grfx::Query* pQuery,
    uint32_t     startIndex,
    uint32_t     numQueries)
{
    PPX_ASSERT_MSG((startIndex + numQueries) <= pQuery->GetCount(), "invalid query index/number");
    ToApi(pQuery)->SetResolveDataStartIndex(startIndex);
    ToApi(pQuery)->SetResolveDataNumQueries(numQueries);
}

void CommandBuffer::ImGuiRender(void (*pFn)(void))
{
    mCommandList.ImGuiRender(pFn);
}

// -------------------------------------------------------------------------------------------------
// CommandPool
// -------------------------------------------------------------------------------------------------
Result CommandPool::CreateApiObjects(const grfx::CommandPoolCreateInfo* pCreateInfo)
{
    return ppx::SUCCESS;
}

void CommandPool::DestroyApiObjects()
{
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
