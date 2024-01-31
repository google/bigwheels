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

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_shading_rate.h"

namespace ppx {
namespace grfx {
namespace vk {

TEST(VRSShadingRateEncoderTest, Only1x1)
{
    ShadingRateCapabilities capabilities = {SHADING_RATE_VRS};
    capabilities.vrs.supportedRateCount  = 1;
    capabilities.vrs.supportedRates[0]   = {1, 1};

    internal::VRSShadingRateEncoder encoder;
    encoder.Initialize(capabilities);

    EXPECT_EQ(encoder.EncodeFragmentSize(1, 1), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(1, 2), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(1, 4), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 1), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 2), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 4), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 1), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 2), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 4), 0);
}

TEST(VRSShadingRateEncoderTest, MultipleSizes)
{
    ShadingRateCapabilities capabilities = {SHADING_RATE_VRS};
    capabilities.vrs.supportedRateCount  = 3;
    capabilities.vrs.supportedRates[0]   = {1, 2};
    capabilities.vrs.supportedRates[1]   = {2, 1};
    capabilities.vrs.supportedRates[2]   = {1, 1};

    internal::VRSShadingRateEncoder encoder;
    encoder.Initialize(capabilities);

    EXPECT_EQ(encoder.EncodeFragmentSize(1, 1), 0);
    EXPECT_EQ(encoder.EncodeFragmentSize(1, 2), 1);
    EXPECT_EQ(encoder.EncodeFragmentSize(1, 4), 1);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 1), 4);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 2), 4);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 4), 4);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 1), 4);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 2), 4);
    EXPECT_EQ(encoder.EncodeFragmentSize(4, 4), 4);
}

TEST(VRSShadingRateEncoderTest, EncodeFragmentDensity)
{
    ShadingRateCapabilities capabilities = {SHADING_RATE_VRS};
    capabilities.vrs.supportedRateCount  = 3;
    capabilities.vrs.supportedRates[0]   = {1, 2};
    capabilities.vrs.supportedRates[1]   = {2, 1};
    capabilities.vrs.supportedRates[2]   = {1, 1};

    internal::VRSShadingRateEncoder encoder;
    encoder.Initialize(capabilities);

    EXPECT_EQ(encoder.EncodeFragmentDensity(255, 255), 0);
    EXPECT_EQ(encoder.EncodeFragmentDensity(255, 127), 1);
    EXPECT_EQ(encoder.EncodeFragmentDensity(127, 255), 4);
    EXPECT_EQ(encoder.EncodeFragmentDensity(127, 127), 4);
}

TEST(FDMShadingRateEncoderTest, EncodeFragmentSize)
{
    internal::FDMShadingRateEncoder encoder;

    EXPECT_EQ(encoder.EncodeFragmentSize(1, 1), 0xFFFF);
    EXPECT_EQ(encoder.EncodeFragmentSize(1, 2), 0x7FFF);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 1), 0xFF7F);
    EXPECT_EQ(encoder.EncodeFragmentSize(2, 2), 0x7F7F);
}

TEST(FDMShadingRateEncoderTest, EncodeFragmentDensity)
{
    internal::FDMShadingRateEncoder encoder;

    EXPECT_EQ(encoder.EncodeFragmentDensity(255, 255), 0xFFFF);
    EXPECT_EQ(encoder.EncodeFragmentDensity(255, 127), 0x7FFF);
    EXPECT_EQ(encoder.EncodeFragmentDensity(127, 255), 0xFF7F);
    EXPECT_EQ(encoder.EncodeFragmentDensity(127, 127), 0x7F7F);
}

} // namespace vk
} // namespace grfx
} // namespace ppx
