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

const char* ToString(GeometryVertexAttributeLayout value)
{
    switch (value) {
        default: break;
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED: return "GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED";
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR: return "GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR";
        case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR: return "GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR";
    }

    return "<unknown GeometryVertexAttributeLayout>";
}

std::ostream& operator<<(std::ostream& o, const GeometryCreateInfo& info)
{
    o << "GeometryCreateInfo{" << ToString(info.indexType) << ", " << ToString(info.vertexAttributeLayout) << "}";
    return o;
}

namespace {

using ::testing::IsNull;
using ::testing::NotNull;

class GeometryTestWithGeometryCreateInfoParam : public testing::TestWithParam<GeometryCreateInfo>
{
};

using GeometryU32Test = GeometryTestWithGeometryCreateInfoParam;

TEST_P(GeometryU32Test, AppendIndicesU32PacksDataAsUint32)
{
    GeometryCreateInfo geometryCreateInfo = GetParam();
    Geometry           planeGeometry;
    EXPECT_EQ(Geometry::Create(geometryCreateInfo, &planeGeometry), ppx::SUCCESS);
    EXPECT_EQ(planeGeometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    const std::array<uint32_t, 3> indices = {0, 1, 2};
    planeGeometry.AppendIndicesU32(indices.size(), indices.data());
    EXPECT_EQ(planeGeometry.GetIndexCount(), 3);

    const Geometry::Buffer* indexBuffer = planeGeometry.GetIndexBuffer();
    ASSERT_THAT(indexBuffer, NotNull());
    EXPECT_EQ(indexBuffer->GetElementSize(), sizeof(uint32_t));
    EXPECT_EQ(indexBuffer->GetElementCount(), 3);
    EXPECT_EQ(indexBuffer->GetSize(), 12);

    const uint32_t* indexBufferData = reinterpret_cast<const uint32_t*>(indexBuffer->GetData());
    EXPECT_EQ(indexBufferData[0], 0);
    EXPECT_EQ(indexBufferData[1], 1);
    EXPECT_EQ(indexBufferData[2], 2);
}

INSTANTIATE_TEST_SUITE_P(GeometryU32Test, GeometryU32Test, testing::Values(GeometryCreateInfo::PlanarU32(), GeometryCreateInfo::PositionPlanarU32(), GeometryCreateInfo::InterleavedU32()));

using GeometryDeathTest = GeometryTestWithGeometryCreateInfoParam;

TEST_P(GeometryDeathTest, AppendIndicesU32DiesIfIndexTypeIsNotU32)
{
    GeometryCreateInfo geometryCreateInfo = GetParam();
    Geometry           planeGeometry;
    EXPECT_EQ(Geometry::Create(geometryCreateInfo, &planeGeometry), ppx::SUCCESS);
    EXPECT_NE(planeGeometry.GetIndexType(), grfx::INDEX_TYPE_UINT32);

    const std::array<uint32_t, 3> indices = {0, 1, 2};
    ASSERT_DEATH(planeGeometry.AppendIndicesU32(indices.size(), indices.data()), "AppendIndicesU32");
}

INSTANTIATE_TEST_SUITE_P(GeometryDeathTest, GeometryDeathTest, testing::Values(GeometryCreateInfo::Planar(), GeometryCreateInfo::PositionPlanar(), GeometryCreateInfo::Interleaved(), GeometryCreateInfo::PlanarU16(), GeometryCreateInfo::PositionPlanarU16(), GeometryCreateInfo::InterleavedU16()));

} // namespace
} // namespace ppx
