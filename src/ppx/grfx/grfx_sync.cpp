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

#include "ppx/grfx/grfx_sync.h"

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// Fence
// -------------------------------------------------------------------------------------------------
Result Fence::WaitAndReset(uint64_t timeout)
{
    Result ppxres = Wait(timeout);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = Reset();
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// Semaphore
// -------------------------------------------------------------------------------------------------
Result Semaphore::Wait(uint64_t value, uint64_t timeout) const
{
    if (this->GetSemaphoreType() != grfx::SEMAPHORE_TYPE_TIMELINE) {
        PPX_ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
        return ppx::ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
    }

    auto ppxres = this->TimelineWait(value, timeout);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Semaphore::Signal(uint64_t value) const
{
    if (this->GetSemaphoreType() != grfx::SEMAPHORE_TYPE_TIMELINE) {
        PPX_ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
        return ppx::ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
    }

    auto ppxres = this->TimelineSignal(value);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

uint64_t Semaphore::GetCounterValue() const
{
    if (this->GetSemaphoreType() != grfx::SEMAPHORE_TYPE_TIMELINE) {
        PPX_ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
        return UINT64_MAX;
    }

    uint64_t value = this->TimelineCounterValue();
    return value;
}

} // namespace grfx
} // namespace ppx
