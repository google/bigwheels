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

#include "ppx/grfx/dx12/dx12_sync.h"
#include "ppx/grfx/dx12/dx12_device.h"

#define REQUIRES_BINARY_MSG "invalid semaphore type: operation requires binary semaphore"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// Fence
// -------------------------------------------------------------------------------------------------
Result Fence::CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo)
{
    D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

    HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateFence(mValue, flags, IID_PPV_ARGS(&mFence));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateFence(fence) failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Fence(Fence), mFence.Get());

    mFenceEventHandle = CreateEventEx(NULL, NULL, false, EVENT_ALL_ACCESS);
    if (mFenceEventHandle == INVALID_HANDLE_VALUE) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Fence::DestroyApiObjects()
{
    CloseHandle(mFenceEventHandle);

    if (mFence) {
        mFence.Reset();
    }
}

UINT64 Fence::GetNextSignalValue()
{
    mValue += 1;
    return mValue;
}

UINT64 Fence::GetWaitForValue() const
{
    return mValue;
}

Result Fence::Wait(uint64_t timeout)
{
    UINT64 completedValue = mFence->GetCompletedValue();
    if (completedValue < GetWaitForValue()) {
        mFence->SetEventOnCompletion(mValue, mFenceEventHandle);

        DWORD dwMillis = (timeout == UINT64_MAX) ? INFINITE : static_cast<DWORD>(timeout / 1000000ULL);
        WaitForSingleObjectEx(mFenceEventHandle, dwMillis, false);
    }
    return ppx::SUCCESS;
}

Result Fence::Reset()
{
    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// Semaphore
// -------------------------------------------------------------------------------------------------
Result Semaphore::CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo)
{
    UINT64 initialValue = mValue;
    if (pCreateInfo->semaphoreType == grfx::SEMAPHORE_TYPE_TIMELINE) {
        initialValue = static_cast<UINT64>(pCreateInfo->initialValue);
    }

    D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

    HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateFence(initialValue, flags, IID_PPV_ARGS(&mFence));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateFence(fence) failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Fence(Semaphore), mFence.Get());

    // Wait event handle
    if (pCreateInfo->semaphoreType == grfx::SEMAPHORE_TYPE_TIMELINE) {
        mWaitEventHandle = CreateEventEx(NULL, NULL, false, EVENT_ALL_ACCESS);
        if (mWaitEventHandle == INVALID_HANDLE_VALUE) {
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

void Semaphore::DestroyApiObjects()
{
    if (mWaitEventHandle != nullptr) {
        CloseHandle(mWaitEventHandle);
    }

    if (mFence) {
        mFence.Reset();
    }
}

UINT64 Semaphore::GetNextSignalValue()
{
    if (this->GetSemaphoreType() != grfx::SEMAPHORE_TYPE_BINARY) {
        PPX_ASSERT_MSG(false, REQUIRES_BINARY_MSG);
        return UINT64_MAX;
    }

    mValue += 1;
    return mValue;
}

UINT64 Semaphore::GetWaitForValue() const
{
    if (this->GetSemaphoreType() != grfx::SEMAPHORE_TYPE_BINARY) {
        PPX_ASSERT_MSG(false, REQUIRES_BINARY_MSG);
        return UINT64_MAX;
    }

    return mValue;
}

Result Semaphore::TimelineWait(uint64_t value, uint64_t timeout) const
{
    UINT64 completedValue = mFence->GetCompletedValue();
    if (completedValue < value) {
        mFence->SetEventOnCompletion(value, mWaitEventHandle);

        DWORD dwMillis = (timeout == UINT64_MAX) ? INFINITE : static_cast<DWORD>(timeout / 1000000ULL);
        WaitForSingleObjectEx(mWaitEventHandle, dwMillis, false);
    }

    return ppx::SUCCESS;
}

Result Semaphore::TimelineSignal(uint64_t value) const
{
    HRESULT hr = mFence->Signal(static_cast<UINT64>(value));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

uint64_t Semaphore::TimelineCounterValue() const
{
    UINT64 value = mFence->GetCompletedValue();
    return static_cast<uint64_t>(value);
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
