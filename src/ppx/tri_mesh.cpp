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

#include "ppx/tri_mesh.h"
#include "ppx/math_util.h"
#include "ppx/timer.h"
#include "ppx/fs.h"

#include "tiny_obj_loader.h"

namespace ppx {

TriMesh::TriMesh()
{
}

TriMesh::TriMesh(grfx::IndexType indexType)
    : mIndexType(indexType)
{
}

TriMesh::TriMesh(TriMeshAttributeDim texCoordDim)
    : mTexCoordDim(texCoordDim)
{
}

TriMesh::TriMesh(grfx::IndexType indexType, TriMeshAttributeDim texCoordDim)
    : mIndexType(indexType), mTexCoordDim(texCoordDim)
{
}

TriMesh::~TriMesh()
{
}

uint32_t TriMesh::GetCountTriangles() const
{
    uint32_t count = 0;
    if (mIndexType != grfx::INDEX_TYPE_UNDEFINED) {
        uint32_t elementSize  = grfx::IndexTypeSize(mIndexType);
        uint32_t elementCount = CountU32(mIndices) / elementSize;
        count                 = elementCount / 3;
    }
    else {
        count = CountU32(mPositions) / 3;
    }
    return count;
}

uint32_t TriMesh::GetCountIndices() const
{
    uint32_t indexSize = grfx::IndexTypeSize(mIndexType);
    if (indexSize == 0) {
        return 0;
    }

    uint32_t count = CountU32(mIndices) / indexSize;
    return count;
}

uint32_t TriMesh::GetCountPositions() const
{
    uint32_t count = CountU32(mPositions);
    return count;
}

uint32_t TriMesh::GetCountColors() const
{
    uint32_t count = CountU32(mColors);
    return count;
}

uint32_t TriMesh::GetCountNormals() const
{
    uint32_t count = CountU32(mNormals);
    return count;
}

uint32_t TriMesh::GetCountTexCoords() const
{
    if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_2) {
        uint32_t count = CountU32(mTexCoords) / 2;
        return count;
    }
    else if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_3) {
        uint32_t count = CountU32(mTexCoords) / 3;
        return count;
    }
    else if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_4) {
        uint32_t count = CountU32(mTexCoords) / 4;
        return count;
    }
    return 0;
}

uint32_t TriMesh::GetCountTangents() const
{
    uint32_t count = CountU32(mTangents);
    return count;
}

uint32_t TriMesh::GetCountBitangents() const
{
    uint32_t count = CountU32(mBitangents);
    return count;
}

uint64_t TriMesh::GetDataSizeIndices() const
{
    uint64_t size = static_cast<uint64_t>(mIndices.size());
    return size;
}

uint64_t TriMesh::GetDataSizePositions() const
{
    uint64_t size = static_cast<uint64_t>(mPositions.size() * sizeof(float3));
    return size;
}

uint64_t TriMesh::GetDataSizeColors() const
{
    uint64_t size = static_cast<uint64_t>(mColors.size() * sizeof(float3));
    return size;
}

uint64_t TriMesh::GetDataSizeNormalls() const
{
    uint64_t size = static_cast<uint64_t>(mNormals.size() * sizeof(float3));
    return size;
}

uint64_t TriMesh::GetDataSizeTexCoords() const
{
    uint64_t size = static_cast<uint64_t>(mTexCoords.size() * sizeof(float));
    return size;
}

uint64_t TriMesh::GetDataSizeTangents() const
{
    uint64_t size = static_cast<uint64_t>(mTangents.size() * sizeof(float3));
    return size;
}

uint64_t TriMesh::GetDataSizeBitangents() const
{
    uint64_t size = static_cast<uint64_t>(mBitangents.size() * sizeof(float3));
    return size;
}

const uint16_t* TriMesh::GetDataIndicesU16(uint32_t index) const
{
    if (mIndexType != grfx::INDEX_TYPE_UINT16) {
        return nullptr;
    }
    uint32_t count = GetCountIndices();
    if (index >= count) {
        return nullptr;
    }
    size_t      offset = sizeof(uint16_t) * index;
    const char* ptr    = reinterpret_cast<const char*>(mIndices.data()) + offset;
    return reinterpret_cast<const uint16_t*>(ptr);
}

const uint32_t* TriMesh::GetDataIndicesU32(uint32_t index) const
{
    if (mIndexType != grfx::INDEX_TYPE_UINT32) {
        return nullptr;
    }
    uint32_t count = GetCountIndices();
    if (index >= count) {
        return nullptr;
    }
    size_t      offset = sizeof(uint32_t) * index;
    const char* ptr    = reinterpret_cast<const char*>(mIndices.data()) + offset;
    return reinterpret_cast<const uint32_t*>(ptr);
}

