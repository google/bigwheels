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

#include "ppx/bitmap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "ppx/fs.h"

namespace ppx {

Bitmap::Bitmap()
{
    InternalCtor();
}

Bitmap::Bitmap(const Bitmap& obj)
{
    Result ppxres = InternalCopy(obj);
    if (Failed(ppxres)) {
        InternalCtor();
        PPX_ASSERT_MSG(false, "copy failed");
    }
}

Bitmap& Bitmap::operator=(const Bitmap& rhs)
{
    if (&rhs != this) {
        Result ppxres = InternalCopy(rhs);
        if (Failed(ppxres)) {
            InternalCtor();
            PPX_ASSERT_MSG(false, "copy failed");
        }
    }
    return *this;
}

void Bitmap::InternalCtor()
{
    mWidth        = 0;
    mHeight       = 0;
    mFormat       = Bitmap::FORMAT_UNDEFINED;
    mChannelCount = 0;
    mPixelStride  = 0;
    mRowStride    = 0;
    mData         = nullptr;
    mInternalStorage.clear();
}

Result Bitmap::InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage)
{
    if (format == Bitmap::FORMAT_UNDEFINED) {
        return ppx::ERROR_IMAGE_INVALID_FORMAT;
    }

    mWidth        = width;
    mHeight       = height;
    mFormat       = format;
    mChannelCount = Bitmap::ChannelCount(format);
    mPixelStride  = Bitmap::FormatSize(format);
    mRowStride    = width * Bitmap::FormatSize(format);
    mData         = pExternalStorage;

    if (IsNull(mData)) {
        size_t n = Bitmap::StorageFootprint(width, height, format);
        if (n > 0) {
            mInternalStorage.resize(n);
            if (mInternalStorage.size() != n) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }

            mData = mInternalStorage.data();
        }
    }

    return ppx::SUCCESS;
}

Result Bitmap::InternalCopy(const Bitmap& obj)
{
    // Copy properties
    mWidth        = obj.mWidth;
    mHeight       = obj.mHeight;
    mFormat       = obj.mFormat;
    mChannelCount = obj.mChannelCount;
    mPixelStride  = obj.mPixelStride;
    mRowStride    = obj.mRowStride;

    // Allocate storage
    size_t footprint = Bitmap::StorageFootprint(mWidth, mHeight, mFormat);
    mInternalStorage.resize(footprint);
    if (mInternalStorage.size() != footprint) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    mData = mInternalStorage.data();

    uint64_t srcFootprint = obj.GetFootprintSize();
    if (srcFootprint != static_cast<uint64_t>(footprint)) {
        return ppx::ERROR_BITMAP_FOOTPRINT_MISMATCH;
    }

    memcpy(mData, obj.mData, footprint);

    return ppx::SUCCESS;
}

