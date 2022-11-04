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

#if defined(USE_DX11)
const grfx::Api kApi = grfx::API_DX_11_1;
#elif defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

int main(int argc, char** argv)
{
    grfx::InstanceCreateInfo createInfo = {};
    createInfo.api                      = kApi;
    createInfo.createDevices            = true; // Tells the instance to automatically create a device for each GPU it finds.
    createInfo.enableDebug              = true; // Enable layers.

    grfx::InstancePtr instance;
    Result            ppxres = grfx::CreateInstance(&createInfo, &instance);
    if (ppxres != ppx::SUCCESS) {
        PPX_ASSERT_MSG(false, "grfx::CreateInstance failed");
        return EXIT_FAILURE;
    }

    ppx::Log::Initialize(LOG_MODE_CONSOLE);
    PPX_LOG_INFO("Graphics instance and devices created successfully.");

    grfx::DestroyInstance(instance);

    return 0;
}