const float3* TriMesh::GetDataPositions(uint32_t index) const
{
    if (index >= mPositions.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mPositions.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

const float3* TriMesh::GetDataColors(uint32_t index) const
{
    if (index >= mColors.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mColors.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

const float3* TriMesh::GetDataNormalls(uint32_t index) const
{
    if (index >= mNormals.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mNormals.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

const float2* TriMesh::GetDataTexCoords2(uint32_t index) const
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_2) {
        return nullptr;
    }
    uint32_t count = GetCountTexCoords();
    if (index >= count) {
        return nullptr;
    }
    size_t      offset = sizeof(float2) * index;
    const char* ptr    = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
    return reinterpret_cast<const float2*>(ptr);
}

const float3* TriMesh::GetDataTexCoords3(uint32_t index) const
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_3) {
        return nullptr;
    }
    uint32_t count = GetCountTexCoords();
    if (index >= count) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

const float4* TriMesh::GetDataTexCoords4(uint32_t index) const
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_4) {
        return nullptr;
    }
    uint32_t count = GetCountTexCoords();
    if (index >= count) {
        return nullptr;
    }
    size_t      offset = sizeof(float4) * index;
    const char* ptr    = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
    return reinterpret_cast<const float4*>(ptr);
}

const float4* TriMesh::GetDataTangents(uint32_t index) const
{
    if (index >= mTangents.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float4) * index;
    const char* ptr    = reinterpret_cast<const char*>(mTangents.data()) + offset;
    return reinterpret_cast<const float4*>(ptr);
}

const float3* TriMesh::GetDataBitangents(uint32_t index) const
{
    if (index >= mBitangents.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mBitangents.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

void TriMesh::AppendIndexU16(uint16_t value)
{
    const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
    mIndices.push_back(pBytes[0]);
    mIndices.push_back(pBytes[1]);
}

void TriMesh::AppendIndexU32(uint32_t value)
{
    const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
    mIndices.push_back(pBytes[0]);
    mIndices.push_back(pBytes[1]);
    mIndices.push_back(pBytes[2]);
    mIndices.push_back(pBytes[3]);
}

void TriMesh::PreallocateForTriangleCount(size_t triangleCount, bool enableColors, bool enableNormals, bool enableTexCoords, bool enableTangents)
{
    size_t vertexCount = triangleCount * 3;

    // Reserve for triangles
    switch (mIndexType) {
        case grfx::INDEX_TYPE_UINT16:
            mIndices.reserve(vertexCount * sizeof(uint16_t));
            break;
        case grfx::INDEX_TYPE_UINT32:
            mIndices.reserve(vertexCount * sizeof(uint32_t));
            break;
        default:
            // Nothing to do; not indexing.
            return;
    }

    // Position per vertex
    mPositions.reserve(vertexCount);
    // Color per vertex
    if (enableColors) {
        mColors.reserve(vertexCount);
    }
    // Normal per vertex
    if (enableNormals) {
        mNormals.reserve(vertexCount);
    }
    // TexCoord per vertex
    if (enableTexCoords) {
        int32_t dimCount = (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_4) ? 4 : ((mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_3) ? 3 : ((mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_2) ? 2 : 0));
        mTexCoords.reserve(vertexCount * dimCount);
    }
    // Tangents = 3 per triangle (NOT necessarily related to vertices!)
    if (enableTangents) {
        mTangents.reserve(triangleCount * 3);
        mBitangents.reserve(triangleCount * 3);
    }
}

uint32_t TriMesh::AppendTriangle(uint32_t v0, uint32_t v1, uint32_t v2)
{
    if (mIndexType == grfx::INDEX_TYPE_UINT16) {
        mIndices.reserve(mIndices.size() + 3 * sizeof(uint16_t));
        AppendIndexU16(static_cast<uint16_t>(v0));
        AppendIndexU16(static_cast<uint16_t>(v1));
        AppendIndexU16(static_cast<uint16_t>(v2));
    }
    else if (mIndexType == grfx::INDEX_TYPE_UINT32) {
        mIndices.reserve(mIndices.size() + 3 * sizeof(uint32_t));
        AppendIndexU32(v0);
        AppendIndexU32(v1);
        AppendIndexU32(v2);
    }
    else {
        PPX_ASSERT_MSG(false, "unknown index type");
        return 0;
    }
    uint32_t count = GetCountTriangles();
    return count;
}

uint32_t TriMesh::AppendPosition(const float3& value)
{
    mPositions.push_back(value);
    // Update bounding box
    uint32_t count = GetCountPositions();
    if (count > 1) {
        mBoundingBoxMin.x = std::min<float>(mBoundingBoxMin.x, value.x);
        mBoundingBoxMin.y = std::min<float>(mBoundingBoxMin.y, value.y);
        mBoundingBoxMin.z = std::min<float>(mBoundingBoxMin.z, value.z);
        mBoundingBoxMax.x = std::max<float>(mBoundingBoxMax.x, value.x);
        mBoundingBoxMax.y = std::max<float>(mBoundingBoxMax.y, value.y);
        mBoundingBoxMax.z = std::max<float>(mBoundingBoxMax.z, value.z);
    }
    else {
        mBoundingBoxMin = mBoundingBoxMax = value;
    }
    return count;
}

uint32_t TriMesh::AppendColor(const float3& value)
{
    mColors.push_back(value);
    uint32_t count = GetCountColors();
    return count;
}

uint32_t TriMesh::AppendTexCoord(const float2& value)
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_2) {
        PPX_ASSERT_MSG(false, "unknown tex coord dim");
        return 0;
    }
    mTexCoords.reserve(mTexCoords.size() + 2);
    mTexCoords.push_back(value.x);
    mTexCoords.push_back(value.y);
    uint32_t count = GetCountTexCoords();
    return count;
}

uint32_t TriMesh::AppendTexCoord(const float3& value)
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_3) {
        PPX_ASSERT_MSG(false, "unknown tex coord dim");
        return 0;
    }
    mTexCoords.reserve(mTexCoords.size() + 3);
    mTexCoords.push_back(value.x);
    mTexCoords.push_back(value.y);
    mTexCoords.push_back(value.z);
    uint32_t count = GetCountTexCoords();
    return count;
}

