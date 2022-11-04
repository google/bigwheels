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

#include "ppx/grfx/grfx_buffer.h"

namespace ppx {
namespace grfx {

Result Buffer::Create(const grfx::BufferCreateInfo* pCreateInfo)
{
#ifndef PPX_DISABLE_MINIMUM_BUFFER_SIZE_CHECK
    // Constant/uniform buffers need to be at least PPX_CONSTANT_BUFFER_ALIGNMENT in size
    if (pCreateInfo->usageFlags.bits.uniformBuffer && (pCreateInfo->size < PPX_CONSTANT_BUFFER_ALIGNMENT)) {
        PPX_ASSERT_MSG(false, "constant/uniform buffer sizes must be at least PPX_CONSTANT_BUFFER_ALIGNMENT (" << PPX_CONSTANT_BUFFER_ALIGNMENT << ")");
        return ppx::ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
    }

    // Storage/structured buffers need to be at least PPX_STORAGE_BUFFER_ALIGNMENT in size
    if (pCreateInfo->usageFlags.bits.uniformBuffer && (pCreateInfo->size < PPX_STUCTURED_BUFFER_ALIGNMENT)) {
        PPX_ASSERT_MSG(false, "storage/structured buffer sizes must be at least PPX_STUCTURED_BUFFER_ALIGNMENT (" << PPX_STUCTURED_BUFFER_ALIGNMENT << ")");
        return ppx::ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
    }
#endif
    Result ppxres = grfx::DeviceObject<grfx::BufferCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Buffer::CopyFromSource(uint32_t dataSize, const void* pSrcData)
{
    if (dataSize > GetSize()) {
        return ppx::ERROR_LIMIT_EXCEEDED;
    }

    // Map
    void*  pBufferAddress = nullptr;
    Result ppxres         = MapMemory(0, &pBufferAddress);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Copy
    std::memcpy(pBufferAddress, pSrcData, dataSize);

    // Unmap
    UnmapMemory();

    return ppx::SUCCESS;
}

Result Buffer::CopyToDest(uint32_t dataSize, void* pDestData)
{
    if (dataSize > GetSize()) {
        return ppx::ERROR_LIMIT_EXCEEDED;
    }

    // Map
    void*  pBufferAddress = nullptr;
    Result ppxres         = MapMemory(0, &pBufferAddress);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Copy
    std::memcpy(pDestData, pBufferAddress, dataSize);

    // Unmap
    UnmapMemory();

    return ppx::SUCCESS;
}

} // namespace grfx
} // namespace ppx
