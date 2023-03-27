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

#ifndef ppx_grfx_dx11_device_h
#define ppx_grfx_dx11_device_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class Device
    : public grfx::Device
{
public:
    Device() {}
    virtual ~Device() {}

    typename D3D11DevicePtr::InterfaceType*        GetDxDevice() const { return mDevice.Get(); }
    typename D3D11DeviceContextPtr::InterfaceType* GetDxDeviceContext() const { return mDeviceContext.Get(); }

    virtual Result WaitIdle() override;

    virtual bool PipelineStatsAvailable() const override;
    virtual bool DynamicRenderingSupported() const override;
    virtual bool IndependentBlendingSupported() const override;

    Result GetStructuredBufferSRV(
        const grfx::Buffer*                                  pBuffer,
        UINT                                                 numElements,
        typename D3D11ShaderResourceViewPtr::InterfaceType** ppSRV);

    Result GetBufferUAV(
        const grfx::Buffer*                                   pBuffer,
        UINT                                                  firstElementInDwords,
        UINT                                                  numElementsInDwords,
        typename D3D11UnorderedAccessViewPtr::InterfaceType** ppUAV);

protected:
    virtual Result AllocateObject(grfx::Buffer** ppObject) override;
    virtual Result AllocateObject(grfx::CommandBuffer** ppObject) override;
    virtual Result AllocateObject(grfx::CommandPool** ppObject) override;
    virtual Result AllocateObject(grfx::ComputePipeline** ppObject) override;
    virtual Result AllocateObject(grfx::DepthStencilView** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorPool** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorSet** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorSetLayout** ppObject) override;
    virtual Result AllocateObject(grfx::Fence** ppObject) override;
    virtual Result AllocateObject(grfx::GraphicsPipeline** ppObject) override;
    virtual Result AllocateObject(grfx::Image** ppObject) override;
    virtual Result AllocateObject(grfx::PipelineInterface** ppObject) override;
    virtual Result AllocateObject(grfx::Queue** ppObject) override;
    virtual Result AllocateObject(grfx::Query** ppObject) override;
    virtual Result AllocateObject(grfx::RenderPass** ppObject) override;
    virtual Result AllocateObject(grfx::RenderTargetView** ppObject) override;
    virtual Result AllocateObject(grfx::SampledImageView** ppObject) override;
    virtual Result AllocateObject(grfx::Sampler** ppObject) override;
    virtual Result AllocateObject(grfx::Semaphore** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderModule** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderProgram** ppObject) override;
    virtual Result AllocateObject(grfx::StorageImageView** ppObject) override;
    virtual Result AllocateObject(grfx::Swapchain** ppObject) override;

protected:
    virtual Result CreateApiObjects(const grfx::DeviceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo);

private:
    D3D11DevicePtr        mDevice;
    D3D11DeviceContextPtr mDeviceContext;

    struct StructuredBufferSRV
    {
        const grfx::Buffer*        pBuffer     = nullptr;
        UINT                       numElements = 0;
        D3D11ShaderResourceViewPtr SRV;
    };

    struct BufferUAV
    {
        const grfx::Buffer*         pBuffer      = nullptr;
        UINT                        firstElement = 0;
        UINT                        numElements  = 0;
        D3D11UnorderedAccessViewPtr UAV;
    };

    std::vector<StructuredBufferSRV> mStructuredBufferSRVs;
    std::vector<BufferUAV>           mBufferUAVs;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_device_h