uint32_t TriMesh::AppendTexCoord(const float4& value)
{
    if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_4) {
        PPX_ASSERT_MSG(false, "unknown tex coord dim");
        return 0;
    }
    mTexCoords.reserve(mTexCoords.size() + 3);
    mTexCoords.push_back(value.x);
    mTexCoords.push_back(value.y);
    mTexCoords.push_back(value.z);
    mTexCoords.push_back(value.w);
    uint32_t count = GetCountTexCoords();
    return count;
}

uint32_t TriMesh::AppendNormal(const float3& value)
{
    mNormals.push_back(value);
    uint32_t count = GetCountNormals();
    return count;
}

uint32_t TriMesh::AppendTangent(const float4& value)
{
    mTangents.push_back(value);
    uint32_t count = GetCountTangents();
    return count;
}

uint32_t TriMesh::AppendBitangent(const float3& value)
{
    mBitangents.push_back(value);
    uint32_t count = GetCountBitangents();
    return count;
}

Result TriMesh::GetTriangle(uint32_t triIndex, uint32_t& v0, uint32_t& v1, uint32_t& v2) const
{
    if (mIndexType == grfx::INDEX_TYPE_UNDEFINED) {
        return ppx::ERROR_GEOMETRY_NO_INDEX_DATA;
    }

    uint32_t triCount = GetCountTriangles();
    if (triIndex >= triCount) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    const uint8_t* pData       = mIndices.data();
    uint32_t       elementSize = grfx::IndexTypeSize(mIndexType);

    if (mIndexType == grfx::INDEX_TYPE_UINT16) {
        size_t          offset     = 3 * triIndex * elementSize;
        const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(pData + offset);
        v0                         = static_cast<uint32_t>(pIndexData[0]);
        v1                         = static_cast<uint32_t>(pIndexData[1]);
        v2                         = static_cast<uint32_t>(pIndexData[2]);
    }
    else if (mIndexType == grfx::INDEX_TYPE_UINT32) {
        size_t          offset     = 3 * triIndex * elementSize;
        const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(pData + offset);
        v0                         = static_cast<uint32_t>(pIndexData[0]);
        v1                         = static_cast<uint32_t>(pIndexData[1]);
        v2                         = static_cast<uint32_t>(pIndexData[2]);
    }

    return ppx::SUCCESS;
}

Result TriMesh::GetVertexData(uint32_t vtxIndex, TriMeshVertexData* pVertexData) const
{
    uint32_t vertexCount = GetCountPositions();
    if (vtxIndex >= vertexCount) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    const float3* pPosition  = GetDataPositions(vtxIndex);
    const float3* pColor     = GetDataColors(vtxIndex);
    const float3* pNormal    = GetDataNormalls(vtxIndex);
    const float2* pTexCoord2 = GetDataTexCoords2(vtxIndex);
    const float4* pTangent   = GetDataTangents(vtxIndex);
    const float3* pBitangent = GetDataBitangents(vtxIndex);

    pVertexData->position = *pPosition;

    if (!IsNull(pColor)) {
        pVertexData->color = *pColor;
    }

    if (!IsNull(pNormal)) {
        pVertexData->normal = *pNormal;
    }

    if (!IsNull(pTexCoord2)) {
        pVertexData->texCoord = *pTexCoord2;
    }

    if (!IsNull(pTangent)) {
        pVertexData->tangent = *pTangent;
    }
    if (!IsNull(pBitangent)) {
        pVertexData->bitangent = *pBitangent;
    }

    return ppx::SUCCESS;
}

