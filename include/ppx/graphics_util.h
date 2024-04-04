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

#ifndef ppx_graphics_util_h
#define ppx_graphics_util_h

#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_queue.h"
#include "ppx/grfx/grfx_texture.h"
#include "ppx/bitmap.h"
#include "ppx/geometry.h"
#include "ppx/mipmap.h"
#include "gli/gli.hpp"

#include <array>
#include <filesystem>
#include <type_traits>

namespace ppx {
namespace grfx_util {

class ImageOptions
{
public:
    ImageOptions() {}
    ~ImageOptions() {}

    // clang-format off
    ImageOptions& AdditionalUsage(grfx::ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
    ImageOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }
    // clang-format on

private:
    grfx::ImageUsageFlags mAdditionalUsage = grfx::ImageUsageFlags();
    uint32_t              mMipLevelCount   = PPX_REMAINING_MIP_LEVELS;

    friend Result CreateImageFromBitmap(
        grfx::Queue*        pQueue,
        const Bitmap*       pBitmap,
        grfx::Image**       ppImage,
        const ImageOptions& options);

    friend Result CreateImageFromCompressedImage(
        grfx::Queue*        pQueue,
        const gli::texture& image,
        grfx::Image**       ppImage,
        const ImageOptions& options);

    friend Result CreateImageFromFile(
        grfx::Queue*                 pQueue,
        const std::filesystem::path& path,
        grfx::Image**                ppImage,
        const ImageOptions&          options,
        bool                         useGpu);

    friend Result CreateImageFromBitmapGpu(
        grfx::Queue*        pQueue,
        const Bitmap*       pBitmap,
        grfx::Image**       ppImage,
        const ImageOptions& options);
};

//! @fn CopyBitmapToImage
//!
//!
Result CopyBitmapToImage(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image*        pImage,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    grfx::ResourceState stateBefore,
    grfx::ResourceState stateAfter);

//! @fn CreateImageFromBitmap
//!
//!
Result CreateImageFromBitmap(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image**       ppImage,
    const ImageOptions& options = ImageOptions());

//! @fn CreateImageFromFile
//!
//!
Result CreateImageFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Image**                ppImage,
    const ImageOptions&          options = ImageOptions(),
    bool                         useGpu  = false);

//! @fn CreateMipMapsForImage
//!
//!
Result CreateImageFromBitmapGpu(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image**       ppImage,
    const ImageOptions& options = ImageOptions());

// -------------------------------------------------------------------------------------------------

class TextureOptions
{
public:
    TextureOptions() {}
    ~TextureOptions() {}

    // clang-format off
    TextureOptions& AdditionalUsage(grfx::ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
    TextureOptions& InitialState(grfx::ResourceState state) { mInitialState = state; return *this; }
    TextureOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }
    TextureOptions& YcbcrConversion(grfx::YcbcrConversion *ycbcrConversion) { mYcbcrConversion = ycbcrConversion; return *this; }
    // clang-format on

private:
    grfx::ImageUsageFlags  mAdditionalUsage = grfx::ImageUsageFlags();
    grfx::ResourceState    mInitialState    = grfx::ResourceState::RESOURCE_STATE_SHADER_RESOURCE;
    uint32_t               mMipLevelCount   = 1;
    grfx::YcbcrConversion* mYcbcrConversion = nullptr;

    friend Result CreateTextureFromBitmap(
        grfx::Queue*          pQueue,
        const Bitmap*         pBitmap,
        grfx::Texture**       ppTexture,
        const TextureOptions& options);

    friend Result CreateTextureFromMipmap(
        grfx::Queue*          pQueue,
        const Mipmap*         pMipmap,
        grfx::Texture**       ppTexture,
        const TextureOptions& options);

    friend Result CreateTextureFromFile(
        grfx::Queue*                 pQueue,
        const std::filesystem::path& path,
        grfx::Texture**              ppTexture,
        const TextureOptions&        options);
};

//! @fn CreateTextureFromBitmap
//!
//!
Result CopyBitmapToTexture(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Texture*      pTexture,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    grfx::ResourceState stateBefore,
    grfx::ResourceState stateAfter);

//! @fn CreateTextureFromBitmap
//!
//!
Result CreateTextureFromBitmap(
    grfx::Queue*          pQueue,
    const Bitmap*         pBitmap,
    grfx::Texture**       ppTexture,
    const TextureOptions& options = TextureOptions());

//! @fn CreateTextureFromMipmap
//!
//! Mip level count from pMipmap is used. Mip level count from options is ignored.
//!
Result CreateTextureFromMipmap(
    grfx::Queue*          pQueue,
    const Mipmap*         pMipmap,
    grfx::Texture**       ppTexture,
    const TextureOptions& options = TextureOptions());

//! @fn CreateTextureFromFile
//!
//!
Result CreateTextureFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Texture**              ppTexture,
    const TextureOptions&        options = TextureOptions());

