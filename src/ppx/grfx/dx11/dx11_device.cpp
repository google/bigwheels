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

#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_buffer.h"
#include "ppx/grfx/dx11/dx11_command.h"
#include "ppx/grfx/dx11/dx11_descriptor.h"
#include "ppx/grfx/dx11/dx11_gpu.h"
#include "ppx/grfx/dx11/dx11_image.h"
#include "ppx/grfx/dx11/dx11_instance.h"
#include "ppx/grfx/dx11/dx11_query.h"
#include "ppx/grfx/dx11/dx11_queue.h"
#include "ppx/grfx/dx11/dx11_pipeline.h"
#include "ppx/grfx/dx11/dx11_render_pass.h"
#include "ppx/grfx/dx11/dx11_shader.h"
#include "ppx/grfx/dx11/dx11_swapchain.h"
#include "ppx/grfx/dx11/dx11_sync.h"

namespace ppx {
namespace grfx {
namespace dx11 {

Result Device::CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo)
{
    // Graphics
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->graphicsQueueCount; ++queueIndex) {
        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_GRAPHICS;
        queueCreateInfo.pApiObject                      = nullptr;

        grfx::QueuePtr tmpQueue;
        Result         ppxres = CreateGraphicsQueue(&queueCreateInfo, &tmpQueue);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Compute
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->computeQueueCount; ++queueIndex) {
        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_COMPUTE;
        queueCreateInfo.pApiObject                      = nullptr;

        grfx::QueuePtr tmpQueue;
        Result         ppxres = CreateComputeQueue(&queueCreateInfo, &tmpQueue);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Transfer
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->transferQueueCount; ++queueIndex) {
        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_TRANSFER;
        queueCreateInfo.pApiObject                      = nullptr;

        grfx::QueuePtr tmpQueue;
        Result         ppxres = CreateTransferQueue(&queueCreateInfo, &tmpQueue);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

Result Device::CreateApiObjects(const grfx::DeviceCreateInfo* pCreateInfo)
{
    // Feature level
    D3D_FEATURE_LEVEL featureLevel = ToApi(pCreateInfo->pGpu)->GetFeatureLevel();

    // Cast to XIDXGIAdapter
    typename DXGIAdapterPtr::InterfaceType* pAdapter = ToApi(pCreateInfo->pGpu)->GetDxAdapter();

    // Flags
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (GetInstance()->IsDebugEnabled()) {
        flags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    //
    // When creating a device from an existing adapter (i.e. pAdapter is non-NULL), DriverType must be D3D_DRIVER_TYPE_UNKNOWN.
    //
    ComPtr<ID3D11Device>        device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    D3D_FEATURE_LEVEL           supportedFeatureLevel = InvalidValue<D3D_FEATURE_LEVEL>();
    //
    HRESULT hr = D3D11CreateDevice(
        pAdapter,                // IDXGIAdapter*            pAdapter
        D3D_DRIVER_TYPE_UNKNOWN, // D3D_DRIVER_TYPE          DriverType
        nullptr,                 // HMODULE                  Software
        flags,                   // UINT                     Flags
        &featureLevel,           // const D3D_FEATURE_LEVEL* pFeatureLevels
        1,                       // UINT                     FeatureLevels
        D3D11_SDK_VERSION,       // UINT                     SDKVersion
        &device,                 // ID3D11Device**           ppDevice
        &supportedFeatureLevel,  // D3D_FEATURE_LEVEL*       pFeatureLevel
        &deviceContext);         // ID3D11DeviceContext**    ppImmediateContex
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    hr = device->QueryInterface(IID_PPV_ARGS(&mDevice));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    hr = deviceContext->QueryInterface(IID_PPV_ARGS(&mDeviceContext));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Device, mDevice.Get());

    // Create queues
    Result ppxres = CreateQueues(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Device::DestroyApiObjects()
{
    if (mDeviceContext) {
        mDeviceContext->ClearState();
        mDeviceContext->Flush();
        mDeviceContext.Reset();
    }

    if (mDevice) {
        mDevice.Reset();
    }
}

Result Device::AllocateObject(grfx::Buffer** ppObject)
{
    dx11::Buffer* pObject = new dx11::Buffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandBuffer** ppObject)
{
    dx11::CommandBuffer* pObject = new dx11::CommandBuffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandPool** ppObject)
{
    dx11::CommandPool* pObject = new dx11::CommandPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ComputePipeline** ppObject)
{
    dx11::ComputePipeline* pObject = new dx11::ComputePipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DepthStencilView** ppObject)
{
    dx11::DepthStencilView* pObject = new dx11::DepthStencilView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorPool** ppObject)
{
    dx11::DescriptorPool* pObject = new dx11::DescriptorPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSet** ppObject)
{
    dx11::DescriptorSet* pObject = new dx11::DescriptorSet();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSetLayout** ppObject)
{
    dx11::DescriptorSetLayout* pObject = new dx11::DescriptorSetLayout();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Fence** ppObject)
{
    dx11::Fence* pObject = new dx11::Fence();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::GraphicsPipeline** ppObject)
{
    dx11::GraphicsPipeline* pObject = new dx11::GraphicsPipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Image** ppObject)
{
    dx11::Image* pObject = new dx11::Image();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::PipelineInterface** ppObject)
{
    dx11::PipelineInterface* pObject = new dx11::PipelineInterface();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Queue** ppObject)
{
    dx11::Queue* pObject = new dx11::Queue();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Query** ppObject)
{
    dx11::Query* pObject = new dx11::Query();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderPass** ppObject)
{
    dx11::RenderPass* pObject = new dx11::RenderPass();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderTargetView** ppObject)
{
    dx11::RenderTargetView* pObject = new dx11::RenderTargetView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::SampledImageView** ppObject)
{
    dx11::SampledImageView* pObject = new dx11::SampledImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Sampler** ppObject)
{
    dx11::Sampler* pObject = new dx11::Sampler();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Semaphore** ppObject)
{
    dx11::Semaphore* pObject = new dx11::Semaphore();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ShaderModule** ppObject)
{
    dx11::ShaderModule* pObject = new dx11::ShaderModule();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ShaderProgram** ppObject)
{
    return ppx::ERROR_FAILED;
}

Result Device::AllocateObject(grfx::StorageImageView** ppObject)
{
    dx11::StorageImageView* pObject = new dx11::StorageImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Swapchain** ppObject)
{
    dx11::Swapchain* pObject = new dx11::Swapchain();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::WaitIdle()
{
    return ppx::ERROR_FAILED;
}

bool Device::PipelineStatsAvailable() const
{
    return true;
}

bool Device::DynamicRenderingSupported() const
{
    return false;
}

Result Device::GetStructuredBufferSRV(
    const grfx::Buffer*                                  pBuffer,
    UINT                                                 numElements,
    typename D3D11ShaderResourceViewPtr::InterfaceType** ppSRV)
{
    typename D3D11ShaderResourceViewPtr::InterfaceType* pSRV = nullptr;
    auto                                                it   = FindIf(
        mStructuredBufferSRVs,
        [pBuffer, numElements](const StructuredBufferSRV& elem) -> bool {
            bool isSameBuffer = (elem.pBuffer == pBuffer);
            bool isSameNumElements = (elem.numElements == numElements);
            bool isSame = isSameBuffer && isSameNumElements;
            return isSame; });
    if (it != std::end(mStructuredBufferSRVs)) {
        pSRV = it->SRV.Get();
    }

    if (IsNull(pSRV)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC1 desc = {};
        desc.Format                           = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension                    = D3D11_SRV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement              = 0;
        desc.Buffer.NumElements               = numElements;

        D3D11ShaderResourceViewPtr SRV;
        HRESULT                    hr = mDevice->CreateShaderResourceView1(ToApi(pBuffer)->GetDxBuffer(), &desc, &SRV);
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }

        pSRV = SRV.Get();

        mStructuredBufferSRVs.emplace_back(StructuredBufferSRV{pBuffer, numElements, SRV});
    }

    *ppSRV = pSRV;

    return ppx::SUCCESS;
}

Result Device::GetBufferUAV(const grfx::Buffer* pBuffer, UINT firstElementInDwords, UINT numElementsInDwords, typename D3D11UnorderedAccessViewPtr::InterfaceType** ppUAV)
{
    typename D3D11UnorderedAccessViewPtr::InterfaceType* pUAV = nullptr;
    auto                                                 it   = FindIf(
        mBufferUAVs,
        [pBuffer, firstElementInDwords, numElementsInDwords](const BufferUAV& elem) -> bool {
            bool isSameBuffer = (elem.pBuffer == pBuffer);
            bool isSameOffset = (elem.firstElement == firstElementInDwords);
            bool isSameSize   = (elem.numElements == numElementsInDwords);
            bool isSame       = isSameBuffer && isSameOffset && isSameSize;
            return isSame; });
    if (it != std::end(mBufferUAVs)) {
        pUAV = it->UAV.Get();
    }

    if (IsNull(pUAV)) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc = {};
        desc.Format                            = DXGI_FORMAT_R32_TYPELESS;
        desc.ViewDimension                     = D3D11_UAV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement               = firstElementInDwords;
        desc.Buffer.NumElements                = numElementsInDwords;
        desc.Buffer.Flags                      = D3D11_BUFFER_UAV_FLAG_RAW;

        D3D11UnorderedAccessViewPtr UAV;
        HRESULT                     hr = mDevice->CreateUnorderedAccessView1(ToApi(pBuffer)->GetDxBuffer(), &desc, &UAV);
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }

        pUAV = UAV.Get();

        mBufferUAVs.emplace_back(BufferUAV{pBuffer, firstElementInDwords, numElementsInDwords, UAV});
    }

    *ppUAV = pUAV;

    return ppx::SUCCESS;
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
