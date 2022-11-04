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
    D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

    HRESULT hr = ToApi(GetDevice())->GetDxDevice()->CreateFence(mValue, flags, IID_PPV_ARGS(&mFence));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateFence(fence) failed");
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Fence(Semaphore), mFence.Get());

    return ppx::SUCCESS;
}

void Semaphore::DestroyApiObjects()
{
    if (mFence) {
        mFence.Reset();
    }
}

UINT64 Semaphore::GetNextSignalValue()
{
    mValue += 1;
    return mValue;
}

UINT64 Semaphore::GetWaitForValue() const
{
    return mValue;
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
