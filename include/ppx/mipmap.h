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

#ifndef ppx_mipmap_h
#define ppx_mipmap_h

#include "ppx/bitmap.h"
#include "ppx/grfx/grfx_constants.h"

namespace ppx {

//! @class MipMap
//!
//! Stores a mipmap as a linear chunk of memory with each mip level accessible
//! as a Bitmap.
//!
//! The expected disk format used by Mipmap::LoadFile is an vertically tailed
//! mip map:
//!   +---------------------+
//!   | MIP 0               |
//!   |                     |
//!   +---------------------+
//!   | MIP 1    |          |
//!   +----------+----------+
//!   | ... |               |
//!   +-----+---------------+
//!
class Mipmap
{
public:
    Mipmap() {}
    // Using the static, shared-memory pool is currently only safe in single-threaded applications!
    // This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
    Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount, bool useStaticPool);
    Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount);
    // Using the static, shared-memory pool is currently only safe in single-threaded applications!
    // This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
    Mipmap(const Bitmap& bitmap, uint32_t levelCount, bool useStaticPool);
    Mipmap(const Bitmap& bitmap, uint32_t levelCount);
    ~Mipmap() {}

    // Returns true if there's at least one mip level, format is valid, and storage is valid
    bool IsOk() const;

    Bitmap::Format GetFormat() const;
    uint32_t       GetLevelCount() const { return CountU32(mMips); }
    Bitmap*        GetMip(uint32_t level);
    const Bitmap*  GetMip(uint32_t level) const;

    uint32_t GetWidth(uint32_t level) const;
    uint32_t GetHeight(uint32_t level) const;

    static uint32_t CalculateLevelCount(uint32_t width, uint32_t height);
    static Result   LoadFile(const std::filesystem::path& path, uint32_t baseWidth, uint32_t baseHeight, Mipmap* pMipmap, uint32_t levelCount = PPX_REMAINING_MIP_LEVELS);
    static Result   SaveFile(const std::filesystem::path& path, const Mipmap* pMipmap, uint32_t levelCount = PPX_REMAINING_MIP_LEVELS);

private:
    std::vector<char>   mData;
    std::vector<Bitmap> mMips;

    // Static, shared-memory pool for temporary mipmap generation.
    // NOTE: This is designed for single-threaded use ONLY as it's an unprotected memory block!
    // This will need locks if the consuming paths ever become multi-threaded.
    static std::vector<char> mStaticData;
    bool                     mUseStaticPool = false;
};

} // namespace ppx

#endif // ppx_mipmap_h
