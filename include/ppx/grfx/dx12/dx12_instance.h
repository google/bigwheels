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

#ifndef ppx_grfx_dx12_instance_h
#define ppx_grfx_dx12_instance_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_instance.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class Instance
    : public grfx::Instance
{
public:
    Instance() {}
    virtual ~Instance() {}

    typename DXGIDebugPtr::InterfaceType* GetDxgiDebug() const
    {
        return mDXGIDebug.Get();
    }

    typename DXGIFactoryPtr::InterfaceType* GetDxFactory() const
    {
        return mFactory.Get();
    }

protected:
    virtual Result AllocateObject(grfx::Device** ppDevice) override;
    virtual Result AllocateObject(grfx::Gpu** ppGpu) override;
    virtual Result AllocateObject(grfx::Surface** ppSurface) override;

protected:
    virtual Result CreateApiObjects(const grfx::InstanceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result EnumerateAndCreateGpus(D3D_FEATURE_LEVEL featureLevel);

private:
    DXGIDebugPtr     mDXGIDebug;
    DXGIInfoQueuePtr mDXGIInfoQueue;
    D3D12DebugPtr    mD3D12Debug;
    DXGIFactoryPtr   mFactory;
};

} // namespace dx12

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_instance_h
