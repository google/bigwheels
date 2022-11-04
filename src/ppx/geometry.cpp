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

#include "ppx/geometry.h"

#define NOT_INTERLEAVED_MSG "cannot append interleaved data if attribute layout is not interleaved"
#define NOT_PLANAR_MSG      "cannot append planar data if attribute layout is not planar"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// GeometryOptions
// -------------------------------------------------------------------------------------------------
GeometryOptions GeometryOptions::InterleavedU16()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
    ci.indexType             = grfx::INDEX_TYPE_UINT16;
    ci.vertexBindingCount    = 1; // Interleave attribute layout always has 1 vertex binding
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::InterleavedU32()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
    ci.indexType             = grfx::INDEX_TYPE_UINT32;
    ci.vertexBindingCount    = 1; // Interleave attribute layout always has 1 vertex binding
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::PlanarU16()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
    ci.indexType             = grfx::INDEX_TYPE_UINT16;
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::PlanarU32()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
    ci.indexType             = grfx::INDEX_TYPE_UINT32;
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::Interleaved()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
    ci.indexType             = grfx::INDEX_TYPE_UNDEFINED;
    ci.vertexBindingCount    = 1; // Interleave attribute layout always has 1 vertex binding
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::Planar()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
    ci.indexType             = grfx::INDEX_TYPE_UNDEFINED;
    ci.AddPosition();
    return ci;
}

GeometryOptions& GeometryOptions::IndexType(grfx::IndexType indexType_)
{
    indexType = indexType_;
    return *this;
}

GeometryOptions& GeometryOptions::IndexTypeU16()
{
    return IndexType(grfx::INDEX_TYPE_UINT16);
}

GeometryOptions& GeometryOptions::IndexTypeU32()
{
    return IndexType(grfx::INDEX_TYPE_UINT32);
}

GeometryOptions& GeometryOptions::AddAttribute(grfx::VertexSemantic semantic, grfx::Format format)
{
    bool exists = false;
    for (uint32_t bindingIndex = 0; bindingIndex < vertexBindingCount; ++bindingIndex) {
        const grfx::VertexBinding& binding = vertexBindings[bindingIndex];
        for (uint32_t attrIndex = 0; attrIndex < binding.GetAttributeCount(); ++attrIndex) {
            const grfx::VertexAttribute* pAttribute = nullptr;
            binding.GetAttribute(attrIndex, &pAttribute);
            exists = (pAttribute->semantic == semantic);
            if (exists) {
                break;
            }
        }
        if (exists) {
            break;
        }
    }

    if (!exists) {
        uint32_t location = 0;
        for (uint32_t bindingIndex = 0; bindingIndex < vertexBindingCount; ++bindingIndex) {
            const grfx::VertexBinding& binding = vertexBindings[bindingIndex];
            location += binding.GetAttributeCount();
        }

        grfx::VertexAttribute attribute = {};
        attribute.semanticName          = ToString(semantic);
        attribute.location              = location;
        attribute.format                = format;
        attribute.binding               = PPX_VALUE_IGNORED; // Determined below
        attribute.offset                = PPX_APPEND_OFFSET_ALIGNED;
        attribute.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
        attribute.semantic              = semantic;

        if (vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
            attribute.binding = 0;
            vertexBindings[0].AppendAttribute(attribute);
        }
        else if (vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
            PPX_ASSERT_MSG(vertexBindingCount < PPX_MAX_VERTEX_BINDINGS, "max vertex bindings exceeded");

            vertexBindings[vertexBindingCount].AppendAttribute(attribute);
            vertexBindings[vertexBindingCount].SetBinding(vertexBindingCount);
            vertexBindingCount += 1;
        }
    }
    return *this;
}

GeometryOptions& GeometryOptions::AddPosition(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_POSITION, format);
    return *this;
}

GeometryOptions& GeometryOptions::AddNormal(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_NORMAL, format);
    return *this;
}

GeometryOptions& GeometryOptions::AddColor(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_COLOR, format);
    return *this;
}

GeometryOptions& GeometryOptions::AddTexCoord(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_TEXCOORD, format);
    return *this;
}

