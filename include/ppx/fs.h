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

/*
 Copyright 2018-2021 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#ifndef ppx_fs_h
#define ppx_fs_h

#include <optional>
#include <vector>
#include <filesystem>
#include <fstream>

#if defined(PPX_ANDROID)
#include <android_native_app_glue.h>
#endif

namespace ppx::fs {

#if defined(PPX_ANDROID)
void set_android_context(android_app* androidContext);
#endif

// Abstract a static, regular file on all platforms.
// This class doesn't handle sockets, or file which content is not constant
// for the lifetime of this class.
class File
{
    // Internal enum to know which handle is currently valid.
    enum FileHandleType
    {
        // Default handle, no file associated.
        BAD_HANDLE = 0,
        // The file is accessible through a stream.
        STREAM_HANDLE = 1,
        // The file is accessible through an Android asset handle.
        ASSET_HANDLE = 2,
    };

public:
    File();
    File(const File& other)  = delete;
    File(const File&& other) = delete;
    ~File();

    // Opens a file given a specific path.
    // path: the path of the file to open.
    //  - On desktop, loads the regular file at `path`. (memory-mapping availability is implementation defined).
    //  - On Android, relative path are assumed to be loaded from the APK, those are memory mapped.
    //                absolute path are loaded as regular files (mapping availability is implementation defined).
    //
    // - This API only supports regular files.
    // - This API expects the file not to change size or content while this handle is open.
    // - This class supports RAII. File will be closed on destroy.
    bool Open(const std::filesystem::path& path);

    // Reads `size` bytes from the file into `buffer`.
    // buffer: a pointer to a buffer with at least `count` writable bytes.
    // count: the maximum number of bytes to write to `buffer`.
    // Returns the number of bytes written to `buffer`.
    //
    // - The file has an internal cursor, meaning the next read will start at the end of the last read.
    // - If the file size is larger than `count`, the read stops at `count` bytes.
    size_t Read(void* buffer, size_t count);

    // Returns true if the file is readable.
    bool IsValid() const;
    // Returns true if the file is mapped in memory. See `File::GetPointer()`.
    bool IsMapped() const;

    // Returns the total size in bytes of the file from the start.
    size_t GetLength() const;
    // Returns a readable pointer to a beginning of the file. Behavior undefined if `File::IsMapped()` is false.
    const void* GetPointer() const;

private:
#if !defined(PPX_ANDROID)
    typedef void AAsset;
#endif

    FileHandleType mHandleType = BAD_HANDLE;
    AAsset*        mAsset      = nullptr;
    const void*    mBuffer     = nullptr;
    std::ifstream  mStream;
    size_t         mFileSize   = 0;
    size_t         mFileOffset = 0;
};

class FileStream : public std::streambuf
{
public:
    bool Open(const char* path);

private:
    std::vector<char> mBuffer;
};

// Opens a regular file and returns its content if the read succeeded.
// `path`: the path of the file to load.
// The path is handled differently depending on the platform:
//  - desktop: all paths are treated the same.
//  - android: relative paths are assumed to be in APK's storage (Asset API). Absolute are loaded from disk.
std::optional<std::vector<char>> load_file(const std::filesystem::path& path);

// Returns true if a given path exists (file or directory).
// `path`: the path to check.
// The path is handled differently depending on the platform:
//  - desktop: all path are treated the same.
//  - android: relative path are assumed to be in APK's storage (Asset API). Absolute are loaded from disk.
bool path_exists(const std::filesystem::path& path);

#if defined(PPX_ANDROID)
// Returns a path to the application's internal data directory (can be used for output).
// NOTE: The internal data path on Android is extremely limited in terms of filesize!
std::filesystem::path GetInternalDataPath();
#endif

} // namespace ppx::fs

#endif // ppx_fs_h
