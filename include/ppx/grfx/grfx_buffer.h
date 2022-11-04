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

#ifndef ppx_grfx_buffer_h
#define ppx_grfx_buffer_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct BufferCreateInfo
//!
//!
struct BufferCreateInfo
{
    uint64_t               size                    = 0;
    uint32_t               structuredElementStride = 0; // HLSL StructuredBuffer<> only
    grfx::BufferUsageFlags usageFlags              = 0;
    grfx::MemoryUsage      memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
    grfx::ResourceState    initialState            = grfx::RESOURCE_STATE_GENERAL;
    grfx::Ownership        ownership               = grfx::OWNERSHIP_REFERENCE;
};

//! @class Buffer
//!
//!
class Buffer
    : public grfx::DeviceObject<grfx::BufferCreateInfo>
{
public:
    Buffer() {}
    virtual ~Buffer() {}

    uint64_t                      GetSize() const { return mCreateInfo.size; }
    uint32_t                      GetStructuredElementStride() const { return mCreateInfo.structuredElementStride; }
    const grfx::BufferUsageFlags& GetUsageFlags() const { return mCreateInfo.usageFlags; }

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) = 0;
    virtual void   UnmapMemory()                                      = 0;

    Result CopyFromSource(uint32_t dataSize, const void* pData);
    Result CopyToDest(uint32_t dataSize, void* pData);

private:
    virtual Result Create(const grfx::BufferCreateInfo* pCreateInfo) override;
    friend class grfx::Device;
};

// -------------------------------------------------------------------------------------------------

struct IndexBufferView
{
    const grfx::Buffer* pBuffer   = nullptr;
    grfx::IndexType     indexType = grfx::INDEX_TYPE_UINT16;
    uint64_t            offset    = 0;

    IndexBufferView() {}

    IndexBufferView(const grfx::Buffer* pBuffer_, grfx::IndexType indexType_, uint64_t offset_ = 0)
        : pBuffer(pBuffer_), indexType(indexType_), offset(offset_) {}
};

// -------------------------------------------------------------------------------------------------

struct VertexBufferView
{
    const grfx::Buffer* pBuffer = nullptr;
    uint32_t            stride  = 0; // [D3D12 - REQUIRED] Stride in bytes of vertex entry
    uint64_t            offset  = 0;

    VertexBufferView() {}

    VertexBufferView(const grfx::Buffer* pBuffer_, uint32_t stride_, uint64_t offset_ = 0)
        : pBuffer(pBuffer_), stride(stride_), offset(offset_) {}
};

//// -------------------------------------------------------------------------------------------------
//
//struct IndexBufferCreateInfo
//{
//    grfx::IndexType indexType  = grfx::INDEX_TYPE_UINT16;
//    uint32_t        indexCount = 0;
//};
//
//class IndexBuffer
//{
//public:
//    IndexBuffer() {}
//    virtual ~IndexBuffer() {}
//
//private:
//    grfx::BufferPtr       mBuffer;
//    grfx::IndexBufferView mView = {};
//};
//
//// -------------------------------------------------------------------------------------------------
//
//struct VertexBufferCreateInfo
//{
//    grfx::VertexBinding binding     = {};
//    uint32_t            vertexCount = 0;
//};
//
//class VertexBuffer
//{
//public:
//    VertexBuffer() {}
//    virtual ~VertexBuffer() {}
//
//private:
//    grfx::BufferPtr mBuffer;
//    grfx::VertexBufferView = {};
//};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_buffer_h
