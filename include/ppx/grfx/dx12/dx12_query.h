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

#ifndef ppx_grfx_dx12_query_h
#define ppx_grfx_dx12_query_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_query.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class Query
    : public grfx::Query
{
public:
    Query();
    virtual ~Query() {}

    typename D3D12QueryHeapPtr::InterfaceType* GetDxQueryHeap() const { return mHeap.Get(); }
    D3D12_QUERY_TYPE                           GetQueryType() const { return mQueryType; }
    typename D3D12ResourcePtr::InterfaceType*  GetReadBackBuffer() const;

    virtual void   Reset(uint32_t firstQuery, uint32_t queryCount) override;
    virtual Result GetData(void* pDstData, uint64_t dstDataSize) override;

protected:
    virtual Result CreateApiObjects(const grfx::QueryCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;
    uint32_t       GetQueryTypeSize(D3D12_QUERY_HEAP_TYPE type) const;

private:
    D3D12QueryHeapPtr mHeap;
    D3D12_QUERY_TYPE  mQueryType;
    grfx::BufferPtr   mBuffer;
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_query_h
