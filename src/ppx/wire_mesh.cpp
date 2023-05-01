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

#include "ppx/wire_mesh.h"
#include "ppx/math_util.h"
#include "ppx/timer.h"

namespace ppx {

WireMesh::WireMesh()
{
}

WireMesh::WireMesh(grfx::IndexType indexType)
    : mIndexType(indexType)
{
}

WireMesh::~WireMesh()
{
}

uint32_t WireMesh::GetCountEdges() const
{
    uint32_t count = 0;
    if (mIndexType != grfx::INDEX_TYPE_UNDEFINED) {
        uint32_t elementSize  = grfx::IndexTypeSize(mIndexType);
        uint32_t elementCount = CountU32(mIndices) / elementSize;
        count                 = elementCount / 2;
    }
    else {
        count = CountU32(mPositions) / 2;
    }
    return count;
}

uint32_t WireMesh::GetCountIndices() const
{
    uint32_t indexSize = grfx::IndexTypeSize(mIndexType);
    if (indexSize == 0) {
        return 0;
    }

    uint32_t count = CountU32(mIndices) / indexSize;
    return count;
}

uint32_t WireMesh::GetCountPositions() const
{
    uint32_t count = CountU32(mPositions);
    return count;
}

uint32_t WireMesh::GetCountColors() const
{
    uint32_t count = CountU32(mColors);
    return count;
}

uint64_t WireMesh::GetDataSizeIndices() const
{
    uint64_t size = static_cast<uint64_t>(mIndices.size());
    return size;
}

uint64_t WireMesh::GetDataSizePositions() const
{
    uint64_t size = static_cast<uint64_t>(mPositions.size() * sizeof(float3));
    return size;
}

uint64_t WireMesh::GetDataSizeColors() const
{
    uint64_t size = static_cast<uint64_t>(mColors.size() * sizeof(float3));
    return size;
}

const uint16_t* WireMesh::GetDataIndicesU16(uint32_t index) const
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

const uint32_t* WireMesh::GetDataIndicesU32(uint32_t index) const
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

const float3* WireMesh::GetDataPositions(uint32_t index) const
{
    if (index >= mPositions.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mPositions.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

const float3* WireMesh::GetDataColors(uint32_t index) const
{
    if (index >= mColors.size()) {
        return nullptr;
    }
    size_t      offset = sizeof(float3) * index;
    const char* ptr    = reinterpret_cast<const char*>(mColors.data()) + offset;
    return reinterpret_cast<const float3*>(ptr);
}

void WireMesh::AppendIndexU16(uint16_t value)
{
    const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
    mIndices.push_back(pBytes[0]);
    mIndices.push_back(pBytes[1]);
}

void WireMesh::AppendIndexU32(uint32_t value)
{
    const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
    mIndices.push_back(pBytes[0]);
    mIndices.push_back(pBytes[1]);
    mIndices.push_back(pBytes[2]);
    mIndices.push_back(pBytes[3]);
}

uint32_t WireMesh::AppendEdge(uint32_t v0, uint32_t v1)
{
    if (mIndexType == grfx::INDEX_TYPE_UINT16) {
        PPX_ASSERT_MSG(v0 <= UINT16_MAX, "v0 is out of range for index type UINT16");
        PPX_ASSERT_MSG(v1 <= UINT16_MAX, "v1 is out of range for index type UINT16");
        mIndices.reserve(mIndices.size() + 2 * sizeof(uint16_t));
        AppendIndexU16(static_cast<uint16_t>(v0));
        AppendIndexU16(static_cast<uint16_t>(v1));
    }
    else if (mIndexType == grfx::INDEX_TYPE_UINT32) {
        mIndices.reserve(mIndices.size() + 2 * sizeof(uint32_t));
        AppendIndexU32(v0);
        AppendIndexU32(v1);
    }
    else {
        PPX_ASSERT_MSG(false, "unknown index type");
        return 0;
    }
    uint32_t count = GetCountEdges();
    return count;
}

uint32_t WireMesh::AppendPosition(const float3& value)
{
    mPositions.push_back(value);
    uint32_t count = GetCountPositions();
    if (count > 0) {
        mBoundingBoxMin.x = std::min<float>(mBoundingBoxMin.x, value.x);
        mBoundingBoxMin.y = std::min<float>(mBoundingBoxMin.y, value.y);
        mBoundingBoxMin.z = std::min<float>(mBoundingBoxMin.z, value.z);
        mBoundingBoxMax.x = std::min<float>(mBoundingBoxMax.x, value.x);
        mBoundingBoxMax.y = std::min<float>(mBoundingBoxMax.y, value.y);
        mBoundingBoxMax.z = std::min<float>(mBoundingBoxMax.z, value.z);
    }
    else {
        mBoundingBoxMin = mBoundingBoxMax = value;
    }
    return count;
}

uint32_t WireMesh::AppendColor(const float3& value)
{
    mColors.push_back(value);
    uint32_t count = GetCountColors();
    return count;
}

Result WireMesh::GetEdge(uint32_t triIndex, uint32_t& v0, uint32_t& v1) const
{
    if (mIndexType == grfx::INDEX_TYPE_UNDEFINED) {
        return ppx::ERROR_NO_INDEX_DATA;
    }

    uint32_t triCount = GetCountEdges();
    if (triIndex >= triCount) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    const uint8_t* pData       = mIndices.data();
    uint32_t       elementSize = grfx::IndexTypeSize(mIndexType);

    if (mIndexType == grfx::INDEX_TYPE_UINT16) {
        size_t          offset     = 2 * triIndex * elementSize;
        const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(pData + offset);
        v0                         = static_cast<uint32_t>(pIndexData[0]);
        v1                         = static_cast<uint32_t>(pIndexData[1]);
    }
    else if (mIndexType == grfx::INDEX_TYPE_UINT32) {
        size_t          offset     = 2 * triIndex * elementSize;
        const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(pData + offset);
        v0                         = static_cast<uint32_t>(pIndexData[0]);
        v1                         = static_cast<uint32_t>(pIndexData[1]);
    }

    return ppx::SUCCESS;
}

Result WireMesh::GetVertexData(uint32_t vtxIndex, WireMeshVertexData* pVertexData) const
{
    uint32_t vertexCount = GetCountPositions();
    if (vtxIndex >= vertexCount) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    const float3* pPosition = GetDataPositions(vtxIndex);
    const float3* pColor    = GetDataColors(vtxIndex);

    pVertexData->position = *pPosition;

    if (!IsNull(pColor)) {
        pVertexData->color = *pColor;
    }

    return ppx::SUCCESS;
}

void WireMesh::AppendIndexAndVertexData(
    std::vector<uint32_t>&    indexData,
    const std::vector<float>& vertexData,
    const uint32_t            expectedVertexCount,
    const WireMeshOptions&    options,
    WireMesh&                 mesh)
{
    grfx::IndexType indexType = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;

    // Verify expected vertex count
    size_t vertexCount = (vertexData.size() * sizeof(float)) / sizeof(WireMeshVertexData);
    PPX_ASSERT_MSG(vertexCount == expectedVertexCount, "unexpected vertex count");

    // Get base pointer to vertex data
    const char* pData = reinterpret_cast<const char*>(vertexData.data());

    if (indexType != grfx::INDEX_TYPE_UNDEFINED) {
        for (size_t i = 0; i < vertexCount; ++i) {
            const WireMeshVertexData* pVertexData = reinterpret_cast<const WireMeshVertexData*>(pData + (i * sizeof(WireMeshVertexData)));

            mesh.AppendPosition(pVertexData->position * options.mScale);

            if (options.mEnableVertexColors || options.mEnableObjectColor) {
                float3 color = options.mEnableObjectColor ? options.mObjectColor : pVertexData->color;
                mesh.AppendColor(color);
            }
        }

        size_t edgeCount = indexData.size() / 2;
        for (size_t triIndex = 0; triIndex < edgeCount; ++triIndex) {
            uint32_t v0 = indexData[2 * triIndex + 0];
            uint32_t v1 = indexData[2 * triIndex + 1];
            mesh.AppendEdge(v0, v1);
        }
    }
    else {
        for (size_t i = 0; i < indexData.size(); ++i) {
            uint32_t                  vi          = indexData[i];
            const WireMeshVertexData* pVertexData = reinterpret_cast<const WireMeshVertexData*>(pData + (vi * sizeof(WireMeshVertexData)));

            mesh.AppendPosition(pVertexData->position * options.mScale);

            if (options.mEnableVertexColors) {
                mesh.AppendColor(pVertexData->color);
            }
        }
    }
}

WireMesh WireMesh::CreatePlane(WireMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options)
{
    const float    hs     = size.x / 2.0f;
    const float    ht     = size.y / 2.0f;
    const float    ds     = size.x / static_cast<float>(usegs);
    const float    dt     = size.y / static_cast<float>(vsegs);
    const uint32_t uverts = usegs + 1;
    const uint32_t vverts = vsegs + 1;

    std::vector<float>    vertexData;
    std::vector<uint32_t> indexData;
    uint32_t              indexCount = 0;
    // U segemnts
    for (uint32_t i = 0; i < uverts; ++i) {
        float s  = i * ds / size.x;
        float t0 = 0;
        float t1 = 1;

        float3 position0 = float3(0);
        float3 position1 = float3(0);
        switch (plane) {
            default: {
                PPX_ASSERT_MSG(false, "unknown plane orientation");
            } break;

            // case WIRE_MESH_PLANE_POSITIVE_X: {
            // } break;
            //
            // case WIRE_MESH_PLANE_NEGATIVE_X: {
            // } break;
            //
            case WIRE_MESH_PLANE_POSITIVE_Y: {
                position0 = float3(s * size.x - hs, 0, t0 * size.y - ht);
                position1 = float3(s * size.x - hs, 0, t1 * size.y - ht);
            } break;

            case WIRE_MESH_PLANE_NEGATIVE_Y: {
                position0 = float3((1.0f - s) * size.x - hs, 0, (1.0f - t0) * size.y - ht);
                position1 = float3((1.0f - s) * size.x - hs, 0, (1.0f - t1) * size.y - ht);
            } break;

                // case WIRE_MESH_PLANE_POSITIVE_Z: {
                // } break;
                //
                // case WIRE_MESH_PLANE_NEGATIVE_Z: {
                // } break;
        }

        float3 color0 = float3(s, 0, 0);
        float3 color1 = float3(s, 1, 0);

        vertexData.push_back(position0.x);
        vertexData.push_back(position0.y);
        vertexData.push_back(position0.z);
        vertexData.push_back(color0.r);
        vertexData.push_back(color0.g);
        vertexData.push_back(color0.b);

        vertexData.push_back(position1.x);
        vertexData.push_back(position1.y);
        vertexData.push_back(position1.z);
        vertexData.push_back(color1.r);
        vertexData.push_back(color1.g);
        vertexData.push_back(color1.b);

        indexData.push_back(indexCount);
        indexCount += 1;
        indexData.push_back(indexCount);
        indexCount += 1;
    }
    // V segemnts
    for (uint32_t j = 0; j < vverts; ++j) {
        float s0 = 0;
        float s1 = 1;
        float t  = j * dt / size.y;

        float3 position0 = float3(0);
        float3 position1 = float3(0);
        switch (plane) {
            default: {
                PPX_ASSERT_MSG(false, "unknown plane orientation");
            } break;

            // case WIRE_MESH_PLANE_POSITIVE_X: {
            // } break;
            //
            // case WIRE_MESH_PLANE_NEGATIVE_X: {
            // } break;
            //
            case WIRE_MESH_PLANE_POSITIVE_Y: {
                position0 = float3(s0 * size.x - hs, 0, t * size.y - ht);
                position1 = float3(s1 * size.x - hs, 0, t * size.y - ht);
            } break;

            case WIRE_MESH_PLANE_NEGATIVE_Y: {
                position0 = float3((1.0f - s0) * size.x - hs, 0, (1.0f - t) * size.y - ht);
                position1 = float3((1.0f - s1) * size.x - hs, 0, (1.0f - t) * size.y - ht);
            } break;

                // case WIRE_MESH_PLANE_POSITIVE_Z: {
                // } break;
                //
                // case WIRE_MESH_PLANE_NEGATIVE_Z: {
                // } break;
        }

        float3 color0 = float3(0, t, 0);
        float3 color1 = float3(1, t, 0);

        vertexData.push_back(position0.x);
        vertexData.push_back(position0.y);
        vertexData.push_back(position0.z);
        vertexData.push_back(color0.r);
        vertexData.push_back(color0.g);
        vertexData.push_back(color0.b);

        vertexData.push_back(position1.x);
        vertexData.push_back(position1.y);
        vertexData.push_back(position1.z);
        vertexData.push_back(color1.r);
        vertexData.push_back(color1.g);
        vertexData.push_back(color1.b);

        indexData.push_back(indexCount);
        indexCount += 1;
        indexData.push_back(indexCount);
        indexCount += 1;
    }

    grfx::IndexType indexType = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    WireMesh        mesh      = WireMesh(indexType);

    uint32_t expectedVertexCount = 2 * (uverts + vverts);
    AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

    return mesh;
}

WireMesh WireMesh::CreateCube(const float3& size, const WireMeshOptions& options)
{
    const float hx = size.x / 2.0f;
    const float hy = size.y / 2.0f;
    const float hz = size.z / 2.0f;

    // clang-format off
    std::vector<float> vertexData = {  
        // position      // vertex colors    
         hx,  hy, -hz,    1.0f, 0.0f, 0.0f,  //  0  -Z side
         hx, -hy, -hz,    1.0f, 0.0f, 0.0f,  //  1
        -hx, -hy, -hz,    1.0f, 0.0f, 0.0f,  //  2
        -hx,  hy, -hz,    1.0f, 0.0f, 0.0f,  //  3
                                             
        -hx,  hy,  hz,    0.0f, 1.0f, 0.0f,  //  4  +Z side
        -hx, -hy,  hz,    0.0f, 1.0f, 0.0f,  //  5
         hx, -hy,  hz,    0.0f, 1.0f, 0.0f,  //  6
         hx,  hy,  hz,    0.0f, 1.0f, 0.0f,  //  7
                                             
        -hx,  hy, -hz,   -0.0f, 0.0f, 1.0f,  //  8  -X side
        -hx, -hy, -hz,   -0.0f, 0.0f, 1.0f,  //  9
        -hx, -hy,  hz,   -0.0f, 0.0f, 1.0f,  // 10
        -hx,  hy,  hz,   -0.0f, 0.0f, 1.0f,  // 11
                                             
         hx,  hy,  hz,    1.0f, 1.0f, 0.0f,  // 12  +X side
         hx, -hy,  hz,    1.0f, 1.0f, 0.0f,  // 13
         hx, -hy, -hz,    1.0f, 1.0f, 0.0f,  // 14
         hx,  hy, -hz,    1.0f, 1.0f, 0.0f,  // 15
                                             
        -hx, -hy,  hz,    1.0f, 0.0f, 1.0f,  // 16  -Y side
        -hx, -hy, -hz,    1.0f, 0.0f, 1.0f,  // 17
         hx, -hy, -hz,    1.0f, 0.0f, 1.0f,  // 18
         hx, -hy,  hz,    1.0f, 0.0f, 1.0f,  // 19
                                             
        -hx,  hy, -hz,    0.0f, 1.0f, 1.0f,  // 20  +Y side
        -hx,  hy,  hz,    0.0f, 1.0f, 1.0f,  // 21
         hx,  hy,  hz,    0.0f, 1.0f, 1.0f,  // 22
         hx,  hy, -hz,    0.0f, 1.0f, 1.0f,  // 23
    };

    std::vector<uint32_t> indexData = {
        0,  1, // -Z side
        1,  2,
        2,  3,
        3,  0,

        4,  5, // +Z side
        5,  6,
        6,  7,
        7,  4,

        8,  9, // -X side
        9, 10,
       10, 11,
       11,  8,

        12, 13, // +X side
        13, 14,
        14, 15,
        15, 12,

        16, 17, // -X side
        17, 18,
        18, 19,
        19, 16,

        20, 21, // +X side
        21, 22,
        22, 23,
        23, 20,
    };
    // clang-format on

    grfx::IndexType indexType = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    WireMesh        mesh      = WireMesh(indexType);

    AppendIndexAndVertexData(indexData, vertexData, 24, options, mesh);

    return mesh;
}

WireMesh WireMesh::CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options)
{
    constexpr float kPi    = glm::pi<float>();
    constexpr float kTwoPi = 2.0f * kPi;

    const uint32_t uverts = usegs + 1;
    const uint32_t vverts = vsegs + 1;

    float dt = kTwoPi / static_cast<float>(usegs);
    float dp = kPi / static_cast<float>(vsegs);

    std::vector<float>    vertexData;
    std::vector<uint32_t> indexData;
    uint32_t              indexCount = 0;
    // U segemnts
    for (uint32_t j = 1; j < (vverts - 1); ++j) {
        for (uint32_t i = 1; i < uverts; ++i) {
            float  theta0    = (i - 1) * dt;
            float  theta1    = (i - 0) * dt;
            float  phi       = j * dp;
            float  u0        = theta0 / kTwoPi;
            float  u1        = theta1 / kTwoPi;
            float  v         = phi / kPi;
            float3 P0        = SphericalToCartesian(theta0, phi);
            float3 P1        = SphericalToCartesian(theta1, phi);
            float3 position0 = radius * P0;
            float3 position1 = radius * P1;
            float3 color0    = float3(u0, v, 0);
            float3 color1    = float3(u1, v, 0);

            vertexData.push_back(position0.x);
            vertexData.push_back(position0.y);
            vertexData.push_back(position0.z);
            vertexData.push_back(color0.r);
            vertexData.push_back(color0.g);
            vertexData.push_back(color0.b);

            vertexData.push_back(position1.x);
            vertexData.push_back(position1.y);
            vertexData.push_back(position1.z);
            vertexData.push_back(color1.r);
            vertexData.push_back(color1.g);
            vertexData.push_back(color1.b);

            indexData.push_back(indexCount);
            indexCount += 1;
            indexData.push_back(indexCount);
            indexCount += 1;
        }
    }
    // V segemnts
    for (uint32_t i = 0; i < (uverts - 1); ++i) {
        for (uint32_t j = 1; j < vverts; ++j) {
            float  theta     = i * dt;
            float  phi0      = (j - 0) * dp;
            float  phi1      = (j - 1) * dp;
            float  u         = theta / kTwoPi;
            float  v0        = phi0 / kPi;
            float  v1        = phi1 / kPi;
            float3 P0        = SphericalToCartesian(theta, phi0);
            float3 P1        = SphericalToCartesian(theta, phi1);
            float3 position0 = radius * P0;
            float3 position1 = radius * P1;
            float3 color0    = float3(u, v0, 0);
            float3 color1    = float3(u, v1, 0);

            vertexData.push_back(position0.x);
            vertexData.push_back(position0.y);
            vertexData.push_back(position0.z);
            vertexData.push_back(color0.r);
            vertexData.push_back(color0.g);
            vertexData.push_back(color0.b);

            vertexData.push_back(position1.x);
            vertexData.push_back(position1.y);
            vertexData.push_back(position1.z);
            vertexData.push_back(color1.r);
            vertexData.push_back(color1.g);
            vertexData.push_back(color1.b);

            indexData.push_back(indexCount);
            indexCount += 1;
            indexData.push_back(indexCount);
            indexCount += 1;
        }
    }

    grfx::IndexType indexType = options.mEnableIndices ? grfx::INDEX_TYPE_UINT32 : grfx::INDEX_TYPE_UNDEFINED;
    WireMesh        mesh      = WireMesh(indexType);

    uint32_t expectedVertexCountU = (uverts - 1) * (vverts - 2);
    uint32_t expectedVertexCountV = (uverts - 1) * (vverts - 1);
    uint32_t expectedVertexCount  = 2 * (expectedVertexCountU + expectedVertexCountV);
    AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

    return mesh;
}

} // namespace ppx
