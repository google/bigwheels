// Copyright 2026 Google LLC
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ppx/geometry.h"

#include <vector>

namespace ppx {
namespace {

// The death tests will always fail with NDEBUG (i.e. Release builds).
// PPX_ASSERT_MSG relies on assert() for death but assert is a no-op with NDEBUG.
#if !defined(NDEBUG)
#define PERFORM_DEATH_TESTS
#endif

using ::testing::ElementsAreArray;
using ::testing::IsNull;
using ::testing::NotNull;

std::vector<uint8_t> GetTriMeshIndexDataU8(const TriMesh& mesh)
{
    const uint8_t* pData = mesh.GetDataIndicesU8();
    if (pData == nullptr) {
        return {};
    }
    return std::vector<uint8_t>(pData, pData + mesh.GetCountIndices());
}

std::vector<uint8_t> GetWireMeshIndexDataU8(const WireMesh& mesh)
{
    const uint8_t* pData = mesh.GetDataIndicesU8();
    if (pData == nullptr) {
        return {};
    }
    return std::vector<uint8_t>(pData, pData + mesh.GetCountIndices());
}

template <typename T>
void ExpectIndexBufferEq(const Geometry& geometry, const std::vector<T>& expected)
{
    const Geometry::Buffer* indexBuffer = geometry.GetIndexBuffer();
    ASSERT_THAT(indexBuffer, NotNull());

    EXPECT_EQ(indexBuffer->GetElementSize(), sizeof(T));
    EXPECT_EQ(indexBuffer->GetElementCount(), static_cast<uint32_t>(expected.size()));
    EXPECT_EQ(indexBuffer->GetSize(), static_cast<uint32_t>(sizeof(T) * expected.size()));

    const char* pData = indexBuffer->GetData();
    ASSERT_THAT(pData, NotNull());
    const T* pIndexData = reinterpret_cast<const T*>(pData);
    EXPECT_THAT(std::vector<T>(pIndexData, pIndexData + expected.size()), ElementsAreArray(expected));
}

TEST(TriMeshUint8IndexTest, AppendTrianglePacksDataAsUint8)
{
    TriMesh mesh(grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(mesh.GetIndexType(), grfx::INDEX_TYPE_UINT8);

    EXPECT_EQ(mesh.AppendTriangle(0, 1, 2), 1u);
    EXPECT_EQ(mesh.AppendTriangle(2, 3, 4), 2u);

    EXPECT_EQ(mesh.GetCountTriangles(), 2u);
    EXPECT_EQ(mesh.GetCountIndices(), 6u);
    EXPECT_EQ(mesh.GetDataSizeIndices(), 6u);
    EXPECT_THAT(mesh.GetDataIndicesU8(), NotNull());
    EXPECT_THAT(mesh.GetDataIndicesU16(), IsNull());
    EXPECT_THAT(mesh.GetDataIndicesU32(), IsNull());
    EXPECT_THAT(mesh.GetDataIndicesU8(6), IsNull());
    const std::vector<uint8_t> expected = {0, 1, 2, 2, 3, 4};
    EXPECT_THAT(GetTriMeshIndexDataU8(mesh), ElementsAreArray(expected));

    uint32_t v0 = PPX_VALUE_IGNORED;
    uint32_t v1 = PPX_VALUE_IGNORED;
    uint32_t v2 = PPX_VALUE_IGNORED;
    EXPECT_EQ(mesh.GetTriangle(1, v0, v1, v2), ppx::SUCCESS);
    EXPECT_EQ(v0, 2u);
    EXPECT_EQ(v1, 3u);
    EXPECT_EQ(v2, 4u);
}

TEST(WireMeshUint8IndexTest, AppendEdgePacksDataAsUint8)
{
    WireMesh mesh(grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(mesh.GetIndexType(), grfx::INDEX_TYPE_UINT8);

    EXPECT_EQ(mesh.AppendEdge(0, 1), 1u);
    EXPECT_EQ(mesh.AppendEdge(1, 2), 2u);

    EXPECT_EQ(mesh.GetCountEdges(), 2u);
    EXPECT_EQ(mesh.GetCountIndices(), 4u);
    EXPECT_EQ(mesh.GetDataSizeIndices(), 4u);
    EXPECT_THAT(mesh.GetDataIndicesU8(), NotNull());
    EXPECT_THAT(mesh.GetDataIndicesU16(), IsNull());
    EXPECT_THAT(mesh.GetDataIndicesU32(), IsNull());
    EXPECT_THAT(mesh.GetDataIndicesU8(4), IsNull());
    const std::vector<uint8_t> expected = {0, 1, 1, 2};
    EXPECT_THAT(GetWireMeshIndexDataU8(mesh), ElementsAreArray(expected));

    uint32_t v0 = PPX_VALUE_IGNORED;
    uint32_t v1 = PPX_VALUE_IGNORED;
    EXPECT_EQ(mesh.GetEdge(1, v0, v1), ppx::SUCCESS);
    EXPECT_EQ(v0, 1u);
    EXPECT_EQ(v1, 2u);
}

TEST(TriMeshUint8IndexTest, CreateCubeCanUseUint8Indices)
{
    TriMesh mesh = TriMesh::CreateCube(float3(2.0f), TriMeshOptions().IndexType(grfx::INDEX_TYPE_UINT8).VertexColors());

    EXPECT_EQ(mesh.GetIndexType(), grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(mesh.GetCountPositions(), 24u);
    EXPECT_EQ(mesh.GetCountColors(), 24u);
    EXPECT_EQ(mesh.GetCountTriangles(), 12u);
    EXPECT_EQ(mesh.GetCountIndices(), 36u);
    EXPECT_EQ(mesh.GetDataSizeIndices(), 36u);
    EXPECT_THAT(mesh.GetDataIndicesU8(), NotNull());

    uint32_t v0 = PPX_VALUE_IGNORED;
    uint32_t v1 = PPX_VALUE_IGNORED;
    uint32_t v2 = PPX_VALUE_IGNORED;
    EXPECT_EQ(mesh.GetTriangle(0, v0, v1, v2), ppx::SUCCESS);
    EXPECT_EQ(v0, 0u);
    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(v2, 2u);
    EXPECT_EQ(mesh.GetTriangle(11, v0, v1, v2), ppx::SUCCESS);
    EXPECT_EQ(v0, 20u);
    EXPECT_EQ(v1, 22u);
    EXPECT_EQ(v2, 23u);
}

TEST(WireMeshUint8IndexTest, CreateCubeCanUseUint8Indices)
{
    WireMesh mesh = WireMesh::CreateCube(float3(2.0f), WireMeshOptions().IndexType(grfx::INDEX_TYPE_UINT8).VertexColors());

    EXPECT_EQ(mesh.GetIndexType(), grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(mesh.GetCountPositions(), 24u);
    EXPECT_EQ(mesh.GetCountColors(), 24u);
    EXPECT_EQ(mesh.GetCountEdges(), 24u);
    EXPECT_EQ(mesh.GetCountIndices(), 48u);
    EXPECT_EQ(mesh.GetDataSizeIndices(), 48u);
    EXPECT_THAT(mesh.GetDataIndicesU8(), NotNull());

    uint32_t v0 = PPX_VALUE_IGNORED;
    uint32_t v1 = PPX_VALUE_IGNORED;
    EXPECT_EQ(mesh.GetEdge(0, v0, v1), ppx::SUCCESS);
    EXPECT_EQ(v0, 0u);
    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(mesh.GetEdge(23, v0, v1), ppx::SUCCESS);
    EXPECT_EQ(v0, 23u);
    EXPECT_EQ(v1, 20u);
}

TEST(TriMeshUint8IndexTest, GeometryCreatePreservesUint8IndexBuffer)
{
    TriMesh mesh = TriMesh::CreateCube(float3(2.0f), TriMeshOptions().IndexType(grfx::INDEX_TYPE_UINT8).VertexColors());

    Geometry geometry;
    EXPECT_EQ(Geometry::Create(mesh, &geometry), ppx::SUCCESS);

    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(geometry.GetIndexCount(), mesh.GetCountIndices());
    ExpectIndexBufferEq(geometry, GetTriMeshIndexDataU8(mesh));
}

TEST(WireMeshUint8IndexTest, GeometryCreatePreservesUint8IndexBuffer)
{
    WireMesh mesh = WireMesh::CreateCube(float3(2.0f), WireMeshOptions().IndexType(grfx::INDEX_TYPE_UINT8).VertexColors());

    Geometry geometry;
    EXPECT_EQ(Geometry::Create(mesh, &geometry), ppx::SUCCESS);

    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT8);
    EXPECT_EQ(geometry.GetIndexCount(), mesh.GetCountIndices());
    ExpectIndexBufferEq(geometry, GetWireMeshIndexDataU8(mesh));
}

#if defined(PERFORM_DEATH_TESTS)
TEST(TriMeshUint8IndexDeathTest, AppendTriangleDiesIfIndexIsOutOfRange)
{
    TriMesh mesh(grfx::INDEX_TYPE_UINT8);
    ASSERT_DEATH(mesh.AppendTriangle(0, 1, 256), "");
}

TEST(WireMeshUint8IndexDeathTest, AppendEdgeDiesIfIndexIsOutOfRange)
{
    WireMesh mesh(grfx::INDEX_TYPE_UINT8);
    ASSERT_DEATH(mesh.AppendEdge(0, 256), "");
}
#endif

} // namespace
} // namespace ppx