GeometryOptions& GeometryOptions::AddTangent(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_TANGENT, format);
    return *this;
}

GeometryOptions& GeometryOptions::AddBitangent(grfx::Format format)
{
    AddAttribute(grfx::VERTEX_SEMANTIC_BITANGENT, format);
    return *this;
}

// -------------------------------------------------------------------------------------------------
// Geometry::Buffer
// -------------------------------------------------------------------------------------------------
uint32_t Geometry::Buffer::GetElementCount() const
{
    size_t   sizeOfData = mData.size();
    uint32_t count      = static_cast<uint32_t>(sizeOfData / mElementSize);
    return count;
}

// -------------------------------------------------------------------------------------------------
// Geometry
// -------------------------------------------------------------------------------------------------
Result Geometry::InternalCtor()
{
    if (mCreateInfo.indexType != grfx::INDEX_TYPE_UNDEFINED) {
        uint32_t elementSize = grfx::IndexTypeSize(mCreateInfo.indexType);

        if (elementSize == 0) {
            // Shouldn't occur unless there's corruption
            PPX_ASSERT_MSG(false, "could not determine index type size");
            return ppx::ERROR_FAILED;
        }

        mIndexBuffer = Buffer(BUFFER_TYPE_INDEX, elementSize);
    }

    if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        mCreateInfo.vertexBindingCount = 1;

        const grfx::VertexBinding& binding     = mCreateInfo.vertexBindings[0];
        uint32_t                   elementSize = binding.GetStride();
        mVertexBuffers.push_back(Buffer(BUFFER_TYPE_VERTEX, elementSize));
    }
    else if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        // Create buffers
        for (uint32_t i = 0; i < mCreateInfo.vertexBindingCount; ++i) {
            const grfx::VertexBinding& binding     = mCreateInfo.vertexBindings[i];
            uint32_t                   elementSize = binding.GetStride();
            mVertexBuffers.push_back(Buffer(BUFFER_TYPE_VERTEX, elementSize));
        }

        // Cache buffer indices if possible
        for (uint32_t i = 0; i < mCreateInfo.vertexBindingCount; ++i) {
            const grfx::VertexBinding&   binding    = mCreateInfo.vertexBindings[i];
            const grfx::VertexAttribute* pAttribute = nullptr;
            Result                       ppxres     = binding.GetAttribute(0, &pAttribute);
            if (Failed(ppxres)) {
                // Shouldn't occur unless there's corruption
                PPX_ASSERT_MSG(false, "could not get attribute at index 0");
                return ppx::ERROR_FAILED;
            }

            // clang-format off
            switch (pAttribute->semantic) {
                default: {
                    return Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC;
                }

                case grfx::VERTEX_SEMANTIC_POSITION  : mPositionBufferIndex  = i; break;
                case grfx::VERTEX_SEMANTIC_NORMAL    : mNormaBufferIndex     = i; break;
                case grfx::VERTEX_SEMANTIC_COLOR     : mColorBufferIndex     = i; break;
                case grfx::VERTEX_SEMANTIC_TANGENT   : mTangentBufferIndex   = i; break;
                case grfx::VERTEX_SEMANTIC_BITANGENT : mBitangentBufferIndex = i; break;
                case grfx::VERTEX_SEMANTIC_TEXCOORD  : mTexCoordBufferIndex  = i; break;
            }
            // clang-format on
        }
    }

    return ppx::SUCCESS;
}

