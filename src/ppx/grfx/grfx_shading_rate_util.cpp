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

#include "ppx/grfx/grfx_shading_rate_util.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace grfx {

void FillShadingRateUniformFragmentSize(ShadingRatePatternPtr pattern, uint32_t fragmentWidth, uint32_t fragmentHeight, Bitmap* bitmap)
{
    FillShadingRateUniformFragmentDensity(pattern, 255u / fragmentWidth, 255u / fragmentHeight, bitmap);
}

void FillShadingRateUniformFragmentDensity(ShadingRatePatternPtr pattern, uint32_t xDensity, uint32_t yDensity, Bitmap* bitmap)
{
    auto     encoder      = pattern->GetShadingRateEncoder();
    uint32_t encoded      = encoder->EncodeFragmentDensity(xDensity, yDensity);
    uint8_t* encodedBytes = reinterpret_cast<uint8_t*>(&encoded);
    bitmap->Fill<uint8_t>(encodedBytes[0], encodedBytes[1], 0, 0);
}

void FillShadingRateRadial(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap)
{
    auto encoder = pattern->GetShadingRateEncoder();
    scale /= std::min<uint32_t>(bitmap->GetWidth(), bitmap->GetHeight());
    for (uint32_t j = 0; j < bitmap->GetHeight(); ++j) {
        float    y    = scale * (2.0 * j - bitmap->GetHeight());
        uint8_t* addr = bitmap->GetPixel8u(0, j);
        for (uint32_t i = 0; i < bitmap->GetWidth(); ++i, addr += bitmap->GetPixelStride()) {
            float          x            = scale * (2.0 * i - bitmap->GetWidth());
            float          r2           = x * x + y * y;
            uint32_t       encoded      = encoder->EncodeFragmentSize(r2 + 1, r2 + 1);
            const uint8_t* encodedBytes = reinterpret_cast<const uint8_t*>(&encoded);
            for (uint32_t k = 0; k < bitmap->GetChannelCount(); ++k) {
                addr[k] = encodedBytes[k];
            }
        }
    }
}

void FillShadingRateAnisotropic(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap)
{
    auto encoder = pattern->GetShadingRateEncoder();
    scale /= std::min<uint32_t>(bitmap->GetWidth(), bitmap->GetHeight());
    for (uint32_t j = 0; j < bitmap->GetHeight(); ++j) {
        float    y    = scale * (2.0 * j - bitmap->GetHeight());
        uint8_t* addr = bitmap->GetPixel8u(0, j);
        for (uint32_t i = 0; i < bitmap->GetWidth(); ++i, addr += bitmap->GetPixelStride()) {
            float          x            = scale * (2.0 * i - bitmap->GetWidth());
            uint32_t       encoded      = encoder->EncodeFragmentSize(x * x + 1, y * y + 1);
            const uint8_t* encodedBytes = reinterpret_cast<const uint8_t*>(&encoded);
            for (uint32_t k = 0; k < bitmap->GetChannelCount(); ++k) {
                addr[k] = encodedBytes[k];
            }
        }
    }
}

} // namespace grfx
} // namespace ppx