void TriMesh::AppendIndexAndVertexData(
    std::vector<uint32_t>&    indexData,
    const std::vector<float>& vertexData,
    const uint32_t            expectedVertexCount,
    const TriMeshOptions&     options,
    TriMesh&                  mesh)
{
    grfx::IndexType indexType = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;

    // Verify expected vertex count
    size_t vertexCount = (vertexData.size() * sizeof(float)) / sizeof(TriMeshVertexData);
    PPX_ASSERT_MSG(vertexCount == expectedVertexCount, "unexpected vertex count");

    // Get base pointer to vertex data
    const char* pData = reinterpret_cast<const char*>(vertexData.data());

    if (indexType != grfx::INDEX_TYPE_UNDEFINED) {
        for (size_t i = 0; i < vertexCount; ++i) {
            const TriMeshVertexData* pVertexData = reinterpret_cast<const TriMeshVertexData*>(pData + (i * sizeof(TriMeshVertexData)));

            mesh.AppendPosition(pVertexData->position * options.mScale);

            if (options.mEnableVertexColors || options.mEnableObjectColor) {
                float3 color = options.mEnableObjectColor ? options.mObjectColor : pVertexData->color;
                mesh.AppendColor(color);
            }

            if (options.mEnableNormals) {
                mesh.AppendNormal(pVertexData->normal);
            }

            if (options.mEnableTexCoords) {
                mesh.AppendTexCoord(pVertexData->texCoord * options.mTexCoordScale);
            }

            if (options.mEnableTangents) {
                mesh.AppendTangent(pVertexData->tangent);
                mesh.AppendBitangent(pVertexData->bitangent);
            }
        }

        size_t triCount = indexData.size() / 3;
        for (size_t triIndex = 0; triIndex < triCount; ++triIndex) {
            uint32_t v0 = indexData[3 * triIndex + 0];
            uint32_t v1 = indexData[3 * triIndex + 1];
            uint32_t v2 = indexData[3 * triIndex + 2];
            mesh.AppendTriangle(v0, v1, v2);
        }
    }
    else {
        for (size_t i = 0; i < indexData.size(); ++i) {
            uint32_t                 vi          = indexData[i];
            const TriMeshVertexData* pVertexData = reinterpret_cast<const TriMeshVertexData*>(pData + (vi * sizeof(TriMeshVertexData)));

            mesh.AppendPosition(pVertexData->position * options.mScale);

            if (options.mEnableVertexColors) {
                mesh.AppendColor(pVertexData->color);
            }

            if (options.mEnableNormals) {
                mesh.AppendNormal(pVertexData->normal);
            }

            if (options.mEnableTexCoords) {
                mesh.AppendTexCoord(pVertexData->texCoord);
            }

            if (options.mEnableTangents) {
                mesh.AppendTangent(pVertexData->tangent);
                mesh.AppendBitangent(pVertexData->bitangent);
            }
        }
    }
}

