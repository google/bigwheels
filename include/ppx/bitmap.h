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

#ifndef ppx_bitmap_h
#define ppx_bitmap_h

#include "ppx/config.h"
#include "ppx/grfx/grfx_format.h"

#include "stb_image_resize.h"

#include <filesystem>

namespace ppx {

//! @class Bitmap
//!
//!
class Bitmap
{
public:
    enum DataType
    {
        DATA_TYPE_UNDEFINED = 0,
        DATA_TYPE_UINT8,
        DATA_TYPE_UINT16,
        DATA_TYPE_UINT32,
        DATA_TYPE_FLOAT,
    };

    enum Format
    {
        FORMAT_UNDEFINED = 0,
        FORMAT_R_UINT8,
        FORMAT_RG_UINT8,
        FORMAT_RGB_UINT8,
        FORMAT_RGBA_UINT8,
        FORMAT_R_UINT16,
        FORMAT_RG_UINT16,
        FORMAT_RGB_UINT16,
        FORMAT_RGBA_UINT16,
        FORMAT_R_UINT32,
        FORMAT_RG_UINT32,
        FORMAT_RGB_UINT32,
        FORMAT_RGBA_UINT32,
        FORMAT_R_FLOAT,
        FORMAT_RG_FLOAT,
        FORMAT_RGB_FLOAT,
        FORMAT_RGBA_FLOAT,
    };

    // ---------------------------------------------------------------------------------------------

    Bitmap();
    Bitmap(const Bitmap& obj);
    ~Bitmap();

    Bitmap& operator=(const Bitmap& rhs);

    //! Creates a bitmap with internal storage.
    static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* pBitmap);
    //! Creates a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
    static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Bitmap* pBitmap);
    //! Creates a bitmap with external storage.
    static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Bitmap* pBitmap);
    //! Returns a bitmap with internal storage.
    static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* pResult = nullptr);
    //! Returns a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
    static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Result* pResult = nullptr);
    //! Returns a bitmap with external storage.
    static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Result* pResult = nullptr);

    // Returns true if dimensions are greater than one, format is valid, and storage is valid
    bool IsOk() const;

    uint32_t       GetWidth() const { return mWidth; }
    uint32_t       GetHeight() const { return mHeight; }
    Bitmap::Format GetFormat() const { return mFormat; };
    uint32_t       GetChannelCount() const { return mChannelCount; }
    uint32_t       GetPixelStride() const { return mPixelStride; }
    uint32_t       GetRowStride() const { return mRowStride; }
    char*          GetData() const { return mData; }
    uint64_t       GetFootprintSize(uint32_t rowStrideAlignment = 1) const;

    Result Resize(uint32_t width, uint32_t height);
    Result ScaleTo(Bitmap* pTargetBitmap) const;
    Result ScaleTo(Bitmap* pTargetBitmap, stbir_filter filterType) const;

    template <typename PixelDataType>
    void Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a);

    // Returns byte address of pixel at (x,y)
    char*       GetPixelAddress(uint32_t x, uint32_t y);
    const char* GetPixelAddress(uint32_t x, uint32_t y) const;

    // These functions will return null if the Bitmap's format doesn't
    // the function. For example, GetPixel8u() returns null if the format
    // is FORMAT_RGBA_FLOAT.
    //
    uint8_t*        GetPixel8u(uint32_t x, uint32_t y);
    const uint8_t*  GetPixel8u(uint32_t x, uint32_t y) const;
    uint16_t*       GetPixel16u(uint32_t x, uint32_t y);
    const uint16_t* GetPixel16u(uint32_t x, uint32_t y) const;
    uint32_t*       GetPixel32u(uint32_t x, uint32_t y);
    const uint32_t* GetPixel32u(uint32_t x, uint32_t y) const;
    float*          GetPixel32f(uint32_t x, uint32_t y);
    const float*    GetPixel32f(uint32_t x, uint32_t y) const;

    static uint32_t         ChannelSize(Bitmap::Format value);
    static uint32_t         ChannelCount(Bitmap::Format value);
    static Bitmap::DataType ChannelDataType(Bitmap::Format value);
    static uint32_t         FormatSize(Bitmap::Format value);
    static uint64_t         StorageFootprint(uint32_t width, uint32_t height, Bitmap::Format format);

    static Result GetFileProperties(const std::filesystem::path& path, uint32_t* pWidth, uint32_t* pHeight, Bitmap::Format* pFormat);
    static Result LoadFile(const std::filesystem::path& path, Bitmap* pBitmap);
    static Result SaveFilePNG(const std::filesystem::path& path, const Bitmap* pBitmap);
    static bool   IsBitmapFile(const std::filesystem::path& path);

    static Result LoadFromMemory(const size_t dataSize, const void* pData, Bitmap* pBitmap);

    // ---------------------------------------------------------------------------------------------

    class PixelIterator
    {
    private:
        friend class ppx::Bitmap;

        PixelIterator(Bitmap* pBitmap)
            : mBitmap(pBitmap)
        {
            Reset();
        }

    public:
        void Reset()
        {
            mX            = 0;
            mY            = 0;
            mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
        }

        bool Done() const
        {
            bool done = (mY >= mBitmap->GetHeight());
            return done;
        }

        bool Next()
        {
            if (Done()) {
                return false;
            }

            mX += 1;
            mPixelAddress += mBitmap->GetPixelStride();
            if (mX == mBitmap->GetWidth()) {
                mY += 1;
                mX            = 0;
                mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
            }

            return Done() ? false : true;
        }

        uint32_t       GetX() const { return mX; }
        uint32_t       GetY() const { return mY; }
        Bitmap::Format GetFormat() const { return mBitmap->GetFormat(); }
        uint32_t       GetChannelCount() const { return Bitmap::ChannelCount(GetFormat()); }

        template <typename T>
        T* GetPixelAddress() const { return reinterpret_cast<T*>(mPixelAddress); }

    private:
        Bitmap*  mBitmap       = nullptr;
        uint32_t mX            = 0;
        uint32_t mY            = 0;
        char*    mPixelAddress = nullptr;
    };

    // ---------------------------------------------------------------------------------------------

    PixelIterator GetPixelIterator() { return PixelIterator(this); }

