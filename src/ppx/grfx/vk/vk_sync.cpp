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

#include "ppx/grfx/vk/vk_sync.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_queue.h"

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// Fence
// -------------------------------------------------------------------------------------------------
Result Fence::CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo)
{
    VkFenceCreateInfo vkci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkci.flags             = pCreateInfo->signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult vkres = vkCreateFence(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mFence);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateFence failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Fence::DestroyApiObjects()
{
    if (mFence) {
        vkDestroyFence(
            ToApi(GetDevice())->GetVkDevice(),
            mFence,
            nullptr);

        mFence.Reset();
    }
}

Result Fence::Wait(uint64_t timeout)
{
    VkResult vkres = vkWaitForFences(
        ToApi(GetDevice())->GetVkDevice(),
        1,
        mFence,
        VK_TRUE,
        timeout);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result Fence::Reset()
{
    VkResult vkres = vkResetFences(
        ToApi(GetDevice())->GetVkDevice(),
        1,
        mFence);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// Semaphore
// -------------------------------------------------------------------------------------------------
Result Semaphore::CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo)
{
    VkSemaphoreTypeCreateInfo timelineCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    timelineCreateInfo.pNext                     = nullptr;
    timelineCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue              = pCreateInfo->initialValue;

    VkSemaphoreCreateInfo vkci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkci.pNext                 = (pCreateInfo->semaphoreType == grfx::SEMAPHORE_TYPE_TIMELINE) ? &timelineCreateInfo : nullptr;
    vkci.flags                 = 0;

    VkResult vkres = vkCreateSemaphore(
        ToApi(GetDevice())->GetVkDevice(),
        &vkci,
        nullptr,
        &mSemaphore);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkCreateSemaphore failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Semaphore::DestroyApiObjects()
{
    if (mSemaphore) {
        vkDestroySemaphore(
            ToApi(GetDevice())->GetVkDevice(),
            mSemaphore,
            nullptr);

        mSemaphore.Reset();
    }
}

Result Semaphore::TimelineWait(uint64_t value, uint64_t timeout) const
{
    PPX_ASSERT_MSG((this->GetSemaphoreType() == grfx::SEMAPHORE_TYPE_TIMELINE), REQUIRES_TIMELINE_MSG);

    VkSemaphore semaphoreHandle = mSemaphore;

    VkSemaphoreWaitInfo waitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    waitInfo.pNext               = nullptr;
    waitInfo.flags               = 0;
    waitInfo.semaphoreCount      = 1;
    waitInfo.pSemaphores         = &semaphoreHandle;
    waitInfo.pValues             = &value;

    VkResult vkres = ToApi(GetDevice())->WaitSemaphores(&waitInfo, timeout);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

Result Semaphore::TimelineSignal(uint64_t value) const
{
    PPX_ASSERT_MSG((this->GetSemaphoreType() == grfx::SEMAPHORE_TYPE_TIMELINE), REQUIRES_TIMELINE_MSG);

    // See header for explanation
    if (value > mTimelineSignaledValue) {
        VkSemaphore semaphoreHandle = mSemaphore;

        VkSemaphoreSignalInfo signalInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
        signalInfo.pNext                 = nullptr;
        signalInfo.semaphore             = mSemaphore;
        signalInfo.value                 = value;

        VkResult vkres = ToApi(GetDevice())->SignalSemaphore(&signalInfo);
        if (vkres != VK_SUCCESS) {
            return ppx::ERROR_API_FAILURE;
        }

        mTimelineSignaledValue = value;
    }

    return ppx::SUCCESS;
}

uint64_t Semaphore::TimelineCounterValue() const
{
    PPX_ASSERT_MSG((this->GetSemaphoreType() == grfx::SEMAPHORE_TYPE_TIMELINE), REQUIRES_TIMELINE_MSG);

    uint64_t value = UINT64_MAX;
    VkResult vkres = ToApi(GetDevice())->GetSemaphoreCounterValue(mSemaphore, &value);

    // Not clear if value is written to upon failure so prefer safety.
    return (vkres == VK_SUCCESS) ? value : UINT64_MAX;
}

} // namespace vk
} // namespace grfx
} // namespace ppx