TriMesh TriMesh::CreatePlane(TriMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options)
{
    const float    hs     = size.x / 2.0f;
    const float    ht     = size.y / 2.0f;
    const float    ds     = size.x / static_cast<float>(usegs);
    const float    dt     = size.y / static_cast<float>(vsegs);
    const uint32_t uverts = usegs + 1;
    const uint32_t vverts = vsegs + 1;

    std::vector<float> vertexData;
    for (uint32_t j = 0; j < vverts; ++j) {
        for (uint32_t i = 0; i < uverts; ++i) {
            float s = i * ds / size.x;
            float t = j * dt / size.y;
            float u = options.mTexCoordScale.x * s;
            float v = options.mTexCoordScale.y * t;

            // float3 position  = float3(s - hx, 0, t - hz);
            float3 position = float3(0);
            switch (plane) {
                default: {
                    PPX_ASSERT_MSG(false, "unknown plane orientation");
                } break;

                // case TRI_MESH_PLANE_POSITIVE_X: {
                // } break;
                //
                // case TRI_MESH_PLANE_NEGATIVE_X: {
                // } break;
                //
                case TRI_MESH_PLANE_POSITIVE_Y: {
                    position = float3(s * size.x - hs, 0, t * size.y - ht);
                } break;

                case TRI_MESH_PLANE_NEGATIVE_Y: {
                    position = float3((1.0f - s) * size.x - hs, 0, (1.0f - t) * size.y - ht);
                } break;

                    // case TRI_MESH_PLANE_POSITIVE_Z: {
                    // } break;
                    //
                    // case TRI_MESH_PLANE_NEGATIVE_Z: {
                    // } break;
            }

            float3 color     = float3(u, v, 0);
            float3 normal    = float3(0, 1, 0);
            float2 texcoord  = float2(u, v);
            float4 tangent   = float4(0.0f, 0.0f, 0.0f, 1.0f);
            float3 bitangent = glm::cross(normal, float3(tangent));

            vertexData.push_back(position.x);
            vertexData.push_back(position.y);
            vertexData.push_back(position.z);
            vertexData.push_back(color.r);
            vertexData.push_back(color.g);
            vertexData.push_back(color.b);
            vertexData.push_back(normal.x);
            vertexData.push_back(normal.y);
            vertexData.push_back(normal.z);
            vertexData.push_back(texcoord.x);
            vertexData.push_back(texcoord.y);
            vertexData.push_back(tangent.x);
            vertexData.push_back(tangent.y);
            vertexData.push_back(tangent.z);
            vertexData.push_back(tangent.w);
            vertexData.push_back(bitangent.x);
            vertexData.push_back(bitangent.y);
            vertexData.push_back(bitangent.z);
        }
    }

    std::vector<uint32_t> indexData;
    for (uint32_t i = 1; i < uverts; ++i) {
        for (uint32_t j = 1; j < vverts; ++j) {
            uint32_t i0 = i - 1;
            uint32_t i1 = i;
            uint32_t j0 = j - 1;
            uint32_t j1 = j;
            uint32_t v0 = i1 * vverts + j0;
            uint32_t v1 = i1 * vverts + j1;
            uint32_t v2 = i0 * vverts + j1;
            uint32_t v3 = i0 * vverts + j0;

            switch (plane) {
                default: {
                    PPX_ASSERT_MSG(false, "unknown plane orientation");
                } break;

                    // case TRI_MESH_PLANE_POSITIVE_X: {
                    // } break;
                    //
                    // case TRI_MESH_PLANE_NEGATIVE_X: {
                    // } break;

                case TRI_MESH_PLANE_POSITIVE_Y: {
                    indexData.push_back(v0);
                    indexData.push_back(v1);
                    indexData.push_back(v2);

                    indexData.push_back(v0);
                    indexData.push_back(v2);
                    indexData.push_back(v3);
                } break;

                case TRI_MESH_PLANE_NEGATIVE_Y: {
                    indexData.push_back(v0);
                    indexData.push_back(v2);
                    indexData.push_back(v1);

                    indexData.push_back(v0);
                    indexData.push_back(v3);
                    indexData.push_back(v2);
                } break;

                    // case TRI_MESH_PLANE_POSITIVE_Z: {
                    // } break;
                    //
                    // case TRI_MESH_PLANE_NEGATIVE_Z: {
                    // } break;
            }
        }
    }

    grfx::IndexType     indexType   = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
    TriMesh             mesh        = TriMesh(indexType, texCoordDim);

    uint32_t expectedVertexCount = uverts * vverts;
    AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

    return mesh;

    /*
    const float hx = size.x / 2.0f;
    const float hz = size.y / 2.0f;
    // clang-format off
    std::vector<float> vertexData = {
        // position       // vertex color     // normal           // texcoord   // tangent                // bitangent
        -hx, 0.0f, -hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
        -hx, 0.0f,  hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
         hx, 0.0f,  hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
         hx, 0.0f, -hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
    };

    std::vector<uint32_t> indexData = {
        0, 1, 2,
        0, 2, 3
    };
    // clang-format on

    grfx::IndexType     indexType   = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
    TriMesh             mesh        = TriMesh(indexType, texCoordDim);

    AppendIndexAndVertexData(indexData, vertexData, 4, options, mesh);

    return mesh;
    */
}

TriMesh TriMesh::CreateCube(const float3& size, const TriMeshOptions& options)
{
    const float hx = size.x / 2.0f;
    const float hy = size.y / 2.0f;
    const float hz = size.z / 2.0f;

    // clang-format off
    std::vector<float> vertexData = {  
        // position      // vertex colors    // normal           // texcoords   // tangents               // bitangents
         hx,  hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  0  -Z side
         hx, -hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  1
        -hx, -hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  2
        -hx,  hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  3

        -hx,  hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  4  +Z side
        -hx, -hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  5
         hx, -hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  6
         hx,  hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  7

        -hx,  hy, -hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  8  -X side
        -hx, -hy, -hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  9
        -hx, -hy,  hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 10
        -hx,  hy,  hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 11

         hx,  hy,  hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 12  +X side
         hx, -hy,  hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 13
         hx, -hy, -hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 14
         hx,  hy, -hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 15

        -hx, -hy,  hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 16  -Y side
        -hx, -hy, -hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 17
         hx, -hy, -hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 18
         hx, -hy,  hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 19

        -hx,  hy, -hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 20  +Y side
        -hx,  hy,  hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 21
         hx,  hy,  hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 22
         hx,  hy, -hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 23
    };

    std::vector<uint32_t> indexData = {
        0,  1,  2, // -Z side
        0,  2,  3,

        4,  5,  6, // +Z side
        4,  6,  7,

        8,  9, 10, // -X side
        8, 10, 11,

        12, 13, 14, // +X side
        12, 14, 15,

        16, 17, 18, // -X side
        16, 18, 19,

        20, 21, 22, // +X side
        20, 22, 23,
    };
    // clang-format on

    grfx::IndexType     indexType   = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
    TriMesh             mesh        = TriMesh(indexType, texCoordDim);

    AppendIndexAndVertexData(indexData, vertexData, 24, options, mesh);

    return mesh;
}

