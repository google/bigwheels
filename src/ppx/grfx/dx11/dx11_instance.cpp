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

#include "ppx/grfx/dx11/dx11_instance.h"
#include "ppx/grfx/dx11/dx11_device.h"
#include "ppx/grfx/dx11/dx11_gpu.h"
#include "ppx/grfx/dx11/dx11_swapchain.h"

namespace ppx {
namespace grfx {
namespace dx11 {

Result Instance::EnumerateAndCreateGpus(D3D_FEATURE_LEVEL featureLevel, bool enableDebug)
{
    // Enumerate GPUs
    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        // We're done if anything other than S_OK is returned
        HRESULT hr = mFactory->EnumAdapters1(index, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        // Filter for only hardware adapters, unless
        // a software renderer is requested.
        DXGI_ADAPTER_DESC1 desc;
        hr = adapter->GetDesc1(&desc);
        bool is_software_adapter = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
        if (FAILED(hr) || (mCreateInfo.useSoftwareRenderer != is_software_adapter)) {
            continue;
        }
        // Store adapters that support the minimum feature level
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        if (enableDebug) {
            flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        //
        // When creating a device from an existing adapter (i.e. pAdapter is non-NULL), DriverType must be D3D_DRIVER_TYPE_UNKNOWN.
        //
        hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, &featureLevel, 1, D3D11_SDK_VERSION, nullptr, nullptr, nullptr);
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
    D3D_FEATURE_LEVEL minFeatureLevelRequired = D3D_FEATURE_LEVEL_11_0;

#if defined(PPX_BUILD_XR)
    if (isXREnabled()) {
        PPX_ASSERT_MSG(pCreateInfo->pXrComponent != nullptr, "XrComponent should not be nullptr!");
        const XrComponent& xrComponent = *pCreateInfo->pXrComponent;

        XrGraphicsRequirementsD3D11KHR        graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR)));
        PPX_ASSERT_MSG(pfnGetD3D11GraphicsRequirementsKHR != nullptr, "Cannot get xrGetD3D11GraphicsRequirementsKHR function pointer!");
        CHECK_XR_CALL(pfnGetD3D11GraphicsRequirementsKHR(xrComponent.GetInstance(), xrComponent.GetSystemId(), &graphicsRequirements));
        minFeatureLevelRequired = graphicsRequirements.minFeatureLevel;
    }
#endif

    D3D_FEATURE_LEVEL featureLevel = ppx::InvalidValue<D3D_FEATURE_LEVEL>();
    switch (pCreateInfo->api) {
        default: break;
        case grfx::API_DX_11_0: featureLevel = D3D_FEATURE_LEVEL_11_0; break;
        case grfx::API_DX_11_1: featureLevel = D3D_FEATURE_LEVEL_11_1; break;
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
    }

    ComPtr<IDXGIFactory4> dxgiFactory;
    // Create factory using IDXGIFactory4
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }
    PPX_LOG_OBJECT_CREATION(DXGIFactory, dxgiFactory.Get());

    // Cast to our version of IDXGIFactory
    hr = dxgiFactory.As(&mFactory);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    Result ppxres = EnumerateAndCreateGpus(featureLevel, pCreateInfo->enableDebug);
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

    if (mDXGIInfoQueue) {
        mDXGIInfoQueue.Reset();
    }

    if (mDXGIDebug) {
        mDXGIDebug.Reset();
    }
}

Result Instance::AllocateObject(grfx::Device** ppDevice)
{
    dx11::Device* pObject = new dx11::Device();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppDevice = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Gpu** ppGpu)
{
    dx11::Gpu* pObject = new dx11::Gpu();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppGpu = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Surface** ppSurface)
{
    dx11::Surface* pObject = new dx11::Surface();
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
    return mXrGraphicsBinding.device != nullptr;
}

void Instance::XrUpdateDeviceInGraphicsBinding()
{
    mXrGraphicsBinding.device = ToApi(mDevices[0])->GetDxDevice();
}
#endif

} // namespace dx11
} // namespace grfx
} // namespace ppx