Result Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* pBitmap)
{
    PPX_ASSERT_NULL_ARG(pBitmap);
    if (IsNull(pBitmap)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    Result ppxres = pBitmap->InternalInitialize(width, height, format, nullptr);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Bitmap* pBitmap)
{
    PPX_ASSERT_NULL_ARG(pBitmap);
    if (IsNull(pBitmap)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    Result ppxres = pBitmap->InternalInitialize(width, height, format, pExternalStorage);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Bitmap Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* pResult)
{
    Bitmap bitmap;
    Result ppxres = Bitmap::Create(width, height, format, &bitmap);
    if (Failed(ppxres)) {
        bitmap.InternalCtor();
    }
    if (!IsNull(pResult)) {
        *pResult = ppxres;
    }
    return bitmap;
}

Bitmap Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Result* pResult)
{
    Bitmap bitmap;
    Result ppxres = Bitmap::Create(width, height, format, pExternalStorage, &bitmap);
    if (Failed(ppxres)) {
        bitmap.InternalCtor();
    }
    if (!IsNull(pResult)) {
        *pResult = ppxres;
    }
    return bitmap;
}

bool Bitmap::IsOk() const
{
    bool isSizeValid    = (mWidth > 0) && (mHeight > 0);
    bool isFormatValid  = (mFormat != Bitmap::FORMAT_UNDEFINED);
    bool isStorageValid = (mData != nullptr);
    return isSizeValid && isFormatValid && isStorageValid;
}

uint64_t Bitmap::GetFootprintSize(uint32_t rowStrideAlignment) const
{
    uint32_t alignedRowStride = RoundUp<uint32_t>(mRowStride, rowStrideAlignment);
    uint64_t size             = alignedRowStride * mHeight;
    return size;
}

Result Bitmap::Resize(uint32_t width, uint32_t height)
{
    // If internal storage is empty then this bitmap is using
    // external storage...so don't resize!
    //
    if (mInternalStorage.empty()) {
        return ppx::ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE;
    }

    mWidth     = width;
    mHeight    = height;
    mRowStride = width * mPixelStride;

    size_t n = Bitmap::StorageFootprint(mWidth, mHeight, mFormat);
    mInternalStorage.resize(n);
    mData = (mInternalStorage.size() > 0) ? mInternalStorage.data() : nullptr;

    return ppx::SUCCESS;
}

Result Bitmap::ScaleTo(Bitmap* pTargetBitmap) const
{
    if (IsNull(pTargetBitmap)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Format must match
    if (pTargetBitmap->GetFormat() != mFormat) {
        return ppx::ERROR_IMAGE_INVALID_FORMAT;
    }

    // clang-format off
    stbir_datatype datatype = InvalidValue<stbir_datatype>();
    switch (ChannelDataType(GetFormat())) {
        default: break;
        case Bitmap::DATA_TYPE_UINT8  : datatype = STBIR_TYPE_UINT8; break;
        case Bitmap::DATA_TYPE_UINT16 : datatype = STBIR_TYPE_UINT16; break;
        case Bitmap::DATA_TYPE_UINT32 : datatype = STBIR_TYPE_UINT32; break;
        case Bitmap::DATA_TYPE_FLOAT  : datatype = STBIR_TYPE_FLOAT; break;
    }
    // clang-format on

    int res = stbir_resize(
        static_cast<const void*>(GetData()),
        static_cast<int>(GetWidth()),
        static_cast<int>(GetHeight()),
        static_cast<int>(GetRowStride()),
        static_cast<void*>(pTargetBitmap->GetData()),
        static_cast<int>(pTargetBitmap->GetWidth()),
        static_cast<int>(pTargetBitmap->GetHeight()),
        static_cast<int>(pTargetBitmap->GetRowStride()),
        datatype,
        static_cast<int>(Bitmap::ChannelCount(GetFormat())),
        -1,
        0,
        STBIR_EDGE_CLAMP,
        STBIR_EDGE_CLAMP,
        STBIR_FILTER_DEFAULT,
        STBIR_FILTER_DEFAULT,
        STBIR_COLORSPACE_LINEAR,
        nullptr);

    if (res == 0) {
        return ERROR_IMAGE_RESIZE_FAILED;
    }

    return ppx::SUCCESS;
}

char* Bitmap::GetPixelAddress(uint32_t x, uint32_t y)
{
    char* pPixel = nullptr;
    if (!IsNull(mData) && (x >= 0) && (x < mWidth) && (y >= 0) && (y < mHeight)) {
        size_t offset = (y * mRowStride) + (x * mPixelStride);
        pPixel        = mData + offset;
    }
    return pPixel;
}

const char* Bitmap::GetPixelAddress(uint32_t x, uint32_t y) const
{
    const char* pPixel = nullptr;
    if (!IsNull(mData) && (x >= 0) && (x < mWidth) && (y >= 0) && (y < mHeight)) {
        size_t offset = (y * mRowStride) + (x * mPixelStride);
        pPixel        = mData + offset;
    }
    return pPixel;
}

uint8_t* Bitmap::GetPixel8u(uint32_t x, uint32_t y)
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT8) {
        return nullptr;
    }
    return reinterpret_cast<uint8_t*>(GetPixelAddress(x, y));
}

