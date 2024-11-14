// Copyright 2023 Google LLC
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

#include "SphereMesh.h"

#include <random>
#include <numeric>

using namespace ppx;

// =====================================================================
// OrderedGrid
// =====================================================================

OrderedGrid::OrderedGrid(uint32_t count, uint32_t randomSeed)
{
    mSizeX = static_cast<uint32_t>(std::cbrt(count));
    mSizeY = mSizeX;
    mSizeZ = static_cast<uint32_t>(std::ceil(count / static_cast<float>(mSizeX * mSizeY)));
    mStep  = 10.0f;

    mOrderedPointIndices.resize(count);
    std::iota(mOrderedPointIndices.begin(), mOrderedPointIndices.end(), 0);

    // Shuffle using the `mersenne_twister` deterministic random number generator to obtain
    // the same sphere indices for a given `kMaxSphereInstanceCount`.
    Shuffle(mOrderedPointIndices.begin(), mOrderedPointIndices.end(), std::mt19937(randomSeed));
}

float4x4 OrderedGrid::GetModelMatrix(uint32_t sphereIndex, bool isXR) const
{
    uint32_t id = mOrderedPointIndices[sphereIndex];
    float    x  = static_cast<float>((id % (mSizeX * mSizeY)) / mSizeY);
    float    y  = static_cast<float>(id % mSizeY);
    float    z  = static_cast<float>(id / (mSizeX * mSizeY));

    // Put it in the center of the screen.
    x -= static_cast<float>(mSizeX - 1) / 2.0;
    y -= static_cast<float>(mSizeY - 1) / 2.0;
    z += static_cast<float>(mSizeZ);
    if (isXR) {
        z *= -1.0;
    }

    return glm::translate(float3(x * mStep, y * mStep, z * mStep)) * glm::scale(float3(2.f, 2.f, 2.f));
}

// =====================================================================
// SphereMesh
// =====================================================================

void SphereMesh::ApplyGrid(const OrderedGrid& grid)
{
    mLowInterleavedSingleSphere  = {};
    mLowPlanarSingleSphere       = {};
    mHighInterleavedSingleSphere = {};
    mHighPlanarSingleSphere      = {};
    mLowInterleaved              = {};
    mLowPlanar                   = {};
    mHighInterleaved             = {};
    mHighPlanar                  = {};

    mSphereCount = grid.GetCount();

    CreateAllGeometries();
    PopulateSingleSpheres();
    PrepareFullGeometries();

    // Iterate through the spheres to adjust data unique to each sphere
    for (uint32_t sphereIndex = 0; sphereIndex < mSphereCount; sphereIndex++) {
        WriteSpherePosition(grid, sphereIndex);
        AppendSphereIndicesToInterleaved(sphereIndex);
    }

    // These planar index buffers are the same as the interleaved ones
    mLowPlanar.SetIndexBuffer(*(mLowInterleaved.GetIndexBuffer()));
    mHighPlanar.SetIndexBuffer(*(mHighInterleaved.GetIndexBuffer()));
}

void SphereMesh::CreateAllGeometries()
{
    // vertexBinding[0] = {stride = 18, attributeCount = 4} // position, texCoord, normal, tangent
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_LOW_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, &mLowInterleavedSingleSphere);
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_LOW_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, &mLowInterleaved);

    // vertexBinding[0] = {stride =  6, attributeCount = 1} // position
    // vertexBinding[1] = {stride = 12, attributeCount = 3} // texCoord, normal, tangent
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_LOW_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, &mLowPlanarSingleSphere);
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_LOW_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, &mLowPlanar);

    // vertexBinding[0] = {stride = 48, attributeCount = 4} // position, texCoord, normal, tangent
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_HIGH_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, &mHighInterleavedSingleSphere);
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_HIGH_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, &mHighInterleaved);

    // vertexBinding[0] = {stride = 12, attributeCount = 1} // position
    // vertexBinding[1] = {stride = 36, attributeCount = 3} // texCoord, normal, tangent
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_HIGH_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, &mHighPlanarSingleSphere);
    CreateSphereGeometry(PrecisionType::PRECISION_TYPE_HIGH_PRECISION, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, &mHighPlanar);
}

