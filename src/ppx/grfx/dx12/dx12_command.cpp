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

#include "ppx/grfx/dx12/dx12_command.h"
#include "ppx/grfx/dx12/dx12_buffer.h"
#include "ppx/grfx/dx12/dx12_descriptor.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_image.h"
#include "ppx/grfx/dx12/dx12_pipeline.h"
#include "ppx/grfx/dx12/dx12_query.h"
#include "ppx/grfx/dx12/dx12_render_pass.h"
#include "ppx/grfx/dx12/dx12_util.h"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// CommandBuffer
// -------------------------------------------------------------------------------------------------
Result CommandBuffer::CreateApiObjects(const grfx::internal::CommandBufferCreateInfo* pCreateInfo)
{
    D3D12DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    UINT                     nodeMask = 0;
    D3D12_COMMAND_LIST_TYPE  type     = ToApi(pCreateInfo->pPool)->GetDxCommandType();
    D3D12_COMMAND_LIST_FLAGS flags    = D3D12_COMMAND_LIST_FLAG_NONE;

    // NOTE: CreateCommandList1 creates a command list in closed state. No need to
    //       call Close() it after creation unlike command lists created with
    //       CreateCommandList.
    //
    HRESULT hr = device->CreateCommandList1(nodeMask, type, flags, IID_PPV_ARGS(&mCommandList));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandList1 failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12GraphicsCommandList, mCommandList.Get());

    //// Store command allocator for reset
    // mCommandAllocator = ToApi(pCreateInfo->pPool)->GetDxCommandAllocator();
    hr = ToApi(GetDevice())->GetDxDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&mCommandAllocator));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandAllocator failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12CommandAllocator, mCommandAllocator.Get());

    // Heap sizes
    mHeapSizeCBVSRVUAV = static_cast<UINT>(pCreateInfo->resourceDescriptorCount);
    mHeapSizeSampler   = static_cast<UINT>(pCreateInfo->samplerDescriptorCount);

    // Allocate CBVSRVUAV heap
    if (mHeapSizeCBVSRVUAV > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors             = mHeapSizeCBVSRVUAV;
        desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask                   = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeapCBVSRVUAV));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateDescriptorHeap(CBVSRVUAV) failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Allocate Sampler heap
    if (mHeapSizeSampler > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors             = mHeapSizeSampler;
        desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask                   = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeapSampler));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateDescriptorHeap(Sampler) failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

void CommandBuffer::DestroyApiObjects()
{
    if (mCommandList) {
        mCommandList.Reset();
    }

    if (mCommandAllocator) {
        mCommandAllocator.Reset();
    }

    if (mHeapCBVSRVUAV) {
        mHeapCBVSRVUAV.Reset();
    }

    if (mHeapSampler) {
        mHeapSampler.Reset();
    }
}

Result CommandBuffer::Begin()
{
    HRESULT hr;

    // Command allocators can only be reset when the GPU is
    // done with associated with command lists.
    //
    hr = mCommandAllocator->Reset();
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12CommandAllocator::Reset failed");
        return ppx::ERROR_API_FAILURE;
    }

    // Normally a command list can be reset immediately after submission
    // if it gets associated with a different command allocator.
    // But since we're trying to align with Vulkan, just keep the
    // command allocator and command list paired.
    //
    hr = mCommandList->Reset(mCommandAllocator.Get(), nullptr);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12CommandList::Reset failed");
        return ppx::ERROR_API_FAILURE;
    }

    // Reset current root signatures
    mCurrentGraphicsInterface = nullptr;
    mCurrentComputeInterface  = nullptr;

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[2]  = {nullptr};
    uint32_t              heapCount = 0;
    if (mHeapCBVSRVUAV) {
        heaps[heapCount] = mHeapCBVSRVUAV.Get();
        ++heapCount;
    }
    if (mHeapSampler) {
        heaps[heapCount] = mHeapSampler.Get();
        ++heapCount;
    }
    if (heapCount > 0) {
        mCommandList->SetDescriptorHeaps(heapCount, heaps);
    }

    // Reset heap offsets
    mHeapOffsetCBVSRVUAV = 0;
    mHeapOffsetSampler   = 0;

    return ppx::SUCCESS;
}

