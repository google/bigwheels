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

#include "ppx/mipmap.h"

#include "stb_image.h"

#include <filesystem>

namespace ppx {

static uint32_t CalculatActualLevelCount(uint32_t width, uint32_t height, uint32_t levelCount)
{
    uint32_t actualLevelCount = 0;
    for (uint32_t i = 0; i < levelCount; ++i) {
        if ((width > 0) && (height > 0)) {
            actualLevelCount += 1;
        }

        width  = width / 2;
        height = height / 2;

        if ((width == 0) || (height == 0)) {
            break;
        }
    }
    return actualLevelCount;
}

static uint64_t CalculateDataSize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount)
{
    bool isValidWidth      = (width > 0);
    bool isValidHeight     = (height > 0);
    bool isValidFormat     = (format != Bitmap::FORMAT_UNDEFINED);
    bool isValidLevelCount = (levelCount > 0);
    bool isValid           = isValidWidth && isValidHeight && isValidFormat && isValidLevelCount;
    if (!isValid) {
        return 0;
    }

    uint64_t totalSize = 0;
    for (uint32_t i = 0; i < levelCount; ++i) {
        uint64_t pixelStride = Bitmap::FormatSize(format);
        uint64_t rowStride   = static_cast<uint64_t>(width) * pixelStride;
        uint64_t size        = rowStride * static_cast<uint64_t>(height);
        totalSize += size;

        width  = width / 2;
        height = height / 2;
    }

    return totalSize;
}

Mipmap::Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount)
{
    levelCount = CalculatActualLevelCount(width, height, levelCount);

    size_t dataSize = static_cast<size_t>(CalculateDataSize(width, height, format, levelCount));
    if (dataSize == 0) {
        return;
    }

    mData.resize(dataSize);
    if (mData.size() != dataSize) {
        return;
    }

    mMips.resize(levelCount);

    const size_t pixelWidth = static_cast<size_t>(Bitmap::FormatSize(format));
    size_t       offset     = 0;
    for (uint32_t i = 0; i < levelCount; ++i) {
        char* pStorage = mData.data() + offset;

        Bitmap& mip = mMips[i];

        Result ppxres = Bitmap::Create(width, height, format, pStorage, &mip);
        if (Failed(ppxres)) {
            mMips.clear();
            return;
        }

        size_t rowStride = width * pixelWidth;
        size_t size      = rowStride * height;
        offset += size;

        width  = width / 2;
        height = height / 2;
    }
}

Mipmap::Mipmap(const Bitmap& bitmap, uint32_t levelCount)
    : Mipmap(bitmap.GetWidth(), bitmap.GetHeight(), bitmap.GetFormat(), levelCount)
{
    Bitmap* pMip0 = GetMip(0);
    if (!IsNull(pMip0)) {
        uint64_t    srcSize  = bitmap.GetFootprintSize();
        const char* pSrcData = bitmap.GetData();
        uint64_t    dstSize  = pMip0->GetFootprintSize();
        char*       pDstData = pMip0->GetData();
        if ((srcSize > 0) && (srcSize == dstSize) && !IsNull(pSrcData) && !IsNull(pDstData)) {
            memcpy(pDstData, pSrcData, srcSize);

            // Generate mip
            for (uint32_t level = 1; level < levelCount; ++level) {
                uint32_t prevLevel = level - 1;
                Bitmap*  pPrevMip  = GetMip(prevLevel);
                Bitmap*  pMip      = GetMip(level);

                Result ppxres = pPrevMip->ScaleTo(pMip);
                if (Failed(ppxres)) {
                    mData.clear();
                    mMips.clear();
                    return;
                }
            }
        }
    }
}