TriMesh TriMesh::CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options)
{
    constexpr float kPi    = glm::pi<float>();
    constexpr float kTwoPi = 2.0f * kPi;

    const uint32_t uverts = usegs + 1;
    const uint32_t vverts = vsegs + 1;

    float dt = kTwoPi / static_cast<float>(usegs);
    float dp = kPi / static_cast<float>(vsegs);

    std::vector<float> vertexData;
    for (uint32_t i = 0; i < uverts; ++i) {
        for (uint32_t j = 0; j < vverts; ++j) {
            float  theta     = i * dt;
            float  phi       = j * dp;
            float  u         = options.mTexCoordScale.x * theta / kTwoPi;
            float  v         = options.mTexCoordScale.y * phi / kPi;
            float3 P         = SphericalToCartesian(theta, phi);
            float3 position  = radius * P;
            float3 color     = float3(u, v, 0);
            float3 normal    = normalize(position);
            float2 texcoord  = float2(u, v);
            float4 tangent   = float4(-SphericalTangent(theta, phi), 1.0);
            float3 bitangent = glm::cross(normal, float3(tangent));

            vertexData.push_back(position.x);
            vertexData.push_back(position.y);
            vertexData.push_back(position.z);
            vertexData.push_back(color.r);
            vertexData.push_back(color.g);
            vertexData.push_back(color.b);
            vertexData.push_back(normal.x);
            vertexData.push_back(normal.y);
            vertexData.push_back(normal.z);
            vertexData.push_back(texcoord.x);
            vertexData.push_back(texcoord.y);
            vertexData.push_back(tangent.x);
            vertexData.push_back(tangent.y);
            vertexData.push_back(tangent.z);
            vertexData.push_back(tangent.w);
            vertexData.push_back(bitangent.x);
            vertexData.push_back(bitangent.y);
            vertexData.push_back(bitangent.z);
        }
    }

    std::vector<uint32_t> indexData;
    for (uint32_t i = 1; i < uverts; ++i) {
        for (uint32_t j = 1; j < vverts; ++j) {
            uint32_t i0 = i - 1;
            uint32_t i1 = i;
            uint32_t j0 = j - 1;
            uint32_t j1 = j;
            uint32_t v0 = i1 * vverts + j0;
            uint32_t v1 = i1 * vverts + j1;
            uint32_t v2 = i0 * vverts + j1;
            uint32_t v3 = i0 * vverts + j0;

            indexData.push_back(v0);
            indexData.push_back(v1);
            indexData.push_back(v2);

            indexData.push_back(v0);
            indexData.push_back(v2);
            indexData.push_back(v3);
        }
    }

    grfx::IndexType     indexType   = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
    TriMesh             mesh        = TriMesh(indexType, texCoordDim);

    uint32_t expectedVertexCount = uverts * vverts;
    AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

    return mesh;
}

