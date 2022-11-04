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

#ifndef ppx_grfx_vk_query_h
#define ppx_grfx_vk_query_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_query.h"

namespace ppx {
namespace grfx {
namespace vk {

class Query
    : public grfx::Query
{
public:
    Query();
    virtual ~Query() {}

    VkQueryPoolPtr GetVkQueryPool() const { return mQueryPool; }
    uint32_t       GetQueryTypeSize() const { return GetQueryTypeSize(mType, mMultiplier); }
    VkBufferPtr    GetReadBackBuffer() const;

    virtual void   Reset(uint32_t firstQuery, uint32_t queryCount) override;
    virtual Result GetData(void* pDstData, uint64_t dstDataSize) override;

protected:
    virtual Result CreateApiObjects(const grfx::QueryCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;
    uint32_t       GetQueryTypeSize(VkQueryType type, uint32_t multiplier) const;
    VkQueryType    GetQueryType() const { return mType; }

private:
    VkQueryPoolPtr  mQueryPool;
    VkQueryType     mType;
    grfx::BufferPtr mBuffer;
    uint32_t        mMultiplier;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_query_h