Result CommandBuffer::End()
{
    HRESULT hr = mCommandList->Close();
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12CommandList::Close failed");
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void CommandBuffer::BeginRenderPassImpl(const grfx::RenderPassBeginInfo* pBeginInfo)
{
    PPX_ASSERT_NULL_ARG(pBeginInfo->pRenderPass);

    const grfx::RenderPass* pRenderPass = pBeginInfo->pRenderPass;

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptors[PPX_MAX_RENDER_TARGETS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDesciptor                           = {};

    // Get handle to render target descirptors
    uint32_t renderTargetCount = pRenderPass->GetRenderTargetCount();
    for (uint32_t i = 0; i < renderTargetCount; ++i) {
        dx12::RenderTargetView* pRTV = ToApi(pRenderPass->GetRenderTargetView(i).Get());
        renderTargetDescriptors[i]   = pRTV->GetCpuDescriptorHandle();
    }

    // Get handle for depth stencil descriptor
    bool hasDepthStencil = false;
    if (pRenderPass->GetDepthStencilView()) {
        depthStencilDesciptor = ToApi(pRenderPass->GetDepthStencilView())->GetCpuDescriptorHandle();
        hasDepthStencil       = true;
    }

    // Set render targets
    mCommandList->OMSetRenderTargets(
        static_cast<UINT>(renderTargetCount),
        renderTargetDescriptors,
        FALSE,
        hasDepthStencil ? &depthStencilDesciptor : nullptr);

    // Clear render targets if load op is clear
    renderTargetCount = std::min(renderTargetCount, pBeginInfo->RTVClearCount);
    for (uint32_t i = 0; i < renderTargetCount; ++i) {
        grfx::AttachmentLoadOp loadOp = pRenderPass->GetRenderTargetView(i)->GetLoadOp();
        if (loadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            const D3D12_CPU_DESCRIPTOR_HANDLE&  handle     = renderTargetDescriptors[i];
            const grfx::RenderTargetClearValue& clearValue = pBeginInfo->RTVClearValues[i];
            mCommandList->ClearRenderTargetView(handle, clearValue.rgba, 0, nullptr);
        }
    }

    // Clear depth/stencil if load op is clear
    // if (hasDepthStencil && (pRenderPass->GetDepthStencilView()->GetDepthLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR)) {
    if (hasDepthStencil) {
        D3D12_CLEAR_FLAGS flags = static_cast<D3D12_CLEAR_FLAGS>(0);
        if (pRenderPass->GetDepthStencilView()->GetDepthLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            flags |= D3D12_CLEAR_FLAG_DEPTH;
        }
        if (pRenderPass->GetDepthStencilView()->GetStencilLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            flags |= D3D12_CLEAR_FLAG_STENCIL;
        }

        if (flags != static_cast<D3D12_CLEAR_FLAGS>(0)) {
            const grfx::DepthStencilClearValue& clearValue = pBeginInfo->DSVClearValue;
            mCommandList->ClearDepthStencilView(
                depthStencilDesciptor,
                flags,
                static_cast<FLOAT>(clearValue.depth),
                static_cast<UINT8>(clearValue.stencil),
                0,
                nullptr);
        }
    }
}

void CommandBuffer::EndRenderPassImpl()
{
    // Nothing to do here for now
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToBeginningAccessType(grfx::AttachmentLoadOp loadOp)
{
    D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE accessType;
    switch (loadOp) {
        default:
            PPX_ASSERT_MSG(false, "Unsupported access type " << loadOp);
            break;
        case grfx::ATTACHMENT_LOAD_OP_CLEAR:
            accessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            break;
        case grfx::ATTACHMENT_LOAD_OP_LOAD:
            accessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            break;
        case grfx::ATTACHMENT_LOAD_OP_DONT_CARE:
            accessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            break;
    }
    return accessType;
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ToEndingAccessType(grfx::AttachmentStoreOp storeOp)
{
    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE accessType;
    switch (storeOp) {
        default:
            PPX_ASSERT_MSG(false, "Unsupported access type" << storeOp);
            break;
        case grfx::ATTACHMENT_STORE_OP_DONT_CARE:
            accessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            break;
        case grfx::ATTACHMENT_STORE_OP_STORE:
            accessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            break;
    }
    return accessType;
}

void CommandBuffer::BeginRenderingImpl(const grfx::RenderingInfo* pRenderingInfo)
{
    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDescs[PPX_MAX_RENDER_TARGETS];
    for (size_t i = 0; i < pRenderingInfo->renderTargetCount; i++) {
        grfx::RenderTargetView*            pRtv                   = pRenderingInfo->pRenderTargetViews[i];
        dx12::RenderTargetView*            pApiRtv                = ToApi(pRenderingInfo->pRenderTargetViews[i]);
        D3D12_CPU_DESCRIPTOR_HANDLE        rtvCPUDescriptorHandle = pApiRtv->GetCpuDescriptorHandle();
        D3D12_RENDER_PASS_BEGINNING_ACCESS rtvBeginningAccess     = {ToBeginningAccessType(pApiRtv->GetLoadOp())};
        D3D12_RENDER_PASS_ENDING_ACCESS    rtvEndingAccess        = {ToEndingAccessType(pApiRtv->GetStoreOp())};
        if (pRtv->GetLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            const grfx::RenderTargetClearValue& cv = pRenderingInfo->RTVClearValues[i];
            D3D12_CLEAR_VALUE                   clearValue{dx::ToDxgiFormat(pRtv->GetFormat())};
            clearValue.Color[0]      = cv.r;
            clearValue.Color[1]      = cv.g;
            clearValue.Color[2]      = cv.b;
            clearValue.Color[3]      = cv.a;
            rtvBeginningAccess.Clear = {clearValue};
        }
        renderTargetDescs[i] = {rtvCPUDescriptorHandle, rtvBeginningAccess, rtvEndingAccess};
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* pDSDesc = nullptr;
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC  renderDepthStencilDesc;
    if (pRenderingInfo->pDepthStencilView) {
        grfx::DepthStencilView*            pDsv                   = pRenderingInfo->pDepthStencilView;
        D3D12_CPU_DESCRIPTOR_HANDLE        dsvCPUDescriptorHandle = ToApi(pDsv)->GetCpuDescriptorHandle();
        D3D12_RENDER_PASS_BEGINNING_ACCESS depthBeginningAccess{ToBeginningAccessType(pDsv->GetDepthLoadOp()), {}};
        D3D12_RENDER_PASS_ENDING_ACCESS    depthEndingAccess{ToEndingAccessType(pDsv->GetDepthStoreOp()), {}};
        D3D12_RENDER_PASS_BEGINNING_ACCESS stencilBeginningAccess{ToBeginningAccessType(pDsv->GetStencilLoadOp()), {}};
        D3D12_RENDER_PASS_ENDING_ACCESS    stencilEndingAccess{ToEndingAccessType(pDsv->GetStencilStoreOp()), {}};
        if (pDsv->GetDepthLoadOp() == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            const grfx::DepthStencilClearValue& cv = pRenderingInfo->DSVClearValue;
            D3D12_CLEAR_VALUE                   clearValue{dx::ToDxgiFormat(pDsv->GetFormat())};
            clearValue.DepthStencil.Depth   = cv.depth;
            clearValue.DepthStencil.Stencil = cv.stencil;
            depthBeginningAccess.Clear      = {clearValue};
        }
        renderDepthStencilDesc = {dsvCPUDescriptorHandle, depthBeginningAccess, stencilBeginningAccess, depthEndingAccess, stencilEndingAccess};
        pDSDesc                = &renderDepthStencilDesc;
    }
    D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;
    if (pRenderingInfo->flags.bits.suspending) {
        flags |= D3D12_RENDER_PASS_FLAG_SUSPENDING_PASS;
    }
    if (pRenderingInfo->flags.bits.resuming) {
        flags |= D3D12_RENDER_PASS_FLAG_RESUMING_PASS;
    }

    mCommandList->BeginRenderPass(pRenderingInfo->renderTargetCount, renderTargetDescs, pDSDesc, flags);
}

void CommandBuffer::EndRenderingImpl()
{
    mCommandList->EndRenderPass();
}

void CommandBuffer::PushDescriptorImpl(
    grfx::CommandType              pipelineBindPoint,
    const grfx::PipelineInterface* pInterface,
    grfx::DescriptorType           descriptorType,
    uint32_t                       binding,
    uint32_t                       set,
    uint32_t                       bufferOffset,
    const grfx::Buffer*            pBuffer,
    const grfx::SampledImageView*  pSampledImageView,
    const grfx::StorageImageView*  pStorageImageView,
    const grfx::Sampler*           pSampler)
{
    auto pLayout = pInterface->GetSetLayout(set);
    PPX_ASSERT_MSG((pLayout != nullptr), "set=" << set << " does not match a set layout in the pipeline interface");
    PPX_ASSERT_MSG(pLayout->IsPushable(), "set=" << set << " refers to a set layout that is not pushable");
    PPX_ASSERT_MSG((pBuffer != nullptr), "pBuffer is null");

    // void these out so compiler doesn't complain about unused varaiables.
    (void)pSampledImageView;
    (void)pStorageImageView;
    (void)pSampler;

    // Find root parameter index
    UINT rootParameterIndex = ToApi(pInterface)->FindParameterIndex(set, binding);
    PPX_ASSERT_MSG((rootParameterIndex != PPX_VALUE_IGNORED), "root parameter index not found for binding=" << binding << ", set=" << set);

    // Calculate GPU virtual address location for buffer
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = ToApi(pBuffer)->GetDxResource()->GetGPUVirtualAddress();
    bufferLocation += static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(bufferOffset);

    // Call appropriate function based on pipeline bind point
    if (pipelineBindPoint == grfx::COMMAND_TYPE_GRAPHICS) {
        SetGraphicsPipelineInterface(pInterface);

        switch (descriptorType) {
            default: PPX_ASSERT_MSG(false, "descriptor is not of pushable type binding=" << binding << ", set=" << set); break;

            case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                mCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferLocation);
            } break;

            case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER: {
                mCommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, bufferLocation);
            } break;

            case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER:
            case grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: {
                mCommandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, bufferLocation);
            } break;
        }
    }
    else if (pipelineBindPoint == grfx::COMMAND_TYPE_COMPUTE) {
        SetComputePipelineInterface(pInterface);

        switch (descriptorType) {
            default: PPX_ASSERT_MSG(false, "descriptor is not of pushable type binding=" << binding << ", set=" << set); break;

            case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                mCommandList->SetComputeRootConstantBufferView(rootParameterIndex, bufferLocation);
            } break;

            case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER: {
                mCommandList->SetComputeRootShaderResourceView(rootParameterIndex, bufferLocation);
            } break;

            case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER:
            case grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: {
                mCommandList->SetComputeRootUnorderedAccessView(rootParameterIndex, bufferLocation);
            } break;
        }
    }
    else {
        PPX_ASSERT_MSG(false, "invalid pipeline bindpoint");
    }
}

