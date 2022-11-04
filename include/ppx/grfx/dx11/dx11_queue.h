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

#ifndef ppx_grfx_dx11_queue_h
#define ppx_grfx_dx11_queue_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/dx11/dx11_command.h"
#include "ppx/grfx/grfx_queue.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Queue
    : public grfx::Queue
{
public:
    Queue() {}
    virtual ~Queue() {}

    typename D3D11DeviceContextPtr::InterfaceType* GetDxDeviceContext() const { return mDeviceContext.Get(); }

    virtual Result WaitIdle() override;

    virtual Result Submit(const grfx::SubmitInfo* pSubmitInfo) override;

    virtual Result GetTimestampFrequency(uint64_t* pFrequency) const override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::QueueCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    D3D11DeviceContextPtr mDeviceContext;
    Result                UpdateTimestampFrequency();

    static const int QUERY_FRAME_DELAY = 3, MAX_QUERIES_IN_FLIGHT = QUERY_FRAME_DELAY + 1;
    ID3D11Query*     mFrequencyQuery[MAX_QUERIES_IN_FLIGHT];
    uint64_t         mFrequency = 0;
    uint32_t         mReadFrequencyQuery, mWriteFrequencyQuery;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_queue_h
