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

#ifndef ppx_grfx_vk_buffer_h
#define ppx_grfx_vk_buffer_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_buffer.h"

namespace ppx {
namespace grfx {
namespace vk {

class Buffer
    : public grfx::Buffer
{
public:
    Buffer() {}
    virtual ~Buffer() {}

    VkBufferPtr GetVkBuffer() const { return mBuffer; }

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) override;
    virtual void   UnmapMemory() override;

protected:
    virtual Result CreateApiObjects(const grfx::BufferCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkBufferPtr       mBuffer;
    VmaAllocationPtr  mAllocation;
    VmaAllocationInfo mAllocationInfo = {};
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_buffer_h
