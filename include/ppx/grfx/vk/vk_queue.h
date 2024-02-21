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

#ifndef ppx_grfx_vk_queue_h
#define ppx_grfx_vk_queue_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_queue.h"

namespace ppx {
namespace grfx {
namespace vk {

class Queue
    : public grfx::Queue
{
public:
    Queue() {}
    virtual ~Queue() {}

    VkQueuePtr GetVkQueue() const { return mQueue; }

    uint32_t GetQueueFamilyIndex() const { return mCreateInfo.queueFamilyIndex; }

    virtual Result WaitIdle() override;

    virtual Result Submit(const grfx::SubmitInfo* pSubmitInfo) override;

    virtual Result QueueWait(grfx::Semaphore* pSemaphore, uint64_t value) override;
    virtual Result QueueSignal(grfx::Semaphore* pSemaphore, uint64_t value) override;

    virtual Result GetTimestampFrequency(uint64_t* pFrequency) const override;

    VkResult TransitionImageLayout(
        VkImage              image,
        VkImageAspectFlags   aspectMask,
        uint32_t             baseMipLevel,
        uint32_t             levelCount,
        uint32_t             baseArrayLayer,
        uint32_t             layerCount,
        VkImageLayout        oldLayout,
        VkImageLayout        newLayout,
        VkPipelineStageFlags newPipelineStage);

protected:
    virtual Result CreateApiObjects(const grfx::internal::QueueCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkQueuePtr       mQueue;
    VkCommandPoolPtr mTransientPool;
    std::mutex       mQueueMutex;
    std::mutex       mCommandPoolMutex;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_queue_h
