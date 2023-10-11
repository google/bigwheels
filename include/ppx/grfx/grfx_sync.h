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
    grfx::SemaphoreType semaphoreType = grfx::SEMAPHORE_TYPE_BINARY;
    uint64_t            initialValue  = 0; // Timeline semaphore only
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

    grfx::SemaphoreType GetSemaphoreType() const { return mCreateInfo.semaphoreType; }
    bool                IsBinary() const { return mCreateInfo.semaphoreType == grfx::SEMAPHORE_TYPE_BINARY; }
    bool                IsTimeline() const { return mCreateInfo.semaphoreType == grfx::SEMAPHORE_TYPE_TIMELINE; }

    // Timeline semaphore wait
    Result Wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;

    // Timeline semaphore signal
    //
    // WARNING: Signaling a value less that's less than what's already been signaled
    //          can cause a block or a race condition.
    //
    // Use forceMonotonicValue=true to use the current timeline semaphore value
    // if it's greater than the passed in value. This is useful when signaling
    // from threads where ordering is not guaranteed.
    //
    Result Signal(uint64_t value, bool forceMonotonicValue = false) const;

    // Returns current timeline semaphore value
    uint64_t GetCounterValue() const;

private:
    virtual Result   TimelineWait(uint64_t value, uint64_t timeout) const = 0;
    virtual Result   TimelineSignal(uint64_t value) const                 = 0;
    virtual uint64_t TimelineCounterValue() const                         = 0;

protected:
    virtual Result CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo) = 0;
    virtual void   DestroyApiObjects()                                            = 0;
    friend class grfx::Device;

private:
    mutable std::mutex mTimelineMutex;
};

// -------------------------------------------------------------------------------------------------

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_sync_h
