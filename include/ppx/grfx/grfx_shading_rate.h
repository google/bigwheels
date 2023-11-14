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

#ifndef ppx_grfx_shading_rate_h
#define ppx_grfx_shading_rate_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

// Maximum number of supported shading rates in ShadingRateCapabilities.
static constexpr size_t kMaxSupportedShadingRateCount = 16;

// ShadingRateCapabilities
//
// Information about GPU support for shading rate features.
struct ShadingRateCapabilities
{
    // Does the device&API support Fragment Density Map?
    bool supportsFDM = false;

    // Does the device&API support Variable Rate Shading at the graphics
    // pipeline level?
    bool supportsPipelineVRS = false;

    // Does the device&API support Variable Rate Shading at the primitive
    // level?
    bool supportsPrimitiveVRS = false;

    // Does the device&API support Variable Rate Shading attachments?
    bool supportsAttachmentVRS = false;

    struct
    {
        // Minumum/maximum size of the region of the render target
        // corresponding to a single pixel in the FDM attachment.
        // This is *not* the minimum/maximum fragment density.
        Extent2D minTexelSize;
        Extent2D maxTexelSize;
    } fdm;

    struct
    {
        // Minumum/maximum size of the region of the render target
        // corresponding to a single pixel in the VRS attachment.
        // This is *not* the shading rate itself.
        Extent2D minTexelSize;
        Extent2D maxTexelSize;

        // List of supported shading rates.
        uint32_t supportedRateCount = 0;
        Extent2D supportedRates[kMaxSupportedShadingRateCount];
    } vrs;
};

} // namespace grfx
} // namespace ppx
#endif // ppx_grfx_shading_rate_h