const uint8_t* Bitmap::GetPixel8u(uint32_t x, uint32_t y) const
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT8) {
        return nullptr;
    }
    return reinterpret_cast<const uint8_t*>(GetPixelAddress(x, y));
}

uint16_t* Bitmap::GetPixel16u(uint32_t x, uint32_t y)
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT16) {
        return nullptr;
    }
    return reinterpret_cast<uint16_t*>(GetPixelAddress(x, y));
}

const uint16_t* Bitmap::GetPixel16u(uint32_t x, uint32_t y) const
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT16) {
        return nullptr;
    }
    return reinterpret_cast<const uint16_t*>(GetPixelAddress(x, y));
}

uint32_t* Bitmap::GetPixel32u(uint32_t x, uint32_t y)
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT32) {
        return nullptr;
    }
    return reinterpret_cast<uint32_t*>(GetPixelAddress(x, y));
}

const uint32_t* Bitmap::GetPixel32u(uint32_t x, uint32_t y) const
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_UINT32) {
        return nullptr;
    }
    return reinterpret_cast<const uint32_t*>(GetPixelAddress(x, y));
}

float* Bitmap::GetPixel32f(uint32_t x, uint32_t y)
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_FLOAT) {
        return nullptr;
    }
    return reinterpret_cast<float*>(GetPixelAddress(x, y));
}

const float* Bitmap::GetPixel32f(uint32_t x, uint32_t y) const
{
    if (Bitmap::ChannelDataType(mFormat) != Bitmap::DATA_TYPE_FLOAT) {
        return nullptr;
    }
    return reinterpret_cast<const float*>(GetPixelAddress(x, y));
}

uint32_t Bitmap::ChannelSize(Bitmap::Format value)
{
    // clang-format off
    switch (value) {
    default: break;
        case Bitmap::FORMAT_R_UINT8     : return 1; break;
        case Bitmap::FORMAT_RG_UINT8    : return 1; break;
        case Bitmap::FORMAT_RGB_UINT8   : return 1; break;
        case Bitmap::FORMAT_RGBA_UINT8  : return 1; break;

        case Bitmap::FORMAT_R_UINT16    : return 2; break;
        case Bitmap::FORMAT_RG_UINT16   : return 2; break;
        case Bitmap::FORMAT_RGB_UINT16  : return 2; break;
        case Bitmap::FORMAT_RGBA_UINT16 : return 2; break;

        case Bitmap::FORMAT_R_UINT32    : return 4; break;
        case Bitmap::FORMAT_RG_UINT32   : return 4; break;
        case Bitmap::FORMAT_RGB_UINT32  : return 4; break;
        case Bitmap::FORMAT_RGBA_UINT32 : return 4; break;

        case Bitmap::FORMAT_R_FLOAT     : return 4; break;
        case Bitmap::FORMAT_RG_FLOAT    : return 4; break;
        case Bitmap::FORMAT_RGB_FLOAT   : return 4; break;
        case Bitmap::FORMAT_RGBA_FLOAT  : return 4; break;
    }
    // clang-format on
    return 0;
}

uint32_t Bitmap::ChannelCount(Bitmap::Format value)
{
    // clang-format off
    switch (value) {
    default: break;
        case Bitmap::FORMAT_R_UINT8     : return 1; break;
        case Bitmap::FORMAT_RG_UINT8    : return 2; break;
        case Bitmap::FORMAT_RGB_UINT8   : return 3; break;
        case Bitmap::FORMAT_RGBA_UINT8  : return 4; break;

        case Bitmap::FORMAT_R_UINT16    : return 1; break;
        case Bitmap::FORMAT_RG_UINT16   : return 2; break;
        case Bitmap::FORMAT_RGB_UINT16  : return 3; break;
        case Bitmap::FORMAT_RGBA_UINT16 : return 4; break;

        case Bitmap::FORMAT_R_UINT32    : return 1; break;
        case Bitmap::FORMAT_RG_UINT32   : return 2; break;
        case Bitmap::FORMAT_RGB_UINT32  : return 3; break;
        case Bitmap::FORMAT_RGBA_UINT32 : return 4; break;

        case Bitmap::FORMAT_R_FLOAT     : return 1; break;
        case Bitmap::FORMAT_RG_FLOAT    : return 2; break;
        case Bitmap::FORMAT_RGB_FLOAT   : return 3; break;
        case Bitmap::FORMAT_RGBA_FLOAT  : return 4; break;
    }
    // clang-format on
    return 0;
}

