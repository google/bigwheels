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

#include "ppx/grfx/dx11/dx11_query.h"
#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_command_list.h"
#include "ppx/grfx/dx11/dx11_command.h"

namespace ppx {
namespace grfx {
namespace dx11 {

Query::Query()
    : mQueryType(InvalidValue<D3D11_QUERY>()), mResolveDataStartIndex(0), mResolveDataNumQueries(0)
{
}

Result Query::CreateApiObjects(const grfx::QueryCreateInfo* pCreateInfo)
{
    mQueryType = ToD3D11QueryType(pCreateInfo->type);
    mHeap.resize(pCreateInfo->count);

    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    for (UINT i = 0; i < pCreateInfo->count; ++i) {
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query            = mQueryType;
        HRESULT hr                 = device->CreateQuery(&queryDesc, &mHeap[i]);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D11Device::CreateQuery failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

void Query::DestroyApiObjects()
{
}

ID3D11Query* Query::GetQuery(uint32_t queryIndex) const
{
    PPX_ASSERT_MSG(queryIndex < mHeap.size(), "invalid query index");
    return mHeap[queryIndex];
}

void Query::Reset(uint32_t firstQuery, uint32_t queryCount)
{
    (void)firstQuery;
    (void)queryCount;
}

Result Query::GetData(void* pDstData, uint64_t dstDataSize)
{
    uint32_t dstStride = (uint32_t)(dstDataSize / GetCount());
    for (uint32_t i = 0; i < mResolveDataNumQueries; ++i) {
        uint32_t queryIdx = mResolveDataStartIndex + i;
        PPX_ASSERT_MSG(queryIdx < mHeap.size(), "invalid query index");
        ID3D11Query* pQuery     = mHeap[queryIdx];
        uint64_t*    pQueryDest = (uint64_t*)pDstData + queryIdx;

        HRESULT queryResult = S_FALSE;
        while (queryResult == S_FALSE) {
            queryResult = ToApi(GetDevice())->GetDxDeviceContext()->GetData(pQuery, pQueryDest, dstStride, 0);
        }

        if (queryResult != S_OK) {
            return ppx::ERROR_FAILED;
        }
    }

    return ppx::SUCCESS;
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