Result Geometry::Create(const GeometryOptions& createInfo, Geometry* pGeometry)
{
    PPX_ASSERT_NULL_ARG(pGeometry);

    *pGeometry = Geometry();

    if (createInfo.primtiveTopology != grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) {
        PPX_ASSERT_MSG(false, "only triangle list is supported");
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    if (createInfo.indexType != grfx::INDEX_TYPE_UNDEFINED) {
        uint32_t elementSize = 0;
        if (createInfo.indexType == grfx::INDEX_TYPE_UINT16) {
            elementSize = sizeof(uint16_t);
        }
        else if (createInfo.indexType == grfx::INDEX_TYPE_UINT32) {
            elementSize = sizeof(uint32_t);
        }
        else {
            PPX_ASSERT_MSG(false, "invalid index type");
            return ppx::ERROR_INVALID_CREATE_ARGUMENT;
        }
    }

    if (createInfo.vertexBindingCount == 0) {
        PPX_ASSERT_MSG(false, "must have at least one vertex binding");
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    if (createInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        for (uint32_t i = 0; i < createInfo.vertexBindingCount; ++i) {
            const grfx::VertexBinding& binding = createInfo.vertexBindings[i];
            if (binding.GetAttributeCount() != 1) {
                PPX_ASSERT_MSG(false, "planar layout binding must have 1 attribute");
                return ppx::ERROR_INVALID_CREATE_ARGUMENT;
            }
        }
    }

    pGeometry->mCreateInfo = createInfo;

    Result ppxres = pGeometry->InternalCtor();
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Geometry::Create(
    const GeometryOptions& createInfo,
    const TriMesh&         mesh,
    Geometry*              pGeometry)
{
    // Create geometry
    Result ppxres = Geometry::Create(createInfo, pGeometry);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "failed creating geometry");
        return ppxres;
    }

    //
    // Target geometry WITHOUT index data
    //
    if (createInfo.indexType == grfx::INDEX_TYPE_UNDEFINED) {
        // Mesh has index data
        if (mesh.GetIndexType() != grfx::INDEX_TYPE_UNDEFINED) {
            // Iterate through the meshes triangles and add vertex data for each triangle vertex
            uint32_t triCount = mesh.GetCountTriangles();
            for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex) {
                uint32_t vtxIndex0 = PPX_VALUE_IGNORED;
                uint32_t vtxIndex1 = PPX_VALUE_IGNORED;
                uint32_t vtxIndex2 = PPX_VALUE_IGNORED;
                ppxres             = mesh.GetTriangle(triIndex, vtxIndex0, vtxIndex1, vtxIndex2);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting triangle indices at triIndex=" << triIndex);
                    return ppxres;
                }

                // First vertex
                TriMeshVertexData vertexData0 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex0, &vertexData0);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex0=" << vtxIndex0);
                    return ppxres;
                }
                // Second vertex
                TriMeshVertexData vertexData1 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex1, &vertexData1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex1=" << vtxIndex1);
                    return ppxres;
                }
                // Third vertex
                TriMeshVertexData vertexData2 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex2, &vertexData2);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex2=" << vtxIndex2);
                    return ppxres;
                }

                pGeometry->AppendVertexData(vertexData0);
                pGeometry->AppendVertexData(vertexData1);
                pGeometry->AppendVertexData(vertexData2);
            }
        }
        // Mesh does not have index data
        else {
            // Iterate through the meshes vertx data and add it to the geometry
            uint32_t vertexCount = mesh.GetCountPositions();
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                TriMeshVertexData vertexData = {};
                ppxres                       = mesh.GetVertexData(vertexIndex, &vertexData);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vertexIndex=" << vertexIndex);
                    return ppxres;
                }
                pGeometry->AppendVertexData(vertexData);
            }
        }
    }
    //
    // Target geometry WITH index data
    //
    else {
        // Mesh has index data
        if (mesh.GetIndexType() != grfx::INDEX_TYPE_UNDEFINED) {
            // Iterate the meshes triangles and add the vertex indices
            uint32_t triCount = mesh.GetCountTriangles();
            for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex) {
                uint32_t v0     = PPX_VALUE_IGNORED;
                uint32_t v1     = PPX_VALUE_IGNORED;
                uint32_t v2     = PPX_VALUE_IGNORED;
                Result   ppxres = mesh.GetTriangle(triIndex, v0, v1, v2);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "couldn't get triangle at triIndex=" << triIndex);
                    return ppxres;
                }
                pGeometry->AppendIndicesTriangle(v0, v1, v2);
            }

            // Iterate through the meshes vertx data and add it to the geometry
            uint32_t vertexCount = mesh.GetCountPositions();
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                TriMeshVertexData vertexData = {};
                ppxres                       = mesh.GetVertexData(vertexIndex, &vertexData);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vertexIndex=" << vertexIndex);
                    return ppxres;
                }
                pGeometry->AppendVertexData(vertexData);
            }
        }
        // Mesh does not have index data
        else {
            // Use every 3 vertices as a triangle and add each as an indexed triangle
            uint32_t triCount = mesh.GetCountPositions() / 3;
            for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex) {
                uint32_t vtxIndex0 = 3 * triIndex + 0;
                uint32_t vtxIndex1 = 3 * triIndex + 1;
                uint32_t vtxIndex2 = 3 * triIndex + 2;

                // First vertex
                TriMeshVertexData vertexData0 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex0, &vertexData0);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex0=" << vtxIndex0);
                    return ppxres;
                }
                // Second vertex
                TriMeshVertexData vertexData1 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex1, &vertexData1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex1=" << vtxIndex1);
                    return ppxres;
                }
                // Third vertex
                TriMeshVertexData vertexData2 = {};
                ppxres                        = mesh.GetVertexData(vtxIndex2, &vertexData2);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex2=" << vtxIndex2);
                    return ppxres;
                }

                // Will append indices if geometry has index buffer
                pGeometry->AppendTriangle(vertexData0, vertexData1, vertexData2);
            }
        }
    }

    return ppx::SUCCESS;
}

