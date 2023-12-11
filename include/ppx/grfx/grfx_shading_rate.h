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
#include "ppx/grfx/grfx_image.h"
#include "ppx/bitmap.h"

namespace ppx {
namespace grfx {

// Maximum number of supported shading rates in ShadingRateCapabilities.
static constexpr size_t kMaxSupportedShadingRateCount = 16;

// ShadingRateCapabilities
//
// Information about GPU support for shading rate features.
struct ShadingRateCapabilities
{
    // The shading rate mode supported by this device.
    ShadingRateMode supportedShadingRateMode = SHADING_RATE_NONE;

    struct
    {
        // Minimum/maximum size of the region of the render target
        // corresponding to a single pixel in the FDM attachment.
        // This is *not* the minimum/maximum fragment density.
        Extent2D minTexelSize;
        Extent2D maxTexelSize;
    } fdm;

    struct
    {
        // Minimum/maximum size of the region of the render target
        // corresponding to a single pixel in the VRS attachment.
        // This is *not* the shading rate itself.
        Extent2D minTexelSize;
        Extent2D maxTexelSize;

        // List of supported shading rates.
        uint32_t supportedRateCount = 0;
        Extent2D supportedRates[kMaxSupportedShadingRateCount];
    } vrs;
};

// ShadingRateEncoder
//
// Encodes fragment densities/sizes into the format needed for a
// ShadingRatePattern.
class ShadingRateEncoder
{
public:
    // Encode a pair of fragment density values.
    //
    // Fragment density values are a ratio over 255, e.g. 255 means shade every
    // pixel, and 128 means shade every other pixel.
    virtual uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const = 0;

    // Encode a pair of fragment size values.
    //
    // The fragmentWidth/fragmentHeight values are in pixels.
    virtual uint32_t EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const = 0;
};

// ShadingRatePatternCreateInfo
//
//
struct ShadingRatePatternCreateInfo
{
    // The size of the framebuffer image that will be used with the created
    // ShadingRatePattern.
    Extent2D framebufferSize;

    // The size of the region of the framebuffer image that will correspond to
    // a single pixel in the ShadingRatePattern image.
    Extent2D texelSize;

    // The shading rate mode (FDM or VRS).
    ShadingRateMode shadingRateMode = SHADING_RATE_NONE;
};

// ShadingRatePattern
//
// An image representing fragment sizes/densities that can be used in a render
// pass to control the shading rate.
class ShadingRatePattern
    : public DeviceObject<ShadingRatePatternCreateInfo>
{
public:
    virtual ~ShadingRatePattern() = default;

    // The shading rate mode (FDM or VRS).
    ShadingRateMode GetShadingRateMode() const { return mShadingRateMode; }

    // The image contaning encoded fragment sizes/densities.
    ImagePtr GetAttachmentImage() const { return mAttachmentImage; }

    // The width/height of the image contaning encoded fragment sizes/densities.
    uint32_t GetAttachmentWidth() const { return mAttachmentImage->GetWidth(); }
    uint32_t GetAttachmentHeight() const { return mAttachmentImage->GetHeight(); }

    // The width/height of the region of the render target image corresponding
    // to a single pixel in the image containing fragment sizes/densities.
    uint32_t GetTexelWidth() const { return mTexelSize.width; }
    uint32_t GetTexelHeight() const { return mTexelSize.height; }

    // Create a bitmap suitable for uploading fragment density/size to this pattern.
    std::unique_ptr<Bitmap> CreateBitmap() const;

    // Load fragment density/size from a bitmap of encoded values.
    Result LoadFromBitmap(Bitmap* bitmap);

    // Get the pixel format of a bitmap that can store the fragment density/size data.
    virtual Bitmap::Format GetBitmapFormat() const = 0;

    // Get an encoder that can encode fragment density/size values for this pattern.
    virtual const ShadingRateEncoder* GetShadingRateEncoder() const = 0;

protected:
    ShadingRateMode mShadingRateMode;
    ImagePtr        mAttachmentImage;
    Extent2D        mTexelSize;
};

} // namespace grfx
} // namespace ppx
#endif // ppx_grfx_shading_rate_h
