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

#include "ppx/fs.h"
#include "ppx/config.h"

#include <filesystem>
#include <vector>
#include <optional>

#if defined(PPX_ANDROID)
#include <android_native_app_glue.h>
android_app* gAndroidContext;
#endif

namespace ppx::fs {

#if defined(PPX_ANDROID)
void set_android_context(android_app* androidContext)
{
    gAndroidContext = androidContext;
}
#endif

File::File()
    : mHandleType(BAD_HANDLE)
{
}

File::~File()
{
    switch (mHandleType) {
        case ASSET_HANDLE:
#if defined(PPX_ANDROID)
            AAsset_close(mAsset);
#else
            PPX_ASSERT_MSG(false, "Bad implem. This case should never be reached.");
#endif
            break;
        case STREAM_HANDLE:
            mStream.close();
            break;
        default:
            break;
    }
}

bool File::Open(const std::filesystem::path& path)
{
#if defined(PPX_ANDROID)
    if (!path.is_absolute()) {
        mAsset      = AAssetManager_open(gAndroidContext->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
        mHandleType = mAsset != nullptr ? ASSET_HANDLE : BAD_HANDLE;
        mFileSize   = mHandleType == ASSET_HANDLE ? AAsset_getLength(mAsset) : 0;
        mFileOffset = 0;
        mBuffer     = AAsset_getBuffer(mAsset);
        return mHandleType == ASSET_HANDLE;
    }
#endif

    mStream.open(path, std::ios::binary);
    if (!mStream.good()) {
        return false;
    }

    mStream.seekg(0, std::ios::end);
    mFileSize = mStream.tellg();
    mStream.seekg(0, std::ios::beg);
    mFileOffset = 0;
    mHandleType = STREAM_HANDLE;
    return true;
}

bool File::IsValid() const
{
    if (mHandleType == STREAM_HANDLE) {
        return mStream.good();
    }
    return mHandleType == ASSET_HANDLE && mAsset != nullptr;
}

bool File::IsMapped() const
{
    return IsValid() && mBuffer != nullptr;
}

size_t File::Read(void* buffer, size_t count)
{
    PPX_ASSERT_MSG(IsValid(), "Calling File::read() on an invalid file.");

    size_t readCount = 0;

    if (IsMapped()) {
        readCount = std::min(count, GetLength() - mFileOffset);
        memcpy(buffer, reinterpret_cast<const uint8_t*>(GetPointer()) + mFileOffset, readCount);
    }
    else if (mHandleType == STREAM_HANDLE) {
        mStream.read(reinterpret_cast<char*>(buffer), count);
        readCount = mStream.gcount();
    }
#if defined(PPX_ANDROID)
    else if (mHandleType == ASSET_HANDLE) {
        readCount = AAsset_read(mAsset, buffer, count);
    }
#endif

    mFileOffset += readCount;
    return readCount;
}

size_t File::GetLength() const
{
    return mFileSize;
}

const void* File::GetPointer() const
{
    PPX_ASSERT_MSG(IsMapped(), "Called GetPointer() on an non-mapped file.");
    return mBuffer;
}

bool FileStream::Open(const char* path)
{
    auto optional_buffer = load_file(path);
    if (!optional_buffer.has_value())
        return false;
    mBuffer = optional_buffer.value();
    setg(mBuffer.data(), mBuffer.data(), mBuffer.data() + mBuffer.size());
    return true;
}

std::optional<std::vector<char>> load_file(const std::filesystem::path& path)
{
    ppx::fs::File file;
    if (!file.Open(path)) {
        return std::nullopt;
    }
    const size_t      size = file.GetLength();
    std::vector<char> buffer(size);
    const size_t      readSize = file.Read(buffer.data(), size);
    if (readSize != size) {
        return std::nullopt;
    }
    return buffer;
}

bool path_exists(const std::filesystem::path& path)
{
#if defined(PPX_ANDROID)
    if (!path.is_absolute()) {
        AAsset* temp_file = AAssetManager_open(gAndroidContext->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
        if (temp_file != nullptr) {
            AAsset_close(temp_file);
            return true;
        }

        // So it's not a file. Check if it's a subdirectory
        AAssetDir* temp_dir = AAssetManager_openDir(gAndroidContext->activity->assetManager, path.c_str());
        if (temp_dir != nullptr) {
            AAssetDir_close(temp_dir);
            return true;
        }
        return false;
    }
#endif

    return std::filesystem::exists(path);
}

#if defined(PPX_ANDROID)
std::filesystem::path GetInternalDataPath()
{
    return std::filesystem::path(gAndroidContext->activity->internalDataPath);
}
#endif

} // namespace ppx::fs