Result Geometry::Create(
    const GeometryOptions& createInfo,
    const WireMesh&        mesh,
    Geometry*              pGeometry)
{
    // Create geometry
    Result ppxres = Geometry::Create(createInfo, pGeometry);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "failed creating geometry");
        return ppxres;
    }

    //
    // Target geometry WITHOUT index data
    //
    if (createInfo.indexType == grfx::INDEX_TYPE_UNDEFINED) {
        // Mesh has index data
        if (mesh.GetIndexType() != grfx::INDEX_TYPE_UNDEFINED) {
            // Iterate through the meshes edges and add vertex data for each edge vertex
            uint32_t edgeCount = mesh.GetCountEdges();
            for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                uint32_t vtxIndex0 = PPX_VALUE_IGNORED;
                uint32_t vtxIndex1 = PPX_VALUE_IGNORED;
                ppxres             = mesh.GetEdge(edgeIndex, vtxIndex0, vtxIndex1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting triangle indices at edgeIndex=" << edgeIndex);
                    return ppxres;
                }

                // First vertex
                WireMeshVertexData vertexData0 = {};
                ppxres                         = mesh.GetVertexData(vtxIndex0, &vertexData0);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex0=" << vtxIndex0);
                    return ppxres;
                }
                // Second vertex
                WireMeshVertexData vertexData1 = {};
                ppxres                         = mesh.GetVertexData(vtxIndex1, &vertexData1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex1=" << vtxIndex1);
                    return ppxres;
                }

                pGeometry->AppendVertexData(vertexData0);
                pGeometry->AppendVertexData(vertexData1);
            }
        }
        // Mesh does not have index data
        else {
            // Iterate through the meshes vertx data and add it to the geometry
            uint32_t vertexCount = mesh.GetCountPositions();
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                WireMeshVertexData vertexData = {};
                ppxres                        = mesh.GetVertexData(vertexIndex, &vertexData);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vertexIndex=" << vertexIndex);
                    return ppxres;
                }
                pGeometry->AppendVertexData(vertexData);
            }
        }
    }
    //
    // Target geometry WITH index data
    //
    else {
        // Mesh has index data
        if (mesh.GetIndexType() != grfx::INDEX_TYPE_UNDEFINED) {
            // Iterate the meshes edges and add the vertex indices
            uint32_t edgeCount = mesh.GetCountEdges();
            for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                uint32_t v0     = PPX_VALUE_IGNORED;
                uint32_t v1     = PPX_VALUE_IGNORED;
                Result   ppxres = mesh.GetEdge(edgeIndex, v0, v1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "couldn't get triangle at edgeIndex=" << edgeIndex);
                    return ppxres;
                }
                pGeometry->AppendIndicesEdge(v0, v1);
            }

            // Iterate through the meshes vertex data and add it to the geometry
            uint32_t vertexCount = mesh.GetCountPositions();
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                WireMeshVertexData vertexData = {};
                ppxres                        = mesh.GetVertexData(vertexIndex, &vertexData);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vertexIndex=" << vertexIndex);
                    return ppxres;
                }
                pGeometry->AppendVertexData(vertexData);
            }
        }
        // Mesh does not have index data
        else {
            // Use every 2 vertices as an edge and add each as an indexed edge
            uint32_t edgeCount = mesh.GetCountPositions() / 2;
            for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                uint32_t vtxIndex0 = 2 * edgeIndex + 0;
                uint32_t vtxIndex1 = 2 * edgeIndex + 1;

                // First vertex
                WireMeshVertexData vertexData0 = {};
                ppxres                         = mesh.GetVertexData(vtxIndex0, &vertexData0);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex0=" << vtxIndex0);
                    return ppxres;
                }
                // Second vertex
                WireMeshVertexData vertexData1 = {};
                ppxres                         = mesh.GetVertexData(vtxIndex1, &vertexData1);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "failed getting vertex data at vtxIndex1=" << vtxIndex1);
                    return ppxres;
                }

                // Will append indices if geometry has index buffer
                pGeometry->AppendEdge(vertexData0, vertexData1);
            }
        }
    }

    return ppx::SUCCESS;
}