private:
    void   InternalCtor();
    Result InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage);
    Result InternalCopy(const Bitmap& obj);
    void   FreeStbiDataIfNeeded();

    // Stbi-specific functions/wrappers.
    // These arguments generally mirror those for stbi_load, except format which is used to determine whether
    // the call should be made to stbi_load or stbi_loadf (that is, whether the file to be read is in integer or floating point format).
    static char* StbiLoad(const std::filesystem::path& path, Bitmap::Format format, int* pWidth, int* pHeight, int* pChannels, int desiredChannels);
    // These arugments mirror those for stbi_info.
    static Result StbiInfo(const std::filesystem::path& path, int* pX, int* pY, int* pComp);

private:
    uint32_t          mWidth           = 0;
    uint32_t          mHeight          = 0;
    Bitmap::Format    mFormat          = Bitmap::FORMAT_UNDEFINED;
    uint32_t          mChannelCount    = 0;
    uint32_t          mPixelStride     = 0;
    uint32_t          mRowStride       = 0;
    char*             mData            = nullptr;
    bool              mDataIsFromStbi  = false;
    std::vector<char> mInternalStorage = {};
};

template <typename PixelDataType>
void Bitmap::Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a)
{
    PPX_ASSERT_MSG(mData != nullptr, "data is null");
    PPX_ASSERT_MSG(mFormat != Bitmap::FORMAT_UNDEFINED, "format is undefined");

    PixelDataType rgba[4] = {r, g, b, a};

    const uint32_t channelCount = Bitmap::ChannelCount(mFormat);

    char* pData = mData;
    for (uint32_t y = 0; y < mHeight; ++y) {
        for (uint32_t x = 0; x < mWidth; ++x) {
            PixelDataType* pPixelData = reinterpret_cast<PixelDataType*>(pData);
            for (uint32_t c = 0; c < channelCount; ++c) {
                pPixelData[c] = rgba[c];
            }
            pData += mPixelStride;
        }
    }
}

} // namespace ppx

#endif // ppx_bitmap_h
