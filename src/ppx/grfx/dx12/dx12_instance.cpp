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

#include "ppx/grfx/dx12/dx12_instance.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_gpu.h"
#include "ppx/grfx/dx12/dx12_swapchain.h"
#include "ppx/grfx/dx12/dx12_queue.h"

namespace ppx {
namespace grfx {
namespace dx12 {

Result Instance::EnumerateAndCreateGpus(D3D_FEATURE_LEVEL featureLevel)
{
    // Enumerate GPUs
    std::vector<CComPtr<IDXGIAdapter1>> adapters;
    for (UINT index = 0;; ++index) {
        CComPtr<IDXGIAdapter1> adapter;
        // We're done if anything other than S_OK is returned
        HRESULT hr = mFactory->EnumAdapters1(index, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        // Filter for only hardware adapters, unless
        // a software renderer is requested.
        DXGI_ADAPTER_DESC1 desc;
        hr                       = adapter->GetDesc1(&desc);
        bool is_software_adapter = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
        if (FAILED(hr) || (mCreateInfo.useSoftwareRenderer != is_software_adapter)) {
            continue;
        }

        // Store adapters that support the minimum feature level
        hr = D3D12CreateDevice(adapter.Get(), featureLevel, __uuidof(ID3D12Device), nullptr);

        if (SUCCEEDED(hr)) {
            adapters.push_back(adapter);
        }
    }

    // Bail if no GPUs are found
    if (adapters.empty()) {
        return ppx::ERROR_NO_GPUS_FOUND;
    }

    // Create GPUs
    for (size_t i = 0; i < adapters.size(); ++i) {
        DXGI_ADAPTER_DESC adapterDesc = {};
        HRESULT           hr          = adapters[i]->GetDesc(&adapterDesc);
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "Failed to get GPU description");
            return ppx::ERROR_API_FAILURE;
        }

        std::string name;
        // Name - hack convert name from wchar to char
        for (size_t i = 0;; ++i) {
            if (adapterDesc.Description[i] == 0) {
                break;
            }
            char c = static_cast<char>(adapterDesc.Description[i]);
            name.push_back(c);
        }

        grfx::internal::GpuCreateInfo gpuCreateInfo = {};
        gpuCreateInfo.featureLevel                  = static_cast<int32_t>(featureLevel);
        gpuCreateInfo.pApiObject                    = static_cast<void*>(adapters[i].Get());

        grfx::GpuPtr tmpGpu;
        Result       ppxres = CreateGpu(&gpuCreateInfo, &tmpGpu);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "Failed creating GPU object using " << name);
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

Result Instance::CreateApiObjects(const grfx::InstanceCreateInfo* pCreateInfo)
{
    D3D_FEATURE_LEVEL minFeatureLevelRequired = D3D_FEATURE_LEVEL_12_0;

#if defined(PPX_BUILD_XR)
    if (isXREnabled()) {
        PPX_ASSERT_MSG(pCreateInfo->pXrComponent != nullptr, "XrComponent should not be nullptr!");
        const XrComponent& xrComponent = *pCreateInfo->pXrComponent;

        XrGraphicsRequirementsD3D12KHR        graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
        PFN_xrGetD3D12GraphicsRequirementsKHR pfnGetD3D12GraphicsRequirementsKHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrGetD3D12GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D12GraphicsRequirementsKHR)));
        PPX_ASSERT_MSG(pfnGetD3D12GraphicsRequirementsKHR != nullptr, "Cannot get xrGetD3D12GraphicsRequirementsKHR function pointer!");
        CHECK_XR_CALL(pfnGetD3D12GraphicsRequirementsKHR(xrComponent.GetInstance(), xrComponent.GetSystemId(), &graphicsRequirements));
        minFeatureLevelRequired = graphicsRequirements.minFeatureLevel;
    }
#endif

    D3D_FEATURE_LEVEL featureLevel = ppx::InvalidValue<D3D_FEATURE_LEVEL>();
    switch (pCreateInfo->api) {
        default: break;
        case grfx::API_DX_12_0: featureLevel = D3D_FEATURE_LEVEL_12_0; break;
        case grfx::API_DX_12_1: featureLevel = D3D_FEATURE_LEVEL_12_1; break;
    }
    if (featureLevel == ppx::InvalidValue<D3D_FEATURE_LEVEL>()) {
        return ppx::ERROR_UNSUPPORTED_API;
    }
    if (minFeatureLevelRequired > featureLevel) {
        PPX_LOG_WARN("D3D feature level increased due to minimum requirements");
        featureLevel = minFeatureLevelRequired;
    }
    UINT dxgiFactoryFlags = 0;
    if (pCreateInfo->enableDebug) {
        // Get DXGI debug interface
        HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDXGIDebug));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "DXGIGetDebugInterface1(DXGIDebug) failed");
            return ppx::ERROR_API_FAILURE;
        }

        // Get DXGI info queue
        hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDXGIInfoQueue));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "DXGIGetDebugInterface1(DXGIInfoQueue) failed");
            return ppx::ERROR_API_FAILURE;
        }

        // Set breaks
        mDXGIInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        mDXGIInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

        // Get D3D12 debug interface
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&mD3D12Debug));
        if (FAILED(hr)) {
            return ppx::ERROR_API_FAILURE;
        }
        // Enable additional debug layers
        mD3D12Debug->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    CComPtr<IDXGIFactory4> dxgiFactory;
    // Create factory using IDXGIFactory4
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(DXGIFactory, dxgiFactory.Get());

    // Cast to our version of IDXGIFactory
    hr = dxgiFactory.QueryInterface(&mFactory);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    Result ppxres = EnumerateAndCreateGpus(featureLevel);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Instance::DestroyApiObjects()
{
    if (mCreateInfo.enableDebug) {
        mDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
    }

    if (mFactory) {
        mFactory.Reset();
    }

    if (mD3D12Debug) {
        mD3D12Debug.Reset();
    }

    if (mDXGIInfoQueue) {
        mDXGIInfoQueue.Reset();
    }

    if (mDXGIDebug) {
        mDXGIDebug.Reset();
    }
}

Result Instance::AllocateObject(grfx::Device** ppDevice)
{
    dx12::Device* pObject = new dx12::Device();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppDevice = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Gpu** ppGpu)
{
    dx12::Gpu* pObject = new dx12::Gpu();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppGpu = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Surface** ppSurface)
{
    dx12::Surface* pObject = new dx12::Surface();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppSurface = pObject;
    return ppx::SUCCESS;
}

#if defined(PPX_BUILD_XR)
const XrBaseInStructure* Instance::XrGetGraphicsBinding() const
{
    PPX_ASSERT_MSG(XrIsGraphicsBindingValid(), "Invalid Graphics Binding!");
    return reinterpret_cast<const XrBaseInStructure*>(&mXrGraphicsBinding);
}

bool Instance::XrIsGraphicsBindingValid() const
{
    return (mXrGraphicsBinding.device != nullptr) && (mXrGraphicsBinding.queue != nullptr);
}

void Instance::XrUpdateDeviceInGraphicsBinding()
{
    mXrGraphicsBinding.device = ToApi(mDevices[0])->GetDxDevice();
    mXrGraphicsBinding.queue  = ToApi(mDevices[0]->GetGraphicsQueue())->GetDxQueue();
}
#endif

} // namespace dx12
} // namespace grfx
} // namespace ppx
