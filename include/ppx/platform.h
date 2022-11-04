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

#ifndef ppx_platform_h
#define ppx_platform_h

#include "ppx/config.h"

namespace ppx {

enum PlatformId
{
    PLATFORM_ID_UNDEFINED = 0,
    PLATFORM_ID_GGP,
    PLATFORM_ID_LINUX,
    PLATFORM_ID_MSW,
};

class CpuInfo
{
public:
    struct Features
    {
        bool sse                 = false;
        bool sse2                = false;
        bool sse3                = false;
        bool ssse3               = false;
        bool sse4_1              = false;
        bool sse4_2              = false;
        bool sse4a               = false;
        bool avx                 = false;
        bool avx2                = false;
        bool avx512f             = false;
        bool avx512cd            = false;
        bool avx512er            = false;
        bool avx512pf            = false;
        bool avx512bw            = false;
        bool avx512dq            = false;
        bool avx512vl            = false;
        bool avx512ifma          = false;
        bool avx512vbmi          = false;
        bool avx512vbmi2         = false;
        bool avx512vnni          = false;
        bool avx512bitalg        = false;
        bool avx512vpopcntdq     = false;
        bool avx512_4vnniw       = false;
        bool avx512_4vbmi2       = false;
        bool avx512_second_fma   = false;
        bool avx512_4fmaps       = false;
        bool avx512_bf16         = false;
        bool avx512_vp2intersect = false;
        bool amx_bf16            = false;
        bool amx_tile            = false;
        bool amx_int8            = false;
    };

    // ---------------------------------------------------------------------------------------------

    CpuInfo() {}
    ~CpuInfo() {}

    const char*     GetBrandString() const { return mBrandString.c_str(); }
    const char*     GetVendorString() const { return mVendorString.c_str(); }
    const char*     GetMicroarchitectureString() const { return mMicroarchitectureString.c_str(); }
    uint32_t        GetL1CacheSize() const { return mL1CacheSize; }
    uint32_t        GetL2CacheSize() const { return mL2CacheSize; }
    uint32_t        GetL3CacheSize() const { return mL3CacheSize; }
    uint32_t        GetL1CacheLineSize() const { return mL1CacheLineSize; }
    uint32_t        GetL2CacheLineSize() const { return mL2CacheLineSize; }
    uint32_t        GetL3CacheLineSize() const { return mL3CacheLineSize; }
    const Features& GetFeatures() const { return mFeatures; }

private:
    friend CpuInfo GetX86CpuInfo();

private:
    std::string mBrandString;
    std::string mVendorString;
    std::string mMicroarchitectureString;
    uint32_t    mL1CacheSize     = 0;
    uint32_t    mL2CacheSize     = 0;
    uint32_t    mL3CacheSize     = 0;
    uint32_t    mL1CacheLineSize = 0;
    uint32_t    mL2CacheLineSize = 0;
    uint32_t    mL3CacheLineSize = 0;
    Features    mFeatures        = {0};
};

class Platform
{
public:
    Platform();
    ~Platform();

    static PlatformId     GetPlatformId();
    static const char*    GetPlatformString();
    static const CpuInfo& GetCpuInfo();

private:
    CpuInfo mCpuInfo;
};

//PlatformId GetPlatformId();
//CpuInfo    GetCpuInfo();
//float      GetCurrentCpuUsage();

} // namespace ppx

#endif // ppx_platform_h
