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

#ifndef ppx_grfx_query_h
#define ppx_grfx_query_h

#include "ppx/grfx/grfx_config.h"

#define PPX_GRFX_PIPELINE_STATISTIC_NUM_ENTRIES 11

namespace ppx {
namespace grfx {

union PipelineStatistics
{
    struct
    {
        uint64_t IAVertices;    // Input Assembly Vertices
        uint64_t IAPrimitives;  // Input Assembly Primitives
        uint64_t VSInvocations; // Vertex Shader Invocations
        uint64_t GSInvocations; // Geometry Shader Invocations
        uint64_t GSPrimitives;  // Geometry Shader Primitives
        uint64_t CInvocations;  // Clipping Invocations
        uint64_t CPrimitives;   // Clipping Primitives
        uint64_t PSInvocations; // Pixel Shader Invocations
        uint64_t HSInvocations; // Hull Shader Invocations
        uint64_t DSInvocations; // Domain Shader Invocations
        uint64_t CSInvocations; // Compute Shader Invocations
    };
    uint64_t Statistics[PPX_GRFX_PIPELINE_STATISTIC_NUM_ENTRIES] = {0};
};

//! @struct QueryCreateInfo
//!
//!
struct QueryCreateInfo
{
    grfx::QueryType type  = QUERY_TYPE_UNDEFINED;
    uint32_t        count = 0;
};

//! @class Query
//!
//!
class Query
    : public grfx::DeviceObject<grfx::QueryCreateInfo>
{
public:
    Query() {}
    virtual ~Query() {}

    grfx::QueryType GetType() const { return mCreateInfo.type; }
    uint32_t        GetCount() const { return mCreateInfo.count; }

    virtual void   Reset(uint32_t firstQuery, uint32_t queryCount) = 0;
    virtual Result GetData(void* pDstData, uint64_t dstDataSize)   = 0;

protected:
    virtual Result Create(const grfx::QueryCreateInfo* pCreateInfo) override;
    friend class grfx::Device;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_query_h
