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

#ifndef ppx_grfx_mesh_h
#define ppx_grfx_mesh_h

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_buffer.h"
#include "ppx/geometry.h"

namespace ppx {
namespace grfx {

struct MeshVertexAttribute
{
    grfx::Format format = grfx::FORMAT_UNDEFINED;

    // Use 0 to have stride calculated from format
    uint32_t stride = 0;

    // Not used for mesh/vertex buffer creation.
    // Gets calculated during creation for queries afterwards.
    uint32_t offset = 0;

    // [OPTIONAL] Useful for debugging.
    grfx::VertexSemantic vertexSemantic = grfx::VERTEX_SEMANTIC_UNDEFINED;
};

struct MeshVertexBufferDescription
{
    uint32_t                  attributeCount                      = 0;
    grfx::MeshVertexAttribute attributes[PPX_MAX_VERTEX_BINDINGS] = {};

    // Use 0 to have stride calculated from attributes
    uint32_t stride = 0;

    grfx::VertexInputRate vertexInputRate = grfx::VERTEX_INPUT_RATE_VERTEX;
};

//! @struct MeshCreateInfo
//!
//! Usage Notes:
//!   - Index and vertex data configuration needs to make sense
//!       - If \b indexCount is 0 then \b vertexCount cannot be 0
//!   - To create a mesh without an index buffer, \b indexType must be grfx::INDEX_TYPE_UNDEFINED
//!   - If \b vertexCount is 0 then no vertex buffers will be created
//!       - This means vertex buffer information will be ignored
//!   - Active elements in \b vertexBuffers cannot have an \b attributeCount of 0
//!
struct MeshCreateInfo
{
    grfx::IndexType                   indexType                              = grfx::INDEX_TYPE_UNDEFINED;
    uint32_t                          indexCount                             = 0;
    uint32_t                          vertexCount                            = 0;
    uint32_t                          vertexBufferCount                      = 0;
    grfx::MeshVertexBufferDescription vertexBuffers[PPX_MAX_VERTEX_BINDINGS] = {};
    grfx::MemoryUsage                 memoryUsage                            = grfx::MEMORY_USAGE_GPU_ONLY;

    MeshCreateInfo() {}
    MeshCreateInfo(const ppx::Geometry& geometry);
};

//! @class Mesh
//!
//! The \b Mesh class is a straight forward geometry container for the GPU.
//! A \b Mesh instance consists of vertex data and an optional index buffer.
//! The vertex data is stored in on or more vertex buffers. Each vertex buffer
//! can store data for one or more attributes. The index data is stored in an
//! index buffer.
//!
//! A \b Mesh instance does not store vertex binding information. Even if the
//! create info is derived from a ppx::Geometry instance. This design is
//! intentional since it enables calling applications to map vertex attributes
//! and vertex buffers to how it sees fit. For convenience, the function
//! \b Mesh::GetDerivedVertexBindings() returns vertex bindings derived from
//! a \Mesh instance's vertex buffer descriptions.
//!
class Mesh
    : public grfx::DeviceObject<grfx::MeshCreateInfo>
{
public:
    Mesh() {}
    virtual ~Mesh() {}

    grfx::IndexType GetIndexType() const { return mCreateInfo.indexType; }
    uint32_t        GetIndexCount() const { return mCreateInfo.indexCount; }
    grfx::BufferPtr GetIndexBuffer() const { return mIndexBuffer; }

    uint32_t                                 GetVertexCount() const { return mCreateInfo.vertexCount; }
    uint32_t                                 GetVertexBufferCount() const { return CountU32(mVertexBuffers); }
    grfx::BufferPtr                          GetVertexBuffer(uint32_t index) const;
    const grfx::MeshVertexBufferDescription* GetVertexBufferDescription(uint32_t index) const;

    //! Returns derived vertex bindings based on the vertex buffer description
    const std::vector<grfx::VertexBinding>& GetDerivedVertexBindings() const { return mDerivedVertexBindings; }

protected:
    virtual Result CreateApiObjects(const grfx::MeshCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    grfx::BufferPtr                                                            mIndexBuffer;
    std::vector<std::pair<grfx::BufferPtr, grfx::MeshVertexBufferDescription>> mVertexBuffers;
    std::vector<grfx::VertexBinding>                                           mDerivedVertexBindings;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_mesh_h