bool Mipmap::IsOk() const
{
    uint32_t levelCount = GetLevelCount();
    if (levelCount == 0) {
        return false;
    }

    const Bitmap& bitmap = mMips[0];

    Bitmap::Format format = bitmap.GetFormat();
    if (format == Bitmap::FORMAT_UNDEFINED) {
        return false;
    }

    uint32_t width       = bitmap.GetWidth();
    uint32_t height      = bitmap.GetHeight();
    uint64_t dataSize    = CalculateDataSize(width, height, format, levelCount);
    uint64_t storageSize = static_cast<uint64_t>(mData.size());

    if (storageSize < dataSize) {
        return false;
    }

    return true;
}

Bitmap::Format Mipmap::GetFormat() const
{
    const Bitmap* pMip = GetMip(0);
    return IsNull(pMip) ? Bitmap::FORMAT_UNDEFINED : pMip->GetFormat();
}

Bitmap* Mipmap::GetMip(uint32_t level)
{
    Bitmap* ptr = nullptr;
    if (level < GetLevelCount()) {
        ptr = &mMips[level];
    }
    return ptr;
}

const Bitmap* Mipmap::GetMip(uint32_t level) const
{
    const Bitmap* ptr = nullptr;
    if (level < GetLevelCount()) {
        ptr = &mMips[level];
    }
    return ptr;
}

uint32_t Mipmap::GetWidth(uint32_t level) const
{
    const Bitmap* pMip = GetMip(0);
    return IsNull(pMip) ? 0 : pMip->GetWidth();
}

uint32_t Mipmap::GetHeight(uint32_t level) const
{
    const Bitmap* pMip = GetMip(0);
    return IsNull(pMip) ? 0 : pMip->GetHeight();
}

uint32_t Mipmap::CalculateLevelCount(uint32_t width, uint32_t height)
{
    uint32_t levelCount = CalculatActualLevelCount(width, height, UINT32_MAX);
    return levelCount;
}

Result Mipmap::LoadFile(const std::filesystem::path& path, Mipmap* pMipmap, uint32_t levelCount)
{
    uint32_t       width  = 0;
    uint32_t       height = 0;
    Bitmap::Format format = Bitmap::FORMAT_UNDEFINED;

    Result ppxres = Bitmap::GetFileProperties(path, &width, &height, &format);
    if (Failed(ppxres)) {
        return ppxres;
    }

    uint32_t maxLevelCount = CalculateLevelCount(width, height);
    levelCount             = std::min<uint32_t>(levelCount, maxLevelCount);

    *pMipmap = Mipmap(width, height, format, levelCount);
    if (!pMipmap->IsOk()) {
        // Something has gone really wrong if this happens
        return ppx::ERROR_FAILED;
    }

    void* pStbiData            = nullptr;
    int   stbiWidth            = 0;
    int   stbiHeight           = 0;
    int   stbiChannels         = 0;
    int   stbiRequiredChannels = 4; // Force to 4 chanenls to make things easier for the graphics APIs
    if (Bitmap::ChannelDataType(format) == Bitmap::DATA_TYPE_UINT8) {
        pStbiData = stbi_load(path.string().c_str(), &stbiWidth, &stbiHeight, &stbiChannels, stbiRequiredChannels);
    }
    else if (Bitmap::ChannelDataType(format) == Bitmap::DATA_TYPE_FLOAT) {
        pStbiData = stbi_loadf(path.string().c_str(), &stbiWidth, &stbiHeight, &stbiChannels, stbiRequiredChannels);
    }

    if (IsNull(pStbiData)) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    // Copy data to mip 0
    Bitmap* pMip0 = pMipmap->GetMip(0);
    memcpy(pMip0->GetData(), pStbiData, pMip0->GetFootprintSize());

    // Free stbi data
    stbi_image_free(pStbiData);

    // Generate mip
    for (uint32_t level = 1; level < levelCount; ++level) {
        uint32_t prevLevel = level - 1;
        Bitmap*  pPrevMip  = pMipmap->GetMip(prevLevel);
        Bitmap*  pMip      = pMipmap->GetMip(level);

        ppxres = pPrevMip->ScaleTo(pMip);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

} // namespace ppx
