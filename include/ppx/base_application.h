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

#ifndef ppx_base_application_h
#define ppx_base_application_h

#include "ppx/config.h"
#include "ppx/platform.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_instance.h"
#include "ppx/fs.h"

#include <filesystem>

#if defined(PPX_ANDROID)
#include <android_native_app_glue.h>
#endif

namespace ppx {

class BaseApplication
{
public:
    BaseApplication();
    virtual ~BaseApplication();

    ppx::PlatformId       GetPlatformId() const;
    uint32_t              GetProcessId() const;
    std::filesystem::path GetApplicationPath() const;

    const std::vector<std::filesystem::path>& GetAssetDirs() const { return mAssetDirs; }
    void                                      AddAssetDir(const std::filesystem::path& path, bool insertAtFront = false);

    // Returns the first valid subPath in the asset directories list
    //
    // Example(s):
    //
    //    mAssetDirs = {"/a/valid/system/path",
    //                  "/another/valid/system/path",
    //                  "/some/valid/system/path"};
    //
    //    GetAssetPath("file.ext") - returns the full path to file.ext if it exists
    //      in any of the paths in mAssetDirs on the file system.
    //      Search starts with mAssetsDir[0].
    //
    //    GetAssetPath("subdir") - returns the full path to subdir if it exists
    //      in any of the paths in mAssetDirs on the file system.
    //      Search starts with mAssetsDir[0].
    //
    std::filesystem::path GetAssetPath(const std::filesystem::path& subPath) const;

#if defined(PPX_ANDROID)
    void SetAndroidContext(android_app* androidContext)
    {
        mAndroidContext = androidContext;
        fs::set_android_context(androidContext);
    }
    android_app* GetAndroidContext() const
    {
        return mAndroidContext;
    }
#endif

private:
#if defined(PPX_ANDROID)
    android_app* mAndroidContext;
#endif
    std::vector<std::filesystem::path> mAssetDirs;
};

} // namespace ppx

#endif // ppx_base_application_h