void SphereMesh::CreateSphereGeometry(PrecisionType precisionType, VertexLayoutType vertexLayoutType, Geometry* geometryPtr)
{
    // Defaults used for all the following:
    // - indexType = INDEX_TYPE_UINT32
    // - primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    // - VertexBinding inputRate = 0
    GeometryCreateInfo geometryCreateInfo;

    if (precisionType == PrecisionType::PRECISION_TYPE_LOW_PRECISION) {
        if (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED) {
            geometryCreateInfo = GeometryCreateInfo::InterleavedU32(grfx::FORMAT_R16G16B16A16_FLOAT)
                                     .AddColor(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddNormal(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddTexCoord(grfx::FORMAT_R16G16_FLOAT)
                                     .AddTangent(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddBitangent(grfx::FORMAT_R8G8B8A8_SNORM);
        }
        else if (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR) {
            geometryCreateInfo = GeometryCreateInfo::PositionPlanarU32(grfx::FORMAT_R16G16B16A16_FLOAT)
                                     .AddColor(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddNormal(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddTexCoord(grfx::FORMAT_R16G16_FLOAT)
                                     .AddTangent(grfx::FORMAT_R8G8B8A8_SNORM)
                                     .AddBitangent(grfx::FORMAT_R8G8B8A8_SNORM);
        }
    }
    else if (precisionType == PrecisionType::PRECISION_TYPE_HIGH_PRECISION) {
        if (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED) {
            geometryCreateInfo = GeometryCreateInfo::InterleavedU32(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddColor(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddNormal(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddTexCoord(grfx::FORMAT_R32G32_FLOAT)
                                     .AddTangent(grfx::FORMAT_R32G32B32A32_FLOAT)
                                     .AddBitangent(grfx::FORMAT_R32G32B32_FLOAT);
        }
        else if (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR) {
            geometryCreateInfo = GeometryCreateInfo::PositionPlanarU32(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddColor(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddNormal(grfx::FORMAT_R32G32B32_FLOAT)
                                     .AddTexCoord(grfx::FORMAT_R32G32_FLOAT)
                                     .AddTangent(grfx::FORMAT_R32G32B32A32_FLOAT)
                                     .AddBitangent(grfx::FORMAT_R32G32B32_FLOAT);
        }
    }
    PPX_ASSERT_MSG(geometryCreateInfo.vertexBindingCount != 0, "Invalid precisionType and/or vertexLayoutType");
    PPX_CHECKED_CALL(Geometry::Create(geometryCreateInfo, geometryPtr));
}

void SphereMesh::PopulateSingleSpheres()
{
    for (uint32_t vertexIndex = 0; vertexIndex < mSingleSphereVertexCount; ++vertexIndex) {
        TriMeshVertexData vertexData = {};
        mSingleSphereMesh.GetVertexData(vertexIndex, &vertexData);
        mHighInterleavedSingleSphere.AppendVertexData(vertexData);
        mHighPlanarSingleSphere.AppendVertexData(vertexData);

        TriMeshVertexDataCompressed vertexDataCompressed = CompressVertexData(vertexData);
        mLowInterleavedSingleSphere.AppendVertexData(vertexDataCompressed);
        mLowPlanarSingleSphere.AppendVertexData(vertexDataCompressed);
    }
}

void SphereMesh::PrepareFullGeometries()
{
    // Copy single sphere vertex buffers into full buffers, since the non-position vertex buffer data is repeated.
    RepeatGeometryNonPositionVertexData(mLowInterleavedSingleSphere, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, mSphereCount, mLowInterleaved);
    RepeatGeometryNonPositionVertexData(mLowPlanarSingleSphere, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, mSphereCount, mLowPlanar);
    RepeatGeometryNonPositionVertexData(mHighInterleavedSingleSphere, VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED, mSphereCount, mHighInterleaved);
    RepeatGeometryNonPositionVertexData(mHighPlanarSingleSphere, VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR, mSphereCount, mHighPlanar);

    // Resize the empty Position Planar vertex buffers for future writes
    uint32_t elementSize = mLowPlanar.GetVertexBuffer(0)->GetElementSize();
    mLowPlanar.GetVertexBuffer(0)->SetSize(mSingleSphereVertexCount * mSphereCount * elementSize);
    elementSize = mHighPlanar.GetVertexBuffer(0)->GetElementSize();
    mHighPlanar.GetVertexBuffer(0)->SetSize(mSingleSphereTriCount * mSphereCount * elementSize);
}

void SphereMesh::RepeatGeometryNonPositionVertexData(const Geometry& srcGeom, VertexLayoutType vertexLayoutType, uint32_t repeatCount, Geometry& dstGeom)
{
    size_t nVertexBufferCount = srcGeom.GetVertexBufferCount();
    PPX_ASSERT_MSG(nVertexBufferCount == dstGeom.GetVertexBufferCount(), "Mismatched source and destination vertex data format");
    bool isValidInterleaved    = (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_INTERLEAVED) && (nVertexBufferCount == 1);
    bool isValidPositionPlanar = (vertexLayoutType == VertexLayoutType::VERTEX_LAYOUT_TYPE_POSITION_PLANAR) && (nVertexBufferCount == 2);
    PPX_ASSERT_MSG(isValidInterleaved || isValidPositionPlanar, "Invalid vertex buffer layout for sphere mesh");

    // If there is one interleaved (1 vb), repeat position data as well
    // For position planar (2 vb), repeat only non-position vertex data, starting from buffer 1
    uint32_t firstBufferToCopy = isValidInterleaved ? 0 : 1;

    for (uint32_t vertexBufferIndex = firstBufferToCopy; vertexBufferIndex < nVertexBufferCount; vertexBufferIndex++) {
        const Geometry::Buffer* srcBufferPtr  = srcGeom.GetVertexBuffer(vertexBufferIndex);
        Geometry::Buffer*       dstBufferPtr  = dstGeom.GetVertexBuffer(vertexBufferIndex);
        uint32_t                srcBufferSize = srcBufferPtr->GetSize();
        uint32_t                dstBufferSize = srcBufferSize * repeatCount;

        dstBufferPtr->SetSize(dstBufferSize);

        size_t offset = 0;
        for (size_t j = 0; j < repeatCount; j++) {
            const void* pSrc = srcBufferPtr->GetData();
            void*       pDst = dstBufferPtr->GetData() + offset;
            memcpy(pDst, pSrc, srcBufferSize);
            offset += srcBufferSize;
        }
    }
}

void SphereMesh::WriteSpherePosition(const OrderedGrid& grid, uint32_t sphereIndex)
{
    float4x4 modelMatrix = grid.GetModelMatrix(sphereIndex, IsXR());

    for (uint32_t j = 0; j < mSingleSphereVertexCount; ++j) {
        TriMeshVertexData vertexData = {};
        mSingleSphereMesh.GetVertexData(j, &vertexData);
        vertexData.position = modelMatrix * float4(vertexData.position, 1);

        TriMeshVertexDataCompressed vertexDataCompressed;
        vertexDataCompressed.position = half4(glm::packHalf1x16(vertexData.position.x), glm::packHalf1x16(vertexData.position.y), glm::packHalf1x16(vertexData.position.z), glm::packHalf1x16(0.0f));

        size_t elementIndex = sphereIndex * mSingleSphereVertexCount + j;
        OverwritePositionData(mLowInterleaved.GetVertexBuffer(0), vertexDataCompressed, elementIndex);
        OverwritePositionData(mLowPlanar.GetVertexBuffer(0), vertexDataCompressed, elementIndex);
        OverwritePositionData(mHighInterleaved.GetVertexBuffer(0), vertexData, elementIndex);
        OverwritePositionData(mHighPlanar.GetVertexBuffer(0), vertexData, elementIndex);
    }
}

void SphereMesh::AppendSphereIndicesToInterleaved(uint32_t sphereIndex)
{
    for (uint32_t k = 0; k < mSingleSphereTriCount; ++k) {
        uint32_t v0 = PPX_VALUE_IGNORED;
        uint32_t v1 = PPX_VALUE_IGNORED;
        uint32_t v2 = PPX_VALUE_IGNORED;
        mSingleSphereMesh.GetTriangle(k, v0, v1, v2);

        // v0/v1/v2 contain the vertex index counting from the beginning of a sphere, so an offset of
        // (sphereIndex * mSingleSphereVertexCount) must be added when considering the full buffer
        uint32_t offset = sphereIndex * mSingleSphereVertexCount;
        mLowInterleaved.AppendIndicesTriangle(offset + v0, offset + v1, offset + v2);
        mHighInterleaved.AppendIndicesTriangle(offset + v0, offset + v1, offset + v2);
    }
}

TriMeshVertexDataCompressed SphereMesh::CompressVertexData(const TriMeshVertexData& vertexData)
{
    TriMeshVertexDataCompressed vertexDataCompressed;

    vertexDataCompressed.position = half4(glm::packHalf1x16(vertexData.position.x), glm::packHalf1x16(vertexData.position.y), glm::packHalf1x16(vertexData.position.z), glm::packHalf1x16(0.0f));
    vertexDataCompressed.color     = i8vec4(MapFloatToInt8(vertexData.color.x), MapFloatToInt8(vertexData.color.y), MapFloatToInt8(vertexData.color.z), MapFloatToInt8(1.0f));
    vertexDataCompressed.normal   = i8vec4(MapFloatToInt8(vertexData.normal.x), MapFloatToInt8(vertexData.normal.y), MapFloatToInt8(vertexData.normal.z), MapFloatToInt8(1.0f));
    vertexDataCompressed.texCoord  = half2(glm::packHalf1x16(vertexData.texCoord.x), glm::packHalf1x16(vertexData.texCoord.y));
    vertexDataCompressed.tangent   = i8vec4(MapFloatToInt8(vertexData.tangent.x), MapFloatToInt8(vertexData.tangent.y), MapFloatToInt8(vertexData.tangent.z), MapFloatToInt8(vertexData.tangent.a));
    vertexDataCompressed.bitangent = i8vec4(MapFloatToInt8(vertexData.bitangent.x), MapFloatToInt8(vertexData.bitangent.y), MapFloatToInt8(vertexData.bitangent.z), MapFloatToInt8(1.0f));

    return vertexDataCompressed;
}

int8_t MapFloatToInt8(float x)
{
    //PPX_ASSERT_MSG(-1.0f <= x && x <= 1.0f, "The value must be between -1.0 and 1.0");
    return static_cast<int8_t>((x + 1.0f) * 127.5f - 128.0f);
}
