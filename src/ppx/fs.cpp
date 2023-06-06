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

bool File::Open(const std::filesystem::path& path)
{
#if defined(PPX_ANDROID)
    mFile = AAssetManager_open(gAndroidContext->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
    return mFile != nullptr;
#else
    mStream.open(path, std::ios::binary);
    return mStream.good();
#endif
}

bool File::IsOpen() const
{
#if defined(PPX_ANDROID)
    return mFile != nullptr;
#else
    return mStream.is_open();
#endif
}

size_t File::Read(void* buf, size_t count)
{
#if defined(PPX_ANDROID)
    return AAsset_read(mFile, buf, count);
#else
    mStream.read(reinterpret_cast<char*>(buf), count);
    return mStream.gcount();
#endif
}

size_t File::GetLength()
{
#if defined(PPX_ANDROID)
    return AAsset_getLength(mFile);
#else
    std::streamoff pos = mStream.tellg();
    mStream.seekg(0, std::ios::end);
    size_t size = mStream.tellg();
    mStream.seekg(pos, std::ios::beg);
    return size;
#endif
}

void File::Close()
{
#if defined(PPX_ANDROID)
    AAsset_close(mFile);
#else
    mStream.close();
#endif
}

#if defined(PPX_ANDROID)
const void* File::GetBuffer()
{
    return AAsset_getBuffer(mFile);
}
#endif

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
    std::vector<char> data;

    ppx::fs::File file;
    if (!file.Open(path)) {
        return std::nullopt;
    }
    size_t size = file.GetLength();
    data.resize(size);

#if defined(PPX_ANDROID)
    // Allow the system to use MMIO if available.
    const void* buf = file.GetBuffer();
    memcpy(data.data(), buf, size);
#else
    file.Read(data.data(), size);
#endif
    file.Close();
    return data;
}

bool path_exists(const std::filesystem::path& path)
{
#if defined(PPX_ANDROID)
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
#else
    return std::filesystem::exists(path);
#endif
}

std::filesystem::path GetValidPathToOutputFile(const std::filesystem::path& path)
{
    PPX_ASSERT_MSG(path.is_relative(), "Only relative paths are valid for output files.");
#if defined(PPX_ANDROID)
    // NOTE: The internal data path on Android is extremely limited in terms of filesize!
    std::filesystem::path internalDataPath(gAndroidContext->activity->internalDataPath);
    std::filesystem::path outputPath = internalDataPath / path;
    std::filesystem::create_directories(outputPath.parent_path());
    return outputPath;
#else
    std::filesystem::create_directories(path.parent_path());
    return path;
#endif
}
} // namespace ppx::fs
