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

#if defined(PPX_ANDROID)
// TODO: Fill out ANDROID-specific header
#elif defined(__aarch64__) || defined(_M_ARM64)
#include "cpuinfo_aarch64.h"
#else
#include "cpuinfo_x86.h"
#endif

#if defined(PPX_MSW)
#include <Windows.h>
#endif

namespace ppx {

static Platform sPlatform = Platform();

// -------------------------------------------------------------------------------------------------
// CpuInfo
// -------------------------------------------------------------------------------------------------
#if defined(PPX_ANDROID)
// TODO: Fill out ANDROID-specific info
#elif defined(__aarch64__) || defined(_M_ARM64)
namespace {

const char* GetAArch64VendorName(int implementer)
{
    // Implementer codes from Linux kernel arch/arm64/include/asm/cputype.h (ARM_CPU_IMP_*).
    switch (implementer) {
        case 0x41: return "ARM";
        case 0x42: return "Broadcom";
        case 0x43: return "Cavium";
        case 0x46: return "Fujitsu";
        case 0x48: return "HiSilicon";
        case 0x4e: return "NVIDIA";
        case 0x50: return "APM";
        case 0x51: return "Qualcomm";
        case 0x61: return "Apple";
        case 0x6d: return "Microsoft";
        case 0xc0: return "Ampere";
        default: break;
    }
    return "Unknown";
}

const char* GetAArch64ArmMicroarchitectureName(int part)
{
    switch (part) {
        case 0xD03: return "Cortex-A53";
        case 0xD04: return "Cortex-A35";
        case 0xD05: return "Cortex-A55";
        case 0xD07: return "Cortex-A57";
        case 0xD08: return "Cortex-A72";
        case 0xD09: return "Cortex-A73";
        case 0xD0A: return "Cortex-A75";
        case 0xD0B: return "Cortex-A76";
        case 0xD0C: return "Neoverse-N1";
        case 0xD0D: return "Cortex-A77";
        case 0xD0E: return "Cortex-A76AE";
        case 0xD40: return "Neoverse-V1";
        case 0xD41: return "Cortex-A78";
        case 0xD42: return "Cortex-A78AE";
        case 0xD44: return "Cortex-X1";
        case 0xD46: return "Cortex-A510";
        case 0xD47: return "Cortex-A710";
        case 0xD48: return "Cortex-X2";
        case 0xD49: return "Neoverse-N2";
        case 0xD4B: return "Cortex-A78C";
        case 0xD4C: return "Cortex-X1C";
        case 0xD4D: return "Cortex-A715";
        case 0xD4E: return "Cortex-X3";
        case 0xD4F: return "Neoverse-V2";
        case 0xD80: return "Cortex-A520";
        case 0xD81: return "Cortex-A720";
        case 0xD82: return "Cortex-X4";
        case 0xD83: return "Neoverse-V3AE";
        case 0xD84: return "Neoverse-V3";
        case 0xD85: return "Cortex-X925";
        case 0xD87: return "Cortex-A725";
        case 0xD89: return "Cortex-A720AE";
        case 0xD8E: return "Neoverse-N3";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64BroadcomMicroarchitectureName(int part)
{
    switch (part) {
        case 0x100: return "Brahma-B53";
        case 0x516: return "Vulcan";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64CaviumMicroarchitectureName(int part)
{
    switch (part) {
        case 0x0A1: return "ThunderX";
        case 0x0A2: return "ThunderX 81xx";
        case 0x0A3: return "ThunderX 83xx";
        case 0x0AF: return "ThunderX2";
        case 0x0B1: return "OcteonTX2 98xx";
        case 0x0B2: return "OcteonTX2 96xx";
        case 0x0B3: return "OcteonTX2 95xx";
        case 0x0B4: return "OcteonTX2 95xxN";
        case 0x0B5: return "OcteonTX2 95xxMM";
        case 0x0B6: return "OcteonTX2 95xxO";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64FujitsuMicroarchitectureName(int part)
{
    switch (part) {
        case 0x001: return "A64FX";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64HiSiliconMicroarchitectureName(int part)
{
    switch (part) {
        case 0xD01: return "TSV110";
        case 0xD02: return "HIP09";
        case 0xD06: return "HIP12";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64NvidiaMicroarchitectureName(int part)
{
    switch (part) {
        case 0x003: return "Denver";
        case 0x004: return "Carmel";
        case 0x010: return "Olympus";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64ApmMicroarchitectureName(int part)
{
    switch (part) {
        case 0x000: return "X-Gene";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64QualcommMicroarchitectureName(int part)
{
    switch (part) {
        case 0x001: return "Oryon X1";
        case 0x200: return "Kryo";
        case 0x800: return "Falkor V1";
        case 0x801: return "Kryo 2xx Silver";
        case 0x802: return "Kryo 3xx Gold";
        case 0x803: return "Kryo 3xx Silver";
        case 0x804: return "Kryo 4xx Gold";
        case 0x805: return "Kryo 4xx Silver";
        case 0xC00: return "Falkor";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64AppleMicroarchitectureName(int part)
{
    switch (part) {
        case 0x022: return "M1 Icestorm";
        case 0x023: return "M1 Firestorm";
        case 0x024: return "M1 Icestorm Pro";
        case 0x025: return "M1 Firestorm Pro";
        case 0x028: return "M1 Icestorm Max";
        case 0x029: return "M1 Firestorm Max";
        case 0x032: return "M2 Blizzard";
        case 0x033: return "M2 Avalanche";
        case 0x034: return "M2 Blizzard Pro";
        case 0x035: return "M2 Avalanche Pro";
        case 0x038: return "M2 Blizzard Max";
        case 0x039: return "M2 Avalanche Max";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64MicrosoftMicroarchitectureName(int part)
{
    switch (part) {
        case 0xD49: return "Azure Cobalt 100";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64AmpereMicroarchitectureName(int part)
{
    switch (part) {
        case 0xAC3: return "Ampere1";
        case 0xAC4: return "Ampere1A";
        default: break;
    }
    return nullptr;
}

const char* GetAArch64MicroarchitectureName(int implementer, int part)
{
    // Part numbers from Linux kernel arch/arm64/include/asm/cputype.h (*_CPU_PART_*).
    const char* name = nullptr;
    switch (implementer) {
        case 0x41: name = GetAArch64ArmMicroarchitectureName(part); break;
        case 0x42: name = GetAArch64BroadcomMicroarchitectureName(part); break;
        case 0x43: name = GetAArch64CaviumMicroarchitectureName(part); break;
        case 0x46: name = GetAArch64FujitsuMicroarchitectureName(part); break;
        case 0x48: name = GetAArch64HiSiliconMicroarchitectureName(part); break;
        case 0x4e: name = GetAArch64NvidiaMicroarchitectureName(part); break;
        case 0x50: name = GetAArch64ApmMicroarchitectureName(part); break;
        case 0x51: name = GetAArch64QualcommMicroarchitectureName(part); break;
        case 0x61: name = GetAArch64AppleMicroarchitectureName(part); break;
        case 0x6d: name = GetAArch64MicrosoftMicroarchitectureName(part); break;
        case 0xc0: name = GetAArch64AmpereMicroarchitectureName(part); break;
        default: break;
    }
    return name ? name : "Unknown AArch64";
}

CpuInfo GetAArch64CpuInfo()
{
    cpu_features::Aarch64Info info = cpu_features::GetAarch64Info();

    CpuInfo cpuInfo                  = {};
    cpuInfo.mVendorString            = GetAArch64VendorName(info.implementer);
    cpuInfo.mMicroarchitectureString = GetAArch64MicroarchitectureName(info.implementer, info.part);
    // cpu_features::Aarch64Info does not carry a brand string, so mBrandString is left empty.

    cpuInfo.mAArch64Features.fp       = static_cast<bool>(info.features.fp);
    cpuInfo.mAArch64Features.asimd    = static_cast<bool>(info.features.asimd);
    cpuInfo.mAArch64Features.aes      = static_cast<bool>(info.features.aes);
    cpuInfo.mAArch64Features.pmull    = static_cast<bool>(info.features.pmull);
    cpuInfo.mAArch64Features.sha1     = static_cast<bool>(info.features.sha1);
    cpuInfo.mAArch64Features.sha2     = static_cast<bool>(info.features.sha2);
    cpuInfo.mAArch64Features.sha512   = static_cast<bool>(info.features.sha512);
    cpuInfo.mAArch64Features.sha3     = static_cast<bool>(info.features.sha3);
    cpuInfo.mAArch64Features.crc32    = static_cast<bool>(info.features.crc32);
    cpuInfo.mAArch64Features.atomics  = static_cast<bool>(info.features.atomics);
    cpuInfo.mAArch64Features.fphp     = static_cast<bool>(info.features.fphp);
    cpuInfo.mAArch64Features.asimdhp  = static_cast<bool>(info.features.asimdhp);
    cpuInfo.mAArch64Features.asimdrdm = static_cast<bool>(info.features.asimdrdm);
    cpuInfo.mAArch64Features.asimddp  = static_cast<bool>(info.features.asimddp);
    cpuInfo.mAArch64Features.asimdfhm = static_cast<bool>(info.features.asimdfhm);
    cpuInfo.mAArch64Features.fcma     = static_cast<bool>(info.features.fcma);
    cpuInfo.mAArch64Features.lrcpc    = static_cast<bool>(info.features.lrcpc);
    cpuInfo.mAArch64Features.dcpop    = static_cast<bool>(info.features.dcpop);
    cpuInfo.mAArch64Features.dit      = static_cast<bool>(info.features.dit);
    cpuInfo.mAArch64Features.ssbs     = static_cast<bool>(info.features.ssbs);
    cpuInfo.mAArch64Features.bti      = static_cast<bool>(info.features.bti);
    cpuInfo.mAArch64Features.paca     = static_cast<bool>(info.features.paca);
    cpuInfo.mAArch64Features.pacg     = static_cast<bool>(info.features.pacg);
    cpuInfo.mAArch64Features.rng      = static_cast<bool>(info.features.rng);
    cpuInfo.mAArch64Features.mte      = static_cast<bool>(info.features.mte);
    cpuInfo.mAArch64Features.sve      = static_cast<bool>(info.features.sve);
    cpuInfo.mAArch64Features.sve2     = static_cast<bool>(info.features.sve2);
    cpuInfo.mAArch64Features.i8mm     = static_cast<bool>(info.features.i8mm);
    cpuInfo.mAArch64Features.bf16     = static_cast<bool>(info.features.bf16);
    cpuInfo.mAArch64Features.sme      = static_cast<bool>(info.features.sme);

    return cpuInfo;
}

} // namespace
#else
namespace {

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
    cpu_features::X86Info              info  = cpu_features::GetX86Info();
    cpu_features::X86Microarchitecture march = cpu_features::GetX86Microarchitecture(&info);

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

} // namespace
#endif

// -------------------------------------------------------------------------------------------------
// Platform
// -------------------------------------------------------------------------------------------------
Platform::Platform()
{
#if defined(PPX_ANDROID)
    // TODO: Call ANDROID-specific function
#elif defined(__aarch64__) || defined(_M_ARM64)
    mCpuInfo = GetAArch64CpuInfo();
#else
    mCpuInfo = GetX86CpuInfo();
#endif
}

Platform::~Platform()
{
}

PlatformId Platform::GetPlatformId()
{
#if defined(PPX_LINUX)
    return ppx::PLATFORM_ID_LINUX;
#elif defined(PPX_MSW)
    return ppx::PLATFORM_ID_MSW;
#else
    return ppx::PLATFORM_ID_UNDEFINED;
#endif
}

const char* Platform::GetPlatformString()
{
#if defined(PPX_LINUX)
    return "Linux";
#elif defined(PPX_MSW)
    return "Windows";
#elif defined(PPX_ANDROID)
    return "Android";
#endif
    return "<unknown platform>";
}

const CpuInfo& Platform::GetCpuInfo()
{
    return sPlatform.mCpuInfo;
}

} // namespace ppx
