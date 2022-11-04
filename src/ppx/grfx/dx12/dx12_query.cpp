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

#include "ppx/grfx/dx12/dx12_query.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_command.h"
#include "ppx/grfx/dx12/dx12_buffer.h"

namespace ppx {
namespace grfx {
namespace dx12 {

Query::Query()
    : mQueryType(InvalidValue<D3D12_QUERY_TYPE>())
{
}

uint32_t Query::GetQueryTypeSize(D3D12_QUERY_HEAP_TYPE type) const
{
    uint32_t result = 0;
    switch (type) {
        case D3D12_QUERY_HEAP_TYPE_OCCLUSION:
            result = 8;
            break;
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
        case D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP:
            result = (uint32_t)sizeof(uint64_t);
            break;
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:
            result = (uint32_t)sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
            break;
        case D3D12_QUERY_HEAP_TYPE_SO_STATISTICS:
            result = (uint32_t)sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
            break;
        default:
            PPX_ASSERT_MSG(false, "Unsupported query type");
            break;
    }
    return result;
}

Result Query::CreateApiObjects(const grfx::QueryCreateInfo* pCreateInfo)
{
    D3D12_QUERY_HEAP_DESC desc = {};
    desc.Type                  = ToD3D12QueryHeapType(pCreateInfo->type);
    desc.Count                 = static_cast<UINT>(pCreateInfo->count);

    D3D12DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    HRESULT hr = device->CreateQueryHeap(&desc, IID_PPV_ARGS(&mHeap));
    if (FAILED(hr)) {
        PPX_ASSERT_MSG(false, "ID3D12Device::CreateQueryHeap failed");
        return ppx::ERROR_API_FAILURE;
    }

    mQueryType = ToD3D12QueryType(pCreateInfo->type);

    grfx::BufferCreateInfo createInfo  = {};
    createInfo.size                    = GetCount() * GetQueryTypeSize(desc.Type);
    createInfo.structuredElementStride = GetQueryTypeSize(desc.Type);
    createInfo.usageFlags              = grfx::BUFFER_USAGE_TRANSFER_DST;
    createInfo.memoryUsage             = grfx::MEMORY_USAGE_GPU_TO_CPU;
    createInfo.initialState            = grfx::RESOURCE_STATE_COPY_DST;
    createInfo.ownership               = grfx::OWNERSHIP_REFERENCE;

    // Create buffer
    Result ppxres = GetDevice()->CreateBuffer(&createInfo, &mBuffer);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Query::DestroyApiObjects()
{
    if (mHeap) {
        mHeap.Reset();
    }

    if (mBuffer) {
        mBuffer.Reset();
    }
}

void Query::Reset(uint32_t firstQuery, uint32_t queryCount)
{
    (void)firstQuery;
    (void)queryCount;
}

Result Query::GetData(void* pDstData, uint64_t dstDataSize)
{
    void*  pMappedAddress = 0;
    Result ppxres         = mBuffer->MapMemory(0, &pMappedAddress);
    if (Failed(ppxres)) {
        return ppxres;
    }

    size_t copySize = std::min<size_t>(dstDataSize, mBuffer->GetSize());
    memcpy(pDstData, pMappedAddress, copySize);

    mBuffer->UnmapMemory();

    return ppx::SUCCESS;
}

typename D3D12ResourcePtr::InterfaceType* Query::GetReadBackBuffer() const
{
    return ToApi(mBuffer.Get())->GetDxResource();
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
