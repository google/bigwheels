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

#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_buffer.h"
#include "ppx/grfx/dx12/dx12_command.h"
#include "ppx/grfx/dx12/dx12_descriptor.h"
#include "ppx/grfx/dx12/dx12_gpu.h"
#include "ppx/grfx/dx12/dx12_image.h"
#include "ppx/grfx/dx12/dx12_instance.h"
#include "ppx/grfx/dx12/dx12_pipeline.h"
#include "ppx/grfx/dx12/dx12_queue.h"
#include "ppx/grfx/dx12/dx12_query.h"
#include "ppx/grfx/dx12/dx12_render_pass.h"
#include "ppx/grfx/dx12/dx12_shader.h"
#include "ppx/grfx/dx12/dx12_swapchain.h"
#include "ppx/grfx/dx12/dx12_sync.h"

#include "ppx/grfx/grfx_scope.h"

namespace ppx {
namespace grfx {
namespace dx12 {

void Device::LoadRootSignatureFunctions()
{
    HMODULE module = ::GetModuleHandle(TEXT("d3d12.dll"));

    // Load root signature version 1.1 functions
    {
        mFnD3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(
            module,
            "D3D12CreateRootSignatureDeserializer");

        mFnD3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(
            module,
            "D3D12SerializeVersionedRootSignature");

        mFnD3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(
            module,
            "D3D12CreateVersionedRootSignatureDeserializer");
    }
}

Result Device::CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo)
{
    // Graphics
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->graphicsQueueCount; ++queueIndex) {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask                 = 0;

        D3D12CommandQueuePtr queue;
        HRESULT              hr = mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandQueue(graphics) failed");
            return ppx::ERROR_API_FAILURE;
        }

        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_GRAPHICS;
        queueCreateInfo.pApiObject                      = queue.Get();

        grfx::QueuePtr tmpQueue;
        Result         ppxres = CreateGraphicsQueue(&queueCreateInfo, &tmpQueue);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Compute
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->computeQueueCount; ++queueIndex) {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type                     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask                 = 0;

        D3D12CommandQueuePtr queue;
        HRESULT              hr = mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandQueue(compute) failed");
            return ppx::ERROR_API_FAILURE;
        }

        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_COMPUTE;
        queueCreateInfo.pApiObject                      = queue.Get();

