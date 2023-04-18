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

#include "ppx/grfx/dx12/dx12_buffer.h"
#include "ppx/grfx/dx12/dx12_device.h"

namespace ppx {
namespace grfx {
namespace dx12 {

Result Buffer::CreateApiObjects(const grfx::BufferCreateInfo* pCreateInfo)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (pCreateInfo->usageFlags.bits.rawStorageBuffer ||
        pCreateInfo->usageFlags.bits.rwStructuredBuffer) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment           = 0;
    resourceDesc.Width               = static_cast<UINT64>(pCreateInfo->size);
    resourceDesc.Height              = 1;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count    = 1; // Must be 1 or will fail
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags               = flags;

    // Save D3D12 allocation heap type to be used by Buffer::MapMemory an Buffer::UnmapMemory
    //
    mHeapType = ToD3D12HeapType(pCreateInfo->memoryUsage);

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType                 = mHeapType;

    D3D12_RESOURCE_STATES initialResourceState = ToD3D12ResourceStates(pCreateInfo->initialState, grfx::COMMAND_TYPE_GRAPHICS);
    // Using D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ
    //
    if (mHeapType == D3D12_HEAP_TYPE_UPLOAD) {
        initialResourceState |= D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    // Using D3D12_HEAP_TYPE_READBACK requires D3D12_RESOURCE_STATE_COPY_DEST
    //
    if (mHeapType == D3D12_HEAP_TYPE_READBACK) {
        initialResourceState |= D3D12_RESOURCE_STATE_COPY_DEST;
    }

    dx12::Device* pDevice = ToApi(GetDevice());
    HRESULT       hr      = pDevice->GetAllocator()->CreateResource(
        &allocationDesc,
        &resourceDesc,
        initialResourceState,
        NULL,
        &mAllocation,
        IID_PPV_ARGS(&mResource));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Resource(Buffer), mResource.Get());

    return ppx::SUCCESS;
}

void Buffer::DestroyApiObjects()
{
    if (mResource) {
        mResource.Reset();
    }

    if (mAllocation) {
        mAllocation->Release();
        mAllocation.Reset();
    }

    mHeapType = InvalidValue<D3D12_HEAP_TYPE>();
}

Result Buffer::MapMemory(uint64_t offset, void** ppMappedAddress)
{
    if (IsNull(ppMappedAddress)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Use {0, 0} if we're not intending to read this resource
    //
    D3D12_RANGE readRange = {0, 0};
    if (mHeapType == D3D12_HEAP_TYPE_READBACK) {
        readRange.Begin = offset;
        readRange.End   = mCreateInfo.size;
    }

    HRESULT hr = mResource->Map(0, &readRange, ppMappedAddress);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    return ppx::SUCCESS;
}

void Buffer::UnmapMemory()
{
    // Use {0, 0} if no data is written
    //
    D3D12_RANGE writtenRange = {0, 0};

    // Use NULL if data might have been written
    //
    D3D12_RANGE* pRange = (mHeapType == D3D12_HEAP_TYPE_UPLOAD) ? nullptr : &writtenRange;

    mResource->Unmap(0, pRange);
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
