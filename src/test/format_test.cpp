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

#include "gtest/gtest.h"

#include "ppx/grfx/grfx_format.h"

namespace ppx {

namespace grfx {

TEST(FormatTest, GetFormatDescriptionLinearFormat)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_B8G8R8A8_UNORM);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_UNORM);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_COLOR);
    EXPECT_EQ(desc->bytesPerTexel, 4);
    EXPECT_EQ(desc->bytesPerComponent, 1);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_LINEAR);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_RED_GREEN_BLUE_ALPHA);
    EXPECT_EQ(desc->componentOffset.red, 2);
    EXPECT_EQ(desc->componentOffset.green, 1);
    EXPECT_EQ(desc->componentOffset.blue, 0);
    EXPECT_EQ(desc->componentOffset.alpha, 3);
}

TEST(FormatTest, GetFormatDescriptionStencilFormat)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_S8_UINT);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_UINT);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_STENCIL);
    EXPECT_EQ(desc->bytesPerTexel, 1);
    EXPECT_EQ(desc->bytesPerComponent, 1);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_LINEAR);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_STENCIL);
    EXPECT_EQ(desc->componentOffset.stencil, 0);
}

TEST(FormatTest, GetFormatDescriptionDepthStencilFormat)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_D16_UNORM_S8_UINT);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_UNORM);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_DEPTH_STENCIL);
    EXPECT_EQ(desc->bytesPerTexel, 3);
    EXPECT_EQ(desc->bytesPerComponent, 2);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_LINEAR);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_DEPTH_STENCIL);
    EXPECT_EQ(desc->componentOffset.depth, 0);
    EXPECT_EQ(desc->componentOffset.stencil, 2);
}

TEST(FormatTest, GetFormatDescriptionCompressedFormat)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_BC1_RGB_SRGB);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_SRGB);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_COLOR);
    EXPECT_EQ(desc->bytesPerTexel, 8);
    EXPECT_EQ(desc->blockWidth, 4);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_COMPRESSED);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_RED_GREEN_BLUE);
}

TEST(FormatTest, GetFormatDescriptionPackedFormat)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_R11G11B10_FLOAT);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_FLOAT);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_COLOR);
    EXPECT_EQ(desc->bytesPerTexel, 4);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_PACKED);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_RED_GREEN_BLUE);
}

TEST(FormatTest, GetFormatDescriptionCompressedBC3Format)
{
    const FormatDesc* desc = GetFormatDescription(FORMAT_BC3_UNORM);
    EXPECT_EQ(desc->dataType, FORMAT_DATA_TYPE_UNORM);
    EXPECT_EQ(desc->aspect, FORMAT_ASPECT_COLOR);
    EXPECT_EQ(desc->bytesPerTexel, 16);
    EXPECT_EQ(desc->blockWidth, 4);
    EXPECT_EQ(desc->layout, FORMAT_LAYOUT_COMPRESSED);
    EXPECT_EQ(desc->componentBits, FORMAT_COMPONENT_RED_GREEN_BLUE_ALPHA);
}

} // namespace grfx
} // namespace ppx
