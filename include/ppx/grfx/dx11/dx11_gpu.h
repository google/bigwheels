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

#ifndef ppx_grfx_dx11_gpu_h
#define ppx_grfx_dx11_gpu_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_gpu.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Gpu
    : public grfx::Gpu
{
public:
    Gpu() {}
    virtual ~Gpu() {}

    typename DXGIAdapterPtr::InterfaceType* GetDxAdapter() const { return mGpu.Get(); }

    D3D_FEATURE_LEVEL GetFeatureLevel() const { return static_cast<D3D_FEATURE_LEVEL>(mCreateInfo.featureLevel); }

    virtual uint32_t GetGraphicsQueueCount() const override;
    virtual uint32_t GetComputeQueueCount() const override;
    virtual uint32_t GetTransferQueueCount() const override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::GpuCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    DXGIAdapterPtr mGpu;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_gpu_h
