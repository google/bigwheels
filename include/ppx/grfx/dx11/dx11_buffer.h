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

#ifndef ppx_grfx_dx11_buffer_h
#define ppx_grfx_dx11_buffer_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_buffer.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Buffer
    : public grfx::Buffer
{
public:
    Buffer() {}
    virtual ~Buffer() {}

    typename D3D11BufferPtr::InterfaceType* GetDxBuffer() const { return mBuffer.Get(); }
    D3D11_MAP                               GetMapType() const;

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) override;
    virtual void   UnmapMemory() override;

protected:
    virtual Result CreateApiObjects(const grfx::BufferCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11_USAGE    mUsage          = InvalidValue<D3D11_USAGE>();
    UINT           mCpuAccessFlags = 0;
    D3D11BufferPtr mBuffer;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_buffer_h
