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

#include "ppx/platform.h"
#include "ppx/string_util.h"

#include "cpuinfo_x86.h"

#if defined(PPX_MSW)
#include <Windows.h>
#endif

namespace ppx {

static Platform sPlatform = Platform();

// -------------------------------------------------------------------------------------------------
// CpuInfo
// -------------------------------------------------------------------------------------------------
const char* GetX86LongMicroarchitectureName(cpu_features::X86Microarchitecture march)
{
    // clang-format off
    switch (march) {
        default: break;
        case cpu_features::INTEL_CORE     : return "Core"; break;
        case cpu_features::INTEL_PNR      : return "Penryn"; break;
        case cpu_features::INTEL_NHM      : return "Nehalem"; break;
        case cpu_features::INTEL_ATOM_BNL : return "Bonnell"; break;
        case cpu_features::INTEL_WSM      : return "Westmere"; break;
        case cpu_features::INTEL_SNB      : return "Sandybridge"; break;
        case cpu_features::INTEL_IVB      : return "Ivybridge"; break;
        case cpu_features::INTEL_ATOM_SMT : return "Silvermont"; break;
        case cpu_features::INTEL_HSW      : return "Haswell"; break;
        case cpu_features::INTEL_BDW      : return "Broadwell"; break;
        case cpu_features::INTEL_SKL      : return "Skylake"; break;
        case cpu_features::INTEL_ATOM_GMT : return "Goldmont"; break;
        case cpu_features::INTEL_KBL      : return "Kaby Lake"; break;
        case cpu_features::INTEL_CFL      : return "Coffee Lake"; break;
        case cpu_features::INTEL_WHL      : return "Whiskey Lake"; break;
        case cpu_features::INTEL_CNL      : return "Cannon Lake"; break;
        case cpu_features::INTEL_ICL      : return "Ice Lake"; break;
        case cpu_features::INTEL_TGL      : return "Tiger Lake"; break;
        case cpu_features::INTEL_SPR      : return "Sapphire Rapids"; break;
        case cpu_features::AMD_HAMMER     : return "K8"; break;
        case cpu_features::AMD_K10        : return "K10"; break;
        case cpu_features::AMD_BOBCAT     : return "K14"; break;
        case cpu_features::AMD_BULLDOZER  : return "K15"; break;
        case cpu_features::AMD_JAGUAR     : return "K16"; break;
        case cpu_features::AMD_ZEN        : return "K17"; break;
    };
    // clang-format on
    return "Unknown X86 Architecture";
}

CpuInfo GetX86CpuInfo()
{
    cpu_features::X86Info              info      = cpu_features::GetX86Info();
    cpu_features::X86Microarchitecture march     = cpu_features::GetX86Microarchitecture(&info);

    CpuInfo cpuInfo                  = {};
    cpuInfo.mBrandString             = string_util::TrimCopy(info.brand_string);
    cpuInfo.mVendorString            = string_util::TrimCopy(info.vendor);
    cpuInfo.mMicroarchitectureString = string_util::TrimCopy(GetX86LongMicroarchitectureName(march));

    cpuInfo.mFeatures.sse                 = static_cast<bool>(info.features.sse);
    cpuInfo.mFeatures.sse2                = static_cast<bool>(info.features.sse2);
    cpuInfo.mFeatures.sse3                = static_cast<bool>(info.features.sse3);
    cpuInfo.mFeatures.ssse3               = static_cast<bool>(info.features.ssse3);
    cpuInfo.mFeatures.sse4_1              = static_cast<bool>(info.features.sse4_1);
    cpuInfo.mFeatures.sse4_2              = static_cast<bool>(info.features.sse4_2);
    cpuInfo.mFeatures.sse4a               = static_cast<bool>(info.features.sse4a);
    cpuInfo.mFeatures.avx                 = static_cast<bool>(info.features.avx);
    cpuInfo.mFeatures.avx2                = static_cast<bool>(info.features.avx2);
    cpuInfo.mFeatures.avx512f             = static_cast<bool>(info.features.avx512f);
    cpuInfo.mFeatures.avx512cd            = static_cast<bool>(info.features.avx512cd);
    cpuInfo.mFeatures.avx512er            = static_cast<bool>(info.features.avx512er);
    cpuInfo.mFeatures.avx512pf            = static_cast<bool>(info.features.avx512pf);
    cpuInfo.mFeatures.avx512bw            = static_cast<bool>(info.features.avx512bw);
    cpuInfo.mFeatures.avx512dq            = static_cast<bool>(info.features.avx512dq);
    cpuInfo.mFeatures.avx512vl            = static_cast<bool>(info.features.avx512vl);
    cpuInfo.mFeatures.avx512ifma          = static_cast<bool>(info.features.avx512ifma);
    cpuInfo.mFeatures.avx512vbmi          = static_cast<bool>(info.features.avx512vbmi);
    cpuInfo.mFeatures.avx512vbmi2         = static_cast<bool>(info.features.avx512vbmi2);
    cpuInfo.mFeatures.avx512vnni          = static_cast<bool>(info.features.avx512vnni);
    cpuInfo.mFeatures.avx512bitalg        = static_cast<bool>(info.features.avx512bitalg);
    cpuInfo.mFeatures.avx512vpopcntdq     = static_cast<bool>(info.features.avx512vpopcntdq);
    cpuInfo.mFeatures.avx512_4vnniw       = static_cast<bool>(info.features.avx512_4vnniw);
    cpuInfo.mFeatures.avx512_4vbmi2       = static_cast<bool>(info.features.avx512_4vbmi2);
    cpuInfo.mFeatures.avx512_second_fma   = static_cast<bool>(info.features.avx512_second_fma);
    cpuInfo.mFeatures.avx512_4fmaps       = static_cast<bool>(info.features.avx512_4fmaps);
    cpuInfo.mFeatures.avx512_bf16         = static_cast<bool>(info.features.avx512_bf16);
    cpuInfo.mFeatures.avx512_vp2intersect = static_cast<bool>(info.features.avx512_vp2intersect);
    cpuInfo.mFeatures.amx_bf16            = static_cast<bool>(info.features.amx_bf16);
    cpuInfo.mFeatures.amx_tile            = static_cast<bool>(info.features.amx_tile);
    cpuInfo.mFeatures.amx_int8            = static_cast<bool>(info.features.amx_int8);

    return cpuInfo;
}

// -------------------------------------------------------------------------------------------------
// Platform
// -------------------------------------------------------------------------------------------------
Platform::Platform()
{
    mCpuInfo = GetX86CpuInfo();
}

Platform::~Platform()
{
}

PlatformId Platform::GetPlatformId()
{
#if defined(PPX_GGP)
    return ppx::PLATFORM_ID_GGP;
#elif defined(PPX_LINUX)
    return ppx::PLATFORM_ID_LINUX;
#elif defined(PPX_MSW)
    return ppx::PLATFORM_ID_MSW;
#else
    return ppx::PLATFORM_ID_UNDEFINED;
#endif
}

const char* Platform::GetPlatformString()
{
#if defined(PPX_GGP)
    return "GGP";
#elif defined(PPX_LINUX)
    return "Linux";
#elif defined(PPX_MSW)
    return "Windows";
#endif
    return "<unknown platform>";
}

const CpuInfo& Platform::GetCpuInfo()
{
    return sPlatform.mCpuInfo;
}

} // namespace ppx