        grfx::QueuePtr tmpQueue;
        Result         ppxres = CreateComputeQueue(&queueCreateInfo, &tmpQueue);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Transfer
    for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->transferQueueCount; ++queueIndex) {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type                     = D3D12_COMMAND_LIST_TYPE_COPY;
        desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask                 = 0;

        D3D12CommandQueuePtr queue;
        HRESULT              hr = mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateCommandQueue(copy/trasfer) failed");
            return ppx::ERROR_API_FAILURE;
        }

        grfx::internal::QueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.commandType                     = grfx::COMMAND_TYPE_TRANSFER;
        queueCreateInfo.pApiObject                      = queue.Get();

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

    //
    // 20220423 (hai): DXIL is now in widespread use and not considered an
    //                 experimental feature anymore - we don't need this block.
    //                 Keeping it the block here but commented out now in case
    //                 someone is still working on an older version of Windows.
    //
    //                 If you're running Windows apps on Wine:
    //                   - VKD3D-Proton currently does not implement
    //                     D3D12EnableExperimentalFeatures so this
    //                     current function will return an API failure.
    //
    /*
    //
    // Enable SM 6.0 for DXIL support
    //
    // NOTE: This requires Windows 10 to be in Developer Mode.
    //
    if (pCreateInfo->enableDXIL) {
        // Create a temp device so we can query it for SM 6.0 support
        D3D12DevicePtr device;
        {
            HRESULT hr = D3D12CreateDevice(pAdapter, featureLevel, IID_PPV_ARGS(&device));
            if (FAILED(hr)) {
                PPX_ASSERT_MSG(false, "D3D12CreateDevice failed: couldn't create temp device for SM 6.0 support query");
                return ppx::ERROR_API_FAILURE;
            }
        }

        D3D12_FEATURE_DATA_SHADER_MODEL featureData = {D3D_SHADER_MODEL_6_0};

        // Check for SM 6.0
        HRESULT hr = device->CheckFeatureSupport(
            D3D12_FEATURE_SHADER_MODEL,
            &featureData,
            sizeof(featureData));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CheckFeatureSupport failed");
            return ppx::ERROR_API_FAILURE;
        }

        // Bail if SM 6.0 isn't supported
        if (featureData.HighestShaderModel != D3D_SHADER_MODEL_6_0) {
            PPX_ASSERT_MSG(false, "D3D12 SM 6.0 not supported");
            return ppx::ERROR_REQUIRED_FEATURE_UNAVAILABLE;
        }

        // Destroy temp device;
        device.Reset();

        // clang-format off
        UUID experimentalFeatures[] = {
            D3D12ExperimentalShaderModels
        };
        // clang-format on

        hr = D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "D3D12EnableExperimentalFeatures failed: couldn't turn on SM 6.0 support - is the system in Developer Mode?");
            return ppx::ERROR_API_FAILURE;
        }

        PPX_LOG_INFO("D3D12 SM 6.0+ support enabled (DXIL)")
    }
*/

    // Create real D3D12 device
    HRESULT hr = D3D12CreateDevice(pAdapter, featureLevel, IID_PPV_ARGS(&mDevice));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(D3D12Device, mDevice.Get());

    // Handle increment sizes
    mHandleIncrementSizeCBVSRVUAV = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mHandleIncrementSizeSampler   = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    // Check for Root signature version 1.1
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {D3D_ROOT_SIGNATURE_VERSION_1_1};

        hr = mDevice->CheckFeatureSupport(
            D3D12_FEATURE_ROOT_SIGNATURE,
            &featureData,
            sizeof(featureData));

        if (FAILED(hr) || (featureData.HighestVersion < D3D_ROOT_SIGNATURE_VERSION_1_1)) {
            return ppx::ERROR_REQUIRED_FEATURE_UNAVAILABLE;
        }
    }

    // Check for Dynamic Rendering capabilities: render passes in DX12 lingo
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport{};

        hr = mDevice->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &featureSupport,
            sizeof(featureSupport));

        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CheckFeatureSupport failed");
            return ppx::ERROR_API_FAILURE;
        }

        mRenderPassTier = featureSupport.RenderPassesTier;
    }
    // Create D3D12MA allocator
    {
        D3D12MA::ALLOCATOR_FLAGS flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        if (GetInstance()->ForceDxDiscreteAllocations()) {
            flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
        }

        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.Flags                   = flags;
        allocatorDesc.pDevice                 = mDevice.Get();
        allocatorDesc.pAdapter                = pAdapter;

        HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &mAllocator);
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Descriptor handle managers
    {
        // RTV
        Result ppxres = mRTVHandleManager.Create(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // DSV
        ppxres = mDSVHandleManager.Create(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Load root signature functions
    LoadRootSignatureFunctions();

    // Create queues
    Result ppxres = CreateQueues(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Disable common noisy warnings from the debug layer.
    CComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)))) {
        std::vector<D3D12_MESSAGE_ID> ignoredDebugMessages = {
            // Causes a lot of noise with headless swapchain implementation,
            // whenever the application does not clear the swapchain's render
            // target to the default color value. This is an optimization
            // warning and not a correctness one anyway.
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_MESSAGE_SEVERITY ignoredSeverities[] = {D3D12_MESSAGE_SEVERITY_INFO};

        D3D12_INFO_QUEUE_FILTER debugFilter = {};
        debugFilter.DenyList.NumSeverities  = 1;
        debugFilter.DenyList.pSeverityList  = ignoredSeverities;
        debugFilter.DenyList.NumIDs         = static_cast<UINT>(ignoredDebugMessages.size());
        debugFilter.DenyList.pIDList        = ignoredDebugMessages.data();

        hr = pInfoQueue->PushStorageFilter(&debugFilter);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "PushStorageFilter failed");
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Success
    return ppx::SUCCESS;
}

void Device::DestroyApiObjects()
{
    mFnD3D12CreateRootSignatureDeserializer          = nullptr;
    mFnD3D12SerializeVersionedRootSignature          = nullptr;
    mFnD3D12CreateVersionedRootSignatureDeserializer = nullptr;

    mRTVHandleManager.Destroy();
    mDSVHandleManager.Destroy();

    if (mAllocator) {
        mAllocator->Release();
        mAllocator.Reset();
    }

    if (mDevice) {
        mDevice.Reset();
    }
}

