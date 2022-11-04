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

#ifndef ppx_grfx_gpu_h
#define ppx_grfx_gpu_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

namespace internal {

struct GpuCreateInfo
{
    int32_t featureLevel = ppx::InvalidValue<int32_t>(); // D3D12
    void*   pApiObject   = nullptr;
};

} // namespace internal

class Gpu
    : public grfx::InstanceObject<grfx::internal::GpuCreateInfo>
{
public:
    Gpu() {}
    virtual ~Gpu() {}

    const char*    GetDeviceName() const { return mDeviceName.c_str(); }
    grfx::VendorId GetDeviceVendorId() const { return mDeviceVendorId; }

    virtual uint32_t GetGraphicsQueueCount() const = 0;
    virtual uint32_t GetComputeQueueCount() const  = 0;
    virtual uint32_t GetTransferQueueCount() const = 0;

protected:
    std::string    mDeviceName;
    grfx::VendorId mDeviceVendorId = grfx::VENDOR_ID_UNKNOWN;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_gpu_h
