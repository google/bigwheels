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

#ifndef ppx_grfx_queue_h
#define ppx_grfx_queue_h

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_command.h"

namespace ppx {
namespace grfx {

struct SubmitInfo
{
    uint32_t                          commandBufferCount   = 0;
    const grfx::CommandBuffer* const* ppCommandBuffers     = nullptr;
    uint32_t                          waitSemaphoreCount   = 0;
    const grfx::Semaphore* const*     ppWaitSemaphores     = nullptr;
    uint32_t                          signalSemaphoreCount = 0;
    grfx::Semaphore**                 ppSignalSemaphores   = nullptr;
    grfx::Fence*                      pFence               = nullptr;
};

namespace internal {

//! @struct QueueCreateInfo
//!
//!
struct QueueCreateInfo
{
    grfx::CommandType commandType      = grfx::COMMAND_TYPE_UNDEFINED;
    uint32_t          queueFamilyIndex = PPX_VALUE_IGNORED; // Vulkan
    uint32_t          queueIndex       = PPX_VALUE_IGNORED; // Vulkan
    void*             pApiObject       = nullptr;           // D3D12
};

} // namespace internal

//! @class Queue
//!
//!
class Queue
    : public grfx::DeviceObject<grfx::internal::QueueCreateInfo>
{
public:
    Queue() {}
    virtual ~Queue() {}

    grfx::CommandType GetCommandType() const { return mCreateInfo.commandType; }

    virtual Result WaitIdle() = 0;

    virtual Result Submit(const grfx::SubmitInfo* pSubmitInfo) = 0;

    // GPU timestamp frequency counter in ticks per second
    virtual Result GetTimestampFrequency(uint64_t* pFrequency) const = 0;

    Result CreateCommandBuffer(
        grfx::CommandBuffer** ppCommandBuffer,
        uint32_t              resourceDescriptorCount = PPX_DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
        uint32_t              samplerDescriptorCount  = PPX_DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
    void DestroyCommandBuffer(const grfx::CommandBuffer* pCommandBuffer);

    // In place copy of buffer to buffer
    Result CopyBufferToBuffer(
        const grfx::BufferToBufferCopyInfo* pCopyInfo,
        grfx::Buffer*                       pSrcBuffer,
        grfx::Buffer*                       pDstBuffer,
        grfx::ResourceState                 stateBefore,
        grfx::ResourceState                 stateAfter);

    // In place copy of buffer to image
    Result CopyBufferToImage(
        const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
        grfx::Buffer*                                   pSrcBuffer,
        grfx::Image*                                    pDstImage,
        uint32_t                                        mipLevel,
        uint32_t                                        mipLevelCount,
        uint32_t                                        arrayLayer,
        uint32_t                                        arrayLayerCount,
        grfx::ResourceState                             stateBefore,
        grfx::ResourceState                             stateAfter);

private:
    struct CommandSet
    {
        grfx::CommandPoolPtr   commandPool;
        grfx::CommandBufferPtr commandBuffer;
    };
    std::vector<CommandSet> mCommandSets;
    std::mutex              mCommandSetMutex;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_queue_h
