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

#ifndef ppx_grfx_vk_sync_h
#define ppx_grfx_vk_sync_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_sync.h"

namespace ppx {
namespace grfx {
namespace vk {

//! @class Fence
//!
//!
class Fence
    : public grfx::Fence
{
public:
    Fence() {}
    virtual ~Fence() {}

    VkFencePtr GetVkFence() const { return mFence; }

    virtual Result Wait(uint64_t timeout = UINT64_MAX) override;
    virtual Result Reset() override;

protected:
    virtual Result CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkFencePtr mFence;
};

// -------------------------------------------------------------------------------------------------

//! @class Semaphore
//!
//!
class Semaphore
    : public grfx::Semaphore
{
public:
    Semaphore() {}
    virtual ~Semaphore() {}

    VkSemaphorePtr GetVkSemaphore() const { return mSemaphore; }

private:
    virtual Result   TimelineWait(uint64_t value, uint64_t timeout) const override;
    virtual Result   TimelineSignal(uint64_t value) const override;
    virtual uint64_t TimelineCounterValue() const override;

protected:
    virtual Result CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    //
    // Why are we storing timeline semaphore signaled values?
    //
    // Direct3D allows fence objects to signal a value if the value is
    // equal to or greater than what's already been signaled.
    //
    // Vulkan does not:
    //   https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreSignalInfo.html#VUID-VkSemaphoreSignalInfo-value-03258
    //
    // This is unfortunate, because there are cases where an application
    // may need to signal a value that is equal to what's been signaled.
    //
    // Even though it's possible to get the current value, add 1 to it,
    // and then signal it - this can create a different problem where a value
    // is signaled too soon and a write-after-read hazard
    // possibly gets introduced.
    //
    mutable uint64_t mTimelineSignaledValue = 0;
    VkSemaphorePtr   mSemaphore;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_sync_h