// Create a 1x1 texture with the specified pixel data. The format
// for the texture is derived from the pixel data type, which
// can be one of uint8, uint16, uint32 or float.
template <typename PixelDataType>
Result CreateTexture1x1(
    grfx::Queue*                       pQueue,
    const std::array<PixelDataType, 4> color,
    grfx::Texture**                    ppTexture,
    const TextureOptions&              options = TextureOptions())
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppTexture);

    Bitmap::Format format = Bitmap::FORMAT_UNDEFINED;
    if constexpr (std::is_same_v<PixelDataType, float>) {
        format = Bitmap::FORMAT_RGBA_FLOAT;
    }
    else if constexpr (std::is_same_v<PixelDataType, uint32_t>) {
        format = Bitmap::FORMAT_RGBA_UINT32;
    }
    else if constexpr (std::is_same_v<PixelDataType, uint16_t>) {
        format = Bitmap::FORMAT_RGBA_UINT16;
    }
    else if constexpr (std::is_same_v<PixelDataType, uint8_t>) {
        format = Bitmap::FORMAT_RGBA_UINT8;
    }
    else {
        PPX_ASSERT_MSG(false, "Invalid pixel data type: must be one of uint8, uint16, uint32 or float.")
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    // Create bitmap
    Result ppxres = ppx::SUCCESS;
    Bitmap bitmap = Bitmap::Create(1, 1, format, &ppxres);
    if (Failed(ppxres)) {
        return ppx::ERROR_BITMAP_CREATE_FAILED;
    }

    // Fill color
    bitmap.Fill(color[0], color[1], color[2], color[3]);

    return CreateTextureFromBitmap(pQueue, &bitmap, ppTexture, options);
}

// Creates an irradiance and an environment texture from an *.ibl file
Result CreateIBLTexturesFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Texture**              ppIrradianceTexture,
    grfx::Texture**              ppEnvironmentTexture);

// -------------------------------------------------------------------------------------------------

// clang-format off
/*
 
Cross Horizontal Left:
           _____
          |  0  |
     _____|_____|_____ _____
    |  1  |  2  |  3  |  4  |
    |_____|_____|_____|_____|
          |  5  |
          |_____|

Cross Horizontal Right:
                 _____
                |  0  |
     ___________|_____|_____ 
    |  1  |  2  |  3  |  4  |
    |_____|_____|_____|_____|
                |  5  |
                |_____|

Cross Vertical Top:
           _____
          |  0  |
     _____|_____|_____ 
    |  1  |  2  |  3  |
    |_____|_____|_____|
          |  4  |
          |_____|
          |  5  |
          |_____|

Cross Vertical Bottom:
           _____
          |  0  |
          |_____|
          |  1  |
     _____|_____|_____
    |  2  |  3  |  4  |
    |_____|_____|_____|
          |  5  |
          |_____|

Lat Long Horizontal:
     _____ _____ _____ 
    |  0  |  1  |  2  |
    |_____|_____|_____|
    |  3  |  4  |  5  |
    |_____|_____|_____|

Lat Long Vertical:
     _____ _____ 
    |  0  |  1  |
    |_____|_____|
    |  2  |  3  |
    |_____|_____|
    |  4  |  5  |
    |_____|_____|

Strip Horizontal:
     _____ _____ _____ _____ _____ _____ 
    |  0  |  1  |  2  |  3  |  4  |  5  |
    |_____|_____|_____|_____|_____|_____|


Strip Vertical:
     _____ 
    |  0  |
    |_____|
    |  1  |
    |_____|
    |  2  |
    |_____|
    |  3  |
    |_____|
    |  4  |
    |_____|
    |  5  |
    |_____|

*/
// clang-format on
enum CubeImageLayout
{
    CUBE_IMAGE_LAYOUT_UNDEFINED              = 0,
    CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT  = 1,
    CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_RIGHT = 2,
    CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP     = 3,
    CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_BOTTOM  = 4,
    CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL    = 5,
    CUBE_IMAGE_LAYOUT_LAT_LONG_VERTICAL      = 6,
    CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL       = 7,
    CUBE_IMAGE_LAYOUT_STRIP_VERTICAL         = 8,
    CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL       = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT,
    CUBE_IMAGE_LAYOUT_CROSS_VERTICAL         = CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP,
    CUBE_IMAGE_LAYOUT_LAT_LONG               = CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL,
    CUBE_IMAGE_LAYOUT_STRIP                  = CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL,
};

