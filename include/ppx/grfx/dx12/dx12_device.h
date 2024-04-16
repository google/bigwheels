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

#ifndef ppx_grfx_dx12_device_h
#define ppx_grfx_dx12_device_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/dx12/dx12_descriptor_helper.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class Device
    : public grfx::Device
{
public:
    Device() {}
    virtual ~Device() {}

    typename D3D12DevicePtr::InterfaceType* GetDxDevice() const { return mDevice.Get(); }
    D3D12MA::Allocator*                     GetAllocator() const { return mAllocator; }

    UINT GetHandleIncrementSizeCBVSRVUAV() const { return mHandleIncrementSizeCBVSRVUAV; }
    UINT GetHandleIncrementSizeSampler() const { return mHandleIncrementSizeSampler; }

    Result AllocateRTVHandle(dx12::DescriptorHandle* pHandle);
    void   FreeRTVHandle(const dx12::DescriptorHandle* pHandle);

    Result AllocateDSVHandle(dx12::DescriptorHandle* pHandle);
    void   FreeDSVHandle(const dx12::DescriptorHandle* pHandle);

    HRESULT CreateRootSignatureDeserializer(
        LPCVOID    pSrcData,
        SIZE_T     SrcDataSizeInBytes,
        const IID& pRootSignatureDeserializerInterface,
        void**     ppRootSignatureDeserializer);

    HRESULT SerializeVersionedRootSignature(
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
        ID3DBlob**                                 ppBlob,
        ID3DBlob**                                 ppErrorBlob);

    HRESULT CreateVersionedRootSignatureDeserializer(
        LPCVOID    pSrcData,
        SIZE_T     SrcDataSizeInBytes,
        const IID& pRootSignatureDeserializerInterface,
        void**     ppRootSignatureDeserializer);

    virtual Result WaitIdle() override;

    virtual bool PipelineStatsAvailable() const override;
    virtual bool DynamicRenderingSupported() const override;
    virtual bool IndependentBlendingSupported() const override;
    virtual bool FragmentStoresAndAtomicsSupported() const override;
    virtual bool PartialDescriptorBindingsSupported() const override;

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
    virtual Result AllocateObject(grfx::SamplerYcbcrConversion** ppObject) override;
    virtual Result AllocateObject(grfx::Semaphore** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderModule** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderProgram** ppObject) override;
    virtual Result AllocateObject(grfx::ShadingRatePattern** ppObject) override;
    virtual Result AllocateObject(grfx::StorageImageView** ppObject) override;
    virtual Result AllocateObject(grfx::Swapchain** ppObject) override;

protected:
    virtual Result CreateApiObjects(const grfx::DeviceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    void   LoadRootSignatureFunctions();
    Result CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo);

private:
    D3D12DevicePtr             mDevice;
    ObjPtr<D3D12MA::Allocator> mAllocator;

    UINT                          mHandleIncrementSizeCBVSRVUAV = 0;
    UINT                          mHandleIncrementSizeSampler   = 0;
    dx12::DescriptorHandleManager mRTVHandleManager;
    dx12::DescriptorHandleManager mDSVHandleManager;

    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           mFnD3D12CreateRootSignatureDeserializer          = nullptr;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           mFnD3D12SerializeVersionedRootSignature          = nullptr;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER mFnD3D12CreateVersionedRootSignatureDeserializer = nullptr;

    std::vector<grfx::BufferPtr> mQueryResolveBuffers;
    uint32_t                     mQueryResolveThreadCount = 0;
    std::mutex                   mQueryResolveMutex;

    D3D12_RENDER_PASS_TIER mRenderPassTier;
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_device_h