void CommandBuffer::ClearRenderTarget(
    grfx::Image*                        pImage,
    const grfx::RenderTargetClearValue& clearValue)
{
    auto pCurrentRenderPass = GetCurrentRenderPass();
    if (IsNull(pCurrentRenderPass)) {
        return;
    }

    // Make sure pImage is a render target in current render pass
    const uint32_t renderTargetIndex = pCurrentRenderPass->GetRenderTargetImageIndex(pImage);
    if (renderTargetIndex == UINT32_MAX) {
        return;
    }

    // Get view at renderTargetIndex
    auto pView = ToApi(pCurrentRenderPass->GetRenderTargetView(renderTargetIndex));

    // Clear value
    FLOAT colorRGBA[4] = {clearValue.r, clearValue.g, clearValue.b, clearValue.a};

    // Render area
    const auto& renderArea = pCurrentRenderPass->GetRenderArea();

    // Rect
    D3D12_RECT rect = {
        static_cast<LONG>(renderArea.x),
        static_cast<LONG>(renderArea.y),
        static_cast<LONG>(renderArea.x + renderArea.width),
        static_cast<LONG>(renderArea.y + renderArea.height),
    };

    mCommandList->ClearRenderTargetView(
        pView->GetCpuDescriptorHandle(),
        colorRGBA,
        1,
        &rect);
}