//! @enum CubeImageLoadOp
//!
//! Rotation is always clockwise.
//!
enum CubeFaceOp
{
    CUBE_FACE_OP_NONE              = 0,
    CUBE_FACE_OP_ROTATE_90         = 1,
    CUBE_FACE_OP_ROTATE_180        = 2,
    CUBE_FACE_OP_ROTATE_270        = 3,
    CUBE_FACE_OP_INVERT_HORIZONTAL = 4,
    CUBE_FACE_OP_INVERT_VERTICAL   = 5,
};

//! @struct CubeMapCreateInfo
//!
//! See enum CubeImageLayout for explanation of layouts
//!
//! Example - Use subimage 0 with 90 degrees CW rotation for posX face:
//!   layout = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL;
//!   posX   = PPX_ENCODE_CUBE_FACE(0, CUBE_FACE_OP_ROTATE_90, :CUBE_FACE_OP_NONE);
//!
#define PPX_CUBE_OP_MASK           0xFF
#define PPX_CUBE_OP_SUBIMAGE_SHIFT 0
#define PPX_CUBE_OP_OP1_SHIFT      8
#define PPX_CUBE_OP_OP2_SHIFT      16

#define PPX_ENCODE_CUBE_FACE(SUBIMAGE, OP1, OP2)              \
    (SUBIMAGE & PPX_CUBE_OP_MASK) |                           \
        ((OP1 & PPX_CUBE_OP_MASK) << PPX_CUBE_OP_OP1_SHIFT) | \
        ((OP1 & PPX_CUBE_OP_MASK) << PPX_CUBE_OP_OP2_SHIFT)

#define PPX_DECODE_CUBE_FACE_SUBIMAGE(FACE) (FACE >> PPX_CUBE_OP_SUBIMAGE_SHIFT) & PPX_CUBE_OP_MASK
#define PPX_DECODE_CUBE_FACE_OP1(FACE)      (FACE >> PPX_CUBE_OP_OP1_SHIFT) & PPX_CUBE_OP_MASK
#define PPX_DECODE_CUBE_FACE_OP2(FACE)      (FACE >> PPX_CUBE_OP_OP2_SHIFT) & PPX_CUBE_OP_MASK

//! @struct CubeMapCreateInfo
//!
//!
struct CubeMapCreateInfo
{
    CubeImageLayout layout = CUBE_IMAGE_LAYOUT_UNDEFINED;
    uint32_t        posX   = PPX_VALUE_IGNORED;
    uint32_t        negX   = PPX_VALUE_IGNORED;
    uint32_t        posY   = PPX_VALUE_IGNORED;
    uint32_t        negY   = PPX_VALUE_IGNORED;
    uint32_t        posZ   = PPX_VALUE_IGNORED;
    uint32_t        negZ   = PPX_VALUE_IGNORED;
};

//! @fn CreateCubeMapFromFile
//!
//!
Result CreateCubeMapFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    const CubeMapCreateInfo*     pCreateInfo,
    grfx::Image**                ppImage,
    const grfx::ImageUsageFlags& additionalImageUsage = grfx::ImageUsageFlags());

// -------------------------------------------------------------------------------------------------

//! @fn CreateMeshFromGeometry
//!
//!
Result CreateMeshFromGeometry(
    grfx::Queue*    pQueue,
    const Geometry* pGeometry,
    grfx::Mesh**    ppMesh);

//! @fn CreateMeshFromTriMesh
//!
//!
Result CreateMeshFromTriMesh(
    grfx::Queue*   pQueue,
    const TriMesh* pTriMesh,
    grfx::Mesh**   ppMesh);

//! @fn CreateMeshFromWireMesh
//!
//!
Result CreateMeshFromWireMesh(
    grfx::Queue*    pQueue,
    const WireMesh* pWireMesh,
    grfx::Mesh**    ppMesh);

//! @fn CreateModelFromFile
//!
//!
Result CreateMeshFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Mesh**                 ppMesh,
    const TriMeshOptions&        options = TriMeshOptions());

// -------------------------------------------------------------------------------------------------

grfx::Format ToGrfxFormat(Bitmap::Format value);

} // namespace grfx_util
} // namespace ppx

#endif // ppx_graphics_util_h