Bitmap::DataType Bitmap::ChannelDataType(Bitmap::Format value)
{
    // clang-format off
    switch (value) {
        default: break;
        case Bitmap::FORMAT_R_UINT8:
        case Bitmap::FORMAT_RG_UINT8:
        case Bitmap::FORMAT_RGB_UINT8:
        case Bitmap::FORMAT_RGBA_UINT8: {
            return Bitmap::DATA_TYPE_UINT8;
        } break;

        case Bitmap::FORMAT_R_UINT16:
        case Bitmap::FORMAT_RG_UINT16:
        case Bitmap::FORMAT_RGB_UINT16:
        case Bitmap::FORMAT_RGBA_UINT16: {
            return Bitmap::DATA_TYPE_UINT16;
        } break;

        case Bitmap::FORMAT_R_UINT32:
        case Bitmap::FORMAT_RG_UINT32:
        case Bitmap::FORMAT_RGB_UINT32:
        case Bitmap::FORMAT_RGBA_UINT32: {
            return Bitmap::DATA_TYPE_UINT32;
        } break;

        case Bitmap::FORMAT_R_FLOAT:
        case Bitmap::FORMAT_RG_FLOAT:
        case Bitmap::FORMAT_RGB_FLOAT:
        case Bitmap::FORMAT_RGBA_FLOAT: {
            return Bitmap::DATA_TYPE_FLOAT;
        } break;
    }
    // clang-format on
    return Bitmap::DATA_TYPE_UNDEFINED;
}

uint32_t Bitmap::FormatSize(Bitmap::Format value)
{
    uint32_t channelSize  = Bitmap::ChannelSize(value);
    uint32_t channelCount = Bitmap::ChannelCount(value);
    uint32_t size         = channelSize * channelCount;
    return size;
}

uint64_t Bitmap::StorageFootprint(uint32_t width, uint32_t height, Bitmap::Format format)
{
    uint64_t size = width * height * Bitmap::FormatSize(format);
    return size;
}

static Result IsRadianceFile(const std::filesystem::path& path, bool& isRadiance)
{
    static const char* kRadianceSig = "#?RADIANCE";

    // Open file

    ppx::fs::File file;
    if (!file.Open(path.string().c_str())) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }
    // Signature buffer
    const size_t kBufferSize      = 10;
    char         buf[kBufferSize] = {0};

    // Read signature
    size_t n = file.Read(buf, kBufferSize);

    // Close file
    file.Close();

    // Only check if kBufferSize bytes were read
    if (n == kBufferSize) {
        int res    = strncmp(buf, kRadianceSig, kBufferSize);
        isRadiance = (res == 0);
    }

    return ppx::SUCCESS;
}

bool Bitmap::IsBitmapFile(const std::filesystem::path& path)
{
    int x, y, comp;
#if defined(PPX_ANDROID)
    auto bitmapBytes = fs::load_file(path.string().c_str());
    if (!bitmapBytes.has_value())
        return stbi__err("can't fopen", "Unable to open file");
    return stbi_info_from_memory((unsigned char*)bitmapBytes->data(), bitmapBytes->size(), &x, &y, &comp);
#else
    return stbi_info(path.string().c_str(), &x, &y, &comp);
#endif
}

