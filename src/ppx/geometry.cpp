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
#include <cmath>

#define NOT_INTERLEAVED_MSG "cannot append interleaved data if attribute layout is not interleaved"
#define NOT_PLANAR_MSG      "cannot append planar data if attribute layout is not planar"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// VertexDataProcessorBase
//     interface for all VertexDataProcessors
//     with helper functions to allow derived classes to access Geometry
// Note that the base and derived VertexDataProcessor classes do not have any data member
//     be careful when adding data members to any of these classes
//     it could create problems for multithreaded cases for multiple geometry objects
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorBase
{
public:
    // Validates the layout
    // returns false if the validation fails
    virtual bool Validate(Geometry* pGeom) = 0;
    // Updates the vertex buffer and the vertex buffer index
    // returns Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC if the sematic is invalid
    virtual Result UpdateVertexBuffer(Geometry* pGeom) = 0;
    // Fetches vertex data from vtx and append it to the geometry
    // Returns 0 if the appending fails
    virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx)                  = 0;
    virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) = 0;
    // Gets the vertex count of the geometry
    virtual uint32_t GetVertexCount(const Geometry* pGeom) = 0;

protected:
    // Prevent from being deleted explicitly
    virtual ~VertexDataProcessorBase() {}

    // ----------------------------------------
    // Helper functions to access Geometry data
    // ----------------------------------------

    // Missing attributes will also result in NOOP.
    // returns the element count of the vertex buffer with the bufferIndex
    // 0 is returned if the bufferIndex is ignored
    template <typename T>
    uint32_t AppendDataToVertexBuffer(Geometry* pGeom, uint32_t bufferIndex, const T& data)
    {
        if (bufferIndex != PPX_VALUE_IGNORED) {
            PPX_ASSERT_MSG((bufferIndex >= 0) && (bufferIndex < pGeom->mVertexBuffers.size()), "buffer index is not valid");
            pGeom->mVertexBuffers[bufferIndex].Append(data);
            return pGeom->mVertexBuffers[bufferIndex].GetElementCount();
        }
        return 0;
    }

    void AddVertexBuffer(Geometry* pGeom, uint32_t bindingIndex)
    {
        pGeom->mVertexBuffers.push_back(Geometry::Buffer(Geometry::BUFFER_TYPE_VERTEX, GetVertexBindingStride(pGeom, bindingIndex)));
    }

    uint32_t GetVertexBufferSize(const Geometry* pGeom, uint32_t bufferIndex) const
    {
        return pGeom->mVertexBuffers[bufferIndex].GetSize();
    }

    uint32_t GetVertexBufferElementCount(const Geometry* pGeom, uint32_t bufferIndex) const
    {
        return pGeom->mVertexBuffers[bufferIndex].GetElementCount();
    }

    uint32_t GetVertexBufferElementSize(const Geometry* pGeom, uint32_t bufferIndex) const
    {
        return pGeom->mVertexBuffers[bufferIndex].GetElementSize();
    }

    uint32_t GetVertexBindingAttributeCount(const Geometry* pGeom, uint32_t bindingIndex) const
    {
        return pGeom->mCreateInfo.vertexBindings[bindingIndex].GetAttributeCount();
    }

    uint32_t GetVertexBindingStride(const Geometry* pGeom, uint32_t bindingIndex) const
    {
        return pGeom->mCreateInfo.vertexBindings[bindingIndex].GetStride();
    }

    uint32_t GetVertexBindingCount(const Geometry* pGeom) const
    {
        return pGeom->mCreateInfo.vertexBindingCount;
    }

    grfx::VertexSemantic GetVertexBindingAttributeSematic(const Geometry* pGeom, uint32_t bindingIndex, uint32_t attrIndex) const
    {
        const grfx::VertexAttribute* pAttribute = nullptr;
        Result                       ppxres     = pGeom->mCreateInfo.vertexBindings[bindingIndex].GetAttribute(attrIndex, &pAttribute);
        PPX_ASSERT_MSG(ppxres == ppx::SUCCESS, "attribute not found at index=" << attrIndex);
        return pAttribute->semantic;
    }

    uint32_t GetPositionBufferIndex(const Geometry* pGeom) const { return pGeom->mPositionBufferIndex; }
    uint32_t GetNormalBufferIndex(const Geometry* pGeom) const { return pGeom->mNormaBufferIndex; }
    uint32_t GetColorBufferIndex(const Geometry* pGeom) const { return pGeom->mColorBufferIndex; }
    uint32_t GetTexCoordBufferIndex(const Geometry* pGeom) const { return pGeom->mTexCoordBufferIndex; }
    uint32_t GetTangentBufferIndex(const Geometry* pGeom) const { return pGeom->mTangentBufferIndex; }
    uint32_t GetBitangentBufferIndex(const Geometry* pGeom) const { return pGeom->mBitangentBufferIndex; }

    void SetPositionBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mPositionBufferIndex = index; }
    void SetNormalBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mNormaBufferIndex = index; }
    void SetColorBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mColorBufferIndex = index; }
    void SetTexCoordBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mTexCoordBufferIndex = index; }
    void SetTangentBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mTangentBufferIndex = index; }
    void SetBitangentBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mBitangentBufferIndex = index; }
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Planar vertex attribute layout
//     Planar: each attribute has its own vertex input binding
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorPlanar : public VertexDataProcessorBase<T>
{
public:
    virtual bool Validate(Geometry* pGeom) override
    {
        const uint32_t vertexBindingCount = GetVertexBindingCount(pGeom);
        for (uint32_t i = 0; i < vertexBindingCount; ++i) {
            if (GetVertexBindingAttributeCount(pGeom, i) != 1) {
                PPX_ASSERT_MSG(false, "planar layout must have 1 attribute");
                return false;
            }
        }
        return true;
    }

    virtual Result UpdateVertexBuffer(Geometry* pGeom) override
    {
        // Create buffers
        const uint32_t vertexBindingCount = GetVertexBindingCount(pGeom);
        for (uint32_t i = 0; i < vertexBindingCount; ++i) {
            AddVertexBuffer(pGeom, i);
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, i, 0);
            // clang-format off
            switch (semantic) {
                default                              : return Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC;
                case grfx::VERTEX_SEMANTIC_POSITION  : SetPositionBufferIndex(pGeom, i); break;
                case grfx::VERTEX_SEMANTIC_NORMAL    : SetNormalBufferIndex(pGeom, i); break;
                case grfx::VERTEX_SEMANTIC_COLOR     : SetColorBufferIndex(pGeom, i); break;
                case grfx::VERTEX_SEMANTIC_TANGENT   : SetTangentBufferIndex(pGeom, i); break;
                case grfx::VERTEX_SEMANTIC_BITANGENT : SetBitangentBufferIndex(pGeom, i); break;
                case grfx::VERTEX_SEMANTIC_TEXCOORD  : SetTexCoordBufferIndex(pGeom, i); break;
            }
            // clang-format on
        }
        return ppx::SUCCESS;
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
    {
        const uint32_t n = AppendDataToVertexBuffer(pGeom, GetPositionBufferIndex(pGeom), vtx.position);
        PPX_ASSERT_MSG(n > 0, "position should always available");
        AppendDataToVertexBuffer(pGeom, GetNormalBufferIndex(pGeom), vtx.normal);
        AppendDataToVertexBuffer(pGeom, GetColorBufferIndex(pGeom), vtx.color);
        AppendDataToVertexBuffer(pGeom, GetTexCoordBufferIndex(pGeom), vtx.texCoord);
        AppendDataToVertexBuffer(pGeom, GetTangentBufferIndex(pGeom), vtx.tangent);
        AppendDataToVertexBuffer(pGeom, GetBitangentBufferIndex(pGeom), vtx.bitangent);
        return n;
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
    {
        const uint32_t n = AppendDataToVertexBuffer(pGeom, GetPositionBufferIndex(pGeom), vtx.position);
        PPX_ASSERT_MSG(n > 0, "position should always available");
        AppendDataToVertexBuffer(pGeom, GetColorBufferIndex(pGeom), vtx.color);
        return n;
    }

    virtual uint32_t GetVertexCount(const Geometry* pGeom) override
    {
        return GetVertexBufferElementCount(pGeom, GetPositionBufferIndex(pGeom));
    }
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Interleaved vertex attribute layout
//     Interleaved: only has 1 vertex input binding, data is interleaved
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorInterleaved : public VertexDataProcessorBase<T>
{
public:
    virtual bool Validate(Geometry* pGeom) override
    {
        const uint32_t vertexBindingCount = GetVertexBindingCount(pGeom);
        if (vertexBindingCount != 1) {
            PPX_ASSERT_MSG(false, "interleaved layout must have 1 binding");
        }
        return true;
    }

    virtual Result UpdateVertexBuffer(Geometry* pGeom) override
    {
        PPX_ASSERT_MSG(1 == GetVertexBindingCount(pGeom), "there should be only 1 binding for planar");
        AddVertexBuffer(pGeom, kBufferIndex);
        return ppx::SUCCESS;
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
    {
        uint32_t       startSize = GetVertexBufferSize(pGeom, kBufferIndex);
        const uint32_t attrCount = GetVertexBindingAttributeCount(pGeom, kBufferIndex);
        for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, kBufferIndex, attrIndex);

            // clang-format off
            switch (semantic) {
                default: break;
                case grfx::VERTEX_SEMANTIC_POSITION  :
                    {
                        const uint32_t n = AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.position);
                        PPX_ASSERT_MSG(n > 0, "position should always available");
                    }
                    break;
                case grfx::VERTEX_SEMANTIC_NORMAL    : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.normal); break;
                case grfx::VERTEX_SEMANTIC_COLOR     : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.color); break;
                case grfx::VERTEX_SEMANTIC_TANGENT   : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.tangent); break;
                case grfx::VERTEX_SEMANTIC_BITANGENT : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.bitangent); break;
                case grfx::VERTEX_SEMANTIC_TEXCOORD  : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.texCoord); break;
            }
            // clang-format on
        }
        uint32_t endSize = GetVertexBufferSize(pGeom, kBufferIndex);

        uint32_t       bytesWritten            = (endSize - startSize);
        const uint32_t vertexBufferElementSize = GetVertexBufferElementSize(pGeom, kBufferIndex);
        PPX_ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");

        return GetVertexBufferElementCount(pGeom, kBufferIndex);
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
    {
        uint32_t       startSize = GetVertexBufferSize(pGeom, kBufferIndex);
        const uint32_t attrCount = GetVertexBindingAttributeCount(pGeom, kBufferIndex);
        for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, kBufferIndex, attrIndex);

            // clang-format off
            switch (semantic) {
                default: break;
                case grfx::VERTEX_SEMANTIC_POSITION: 
                    {
                        const uint32_t n = AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.position); 
                        PPX_ASSERT_MSG(n > 0, "position should always available");
                    }
                    break;
                case grfx::VERTEX_SEMANTIC_COLOR   : AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.color); break;
            }
            // clang-format on
        }
        uint32_t endSize = GetVertexBufferSize(pGeom, kBufferIndex);

        uint32_t       bytesWritten            = (endSize - startSize);
        const uint32_t vertexBufferElementSize = GetVertexBufferElementSize(pGeom, kBufferIndex);
        PPX_ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");

        return GetVertexBufferElementCount(pGeom, kBufferIndex);
    }

    virtual uint32_t GetVertexCount(const Geometry* pGeom) override
    {
        return GetVertexBufferElementCount(pGeom, kBufferIndex);
    }

