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

std::ostream& operator<<(std::ostream& o, const grfx::IndexType& indexType)
{
    o << ToString(indexType);
    return o;
}

namespace {

using ::testing::IsNull;
using ::testing::NotNull;

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
    EXPECT_EQ(indexBufferData[0], 0);
    EXPECT_EQ(indexBufferData[1], 1);
    EXPECT_EQ(indexBufferData[2], 2);
}

class GeometryTestWithIndexTypeParam : public testing::TestWithParam<grfx::IndexType>
{
};

using GeometryDeathTest = GeometryTestWithIndexTypeParam;

TEST_P(GeometryDeathTest, AppendIndicesU32DiesIfIndexTypeIsNotU32)
{
    grfx::IndexType indexType = GetParam();
    Geometry        geometry;
    EXPECT_EQ(Geometry::Create(GeometryCreateInfo{}.IndexType(indexType).AddPosition(), &geometry), ppx::SUCCESS);
    EXPECT_NE(geometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    const std::array<uint32_t, 3> indices = {0, 1, 2};
    ASSERT_DEATH(geometry.AppendIndicesU32(indices.size(), indices.data()), "AppendIndicesU32");
}

INSTANTIATE_TEST_SUITE_P(GeometryDeathTest, GeometryDeathTest, testing::Values(grfx::INDEX_TYPE_UINT16, grfx::INDEX_TYPE_UNDEFINED));

} // namespace
} // namespace ppx