Result Geometry::Create(const TriMesh& mesh, Geometry* pGeometry)
{
    GeometryOptions createInfo       = {};
    createInfo.vertexAttributeLayout = ppx::GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
    createInfo.indexType             = mesh.GetIndexType();
    createInfo.primtiveTopology      = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    createInfo.AddPosition();

    if (mesh.HasColors()) {
        createInfo.AddColor();
    }
    if (mesh.HasNormals()) {
        createInfo.AddNormal();
    }
    if (mesh.HasTexCoords()) {
        createInfo.AddTexCoord();
    }
    if (mesh.HasTangents()) {
        createInfo.AddTangent();
    }
    if (mesh.HasBitangents()) {
        createInfo.AddBitangent();
    }

    Result ppxres = Create(createInfo, mesh, pGeometry);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Geometry::Create(const WireMesh& mesh, Geometry* pGeometry)
{
    GeometryOptions createInfo       = {};
    createInfo.vertexAttributeLayout = ppx::GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
    createInfo.indexType             = mesh.GetIndexType();
    createInfo.primtiveTopology      = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    createInfo.AddPosition();

    if (mesh.HasColors()) {
        createInfo.AddColor();
    }

    Result ppxres = Create(createInfo, mesh, pGeometry);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

const grfx::VertexBinding* Geometry::GetVertexBinding(uint32_t index) const
{
    const grfx::VertexBinding* pBinding = nullptr;
    if (index < mCreateInfo.vertexBindingCount) {
        pBinding = &mCreateInfo.vertexBindings[index];
    }
    return pBinding;
}

uint32_t Geometry::GetIndexCount() const
{
    uint32_t count = 0;
    if (mCreateInfo.indexType != grfx::INDEX_TYPE_UNDEFINED) {
        count = mIndexBuffer.GetElementCount();
    }
    return count;
}

uint32_t Geometry::GetVertexCount() const
{
    uint32_t count = 0;
    if (mCreateInfo.vertexAttributeLayout == ppx::GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        count = mVertexBuffers[0].GetElementCount();
    }
    else if (mCreateInfo.vertexAttributeLayout == ppx::GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        count = mVertexBuffers[mPositionBufferIndex].GetElementCount();
    }
    return count;
}

const Geometry::Buffer* Geometry::GetVertexBuffer(uint32_t index) const
{
    const Geometry::Buffer* pBuffer = nullptr;
    if (IsIndexInRange(index, mVertexBuffers)) {
        pBuffer = &mVertexBuffers[index];
    }
    return pBuffer;
}

uint32_t Geometry::GetLargestBufferSize() const
{
    uint32_t size = mIndexBuffer.GetSize();
    for (size_t i = 0; i < mVertexBuffers.size(); ++i) {
        size = std::max(size, mVertexBuffers[i].GetSize());
    }
    return size;
}

void Geometry::AppendIndicesTriangle(uint32_t vtx0, uint32_t vtx1, uint32_t vtx2)
{
    if (mCreateInfo.indexType == grfx::INDEX_TYPE_UINT16) {
        mIndexBuffer.Append(static_cast<uint16_t>(vtx0));
        mIndexBuffer.Append(static_cast<uint16_t>(vtx1));
        mIndexBuffer.Append(static_cast<uint16_t>(vtx2));
    }
    else if (mCreateInfo.indexType == grfx::INDEX_TYPE_UINT32) {
        mIndexBuffer.Append(vtx0);
        mIndexBuffer.Append(vtx1);
        mIndexBuffer.Append(vtx2);
    }
}

void Geometry::AppendIndicesEdge(uint32_t vtx0, uint32_t vtx1)
{
    if (mCreateInfo.indexType == grfx::INDEX_TYPE_UINT16) {
        mIndexBuffer.Append(static_cast<uint16_t>(vtx0));
        mIndexBuffer.Append(static_cast<uint16_t>(vtx1));
    }
    else if (mCreateInfo.indexType == grfx::INDEX_TYPE_UINT32) {
        mIndexBuffer.Append(vtx0);
        mIndexBuffer.Append(vtx1);
    }
}

uint32_t Geometry::AppendVertexInterleaved(const TriMeshVertexData& vtx)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        PPX_ASSERT_MSG(false, NOT_INTERLEAVED_MSG);
        return PPX_VALUE_IGNORED;
    }

    uint32_t       startSize = mVertexBuffers[0].GetSize();
    const uint32_t attrCount = mCreateInfo.vertexBindings[0].GetAttributeCount();
    for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        const grfx::VertexAttribute* pAttribute = nullptr;
        Result                       ppxres     = mCreateInfo.vertexBindings[0].GetAttribute(attrIndex, &pAttribute);
        PPX_ASSERT_MSG((ppxres == ppx::SUCCESS), "attribute not found at index=" << attrIndex);

        // clang-format off
        switch (pAttribute->semantic) {
            default: break;
            case grfx::VERTEX_SEMANTIC_POSITION  : mVertexBuffers[0].Append(vtx.position); break;
            case grfx::VERTEX_SEMANTIC_NORMAL    : mVertexBuffers[0].Append(vtx.normal); break;
            case grfx::VERTEX_SEMANTIC_COLOR     : mVertexBuffers[0].Append(vtx.color); break;
            case grfx::VERTEX_SEMANTIC_TANGENT   : mVertexBuffers[0].Append(vtx.tangent); break;
            case grfx::VERTEX_SEMANTIC_BITANGENT : mVertexBuffers[0].Append(vtx.bitangent); break;
            case grfx::VERTEX_SEMANTIC_TEXCOORD  : mVertexBuffers[0].Append(vtx.texCoord); break;
        }
        // clang-format on
    }
    uint32_t endSize = mVertexBuffers[0].GetSize();

    uint32_t bytesWritten = (endSize - startSize);
    PPX_ASSERT_MSG((bytesWritten == mVertexBuffers[0].GetElementSize()), "size of vertex data written does not match buffer's element size");

    uint32_t n = mVertexBuffers[0].GetElementCount();
    return n;
}