void CommandBuffer::ClearDepthStencil(
    grfx::Image*                        pImage,
    const grfx::DepthStencilClearValue& clearValue,
    uint32_t                            clearFlags)
{
    auto pCurrentRenderPass = GetCurrentRenderPass();
    if (IsNull(pCurrentRenderPass)) {
        return;
    }

    // Make sure pImage is depth stencil in current render pass
    if (pCurrentRenderPass->GetDepthStencilImage().Get() != pImage) {
        return;
    }

    // Get view
    auto pView = ToApi(pCurrentRenderPass->GetDepthStencilView());

    // Clear flags
    D3D12_CLEAR_FLAGS dxClearFlags = static_cast<D3D12_CLEAR_FLAGS>(0);
    if (clearFlags & grfx::CLEAR_FLAG_DEPTH) {
        dxClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    }
    if (clearFlags & grfx::CLEAR_FLAG_STENCIL) {
        dxClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
    }

    // Render area
    const auto& renderArea = pCurrentRenderPass->GetRenderArea();

    // Rect
    D3D12_RECT rect = {
        static_cast<LONG>(renderArea.x),
        static_cast<LONG>(renderArea.y),
        static_cast<LONG>(renderArea.x + renderArea.width),
        static_cast<LONG>(renderArea.y + renderArea.height),
    };

    mCommandList->ClearDepthStencilView(
        pView->GetCpuDescriptorHandle(),
        dxClearFlags,
        clearValue.depth,
        static_cast<UINT8>(clearValue.stencil),
        1,
        &rect);
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
    PPX_ASSERT_NULL_ARG(pImage);

    (void)pSrcQueue;
    (void)pDstQueue;

    if (beforeState == afterState) {
        return;
    }

    bool allMipLevels    = (mipLevel == 0) && (mipLevelCount == PPX_REMAINING_MIP_LEVELS);
    bool allArrayLayers  = (arrayLayer == 0) && (arrayLayerCount == PPX_REMAINING_ARRAY_LAYERS);
    bool allSubresources = allMipLevels && allArrayLayers;

    if (mipLevelCount == PPX_REMAINING_MIP_LEVELS) {
        mipLevelCount = pImage->GetMipLevelCount();
    }

    if (arrayLayerCount == PPX_REMAINING_ARRAY_LAYERS) {
        arrayLayerCount = pImage->GetArrayLayerCount();
    }

    grfx::CommandType commandType = GetCommandType();

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    if (allSubresources) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = ToApi(pImage)->GetDxResource();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = ToD3D12ResourceStates(beforeState, commandType);
        barrier.Transition.StateAfter  = ToD3D12ResourceStates(afterState, commandType);

        barriers.push_back(barrier);
    }
    else {
        //
        // For details about subresource indexing see this:
        //   https://docs.microsoft.com/en-us/windows/win32/direct3d12/subresources
        //

        uint32_t mipSpan = pImage->GetMipLevelCount();

        for (uint32_t i = 0; i < arrayLayerCount; ++i) {
            uint32_t baseSubresource = (arrayLayer + i) * mipSpan;
            for (uint32_t j = 0; j < mipLevelCount; ++j) {
                uint32_t targetSubResource = baseSubresource + (mipLevel + j);

                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource   = ToApi(pImage)->GetDxResource();
                barrier.Transition.Subresource = static_cast<UINT>(targetSubResource);
                barrier.Transition.StateBefore = ToD3D12ResourceStates(beforeState, commandType);
                barrier.Transition.StateAfter  = ToD3D12ResourceStates(afterState, commandType);

                barriers.push_back(barrier);
            }
        }
    }

    if (barriers.empty()) {
        PPX_ASSERT_MSG(false, "parameters resulted in no barriers - try not to do this!")
    }

    mCommandList->ResourceBarrier(
        static_cast<UINT>(barriers.size()),
        DataPtr(barriers));
}

void CommandBuffer::BufferResourceBarrier(
    const grfx::Buffer* pBuffer,
    grfx::ResourceState beforeState,
    grfx::ResourceState afterState,
    const grfx::Queue*  pSrcQueue,
    const grfx::Queue*  pDstQueue)
{
    PPX_ASSERT_NULL_ARG(pBuffer);

    (void)pSrcQueue;
    (void)pDstQueue;

    if (beforeState == afterState) {
        return;
    }

    grfx::CommandType commandType = GetCommandType();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = ToApi(pBuffer)->GetDxResource();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = ToD3D12ResourceStates(beforeState, commandType);
    barrier.Transition.StateAfter  = ToD3D12ResourceStates(afterState, commandType);
    mCommandList->ResourceBarrier(1, &barrier);
}

void CommandBuffer::SetViewports(
    uint32_t              viewportCount,
    const grfx::Viewport* pViewports)
{
    D3D12_VIEWPORT viewports[PPX_MAX_VIEWPORTS] = {};
    for (uint32_t i = 0; i < viewportCount; ++i) {
        viewports[i].TopLeftX = pViewports[i].x;
        viewports[i].TopLeftY = pViewports[i].y;
        viewports[i].Width    = pViewports[i].width;
        viewports[i].Height   = pViewports[i].height;
        viewports[i].MinDepth = pViewports[i].minDepth;
        viewports[i].MaxDepth = pViewports[i].maxDepth;
    }

    mCommandList->RSSetViewports(static_cast<UINT>(viewportCount), viewports);
}

void CommandBuffer::SetScissors(
    uint32_t          scissorCount,
    const grfx::Rect* pScissors)
{
    D3D12_RECT rects[PPX_MAX_SCISSORS] = {};
    for (uint32_t i = 0; i < scissorCount; ++i) {
        rects[i].left   = pScissors[i].x;
        rects[i].top    = pScissors[i].y;
        rects[i].right  = pScissors[i].x + pScissors[i].width;
        rects[i].bottom = pScissors[i].y + pScissors[i].height;
    }

    mCommandList->RSSetScissorRects(static_cast<UINT>(scissorCount), rects);
}

void CommandBuffer::SetGraphicsPipelineInterface(const grfx::PipelineInterface* pInterface)
{
    // Only set root signature if we have to
    if (pInterface != mCurrentGraphicsInterface) {
        mCurrentGraphicsInterface = pInterface;
        mCommandList->SetGraphicsRootSignature(ToApi(mCurrentGraphicsInterface)->GetDxRootSignature().Get());
    }
}

void CommandBuffer::SetComputePipelineInterface(const grfx::PipelineInterface* pInterface)
{
    // Only set root signature if we have to
    if (pInterface != mCurrentComputeInterface) {
        mCurrentComputeInterface = pInterface;
        mCommandList->SetComputeRootSignature(ToApi(mCurrentComputeInterface)->GetDxRootSignature().Get());
    }
}

