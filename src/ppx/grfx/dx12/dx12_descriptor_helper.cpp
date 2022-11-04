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

#include "ppx/grfx/dx12/dx12_descriptor_helper.h"
#include "ppx/grfx/dx12/dx12_device.h"

namespace ppx {
namespace grfx {
namespace dx12 {

// -------------------------------------------------------------------------------------------------
// DescriptorHandleHeap
// -------------------------------------------------------------------------------------------------
DescriptorHandleAllocator::DescriptorHandleAllocator()
{
}

DescriptorHandleAllocator::~DescriptorHandleAllocator()
{
}

Result DescriptorHandleAllocator::Create(dx12::Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    mHandles.resize(MAX_DESCRIPTOR_HANDLE_HEAP_SIZE);

    D3D12_DESCRIPTOR_HEAP_DESC d3d12Desc = {};
    d3d12Desc.Type                       = type;
    d3d12Desc.NumDescriptors             = MAX_DESCRIPTOR_HANDLE_HEAP_SIZE;
    d3d12Desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    d3d12Desc.NodeMask                   = 0;

    HRESULT hr = pDevice->GetDxDevice()->CreateDescriptorHeap(&d3d12Desc, IID_PPV_ARGS(&mHeap));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    mHeapStart     = mHeap->GetCPUDescriptorHandleForHeapStart();
    mIncrementSize = pDevice->GetDxDevice()->GetDescriptorHandleIncrementSize(type);

    return ppx::SUCCESS;
}

void DescriptorHandleAllocator::Destroy()
{
    if (mHeap) {
        mHeap.Reset();
    }
}

UINT DescriptorHandleAllocator::FirstAvailableIndex() const
{
    UINT index = UINT_MAX;
    for (size_t i = 0; i < mHandles.size(); ++i) {
        const DescriptorHandle& handle = mHandles[i];
        if ((handle.offset == UINT_MAX) && (handle.handle.ptr == dx12::INVALID_D3D12_DESCRIPTOR_HANDLE)) {
            index = static_cast<UINT>(i);
            break;
        }
    }
    return index;
}

Result DescriptorHandleAllocator::AllocateHandle(DescriptorHandle* pHandle)
{
    UINT offset = FirstAvailableIndex();
    if (offset == UINT_MAX) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    mHandles[offset].offset     = offset;
    mHandles[offset].handle.ptr = mHeapStart.ptr + (INT64(offset) * UINT64(mIncrementSize));

    *pHandle = mHandles[offset];

    return ppx::SUCCESS;
}

void DescriptorHandleAllocator::FreeHandle(const dx12::DescriptorHandle& handle)
{
    UINT index        = handle.offset;
    bool isSameOffset = (handle.offset == mHandles[index].offset);
    bool isSamePtr    = (handle.handle.ptr == mHandles[index].handle.ptr);
    if (isSameOffset && isSamePtr) {
        mHandles[index].Reset();
    }
}

bool DescriptorHandleAllocator::HasHandle(const dx12::DescriptorHandle& handle) const
{
    auto it = std::find_if(
        std::begin(mHandles),
        std::end(mHandles),
        [handle](const dx12::DescriptorHandle& elem) -> bool {
            bool isSameOffset = (handle.offset == elem.offset);
            bool isSamePtr    = (handle.handle.ptr == elem.handle.ptr);
            return isSameOffset && isSamePtr;
        });
    bool found = (it != std::end(mHandles));
    return found;
}

bool DescriptorHandleAllocator::HasAvailableHandle() const
{
    UINT index  = FirstAvailableIndex();
    bool result = (index != UINT_MAX) ? true : false;
    return result;
}

// -------------------------------------------------------------------------------------------------
// DescriptorHandleHeap
// -------------------------------------------------------------------------------------------------
DescriptorHandleManager::DescriptorHandleManager()
{
}

DescriptorHandleManager::~DescriptorHandleManager()
{
}

Result DescriptorHandleManager::Create(dx12::Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    mDevice = pDevice;
    mType   = type;

    dx12::DescriptorHandleAllocator* pHeap = new dx12::DescriptorHandleAllocator();
    if (IsNull(pHeap)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    Result result = pHeap->Create(mDevice, type);
    if (result != ppx::SUCCESS) {
        delete pHeap;
        return result;
    }

    mAllocators.push_back(pHeap);

    return ppx::SUCCESS;
}

void DescriptorHandleManager::Destroy()
{
    for (size_t i = 0; i < mAllocators.size(); ++i) {
        delete mAllocators[i];
        mAllocators[i] = nullptr;
    }
    mAllocators.clear();

    mDevice = nullptr;
    mType   = InvalidValue<D3D12_DESCRIPTOR_HEAP_TYPE>();
}

Result DescriptorHandleManager::AllocateHandle(DescriptorHandle* pHandle)
{
    bool allocated = false;
    for (size_t i = 0; i < mAllocators.size(); ++i) {
        Result result = mAllocators[i]->AllocateHandle(pHandle);
        if (result == ppx::SUCCESS) {
            allocated = true;
            break;
        }
    }

    if (!allocated) {
        dx12::DescriptorHandleAllocator* pHeap = new dx12::DescriptorHandleAllocator();
        if (IsNull(pHeap)) {
            return ppx::ERROR_ALLOCATION_FAILED;
        }

        Result result = pHeap->Create(mDevice, mType);
        if (result != ppx::SUCCESS) {
            delete pHeap;
            return result;
        }

        mAllocators.push_back(pHeap);

        result = pHeap->AllocateHandle(pHandle);
        if (result != ppx::SUCCESS) {
            return result;
        }
    }

    return ppx::SUCCESS;
}

void DescriptorHandleManager::FreeHandle(const DescriptorHandle& handle)
{
    dx12::DescriptorHandleAllocator* pHeap = nullptr;
    for (size_t i = 0; i < mAllocators.size(); ++i) {
        bool found = mAllocators[i]->HasHandle(handle);
        if (found) {
            pHeap = mAllocators[i];
            break;
        }
    }

    if (!IsNull(pHeap)) {
        pHeap->FreeHandle(handle);
    }
}

bool DescriptorHandleManager::HasHandle(const DescriptorHandle& handle) const
{
    bool found = false;
    for (size_t i = 0; i < mAllocators.size(); ++i) {
        found = mAllocators[i]->HasHandle(handle);
        if (found) {
            break;
        }
    }
    return found;
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
