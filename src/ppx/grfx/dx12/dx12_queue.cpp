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

#include "ppx/grfx/dx12/dx12_queue.h"
#include "ppx/grfx/dx12/dx12_command.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_sync.h"

namespace ppx {
namespace grfx {
namespace dx12 {

Result Queue::CreateApiObjects(const grfx::internal::QueueCreateInfo* pCreateInfo)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo->pApiObject);
    if (IsNull(pCreateInfo->pApiObject)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    CComPtr<ID3D12CommandQueue> queue = static_cast<ID3D12CommandQueue*>(pCreateInfo->pApiObject);

    HRESULT hr = queue.QueryInterface(&mCommandQueue);
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "failed casting to ID3DCommandQueue");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12CommandQueue, mCommandQueue.Get());

    grfx::FenceCreateInfo fenceCreateInfo = {};
    Result                ppxres          = GetDevice()->CreateFence(&fenceCreateInfo, &mWaitIdleFence);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "wait for idle fence create failed");
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Queue::DestroyApiObjects()
{
    WaitIdle();

    if (mWaitIdleFence) {
        mWaitIdleFence.Reset();
    }

    if (mCommandQueue) {
        mCommandQueue.Reset();
    }
}

Result Queue::WaitIdle()
{
    dx12::Fence* pFence = ToApi(mWaitIdleFence.Get());
    UINT64       value  = pFence->GetNextSignalValue();
    mCommandQueue->Signal(pFence->GetDxFence(), value);
    Result ppxres = pFence->Wait();
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

Result Queue::Submit(const grfx::SubmitInfo* pSubmitInfo)
{
    if (mListBuffer.size() < pSubmitInfo->commandBufferCount) {
        mListBuffer.resize(pSubmitInfo->commandBufferCount);
    }

    for (uint32_t i = 0; i < pSubmitInfo->commandBufferCount; ++i) {
        mListBuffer[i] = ToApi(pSubmitInfo->ppCommandBuffers[i])->GetDxCommandList();
    }

    for (uint32_t i = 0; i < pSubmitInfo->waitSemaphoreCount; ++i) {
        ID3D12Fence* pDxFence = ToApi(pSubmitInfo->ppWaitSemaphores[i])->GetDxFence();
        UINT64       value    = ToApi(pSubmitInfo->ppWaitSemaphores[i])->GetWaitForValue();
        HRESULT      hr       = mCommandQueue->Wait(pDxFence, value);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12CommandQueue::Wait failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    mCommandQueue->ExecuteCommandLists(
        static_cast<UINT>(pSubmitInfo->commandBufferCount),
        mListBuffer.data());

    for (uint32_t i = 0; i < pSubmitInfo->signalSemaphoreCount; ++i) {
        ID3D12Fence* pDxFence = ToApi(pSubmitInfo->ppSignalSemaphores[i])->GetDxFence();
        UINT64       value    = ToApi(pSubmitInfo->ppSignalSemaphores[i])->GetNextSignalValue();
        HRESULT      hr       = mCommandQueue->Signal(pDxFence, value);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12CommandQueue::Signal failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    if (!IsNull(pSubmitInfo->pFence)) {
        dx12::Fence* pFence = ToApi(pSubmitInfo->pFence);
        UINT64       value  = pFence->GetNextSignalValue();
        HRESULT      hr     = mCommandQueue->Signal(pFence->GetDxFence(), value);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12CommandQueue::Signal failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

Result Queue::GetTimestampFrequency(uint64_t* pFrequency) const
{
    if (IsNull(pFrequency)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    HRESULT hr = mCommandQueue->GetTimestampFrequency(reinterpret_cast<UINT64*>(pFrequency));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    return ppx::SUCCESS;
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