uint32_t Geometry::AppendVertexInterleaved(const WireMeshVertexData& vtx)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        PPX_ASSERT_MSG(false, NOT_INTERLEAVED_MSG);
        return PPX_VALUE_IGNORED;
    }

    uint32_t       startSize = mVertexBuffers[0].GetSize();
    const uint32_t attrCount = mCreateInfo.vertexBindings[0].GetAttributeCount();
    for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        const grfx::VertexAttribute* pAttribute = nullptr;
        Result                       ppxres     = mCreateInfo.vertexBindings[0].GetAttribute(attrIndex, &pAttribute);
        PPX_ASSERT_MSG((ppxres == ppx::SUCCESS), "attribute not found at index=" << attrIndex);

        // clang-format off
        switch (pAttribute->semantic) {
            default: break;
            case grfx::VERTEX_SEMANTIC_POSITION  : mVertexBuffers[0].Append(vtx.position); break;
            case grfx::VERTEX_SEMANTIC_COLOR     : mVertexBuffers[0].Append(vtx.color); break;
        }
        // clang-format on
    }
    uint32_t endSize = mVertexBuffers[0].GetSize();

    uint32_t bytesWritten = (endSize - startSize);
    PPX_ASSERT_MSG((bytesWritten == mVertexBuffers[0].GetElementSize()), "size of vertex data written does not match buffer's element size");

    uint32_t n = mVertexBuffers[0].GetElementCount();
    return n;
}

