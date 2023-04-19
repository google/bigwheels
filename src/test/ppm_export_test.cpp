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

#include "ppx/ppm_export.h"

#include <optional>
#include <sstream>

class PPMData
{
public:
    static std::optional<PPMData> FromStream(std::stringstream&& stream)
    {
        std::string magic;
        stream >> magic;
        if (magic != "P6") {
            return std::nullopt;
        }

        uint32_t width = 0, height = 0, maxTexelValue = 0;
        stream >> width >> height >> maxTexelValue;
        stream.ignore(1);
        if (!stream) {
            return std::nullopt;
        }

        std::vector<unsigned char> texels(width * height * 3);
        stream.read((char*)texels.data(), texels.size());
        if (!stream) {
            return std::nullopt;
        }

        return PPMData(width, height, maxTexelValue, texels);
    }

    const uint32_t                   width         = 0;
    const uint32_t                   height        = 0;
    const uint32_t                   maxTexelValue = 0;
    const std::vector<unsigned char> texels;

private:
    PPMData(uint32_t width, uint32_t height, uint32_t maxTexelValue, const std::vector<unsigned char>& texels)
        : width(width), height(height), maxTexelValue(maxTexelValue), texels(texels)
    {
    }
};

namespace ppx {

TEST(PPMExport, ExportRGB_UINT)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 22, 33, 44, 55, 66, 77, 88, 99};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_UINT, texels.data(), 3, 2, 9);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 3);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);
    EXPECT_EQ(data->texels, texels);
}

TEST(PPMExport, ExportRGB_UNORM)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 22, 33, 44, 55, 66, 77, 88, 99};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_UINT, texels.data(), 3, 2, 9);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 3);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);
    EXPECT_EQ(data->texels, texels);
}

TEST(PPMExport, ExportRGB_SINT)
{
    std::stringstream buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<signed char> texels = {-128, -127, -126, 0, 1, 2, 127, 126, 125, -1, -2, -3};
    Result            res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_SINT, texels.data(), 2, 2, 6);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {0, 1, 2, 128, 129, 130, 255, 254, 253, 127, 126, 125};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, ExportRGB_SNORM)
{
    std::stringstream buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<signed char> texels = {-128, -127, -126, 0, 1, 2, 127, 126, 125, -1, -2, -3};
    Result            res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_SNORM, texels.data(), 2, 2, 6);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {0, 1, 2, 128, 129, 130, 255, 254, 253, 127, 126, 125};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, ExportBGR)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 22, 33, 44, 55, 66, 77, 88, 99};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_B8G8R8_UINT, texels.data(), 3, 2, 9);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 3);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {2, 1, 0, 5, 4, 3, 8, 7, 6, 33, 22, 11, 66, 55, 44, 99, 88, 77};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, ExportTwoChannels)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 3, 4, 10, 11, 55, 66};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8_UINT, texels.data(), 2, 2, 4);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {0, 1, 0, 3, 4, 0, 10, 11, 0, 55, 66, 0};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, ExportOneChannel)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {1, 3, 10, 55};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8_UINT, texels.data(), 2, 2, 2);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {1, 0, 0, 3, 0, 0, 10, 0, 0, 55, 0, 0};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, Export1DImage)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 22, 33, 44, 55, 66, 77, 88, 99};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_UINT, texels.data(), 6, 1, 18);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 6);
    EXPECT_EQ(data->height, 1);
    EXPECT_EQ(data->maxTexelValue, 255);
    EXPECT_EQ(data->texels, texels);
}

TEST(PPMExport, AlphaIsIgnored)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 255, 3, 4, 5, 255, 10, 11, 12, 255, 55, 66, 77, 255};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8A8_UINT, texels.data(), 2, 2, 8);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {0, 1, 2, 3, 4, 5, 10, 11, 12, 55, 66, 77};
    EXPECT_EQ(data->texels, wantTexels);
}

TEST(PPMExport, RowStrideLargerThanRowBytes)
{
    std::stringstream          buffer(std::stringstream::out | std::stringstream::in | std::ios::binary);
    std::vector<unsigned char> texels = {0, 1, 2, 3, 4, 5, 255, 255, 255, 255, 10, 11, 12, 55, 66, 77, 255, 255, 255, 255};
    Result                     res    = ExportToPPM(buffer, grfx::FORMAT_R8G8B8_UINT, texels.data(), 2, 2, 10);
    EXPECT_EQ(res, 0);

    auto data = PPMData::FromStream(std::move(buffer));
    ASSERT_TRUE(data.has_value());

    EXPECT_EQ(data->width, 2);
    EXPECT_EQ(data->height, 2);
    EXPECT_EQ(data->maxTexelValue, 255);

    std::vector<unsigned char> wantTexels = {0, 1, 2, 3, 4, 5, 10, 11, 12, 55, 66, 77};
    EXPECT_EQ(data->texels, wantTexels);
}

// Errors and unsupported formats.
TEST(PPMExport, InvalidSize)
{
    std::stringstream buffer(std::ios::binary);
    const int16_t     texels[] = {0, 1, 2, 3};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_R16G16B16A16_UINT, texels, 0, 1, 8);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_INVALID_SIZE);

    res = ExportToPPM(buffer, grfx::FORMAT_R16G16B16A16_UINT, texels, 1, 0, 8);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_INVALID_SIZE);
}

TEST(PPMExport, LargeBitFormatsNotSupported)
{
    std::stringstream buffer(std::ios::binary);
    const int16_t     texels[] = {0, 1, 2, 3};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_R16G16B16A16_UINT, texels, 1, 1, 8);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);
}

TEST(PPMExport, FloatFormatsNotSupported)
{
    std::stringstream buffer(std::ios::binary);
    const float       texels[] = {0, 1, 2, 3};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_R32G32B32A32_FLOAT, texels, 1, 1, 16);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);
}

TEST(PPMExport, CompressedFormatsNotSupported)
{
    std::stringstream buffer(std::ios::binary);
    const char        texels[] = {0, 1, 2, 3};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_BC1_RGBA_SRGB, texels, 1, 1, 4);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);
}

TEST(PPMExport, PackedFormatsNotSupported)
{
    std::stringstream buffer(std::ios::binary);
    const int16_t     texels[] = {0, 1, 2, 3};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_R10G10B10A2_UNORM, texels, 1, 1, 4);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);
}

TEST(PPMExport, DepthStencilFormatsNotSupported)
{
    std::stringstream buffer(std::ios::binary);
    const float       texels[] = {0};
    Result            res      = ExportToPPM(buffer, grfx::FORMAT_D32_FLOAT, texels, 1, 1, 4);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);

    res = ExportToPPM(buffer, grfx::FORMAT_S8_UINT, texels, 4, 1, 4);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);

    res = ExportToPPM(buffer, grfx::FORMAT_D16_UNORM_S8_UINT, texels, 4, 1, 4);
    EXPECT_EQ(res, ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED);
}

} // namespace ppx