Result Bitmap::GetFileProperties(const std::filesystem::path& path, uint32_t* pWidth, uint32_t* pHeight, Bitmap::Format* pFormat)
{
    if (!ppx::fs::path_exists(path)) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    int x    = 0;
    int y    = 0;
    int comp = 0;
    stbi_info(path.string().c_str(), &x, &y, &comp);

    bool   isRadiance = false;
    Result ppxres     = IsRadianceFile(path, isRadiance);
    if (Failed(ppxres)) {
        return ppxres;
    }

    if (!IsNull(pWidth)) {
        *pWidth = static_cast<uint32_t>(x);
    }

    if (!IsNull(pHeight)) {
        *pHeight = static_cast<uint32_t>(y);
    }

    // Force to 4 chanenls to make things easier for the graphics APIs
    if (!IsNull(pFormat)) {
        *pFormat = isRadiance ? Bitmap::FORMAT_RGBA_FLOAT : Bitmap::FORMAT_RGBA_UINT8;
    }

    return ppx::SUCCESS;
}

Result Bitmap::LoadFile(const std::filesystem::path& path, Bitmap* pBitmap)
{
    if (!ppx::fs::path_exists(path)) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    bool   isRadiance = false;
    Result ppxres     = IsRadianceFile(path, isRadiance);
    if (Failed(ppxres)) {
        return ppxres;
    }

    auto bitmapBytes = ppx::fs::load_file(path);
    if (!bitmapBytes.has_value()) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    // @TODO: Refactor to remove redundancies from both blocks
    //
    if (isRadiance) {
        int width            = 0;
        int height           = 0;
        int channels         = 0;
        int requiredChannels = 4; // Force to 4 chanenls to make things easier for the graphics APIs

        float* pData = stbi_loadf_from_memory((unsigned char*)bitmapBytes.value().data(), bitmapBytes.value().size(), &width, &height, &channels, requiredChannels);
        if (pData == nullptr) {
            return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
        }

        Bitmap::Format format = Bitmap::FORMAT_RGBA_FLOAT;

        ppxres = Bitmap::Create(width, height, format, pBitmap);
        if (!pBitmap->IsOk()) {
            // Something has gone really wrong if this happens
            stbi_image_free(pData);
            return ppx::ERROR_FAILED;
        }

        size_t nbytes = Bitmap::StorageFootprint(static_cast<uint32_t>(width), static_cast<uint32_t>(height), format);
        std::memcpy(pBitmap->GetData(), pData, nbytes);

        stbi_image_free(pData);
    }
    else {
        int width            = 0;
        int height           = 0;
        int channels         = 0;
        int requiredChannels = 4; // Force to 4 channels to make things easier for the graphics APIs

        unsigned char* pData = stbi_load_from_memory((unsigned char*)bitmapBytes.value().data(), bitmapBytes.value().size(), &width, &height, &channels, requiredChannels);
        if (IsNull(pData)) {
            return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
        }

        Bitmap::Format format = Bitmap::FORMAT_RGBA_UINT8;

        ppxres = Bitmap::Create(width, height, format, pBitmap);
        if (Failed(ppxres)) {
            // Something has gone really wrong if this happens
            stbi_image_free(pData);
            return ppx::ERROR_FAILED;
        }

        size_t nbytes = Bitmap::StorageFootprint(static_cast<uint32_t>(width), static_cast<uint32_t>(height), format);
        std::memcpy(pBitmap->GetData(), pData, nbytes);

        stbi_image_free(pData);
    }

    return ppx::SUCCESS;
}

Result Bitmap::SaveFilePNG(const std::filesystem::path& path, const Bitmap* pBitmap)
{
#if defined(PPX_ANDROID)
    PPX_ASSERT_MSG(false, "SaveFilePNG not supported on Android");
    return ppx::ERROR_IMAGE_FILE_SAVE_FAILED;
#else
    int res = stbi_write_png(path.string().c_str(), pBitmap->GetWidth(), pBitmap->GetHeight(), pBitmap->GetChannelCount(), pBitmap->GetData(), pBitmap->GetRowStride());
    if (res == 0) {
        return ppx::ERROR_IMAGE_FILE_SAVE_FAILED;
    }
    return ppx::SUCCESS;
#endif
}

} // namespace ppx
