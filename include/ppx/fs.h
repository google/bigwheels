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
#include <game-activity/native_app_glue/android_native_app_glue.h>
#endif

namespace ppx::fs {

#if defined(PPX_ANDROID)
void set_android_context(android_app* androidContext);
#endif

//! @class File
//!
//!
class File
{
public:
    bool   Open(const std::filesystem::path& path);
    bool   IsOpen() const;
    size_t Read(void* buf, size_t count);
    size_t GetLength();
    void   Close();

private:
#if defined(PPX_ANDROID)
    AAsset* mFile = nullptr;
#else
    std::ifstream mStream;
#endif
};

class FileStream : public std::streambuf
{
public:
    bool Open(const char* path);

private:
    std::vector<char> mBuffer;
};

std::optional<std::vector<char>> load_file(const std::filesystem::path& path);
bool                             path_exists(const std::filesystem::path& path);

} // namespace ppx::fs

#endif // ppx_fs_h
