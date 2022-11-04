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

#include "ppx/grfx/dx11/dx11_sync.h"

namespace ppx {
namespace grfx {
namespace dx11 {

// -------------------------------------------------------------------------------------------------
// Fence
// -------------------------------------------------------------------------------------------------
Result Fence::CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo)
{
    return ppx::SUCCESS;
}

void Fence::DestroyApiObjects()
{
}

Result Fence::Wait(uint64_t timeout)
{
    return ppx::SUCCESS;
}

Result Fence::Reset()
{
    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// Semaphore
// -------------------------------------------------------------------------------------------------
Result Semaphore::CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo)
{
    return ppx::SUCCESS;
}

void Semaphore::DestroyApiObjects()
{
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
