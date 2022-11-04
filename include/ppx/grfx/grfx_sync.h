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

#ifndef ppx_grfx_sync_h
#define ppx_grfx_sync_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct FenceCreateInfo
//!
//!
struct FenceCreateInfo
{
    bool signaled = false;
};

//! @class Fence
//!
//!
class Fence
    : public grfx::DeviceObject<grfx::FenceCreateInfo>
{
public:
    Fence() {}
    virtual ~Fence() {}

    virtual Result Wait(uint64_t timeout = UINT64_MAX) = 0;
    virtual Result Reset()                             = 0;

    Result WaitAndReset(uint64_t timeout = UINT64_MAX);

protected:
    virtual Result CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo) = 0;
    virtual void   DestroyApiObjects()                                        = 0;
    friend class grfx::Device;
};

// -------------------------------------------------------------------------------------------------

//! @struct SemaphoreCreateInfo
//!
//!
struct SemaphoreCreateInfo
{
};

//! @class Semaphore
//!
//!
class Semaphore
    : public grfx::DeviceObject<grfx::SemaphoreCreateInfo>
{
public:
    Semaphore() {}
    virtual ~Semaphore() {}

protected:
    virtual Result CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo) = 0;
    virtual void   DestroyApiObjects()                                            = 0;
    friend class grfx::Device;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_sync_h
