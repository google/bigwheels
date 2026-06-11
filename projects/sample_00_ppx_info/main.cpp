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

#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

static void LogCpuInfo()
{
    const CpuInfo& cpu = Platform::GetCpuInfo();
    // clang-format off
    PPX_LOG_INFO("Platform         : " << Platform::GetPlatformString());
    PPX_LOG_INFO("CPU vendor       : " << cpu.GetVendorString());
    PPX_LOG_INFO("CPU microarch    : " << cpu.GetMicroarchitectureString());
#if defined(__aarch64__) || defined(_M_ARM64)
    const CpuInfo::AArch64Features& f = cpu.GetAArch64Features();
    PPX_LOG_INFO("   fp            : " << f.fp);
    PPX_LOG_INFO("   asimd         : " << f.asimd);
    PPX_LOG_INFO("   aes           : " << f.aes);
    PPX_LOG_INFO("   pmull         : " << f.pmull);
    PPX_LOG_INFO("   sha1          : " << f.sha1);
    PPX_LOG_INFO("   sha2          : " << f.sha2);
    PPX_LOG_INFO("   sha512        : " << f.sha512);
    PPX_LOG_INFO("   sha3          : " << f.sha3);
    PPX_LOG_INFO("   crc32         : " << f.crc32);
    PPX_LOG_INFO("   atomics       : " << f.atomics);
    PPX_LOG_INFO("   fphp          : " << f.fphp);
    PPX_LOG_INFO("   asimdhp       : " << f.asimdhp);
    PPX_LOG_INFO("   asimdrdm      : " << f.asimdrdm);
    PPX_LOG_INFO("   asimddp       : " << f.asimddp);
    PPX_LOG_INFO("   asimdfhm      : " << f.asimdfhm);
    PPX_LOG_INFO("   fcma          : " << f.fcma);
    PPX_LOG_INFO("   lrcpc         : " << f.lrcpc);
    PPX_LOG_INFO("   dcpop         : " << f.dcpop);
    PPX_LOG_INFO("   dit           : " << f.dit);
    PPX_LOG_INFO("   ssbs          : " << f.ssbs);
    PPX_LOG_INFO("   bti           : " << f.bti);
    PPX_LOG_INFO("   paca          : " << f.paca);
    PPX_LOG_INFO("   pacg          : " << f.pacg);
    PPX_LOG_INFO("   rng           : " << f.rng);
    PPX_LOG_INFO("   mte           : " << f.mte);
    PPX_LOG_INFO("   sve           : " << f.sve);
    PPX_LOG_INFO("   sve2          : " << f.sve2);
    PPX_LOG_INFO("   i8mm          : " << f.i8mm);
    PPX_LOG_INFO("   bf16          : " << f.bf16);
    PPX_LOG_INFO("   sme           : " << f.sme);
#else
    const CpuInfo::Features& f = cpu.GetFeatures();
    PPX_LOG_INFO("   SSE           : " << f.sse);
    PPX_LOG_INFO("   SSE2          : " << f.sse2);
    PPX_LOG_INFO("   SSE3          : " << f.sse3);
    PPX_LOG_INFO("   SSSE3         : " << f.ssse3);
    PPX_LOG_INFO("   SSE4_1        : " << f.sse4_1);
    PPX_LOG_INFO("   SSE4_2        : " << f.sse4_2);
    PPX_LOG_INFO("   SSE4A         : " << f.sse4a);
    PPX_LOG_INFO("   AVX           : " << f.avx);
    PPX_LOG_INFO("   AVX2          : " << f.avx2);
#endif
    // clang-format on
}

int main(int argc, char** argv)
{
    ppx::Log::Initialize(LOG_MODE_CONSOLE);

    LogCpuInfo();

    grfx::InstanceCreateInfo createInfo = {};
    createInfo.api                      = kApi;
    createInfo.createDevices            = true; // Tells the instance to automatically create a device for each GPU it finds.
    createInfo.enableDebug              = true; // Enable layers.

    grfx::InstancePtr instance;
    Result            ppxres = grfx::CreateInstance(&createInfo, &instance);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "grfx::CreateInstance failed");
        return EXIT_FAILURE;
    }

    grfx::DestroyInstance(instance);

    return 0;
}