Result TriMesh::CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options, TriMesh* pTriMesh)
{
    if (IsNull(pTriMesh)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    Timer timer;
    PPX_ASSERT_MSG(timer.Start() == ppx::TIMER_RESULT_SUCCESS, "timer start failed");
    double fnStartTime = timer.SecondsSinceStart();

    // Determine index type and tex coord dim
    grfx::IndexType     indexType   = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;

    // Create new mesh
    *pTriMesh = TriMesh(indexType, texCoordDim);

    const std::vector<float3> colors = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
    };

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    ppx::fs::FileStream objStream;
    if (!objStream.Open(path.string().c_str())) {
        return ppx::ERROR_GEOMETRY_FILE_LOAD_FAILED;
    }

    std::string  warn;
    std::string  err;
    std::istream istr(&objStream);
    bool         loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &istr, nullptr, true);
    if (!loaded || !err.empty()) {
        return ppx::ERROR_GEOMETRY_FILE_LOAD_FAILED;
    }

    size_t numShapes = shapes.size();
    if (numShapes == 0) {
        return ppx::ERROR_GEOMETRY_FILE_NO_DATA;
    }

    //// Check to see if data can be indexed
    // bool indexable = true;
    // for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx) {
    //     const tinyobj::shape_t& shape     = shapes[shapeIdx];
    //     const tinyobj::mesh_t&  shapeMesh = shape.mesh;
    //
    //     size_t numTriangles = shapeMesh.indices.size() / 3;
    //     for (size_t triIdx = 0; triIdx < numTriangles; ++triIdx) {
    //         size_t triVtxIdx0 = triIdx * 3 + 0;
    //         size_t triVtxIdx1 = triIdx * 3 + 1;
    //         size_t triVtxIdx2 = triIdx * 3 + 2;
    //
    //         // Index data
    //         const tinyobj::index_t& dataIdx0 = shapeMesh.indices[triVtxIdx0];
    //         const tinyobj::index_t& dataIdx1 = shapeMesh.indices[triVtxIdx1];
    //         const tinyobj::index_t& dataIdx2 = shapeMesh.indices[triVtxIdx2];
    //
    //         bool sameIdx0 = (dataIdx0.vertex_index == dataIdx0.normal_index) && (dataIdx0.normal_index == dataIdx0.texcoord_index);
    //         bool sameIdx1 = (dataIdx1.vertex_index == dataIdx1.normal_index) && (dataIdx1.normal_index == dataIdx1.texcoord_index);
    //         bool sameIdx2 = (dataIdx2.vertex_index == dataIdx2.normal_index) && (dataIdx2.normal_index == dataIdx2.texcoord_index);
    //         bool same     = sameIdx0 && sameIdx1 && sameIdx2;
    //         if (!same) {
    //            indexable = false;
    //            break;
    //         }
    //     }
    // }

    // Preallocate based on the total number of triangles.
    size_t totalTriangles = 0;
    for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx) {
        totalTriangles += shapes[shapeIdx].mesh.indices.size() / 3;
    }
    pTriMesh->PreallocateForTriangleCount(totalTriangles,
                                          /* enableColors= */ (options.mEnableVertexColors || options.mEnableObjectColor),
                                          options.mEnableNormals,
                                          options.mEnableTexCoords,
                                          options.mEnableTangents);

    // Build geometry
    for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx) {
        const tinyobj::shape_t& shape     = shapes[shapeIdx];
        const tinyobj::mesh_t&  shapeMesh = shape.mesh;

        size_t numTriangles = shapeMesh.indices.size() / 3;
        for (size_t triIdx = 0; triIdx < numTriangles; ++triIdx) {
            size_t triVtxIdx0 = triIdx * 3 + 0;
            size_t triVtxIdx1 = triIdx * 3 + 1;
            size_t triVtxIdx2 = triIdx * 3 + 2;

            // Index data
            const tinyobj::index_t& dataIdx0 = shapeMesh.indices[triVtxIdx0];
            const tinyobj::index_t& dataIdx1 = shapeMesh.indices[triVtxIdx1];
            const tinyobj::index_t& dataIdx2 = shapeMesh.indices[triVtxIdx2];

            // Vertex data
            TriMeshVertexData vtx0 = {};
            TriMeshVertexData vtx1 = {};
            TriMeshVertexData vtx2 = {};

            // Pick a face color
            float3 faceColor = colors[triIdx % colors.size()];
            vtx0.color       = faceColor;
            vtx1.color       = faceColor;
            vtx2.color       = faceColor;

            // Vertex positions
            {
                int i0        = 3 * dataIdx0.vertex_index + 0;
                int i1        = 3 * dataIdx0.vertex_index + 1;
                int i2        = 3 * dataIdx0.vertex_index + 2;
                vtx0.position = float3(attrib.vertices[i0], attrib.vertices[i1], attrib.vertices[i2]);

                i0            = 3 * dataIdx1.vertex_index + 0;
                i1            = 3 * dataIdx1.vertex_index + 1;
                i2            = 3 * dataIdx1.vertex_index + 2;
                vtx1.position = float3(attrib.vertices[i0], attrib.vertices[i1], attrib.vertices[i2]);

                i0            = 3 * dataIdx2.vertex_index + 0;
                i1            = 3 * dataIdx2.vertex_index + 1;
                i2            = 3 * dataIdx2.vertex_index + 2;
                vtx2.position = float3(attrib.vertices[i0], attrib.vertices[i1], attrib.vertices[i2]);
            }

            // Normals
            if ((dataIdx0.normal_index != -1) && (dataIdx1.normal_index != -1) && (dataIdx2.normal_index != -1)) {
                int i0      = 3 * dataIdx0.normal_index + 0;
                int i1      = 3 * dataIdx0.normal_index + 1;
                int i2      = 3 * dataIdx0.normal_index + 2;
                vtx0.normal = float3(attrib.normals[i0], attrib.normals[i1], attrib.normals[i2]);

                i0          = 3 * dataIdx1.normal_index + 0;
                i1          = 3 * dataIdx1.normal_index + 1;
                i2          = 3 * dataIdx1.normal_index + 2;
                vtx1.normal = float3(attrib.normals[i0], attrib.normals[i1], attrib.normals[i2]);

                i0          = 3 * dataIdx2.normal_index + 0;
                i1          = 3 * dataIdx2.normal_index + 1;
                i2          = 3 * dataIdx2.normal_index + 2;
                vtx2.normal = float3(attrib.normals[i0], attrib.normals[i1], attrib.normals[i2]);
            }

            // Texture coordinates
            if ((dataIdx0.texcoord_index != -1) && (dataIdx1.texcoord_index != -1) && (dataIdx2.texcoord_index != -1)) {
                int i0        = 2 * dataIdx0.texcoord_index + 0;
                int i1        = 2 * dataIdx0.texcoord_index + 1;
                vtx0.texCoord = float2(attrib.texcoords[i0], attrib.texcoords[i1]);

                i0            = 2 * dataIdx1.texcoord_index + 0;
                i1            = 2 * dataIdx1.texcoord_index + 1;
                vtx1.texCoord = float2(attrib.texcoords[i0], attrib.texcoords[i1]);

                i0            = 2 * dataIdx2.texcoord_index + 0;
                i1            = 2 * dataIdx2.texcoord_index + 1;
                vtx2.texCoord = float2(attrib.texcoords[i0], attrib.texcoords[i1]);

                // Scale tex coords
                vtx0.texCoord *= options.mTexCoordScale;
                vtx1.texCoord *= options.mTexCoordScale;
                vtx2.texCoord *= options.mTexCoordScale;

                if (options.mInvertTexCoordsV) {
                    vtx0.texCoord.y = 1.0f - vtx0.texCoord.y;
                    vtx1.texCoord.y = 1.0f - vtx1.texCoord.y;
                    vtx2.texCoord.y = 1.0f - vtx2.texCoord.y;
                }
            }

            float3 pos0 = (vtx0.position * options.mScale) + options.mTranslate;
            float3 pos1 = (vtx1.position * options.mScale) + options.mTranslate;
            float3 pos2 = (vtx2.position * options.mScale) + options.mTranslate;

            uint32_t triVtx0 = pTriMesh->AppendPosition(pos0) - 1;
            uint32_t triVtx1 = pTriMesh->AppendPosition(pos1) - 1;
            uint32_t triVtx2 = pTriMesh->AppendPosition(pos2) - 1;

            if (options.mEnableVertexColors || options.mEnableObjectColor) {
                if (options.mEnableObjectColor) {
                    vtx0.color = options.mObjectColor;
                    vtx1.color = options.mObjectColor;
                    vtx2.color = options.mObjectColor;
                }
                pTriMesh->AppendColor(vtx0.color);
                pTriMesh->AppendColor(vtx1.color);
                pTriMesh->AppendColor(vtx2.color);
            }

            if (options.mEnableNormals) {
                pTriMesh->AppendNormal(vtx0.normal);
                pTriMesh->AppendNormal(vtx1.normal);
                pTriMesh->AppendNormal(vtx2.normal);
            }

            if (options.mEnableTexCoords) {
                pTriMesh->AppendTexCoord(vtx0.texCoord);
                pTriMesh->AppendTexCoord(vtx1.texCoord);
                pTriMesh->AppendTexCoord(vtx2.texCoord);
            }

            if (options.mEnableTangents) {
                float3 edge1 = vtx1.position - vtx0.position;
                float3 edge2 = vtx2.position - vtx0.position;
                float2 duv1  = vtx1.texCoord - vtx0.texCoord;
                float2 duv2  = vtx2.texCoord - vtx0.texCoord;
                float  r     = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);

                float3 tangent = float3(
                    ((edge1.x * duv2.y) - (edge2.x * duv1.y)) * r,
                    ((edge1.y * duv2.y) - (edge2.y * duv1.y)) * r,
                    ((edge1.z * duv2.y) - (edge2.z * duv1.y)) * r);

                float3 bitangent = float3(
                    ((edge1.x * duv2.x) - (edge2.x * duv1.x)) * r,
                    ((edge1.y * duv2.x) - (edge2.y * duv1.x)) * r,
                    ((edge1.z * duv2.x) - (edge2.z * duv1.x)) * r);

                tangent = glm::normalize(tangent - vtx0.normal * glm::dot(vtx0.normal, tangent));
                float w = 1.0f;

                pTriMesh->AppendTangent(float4(-tangent, w));
                pTriMesh->AppendTangent(float4(-tangent, w));
                pTriMesh->AppendTangent(float4(-tangent, w));
                pTriMesh->AppendBitangent(-bitangent);
                pTriMesh->AppendBitangent(-bitangent);
                pTriMesh->AppendBitangent(-bitangent);
            }

            if (indexType != grfx::INDEX_TYPE_UNDEFINED) {
                if (options.mInvertWinding) {
                    pTriMesh->AppendTriangle(triVtx0, triVtx2, triVtx1);
                }
                else {
                    pTriMesh->AppendTriangle(triVtx0, triVtx1, triVtx2);
                }
            }
        }
    }

    // if (options.mEnableTangents) {
    //     size_t numPositions  = mesh.mPositions.size();
    //     size_t numNormals    = mesh.mNormals.size();
    //     size_t numTangents   = mesh.mTangents.size();
    //     size_t numBitangents = mesh.mBitangents.size();
    //     PPX_ASSERT_MSG(numPositions == numNormals == numTangents == numBitangents, "misaligned data for tangent calculation");
    //
    //     for (size_t i = 0; i < numPositions; ++i) {
    //         const float3& T = mesh.mTangents[i];
    //         const float3& B = mesh.mBitangents[i];
    //     }
    // }

    double fnEndTime = timer.SecondsSinceStart();
    float  fnElapsed = static_cast<float>(fnEndTime - fnStartTime);
    PPX_LOG_INFO("Created mesh from OBJ file: " << path << " (" << FloatString(fnElapsed) << " seconds, " << numShapes << " shapes, " << totalTriangles << " triangles)");

    return ppx::SUCCESS;
}

TriMesh TriMesh::CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options)
{
    TriMesh mesh;
    PPX_CHECKED_CALL(CreateFromOBJ(path, options, &mesh));
    return mesh;
}

} // namespace ppx