void CommandBuffer::BindDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets,
    size_t&                           rdtCountCBVSRVUAV,
    size_t&                           rdtCountSampler)
{
    dx12::Device*                  pApiDevice             = ToApi(GetDevice());
    D3D12DevicePtr                 device                 = pApiDevice->GetDxDevice();
    const dx12::PipelineInterface* pApiPipelineInterface  = ToApi(pInterface);
    const std::vector<uint32_t>&   setNumbers             = pApiPipelineInterface->GetSetNumbers();
    UINT                           incrementSizeCBVSRVUAV = pApiDevice->GetHandleIncrementSizeCBVSRVUAV();
    UINT                           incrementSizeSampler   = pApiDevice->GetHandleIncrementSizeSampler();

    uint32_t parameterIndexCount = pApiPipelineInterface->GetParameterIndexCount();
    if (parameterIndexCount > mRootDescriptorTablesCBVSRVUAV.size()) {
        mRootDescriptorTablesCBVSRVUAV.resize(parameterIndexCount);
        mRootDescriptorTablesSampler.resize(parameterIndexCount);
    }

    // Root descriptor tables
    rdtCountCBVSRVUAV = 0;
    rdtCountSampler   = 0;
    for (uint32_t setIndex = 0; setIndex < setCount; ++setIndex) {
        PPX_ASSERT_MSG(ppSets[setIndex] != nullptr, "ppSets[" << setIndex << "] is null");
        uint32_t                   set      = setNumbers[setIndex];
        const dx12::DescriptorSet* pApiSet  = ToApi(ppSets[setIndex]);
        auto&                      bindings = pApiSet->GetLayout()->GetBindings();

        // Copy the descriptors
        {
            UINT numDescriptors = pApiSet->GetNumDescriptorsCBVSRVUAV();
            if (numDescriptors > 0) {
                D3D12_CPU_DESCRIPTOR_HANDLE dstRangeStart = mHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
                D3D12_CPU_DESCRIPTOR_HANDLE srcRangeStart = pApiSet->GetHeapCBVSRVUAV()->GetCPUDescriptorHandleForHeapStart();

                dstRangeStart.ptr += (mHeapOffsetCBVSRVUAV * incrementSizeCBVSRVUAV);

                device->CopyDescriptorsSimple(
                    numDescriptors,
                    dstRangeStart,
                    srcRangeStart,
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            numDescriptors = pApiSet->GetNumDescriptorsSampler();
            if (numDescriptors > 0) {
                D3D12_CPU_DESCRIPTOR_HANDLE dstRangeStart = mHeapSampler->GetCPUDescriptorHandleForHeapStart();
                D3D12_CPU_DESCRIPTOR_HANDLE srcRangeStart = pApiSet->GetHeapSampler()->GetCPUDescriptorHandleForHeapStart();

                dstRangeStart.ptr += (mHeapOffsetSampler * incrementSizeSampler);

                device->CopyDescriptorsSimple(
                    numDescriptors,
                    dstRangeStart,
                    srcRangeStart,
                    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }
        }

        size_t bindingCount = bindings.size();
        for (size_t bindingIndex = 0; bindingIndex < bindingCount; ++bindingIndex) {
            auto& binding        = bindings[bindingIndex];
            UINT  parameterIndex = pApiPipelineInterface->FindParameterIndex(set, binding.binding);
            PPX_ASSERT_MSG(parameterIndex != UINT32_MAX, "invalid parameter index for set=" << set << ", binding=" << binding.binding);

            if (binding.type == grfx::DESCRIPTOR_TYPE_SAMPLER) {
                RootDescriptorTable& rdt = mRootDescriptorTablesSampler[rdtCountSampler];
                rdt.parameterIndex       = parameterIndex;
                rdt.baseDescriptor       = mHeapSampler->GetGPUDescriptorHandleForHeapStart();
                rdt.baseDescriptor.ptr += (mHeapOffsetSampler * incrementSizeSampler);

                mHeapOffsetSampler += static_cast<UINT>(binding.arrayCount);
                rdtCountSampler += 1;
            }
            else {
                RootDescriptorTable& rdt = mRootDescriptorTablesCBVSRVUAV[rdtCountCBVSRVUAV];
                rdt.parameterIndex       = parameterIndex;
                rdt.baseDescriptor       = mHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
                rdt.baseDescriptor.ptr += (mHeapOffsetCBVSRVUAV * incrementSizeCBVSRVUAV);

                mHeapOffsetCBVSRVUAV += static_cast<UINT>(binding.arrayCount);
                rdtCountCBVSRVUAV += 1;
            }
        }
    }
}

void CommandBuffer::BindGraphicsDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    // Set root signature
    SetGraphicsPipelineInterface(pInterface);

    // Fill out mRootDescriptorTablesCBVSRVUAV and mRootDescriptorTablesSampler
    size_t rdtCountCBVSRVUAV = 0;
    size_t rdtCountSampler   = 0;
    BindDescriptorSets(pInterface, setCount, ppSets, rdtCountCBVSRVUAV, rdtCountSampler);

    // Set CBVSRVUAV root descriptor tables
    for (uint32_t i = 0; i < rdtCountCBVSRVUAV; ++i) {
        const RootDescriptorTable& rdt = mRootDescriptorTablesCBVSRVUAV[i];
        mCommandList->SetGraphicsRootDescriptorTable(rdt.parameterIndex, rdt.baseDescriptor);
    }

    // Set Sampler root descriptor tables
    for (uint32_t i = 0; i < rdtCountSampler; ++i) {
        const RootDescriptorTable& rdt = mRootDescriptorTablesSampler[i];
        mCommandList->SetGraphicsRootDescriptorTable(rdt.parameterIndex, rdt.baseDescriptor);
    }
}

void CommandBuffer::PushGraphicsConstants(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       count,
    const void*                    pValues,
    uint32_t                       dstOffset)
{
    PPX_ASSERT_MSG(((dstOffset + count) <= PPX_MAX_PUSH_CONSTANTS), "dstOffset + count (" << (dstOffset + count) << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");

    // Set root signature
    SetGraphicsPipelineInterface(pInterface);

    UINT rootParameterIndex = static_cast<UINT>(ToApi(pInterface)->GetRootConstantsParameterIndex());
    mCommandList->SetGraphicsRoot32BitConstants(
        rootParameterIndex,
        static_cast<UINT>(count),
        pValues,
        static_cast<UINT>(dstOffset));
}

void CommandBuffer::BindGraphicsPipeline(const grfx::GraphicsPipeline* pPipeline)
{
    mCommandList->SetPipelineState(ToApi(pPipeline)->GetDxPipeline().Get());
    mCommandList->IASetPrimitiveTopology(ToApi(pPipeline)->GetPrimitiveTopology());
}

void CommandBuffer::BindComputeDescriptorSets(
    const grfx::PipelineInterface*    pInterface,
    uint32_t                          setCount,
    const grfx::DescriptorSet* const* ppSets)
{
    // Set root signature
    SetComputePipelineInterface(pInterface);

    // Fill out mRootDescriptorTablesCBVSRVUAV and mRootDescriptorTablesSampler
    size_t rdtCountCBVSRVUAV = 0;
    size_t rdtCountSampler   = 0;
    BindDescriptorSets(pInterface, setCount, ppSets, rdtCountCBVSRVUAV, rdtCountSampler);

    // Set CBVSRVUAV root descriptor tables
    for (uint32_t i = 0; i < rdtCountCBVSRVUAV; ++i) {
        const RootDescriptorTable& rdt = mRootDescriptorTablesCBVSRVUAV[i];
        mCommandList->SetComputeRootDescriptorTable(rdt.parameterIndex, rdt.baseDescriptor);
    }

    // Set Sampler root descriptor tables
    for (uint32_t i = 0; i < rdtCountSampler; ++i) {
        const RootDescriptorTable& rdt = mRootDescriptorTablesSampler[i];
        mCommandList->SetComputeRootDescriptorTable(rdt.parameterIndex, rdt.baseDescriptor);
    }
}

void CommandBuffer::PushComputeConstants(
    const grfx::PipelineInterface* pInterface,
    uint32_t                       count,
    const void*                    pValues,
    uint32_t                       dstOffset)
{
    PPX_ASSERT_MSG(((dstOffset + count) <= PPX_MAX_PUSH_CONSTANTS), "dstOffset + count (" << (dstOffset + count) << ") exceeds PPX_MAX_PUSH_CONSTANTS (" << PPX_MAX_PUSH_CONSTANTS << ")");

    // Set root signature
    SetComputePipelineInterface(pInterface);

    UINT rootParameterIndex = static_cast<UINT>(ToApi(pInterface)->GetRootConstantsParameterIndex());
    mCommandList->SetComputeRoot32BitConstants(
        rootParameterIndex,
        static_cast<UINT>(count),
        pValues,
        static_cast<UINT>(dstOffset));
}

void CommandBuffer::BindComputePipeline(const grfx::ComputePipeline* pPipeline)
{
    mCommandList->SetPipelineState(ToApi(pPipeline)->GetDxPipeline().Get());
}

void CommandBuffer::BindIndexBuffer(const grfx::IndexBufferView* pView)
{
    D3D12_GPU_VIRTUAL_ADDRESS baseAddress = ToApi(pView->pBuffer)->GetDxResource()->GetGPUVirtualAddress();
    UINT                      sizeInBytes = static_cast<UINT>((pView->size == PPX_WHOLE_SIZE) ? pView->pBuffer->GetSize() : pView->size);

    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation          = baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(pView->offset);
    view.SizeInBytes             = sizeInBytes;
    view.Format                  = ToD3D12IndexFormat(pView->indexType);
    PPX_ASSERT_MSG(view.Format != DXGI_FORMAT_UNKNOWN, "unknown index  format");

    mCommandList->IASetIndexBuffer(&view);
}

void CommandBuffer::BindVertexBuffers(
    uint32_t                      viewCount,
    const grfx::VertexBufferView* pViews)
{
    D3D12_VERTEX_BUFFER_VIEW views[PPX_MAX_RENDER_TARGETS] = {};
    for (uint32_t i = 0; i < viewCount; ++i) {
        D3D12_GPU_VIRTUAL_ADDRESS baseAddress = ToApi(pViews[i].pBuffer)->GetDxResource()->GetGPUVirtualAddress();
        UINT                      sizeInBytes = static_cast<UINT>((pViews[i].size == PPX_WHOLE_SIZE) ? pViews[i].pBuffer->GetSize() : pViews[i].size);

        views[i].BufferLocation = baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(pViews[i].offset);
        views[i].SizeInBytes    = sizeInBytes;
        views[i].StrideInBytes  = static_cast<UINT>(pViews[i].stride);
    }

    mCommandList->IASetVertexBuffers(
        0,
        static_cast<UINT>(viewCount),
        views);
}

void CommandBuffer::Draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    mCommandList->DrawInstanced(
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
    mCommandList->DrawIndexedInstanced(
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
    mCommandList->Dispatch(
        static_cast<UINT>(groupCountX),
        static_cast<UINT>(groupCountY),
        static_cast<UINT>(groupCountZ));
}

void CommandBuffer::CopyBufferToBuffer(
    const grfx::BufferToBufferCopyInfo* pCopyInfo,
    grfx::Buffer*                       pSrcBuffer,
    grfx::Buffer*                       pDstBuffer)
{
    mCommandList->CopyBufferRegion(
        ToApi(pDstBuffer)->GetDxResource(),
        static_cast<UINT64>(pCopyInfo->dstBuffer.offset),
        ToApi(pSrcBuffer)->GetDxResource(),
        static_cast<UINT64>(pCopyInfo->srcBuffer.offset),
        static_cast<UINT64>(pCopyInfo->size));
}

void CommandBuffer::CopyBufferToImage(
    const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
    grfx::Buffer*                                   pSrcBuffer,
    grfx::Image*                                    pDstImage)
{
    for (auto& pCopyInfo : pCopyInfos) {
        CopyBufferToImage(&pCopyInfo, pSrcBuffer, pDstImage);
    }
}

void CommandBuffer::CopyBufferToImage(
    const grfx::BufferToImageCopyInfo* pCopyInfo,
    grfx::Buffer*                      pSrcBuffer,
    grfx::Image*                       pDstImage)
{
    D3D12DevicePtr      device        = ToApi(GetDevice())->GetDxDevice();
    D3D12_RESOURCE_DESC resouceDesc   = ToApi(pDstImage)->GetDxResource()->GetDesc();
    const uint32_t      mipLevelCount = pDstImage->GetMipLevelCount();

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource                   = ToApi(pDstImage)->GetDxResource();
    dst.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource                   = ToApi(pSrcBuffer)->GetDxResource();
    src.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (uint32_t i = 0; i < pCopyInfo->dstImage.arrayLayerCount; ++i) {
        uint32_t arrayLayer = pCopyInfo->dstImage.arrayLayer + i;

        dst.SubresourceIndex = static_cast<UINT>((arrayLayer * mipLevelCount) + pCopyInfo->dstImage.mipLevel);

        UINT   numSubresources = 1;
        UINT   numRows         = 0;
        UINT64 rowSizeInBytes  = 0;
        UINT64 totalBytes      = 0;
        // Grab the format
        device->GetCopyableFootprints(
            &resouceDesc,
            dst.SubresourceIndex,
            numSubresources,
            static_cast<UINT64>(pCopyInfo->srcBuffer.footprintOffset),
            &src.PlacedFootprint,
            &numRows,
            &rowSizeInBytes,
            &totalBytes);

        //
        // Replace the values in case the footprint is a submimage
        //
        // NOTE: D3D12's debug layer will throw an error if RowPitch
        //       isn't aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT(256).
        //       But generally, we want to do this in the calling code
        //       and not here.
        //
        src.PlacedFootprint.Offset             = static_cast<UINT64>(pCopyInfo->srcBuffer.footprintOffset);
        src.PlacedFootprint.Footprint.Width    = static_cast<UINT>(pCopyInfo->srcBuffer.footprintWidth);
        src.PlacedFootprint.Footprint.Height   = static_cast<UINT>(pCopyInfo->srcBuffer.footprintHeight);
        src.PlacedFootprint.Footprint.Depth    = static_cast<UINT>(pCopyInfo->srcBuffer.footprintDepth);
        src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(pCopyInfo->srcBuffer.imageRowStride);

        mCommandList->CopyTextureRegion(
            &dst,
            static_cast<UINT>(pCopyInfo->dstImage.x),
            static_cast<UINT>(pCopyInfo->dstImage.y),
            static_cast<UINT>(pCopyInfo->dstImage.z),
            &src,
            nullptr);
    }
}

grfx::ImageToBufferOutputPitch CommandBuffer::CopyImageToBuffer(
    const grfx::ImageToBufferCopyInfo* pCopyInfo,
    grfx::Image*                       pSrcImage,
    grfx::Buffer*                      pDstBuffer)
{
    D3D12DevicePtr      device      = ToApi(GetDevice())->GetDxDevice();
    D3D12_RESOURCE_DESC resouceDesc = ToApi(pSrcImage)->GetDxResource()->GetDesc();

    const grfx::FormatDesc* srcDesc = grfx::GetFormatDescription(pSrcImage->GetFormat());

    // For depth-stencil images, each plane must be copied separately.
    uint32_t numPlanesToCopy = srcDesc->aspect == grfx::FORMAT_ASPECT_DEPTH_STENCIL ? 2 : 1;

    UINT64   currentOffset = 0;
    uint32_t rowPitch      = 0;
    for (uint32_t l = 0; l < pCopyInfo->srcImage.arrayLayerCount; ++l) {
        for (uint32_t p = 0; p < numPlanesToCopy; ++p) {
            D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
            srcLoc.pResource                   = ToApi(pSrcImage)->GetDxResource();
            srcLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            srcLoc.SubresourceIndex            = ToSubresourceIndex(pCopyInfo->srcImage.mipLevel, pCopyInfo->srcImage.arrayLayer + l, p, pSrcImage->GetMipLevelCount(), pSrcImage->GetArrayLayerCount());

            D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
            dstLoc.pResource                   = ToApi(pDstBuffer)->GetDxResource();
            dstLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

            UINT   numRows        = 0;
            UINT64 rowSizeInBytes = 0;
            UINT64 totalBytes     = 0;
            device->GetCopyableFootprints(
                &resouceDesc,
                srcLoc.SubresourceIndex,
                1,
                currentOffset,
                &dstLoc.PlacedFootprint,
                &numRows,
                &rowSizeInBytes,
                &totalBytes);

            // Depth-stencil textures can only be copied in full.
            if (pSrcImage->GetUsageFlags().bits.depthStencilAttachment) {
                mCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            }
            else {
                // Fix the footprint in case we are copying a portion of the image only.
                dstLoc.PlacedFootprint.Footprint.Width  = pCopyInfo->extent.x;
                dstLoc.PlacedFootprint.Footprint.Height = std::max(1u, pCopyInfo->extent.y);
                dstLoc.PlacedFootprint.Footprint.Depth  = std::max(1u, pCopyInfo->extent.z);

                UINT bytesPerTexel                        = srcDesc->bytesPerTexel;
                UINT bytesPerRow                          = bytesPerTexel * pCopyInfo->extent.x;
                dstLoc.PlacedFootprint.Footprint.RowPitch = RoundUp<UINT>(bytesPerRow, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

                D3D12_BOX srcBox = {0, 0, 0, 1, 1, 1};
                srcBox.left      = pCopyInfo->srcImage.offset.x;
                srcBox.right     = pCopyInfo->srcImage.offset.x + pCopyInfo->extent.x;
                if (pSrcImage->GetType() != IMAGE_TYPE_1D) { // Can only be set for 2D and 3D textures.
                    srcBox.top    = pCopyInfo->srcImage.offset.y;
                    srcBox.bottom = pCopyInfo->srcImage.offset.y + pCopyInfo->extent.y;
                }
                if (pSrcImage->GetType() == IMAGE_TYPE_3D) { // Can only be set for 3D textures.
                    srcBox.front = pCopyInfo->srcImage.offset.z;
                    srcBox.back  = pCopyInfo->srcImage.offset.z + pCopyInfo->extent.z;
                }

                mCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &srcBox);
            }

            currentOffset += static_cast<UINT64>(dstLoc.PlacedFootprint.Footprint.RowPitch) * dstLoc.PlacedFootprint.Footprint.Height;
            rowPitch = dstLoc.PlacedFootprint.Footprint.RowPitch;
        }
    }

    grfx::ImageToBufferOutputPitch outPitch;
    outPitch.rowPitch = rowPitch;
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

    // For depth-stencil images, each plane must be copied separately.
    uint32_t numPlanesToCopy = isSourceDepthStencil ? 2 : 1;

    for (uint32_t l = 0; l < pCopyInfo->srcImage.arrayLayerCount; ++l) {
        for (uint32_t p = 0; p < numPlanesToCopy; ++p) {
            D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
            srcLoc.pResource                   = ToApi(pSrcImage)->GetDxResource();
            srcLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            srcLoc.SubresourceIndex            = ToSubresourceIndex(pCopyInfo->srcImage.mipLevel, pCopyInfo->srcImage.arrayLayer + l, p, pSrcImage->GetMipLevelCount(), pSrcImage->GetArrayLayerCount());

            D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
            dstLoc.pResource                   = ToApi(pDstImage)->GetDxResource();
            dstLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLoc.SubresourceIndex            = ToSubresourceIndex(pCopyInfo->dstImage.mipLevel, pCopyInfo->dstImage.arrayLayer + l, p, pDstImage->GetMipLevelCount(), pDstImage->GetArrayLayerCount());

            // Depth-stencil textures can only be copied in full.
            if (isSourceDepthStencil) {
                mCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            }
            else {
                D3D12_BOX srcBox = {0, 0, 0, 1, 1, 1};
                srcBox.left      = pCopyInfo->srcImage.offset.x;
                srcBox.right     = pCopyInfo->srcImage.offset.x + pCopyInfo->extent.x;
                if (pSrcImage->GetType() != IMAGE_TYPE_1D) { // Can only be set for 2D and 3D textures.
                    srcBox.top    = pCopyInfo->srcImage.offset.y;
                    srcBox.bottom = pCopyInfo->srcImage.offset.y + pCopyInfo->extent.y;
                }
                if (pSrcImage->GetType() == IMAGE_TYPE_3D) { // Can only be set for 3D textures.
                    srcBox.front = pCopyInfo->srcImage.offset.z;
                    srcBox.back  = pCopyInfo->srcImage.offset.z + pCopyInfo->extent.z;
                }

                mCommandList->CopyTextureRegion(&dstLoc, pCopyInfo->dstImage.offset.x, pCopyInfo->dstImage.offset.y, pCopyInfo->dstImage.offset.z, &srcLoc, &srcBox);
            }
        }
    }
}

void CommandBuffer::BlitImage(
    const grfx::ImageBlitInfo* pCopyInfo,
    grfx::Image*               pSrcImage,
    grfx::Image*               pDstImage)
{
    PPX_ASSERT_MSG(false, "BlitImage is not implemented in DX12 backend");
}

void CommandBuffer::BeginQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    mCommandList->BeginQuery(
        ToApi(pQuery)->GetDxQueryHeap(),
        ToD3D12QueryType(pQuery->GetType()),
        static_cast<UINT>(queryIndex));
}

void CommandBuffer::EndQuery(
    const grfx::Query* pQuery,
    uint32_t           queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    mCommandList->EndQuery(
        ToApi(pQuery)->GetDxQueryHeap(),
        ToD3D12QueryType(pQuery->GetType()),
        static_cast<UINT>(queryIndex));
}

void CommandBuffer::WriteTimestamp(
    const grfx::Query*  pQuery,
    grfx::PipelineStage pipelineStage,
    uint32_t            queryIndex)
{
    PPX_ASSERT_NULL_ARG(pQuery);
    PPX_ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

    // NOTE: D3D12 timestamp queries only uses EndQuery, using BeginQuery
    //       will result in an error:
    //          D3D12 ERROR: ID3D12GraphicsCommandList::{Begin,End}Query: BeginQuery is not
    //          supported with D3D12_QUERY_TYPE specified.  Examples include
    //          D3D12_QUERY_TYPE_TIMESTAMP and D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS.
    //          [ EXECUTION ERROR #731: BEGIN_END_QUERY_INVALID_PARAMETERS]
    //
    PPX_ASSERT_MSG(ToApi(pQuery)->GetQueryType() == D3D12_QUERY_TYPE_TIMESTAMP, "invalid query type");
    mCommandList->EndQuery(
        ToApi(pQuery)->GetDxQueryHeap(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        static_cast<UINT>(queryIndex));
}

void CommandBuffer::ResolveQueryData(
    grfx::Query* pQuery,
    uint32_t     startIndex,
    uint32_t     numQueries)
{
    PPX_ASSERT_MSG((startIndex + numQueries) <= pQuery->GetCount(), "invalid query index/number");
    mCommandList->ResolveQueryData(ToApi(pQuery)->GetDxQueryHeap(), ToApi(pQuery)->GetQueryType(), startIndex, numQueries, ToApi(pQuery)->GetReadBackBuffer(), 0);
}

// -------------------------------------------------------------------------------------------------
// CommandPool
// -------------------------------------------------------------------------------------------------
Result CommandPool::CreateApiObjects(const grfx::CommandPoolCreateInfo* pCreateInfo)
{
    // clang-format off
    switch (pCreateInfo->pQueue->GetCommandType()) {
        default: break;
        case grfx::COMMAND_TYPE_GRAPHICS: mCommandType = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
        case grfx::COMMAND_TYPE_COMPUTE : mCommandType = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
        case grfx::COMMAND_TYPE_TRANSFER: mCommandType = D3D12_COMMAND_LIST_TYPE_COPY; break;
    }
    // clang-format on
    if (mCommandType == ppx::InvalidValue<D3D12_COMMAND_LIST_TYPE>()) {
        PPX_ASSERT_MSG(false, "invalid command type");
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    // HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateCommandAllocator(mCommandType, IID_PPV_ARGS(&mCommandAllocator));
    // if (FAILED(hr)) {
    //     PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandAllocator failed");
    //     return ppx::ERROR_API_FAILURE;
    // }
    // PPX_LOG_OBJECT_CREATION(D3D12CommandAllocator, mCommandAllocator.Get());

    return ppx::SUCCESS;
}

void CommandPool::DestroyApiObjects()
{
    // if (mCommandAllocator) {
    //     mCommandAllocator.Reset();
    // }
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
