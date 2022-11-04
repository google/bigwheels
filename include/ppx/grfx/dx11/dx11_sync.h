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

#ifndef ppx_grfx_dx11_sync_h
#define ppx_grfx_dx11_sync_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_sync.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Fence
    : public grfx::Fence
{
public:
    Fence() {}
    virtual ~Fence() {}

    virtual Result Wait(uint64_t timeout = UINT64_MAX) override;
    virtual Result Reset() override;

protected:
    virtual Result CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
};

// -------------------------------------------------------------------------------------------------

class Semaphore
    : public grfx::Semaphore
{
public:
    Semaphore() {}
    virtual ~Semaphore() {}

    UINT64 GetNextSignalValue();
    UINT64 GetWaitForValue() const;

protected:
    virtual Result CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_sync_h
