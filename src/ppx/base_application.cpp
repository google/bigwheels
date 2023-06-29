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

#include "ppx/base_application.h"

#if defined(__linux__) || defined(__MINGW32__)
#include <unistd.h>
#endif

namespace ppx {

BaseApplication::BaseApplication()
{
}

BaseApplication::~BaseApplication()
{
}

ppx::PlatformId BaseApplication::GetPlatformId() const
{
    ppx::PlatformId platform = Platform::GetPlatformId();
    return platform;
}

uint32_t BaseApplication::GetProcessId() const
{
    uint32_t pid = UINT32_MAX;
#if defined(PPX_LINUX)
    pid = static_cast<uint32_t>(getpid());
#elif defined(PPX_MSW)
    pid                       = static_cast<uint32_t>(::GetCurrentProcessId());
#endif
    return pid;
}

std::filesystem::path BaseApplication::GetApplicationPath() const
{
    std::filesystem::path path;
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
    char buf[PATH_MAX];
    std::memset(buf, 0, PATH_MAX);
    readlink("/proc/self/exe", buf, PATH_MAX);
    path = std::filesystem::path(buf);
#elif defined(PPX_MSW)
    HMODULE this_win32_module = GetModuleHandleA(nullptr);
    char    buf[MAX_PATH];
    std::memset(buf, 0, MAX_PATH);
    GetModuleFileNameA(this_win32_module, buf, MAX_PATH);
    path = std::filesystem::path(buf);
#else
#error "not implemented"
#endif
    return path;
}

void BaseApplication::AddAssetDir(const std::filesystem::path& path, bool insertAtFront)
{
    auto it = Find(mAssetDirs, path);
    if (it != std::end(mAssetDirs)) {
        return;
    }

#if !defined(PPX_ANDROID)
    if (!std::filesystem::is_directory(path)) {
        return;
    }
#endif

    mAssetDirs.push_back(path);

    if (insertAtFront) {
        // Rotate to front
        std::rotate(
            std::rbegin(mAssetDirs),
            std::rbegin(mAssetDirs) + 1,
            std::rend(mAssetDirs));
    }
}

std::filesystem::path BaseApplication::GetAssetPath(const std::filesystem::path& subPath) const
{
    std::filesystem::path assetPath;
    for (const auto& assetDir : mAssetDirs) {
        std::filesystem::path path = assetDir / subPath;
        if (ppx::fs::path_exists(path)) {
            assetPath = path;
            break;
        }
    }
    return assetPath;
}

} // namespace ppx