private:
    // for VertexDataProcessorInterleaved, there is only 1 binding, so the index is always 0
    const uint32_t kBufferIndex = 0;
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Position Planar vertex attribute layout
//     Position Planar: only has 2 vertex input bindings
//        - Binding 0 only has Position data
//        - Binding 1 contains all non-position data, interleaved
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorPositionPlanar : public VertexDataProcessorBase<T>
{
public:
    virtual bool Validate(Geometry* pGeom) override
    {
        const uint32_t vertexBindingCount = GetVertexBindingCount(pGeom);
        if (vertexBindingCount != 2) {
            PPX_ASSERT_MSG(false, "position planar layout must have 2 bindings");
        }
        return true;
    }

    virtual Result UpdateVertexBuffer(Geometry* pGeom) override
    {
        PPX_ASSERT_MSG(2 == GetVertexBindingCount(pGeom), "there should be 2 binding for position planar");
        // Position
        AddVertexBuffer(pGeom, kPositionBufferIndex);
        // Non-Position data
        AddVertexBuffer(pGeom, kNonPositionBufferIndex);

        SetPositionBufferIndex(pGeom, kPositionBufferIndex);

        const uint32_t attributeCount = GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
        for (uint32_t i = 0; i < attributeCount; ++i) {
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, i);
            // clang-format off
            switch (semantic) {
                default                              : return Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC;
                case grfx::VERTEX_SEMANTIC_POSITION  : PPX_ASSERT_MSG(false, "position should be in binding 0"); break;
                case grfx::VERTEX_SEMANTIC_NORMAL    : SetNormalBufferIndex(pGeom, kNonPositionBufferIndex); break;
                case grfx::VERTEX_SEMANTIC_COLOR     : SetColorBufferIndex(pGeom, kNonPositionBufferIndex); break;
                case grfx::VERTEX_SEMANTIC_TANGENT   : SetTangentBufferIndex(pGeom, kNonPositionBufferIndex); break;
                case grfx::VERTEX_SEMANTIC_BITANGENT : SetBitangentBufferIndex(pGeom, kNonPositionBufferIndex); break;
                case grfx::VERTEX_SEMANTIC_TEXCOORD  : SetTexCoordBufferIndex(pGeom, kNonPositionBufferIndex); break;
            }
            // clang-format on
        }

        return ppx::SUCCESS;
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
    {
        const uint32_t n = AppendDataToVertexBuffer(pGeom, GetPositionBufferIndex(pGeom), vtx.position);
        PPX_ASSERT_MSG(n > 0, "position should always available");

        uint32_t       startSize = GetVertexBufferSize(pGeom, kNonPositionBufferIndex);
        const uint32_t attrCount = GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
        for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, attrIndex);

            // clang-format off
            switch (semantic) {
                default                              : PPX_ASSERT_MSG(false, "should not have other sematic"); break;
                case grfx::VERTEX_SEMANTIC_POSITION  : PPX_ASSERT_MSG(false, "position should be in binding 0"); break;
                case grfx::VERTEX_SEMANTIC_NORMAL    : AppendDataToVertexBuffer(pGeom, GetNormalBufferIndex(pGeom), vtx.normal); break;
                case grfx::VERTEX_SEMANTIC_COLOR     : AppendDataToVertexBuffer(pGeom, GetColorBufferIndex(pGeom), vtx.color); break;
                case grfx::VERTEX_SEMANTIC_TANGENT   : AppendDataToVertexBuffer(pGeom, GetTangentBufferIndex(pGeom), vtx.tangent); break;
                case grfx::VERTEX_SEMANTIC_BITANGENT : AppendDataToVertexBuffer(pGeom, GetBitangentBufferIndex(pGeom), vtx.bitangent); break;
                case grfx::VERTEX_SEMANTIC_TEXCOORD  : AppendDataToVertexBuffer(pGeom, GetTexCoordBufferIndex(pGeom), vtx.texCoord); break;
            }
            // clang-format on
        }
        uint32_t endSize = GetVertexBufferSize(pGeom, kNonPositionBufferIndex);

        uint32_t       bytesWritten            = (endSize - startSize);
        const uint32_t vertexBufferElementSize = GetVertexBufferElementSize(pGeom, kNonPositionBufferIndex);
        PPX_ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");
        return n;
    }

    virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
    {
        const uint32_t n = AppendDataToVertexBuffer(pGeom, kPositionBufferIndex, vtx.position);
        PPX_ASSERT_MSG(n > 0, "position should always available");
        uint32_t       startSize = GetVertexBufferSize(pGeom, kNonPositionBufferIndex);
        const uint32_t attrCount = GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
        for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
            const grfx::VertexSemantic semantic = GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, attrIndex);

            // clang-format off
            switch (semantic) {
                default                              : PPX_ASSERT_MSG(false, "should not have other sematic"); break;
                case grfx::VERTEX_SEMANTIC_POSITION  : PPX_ASSERT_MSG(false, "position should be in binding 0"); break;
                case grfx::VERTEX_SEMANTIC_COLOR     : AppendDataToVertexBuffer(pGeom, GetColorBufferIndex(pGeom), vtx.color); break;
            }
            // clang-format on
        }
        uint32_t endSize = GetVertexBufferSize(pGeom, kNonPositionBufferIndex);

        uint32_t bytesWritten = (endSize - startSize);

        const uint32_t vertexBufferElementSize = GetVertexBufferElementSize(pGeom, kNonPositionBufferIndex);
        PPX_ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");
        return n;
    }

    virtual uint32_t GetVertexCount(const Geometry* pGeom) override
    {
        return GetVertexBufferElementCount(pGeom, GetPositionBufferIndex(pGeom));
    }

