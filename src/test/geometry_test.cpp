// Copyright 2024 Google LLC
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
#include "ppx/grfx/grfx_util.h"

namespace ppx {
namespace {

// The death tests will always fail with NDEBUG (i.e. Release builds).
// PPX_ASSERT_MSG relies on assert() for death but assert is a no-op with NDEBUG.
#if !defined(NDEBUG)
#define PERFORM_DEATH_TESTS
#endif

using ::testing::AllOf;
using ::testing::ElementsAreArray;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Property;

MATCHER(BufferIsEmpty, "")
{
    EXPECT_EQ(arg.GetElementSize(), 0);
    EXPECT_EQ(arg.GetElementCount(), 0);
    EXPECT_EQ(arg.GetSize(), 0);
    EXPECT_THAT(arg.GetData(), IsNull());
    return true;
}

MATCHER_P(BufferEq, expected, "")
{
    constexpr auto valueTypeSize = sizeof(typename decltype(expected)::value_type);
    EXPECT_EQ(arg.GetElementSize(), valueTypeSize);
    EXPECT_EQ(arg.GetElementCount(), expected.size());
    EXPECT_EQ(arg.GetSize(), valueTypeSize * expected.size());
    if (arg.GetData() == nullptr) {
        return false;
    }
    const auto* data = reinterpret_cast<const typename decltype(expected)::value_type*>(arg.GetData());
    EXPECT_THAT(std::vector<typename decltype(expected)::value_type>(data, data + arg.GetElementCount()), ElementsAreArray(expected));
    return true;
}

TEST(GeometryTest, AppendIndicesU32PacksDataAsUint32)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT32).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    const std::array<uint32_t, 3> indices = {0, 1, 2};
    geometry.AppendIndicesU32(indices.size(), indices.data());
    EXPECT_EQ(geometry.GetIndexCount(), 3);

    const Geometry::Buffer* indexBuffer = geometry.GetIndexBuffer();
    ASSERT_THAT(indexBuffer, NotNull());
    EXPECT_EQ(indexBuffer->GetElementSize(), sizeof(uint32_t));
    EXPECT_EQ(indexBuffer->GetElementCount(), 3);
    EXPECT_EQ(indexBuffer->GetSize(), 12);

    const uint32_t* indexBufferData = reinterpret_cast<const uint32_t*>(indexBuffer->GetData());
    ASSERT_THAT(indexBufferData, NotNull());
    EXPECT_EQ(indexBufferData[0], 0);
    EXPECT_EQ(indexBufferData[1], 1);
    EXPECT_EQ(indexBufferData[2], 2);
}

class GeometryTestWithIndexTypeParam : public testing::TestWithParam<grfx::IndexType>
{
};

#if defined(PERFORM_DEATH_TESTS)
using GeometryDeathTest = GeometryTestWithIndexTypeParam;

TEST_P(GeometryDeathTest, AppendIndicesU32DiesIfIndexTypeIsNotU32)
{
    grfx::IndexType indexType = GetParam();
    Geometry        geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(indexType).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_NE(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    const std::array<uint32_t, 3> indices = {0, 1, 2};
    ASSERT_DEATH(geometry.AppendIndicesU32(indices.size(), indices.data()), "");
}

INSTANTIATE_TEST_SUITE_P(GeometryDeathTest, GeometryDeathTest, testing::Values(grfx::INDEX_TYPE_UINT16, grfx::INDEX_TYPE_UNDEFINED), [](const testing::TestParamInfo<GeometryDeathTest::ParamType>& info) {
    return ToString(info.param);
});
#endif

TEST(GeometryUint16IndexTest, AppendIndexPacksDataAsUint16)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT16).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT16);

    geometry.AppendIndex(0);
    geometry.AppendIndex(1);
    geometry.AppendIndex(2);
    EXPECT_EQ(geometry.GetIndexCount(), 3);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint16_t>({0, 1, 2})));
}

TEST(GeometryUint16IndexTest, AppendIndicesTrianglePacksDataAsUint16)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT16).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT16);

    geometry.AppendIndicesTriangle(0, 1, 2);
    EXPECT_EQ(geometry.GetIndexCount(), 3);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint16_t>({0, 1, 2})));
}

TEST(GeometryUint16IndexTest, AppendIndicesEdgePacksDataAsUint16)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT16).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT16);

    geometry.AppendIndicesEdge(0, 1);
    EXPECT_EQ(geometry.GetIndexCount(), 2);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint16_t>({0, 1})));
}

TEST(GeometryUint32IndexTest, AppendIndexPacksDataAsUint32)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT32).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    geometry.AppendIndex(0);
    geometry.AppendIndex(1);
    geometry.AppendIndex(2);
    EXPECT_EQ(geometry.GetIndexCount(), 3);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint32_t>({0, 1, 2})));
}

TEST(GeometryUint32IndexTest, AppendIndicesTrianglePacksDataAsUint32)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT32).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    geometry.AppendIndicesTriangle(0, 1, 2);
    EXPECT_EQ(geometry.GetIndexCount(), 3);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint32_t>({0, 1, 2})));
}

TEST(GeometryUint32IndexTest, AppendIndicesEdgePacksDataAsUint32)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UINT32).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    geometry.AppendIndicesEdge(0, 1);
    EXPECT_EQ(geometry.GetIndexCount(), 2);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferEq(std::vector<uint32_t>({0, 1})));
}

TEST(GeometryUndefinedIndexTest, AppendIndexDoesNothing)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UNDEFINED).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UNDEFINED);

    geometry.AppendIndex(0);
    geometry.AppendIndex(1);
    geometry.AppendIndex(2);
    EXPECT_EQ(geometry.GetIndexCount(), 0);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferIsEmpty());
}

TEST(GeometryUndefinedIndexTest, AppendIndicesTriangleDoesNothing)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UNDEFINED).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UNDEFINED);

    geometry.AppendIndicesTriangle(0, 1, 2);
    EXPECT_EQ(geometry.GetIndexCount(), 0);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferIsEmpty());
}

TEST(GeometryUndefinedIndexTest, AppendIndicesEdgeDoesNothing)
{
    Geometry geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(grfx::INDEX_TYPE_UNDEFINED).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_EQ(geometry.GetIndexType(), grfx::INDEX_TYPE_UNDEFINED);

    geometry.AppendIndicesEdge(0, 1);
    EXPECT_EQ(geometry.GetIndexCount(), 0);
    ASSERT_THAT(geometry.GetIndexBuffer(), NotNull());
    EXPECT_THAT(*geometry.GetIndexBuffer(), BufferIsEmpty());
}

} // namespace
} // namespace ppx
