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

#include "ppx/grfx/dx11/dx11_shader.h"
#include "ppx/grfx/dx11/dx11_device.h"

namespace ppx {
namespace grfx {
namespace dx11 {

Result ShaderModule::CreateApiObjects(const grfx::ShaderModuleCreateInfo* pCreateInfo)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo->pCode);
    if (IsNull(pCreateInfo->pCode) || (pCreateInfo->size == 0)) {
        return ppx::ERROR_INVALID_CREATE_ARGUMENT;
    }

    mCode.resize(pCreateInfo->size);
    std::memcpy(mCode.data(), pCreateInfo->pCode, pCreateInfo->size);

    return ppx::SUCCESS;
}

void ShaderModule::DestroyApiObjects()
{
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