private:
    const uint32_t kPositionBufferIndex    = 0;
    const uint32_t kNonPositionBufferIndex = 1;
};

static VertexDataProcessorPlanar<TriMeshVertexData>                   sVDProcessorPlanar;
static VertexDataProcessorPlanar<TriMeshVertexDataCompressed>         sVDProcessorPlanarCompressed;
static VertexDataProcessorInterleaved<TriMeshVertexData>              sVDProcessorInterleaved;
static VertexDataProcessorInterleaved<TriMeshVertexDataCompressed>    sVDProcessorInterleavedCompressed;
static VertexDataProcessorPositionPlanar<TriMeshVertexData>           sVDProcessorPositionPlanar;
static VertexDataProcessorPositionPlanar<TriMeshVertexDataCompressed> sVDProcessorPositionPlanarCompressed;

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

GeometryOptions GeometryOptions::PositionPlanarU16()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
    ci.indexType             = grfx::INDEX_TYPE_UINT16;
    ci.AddPosition();
    return ci;
}

GeometryOptions GeometryOptions::PositionPlanarU32()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
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

GeometryOptions GeometryOptions::PositionPlanar()
{
    GeometryOptions ci       = {};
    ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
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

        switch (vertexAttributeLayout) {
            case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED:
                attribute.binding = 0;
                vertexBindings[0].AppendAttribute(attribute);
                vertexBindingCount = 1;
                break;
            case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR:
                PPX_ASSERT_MSG(vertexBindingCount < PPX_MAX_VERTEX_BINDINGS, "max vertex bindings exceeded");
                vertexBindings[vertexBindingCount].AppendAttribute(attribute);
                vertexBindings[vertexBindingCount].SetBinding(vertexBindingCount);
                vertexBindingCount += 1;
                break;
            case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR:
                if (semantic == grfx::VERTEX_SEMANTIC_POSITION) {
                    attribute.binding = 0;
                    vertexBindings[0].AppendAttribute(attribute);
                }
                else {
                    vertexBindings[1].AppendAttribute(attribute);
                    vertexBindings[1].SetBinding(1);
                }
                vertexBindingCount = 2;
                break;
            default:
                PPX_ASSERT_MSG(false, "unsupported vertex attribute layout type");
                break;
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
    size_t sizeOfData = mData.size();
    // round up for the case of interleaved buffers
    uint32_t count = static_cast<uint32_t>(std::ceil(static_cast<double>(sizeOfData) / static_cast<double>(mElementSize)));
    return count;
}

// -------------------------------------------------------------------------------------------------
// Geometry
// -------------------------------------------------------------------------------------------------
Result Geometry::InternalCtor()
{
    switch (mCreateInfo.vertexAttributeLayout) {
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED:
            mVDProcessor           = &sVDProcessorInterleaved;
            mVDProcessorCompressed = &sVDProcessorInterleavedCompressed;
            break;
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR:
            mVDProcessor           = &sVDProcessorPlanar;
            mVDProcessorCompressed = &sVDProcessorPlanarCompressed;
            break;
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR:
            mVDProcessor           = &sVDProcessorPositionPlanar;
            mVDProcessorCompressed = &sVDProcessorPositionPlanarCompressed;
            break;
        default:
            PPX_ASSERT_MSG(false, "unsupported vertex attribute layout type");
            return ppx::ERROR_FAILED;
    }

    if (!mVDProcessor->Validate(this)) {
        return ppx::ERROR_FAILED;
    }

    if (mCreateInfo.indexType != grfx::INDEX_TYPE_UNDEFINED) {
        uint32_t elementSize = grfx::IndexTypeSize(mCreateInfo.indexType);

        if (elementSize == 0) {
            // Shouldn't occur unless there's corruption
            PPX_ASSERT_MSG(false, "could not determine index type size");
            return ppx::ERROR_FAILED;
        }

        mIndexBuffer = Buffer(BUFFER_TYPE_INDEX, elementSize);
    }

    return mVDProcessor->UpdateVertexBuffer(this);
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
    PPX_ASSERT_MSG(mVDProcessor != nullptr, "Geometry is not initialized");
    return mVDProcessor->GetVertexCount(this);
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

uint32_t Geometry::AppendVertexData(const TriMeshVertexData& vtx)
{
    return mVDProcessor->AppendVertexData(this, vtx);
}

uint32_t Geometry::AppendVertexData(const TriMeshVertexDataCompressed& vtx)
{
    return mVDProcessorCompressed->AppendVertexData(this, vtx);
}

uint32_t Geometry::AppendVertexData(const WireMeshVertexData& vtx)
{
    return mVDProcessor->AppendVertexData(this, vtx);
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

} // namespace ppx