uint32_t Geometry::AppendVertexData(const TriMeshVertexData& vtx)
{
    uint32_t n = 0;
    if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        n = AppendVertexInterleaved(vtx);
    }
    else if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        n = AppendPosition(vtx.position);
        AppendNormal(vtx.normal);
        AppendColor(vtx.color);
        AppendTexCoord(vtx.texCoord);
        AppendTangent(vtx.tangent);
        AppendBitangent(vtx.bitangent);
    }
    else {
        // Something went horribly wrong
        PPX_ASSERT_MSG(false, "unknown attribute layout");
    }
    return n;
}

uint32_t Geometry::AppendVertexData(const WireMeshVertexData& vtx)
{
    uint32_t n = 0;
    if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED) {
        n = AppendVertexInterleaved(vtx);
    }
    else if (mCreateInfo.vertexAttributeLayout == GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        n = AppendPosition(vtx.position);
        AppendColor(vtx.color);
    }
    else {
        // Something went horribly wrong
        PPX_ASSERT_MSG(false, "unknown attribute layout");
    }
    return n;
}

void Geometry::AppendTriangle(const TriMeshVertexData& vtx0, const TriMeshVertexData& vtx1, const TriMeshVertexData& vtx2)
{
    uint32_t n0 = AppendVertexData(vtx0) - 1;
    uint32_t n1 = AppendVertexData(vtx1) - 1;
    uint32_t n2 = AppendVertexData(vtx2) - 1;

    // Will only append indices if geometry has an index buffer
    AppendIndicesTriangle(n0, n1, n2);
}

void Geometry::AppendEdge(const WireMeshVertexData& vtx0, const WireMeshVertexData& vtx1)
{
    uint32_t n0 = AppendVertexData(vtx0) - 1;
    uint32_t n1 = AppendVertexData(vtx1) - 1;

    // Will only append indices if geometry has an index buffer
    AppendIndicesEdge(n0, n1);
}

uint32_t Geometry::AppendPosition(const float3& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return PPX_VALUE_IGNORED;
    }

    if (mPositionBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mPositionBufferIndex].Append(value);

        uint32_t n = mVertexBuffers[mPositionBufferIndex].GetElementCount();
        return n;
    }

    return PPX_VALUE_IGNORED;
}

void Geometry::AppendNormal(const float3& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return;
    }

    if (mNormaBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mNormaBufferIndex].Append(value);
    }
}

void Geometry::AppendColor(const float3& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return;
    }

    if (mColorBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mColorBufferIndex].Append(value);
    }
}

void Geometry::AppendTexCoord(const float2& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return;
    }

    if (mTexCoordBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mTexCoordBufferIndex].Append(value);
    }
}

void Geometry::AppendTangent(const float4& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return;
    }

    if (mTangentBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mTangentBufferIndex].Append(value);
    }
}

void Geometry::AppendBitangent(const float3& value)
{
    if (mCreateInfo.vertexAttributeLayout != GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR) {
        PPX_ASSERT_MSG(false, NOT_PLANAR_MSG);
        return;
    }

    if (mBitangentBufferIndex != PPX_VALUE_IGNORED) {
        mVertexBuffers[mBitangentBufferIndex].Append(value);
    }
}

} // namespace ppx