Result Device::AllocateRTVHandle(dx12::DescriptorHandle* pHandle)
{
    Result ppxres = mRTVHandleManager.AllocateHandle(pHandle);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

void Device::FreeRTVHandle(const dx12::DescriptorHandle* pHandle)
{
    if (IsNull(pHandle)) {
        return;
    }
    mRTVHandleManager.FreeHandle(*pHandle);
}

Result Device::AllocateDSVHandle(dx12::DescriptorHandle* pHandle)
{
    Result ppxres = mDSVHandleManager.AllocateHandle(pHandle);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

void Device::FreeDSVHandle(const dx12::DescriptorHandle* pHandle)
{
    if (IsNull(pHandle)) {
        return;
    }
    mDSVHandleManager.FreeHandle(*pHandle);
}

HRESULT Device::CreateRootSignatureDeserializer(
    LPCVOID    pSrcData,
    SIZE_T     SrcDataSizeInBytes,
    const IID& pRootSignatureDeserializerInterface,
    void**     ppRootSignatureDeserializer)
{
    if (mFnD3D12CreateRootSignatureDeserializer == nullptr) {
        return E_NOTIMPL;
    }

    HRESULT hr = mFnD3D12CreateRootSignatureDeserializer(
        pSrcData,
        SrcDataSizeInBytes,
        pRootSignatureDeserializerInterface,
        ppRootSignatureDeserializer);
    return hr;
}

HRESULT Device::SerializeVersionedRootSignature(
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
    ID3DBlob**                                 ppBlob,
    ID3DBlob**                                 ppErrorBlob)
{
    if (mFnD3D12CreateRootSignatureDeserializer == nullptr) {
        return E_NOTIMPL;
    }

    HRESULT hr = mFnD3D12SerializeVersionedRootSignature(
        pRootSignature,
        ppBlob,
        ppErrorBlob);
    return hr;
}

HRESULT Device::CreateVersionedRootSignatureDeserializer(
    LPCVOID    pSrcData,
    SIZE_T     SrcDataSizeInBytes,
    const IID& pRootSignatureDeserializerInterface,
    void**     ppRootSignatureDeserializer)
{
    if (mFnD3D12CreateRootSignatureDeserializer == nullptr) {
        return E_NOTIMPL;
    }

    HRESULT hr = mFnD3D12CreateRootSignatureDeserializer(
        pSrcData,
        SrcDataSizeInBytes,
        pRootSignatureDeserializerInterface,
        ppRootSignatureDeserializer);
    return hr;
}

Result Device::AllocateObject(grfx::Buffer** ppObject)
{
    dx12::Buffer* pObject = new dx12::Buffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandBuffer** ppObject)
{
    dx12::CommandBuffer* pObject = new dx12::CommandBuffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandPool** ppObject)
{
    dx12::CommandPool* pObject = new dx12::CommandPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ComputePipeline** ppObject)
{
    dx12::ComputePipeline* pObject = new dx12::ComputePipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DepthStencilView** ppObject)
{
    dx12::DepthStencilView* pObject = new dx12::DepthStencilView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorPool** ppObject)
{
    dx12::DescriptorPool* pObject = new dx12::DescriptorPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSet** ppObject)
{
    dx12::DescriptorSet* pObject = new dx12::DescriptorSet();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSetLayout** ppObject)
{
    dx12::DescriptorSetLayout* pObject = new dx12::DescriptorSetLayout();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Fence** ppObject)
{
    dx12::Fence* pObject = new dx12::Fence();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::GraphicsPipeline** ppObject)
{
    dx12::GraphicsPipeline* pObject = new dx12::GraphicsPipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Image** ppObject)
{
    dx12::Image* pObject = new dx12::Image();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::PipelineInterface** ppObject)
{
    dx12::PipelineInterface* pObject = new dx12::PipelineInterface();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Queue** ppObject)
{
    dx12::Queue* pObject = new dx12::Queue();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Query** ppObject)
{
    dx12::Query* pObject = new dx12::Query();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderPass** ppObject)
{
    dx12::RenderPass* pObject = new dx12::RenderPass();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderTargetView** ppObject)
{
    dx12::RenderTargetView* pObject = new dx12::RenderTargetView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::SampledImageView** ppObject)
{
    dx12::SampledImageView* pObject = new dx12::SampledImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Sampler** ppObject)
{
    dx12::Sampler* pObject = new dx12::Sampler();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Semaphore** ppObject)
{
    dx12::Semaphore* pObject = new dx12::Semaphore();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ShaderModule** ppObject)
{
    dx12::ShaderModule* pObject = new dx12::ShaderModule();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ShaderProgram** ppObject)
{
    return ppx::ERROR_ALLOCATION_FAILED;
}

Result Device::AllocateObject(grfx::StorageImageView** ppObject)
{
    dx12::StorageImageView* pObject = new dx12::StorageImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Swapchain** ppObject)
{
    dx12::Swapchain* pObject = new dx12::Swapchain();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::WaitIdle()
{
    for (auto& queue : mGraphicsQueues) {
        Result ppxres = ToApi(queue)->WaitIdle();
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    for (auto& queue : mComputeQueues) {
        Result ppxres = ToApi(queue)->WaitIdle();
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    for (auto& queue : mTransferQueues) {
        Result ppxres = ToApi(queue)->WaitIdle();
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

bool Device::PipelineStatsAvailable() const
{
    return true;
}

bool Device::DynamicRenderingSupported() const
{
    return mRenderPassTier > D3D12_RENDER_PASS_TIER_0;
}

bool Device::IndependentBlendingSupported() const
{
    return true;
}

bool Device::FragmentStoresAndAtomicsSupported() const
{
    return true;
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
