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

#include "ppx/grfx/grfx_instance.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_gpu.h"
#if defined(PPX_D3D11)
#include "ppx/grfx/dx11/dx11_instance.h"
#endif // defined(PPX_D3D12)
#if defined(PPX_D3D12)
#include "ppx/grfx/dx12/dx12_instance.h"
#endif // defined(PPX_D3D12)
#if defined(PPX_VULKAN)
#include "ppx/grfx/vk/vk_instance.h"
#endif // defined(PPX_VULKAN)

namespace ppx {
namespace grfx {

static grfx::InstancePtr sInstance;

Result Instance::Create(const grfx::InstanceCreateInfo* pCreateInfo)
{
    mCreateInfo = *pCreateInfo;

    Result ppxres = CreateApiObjects(&mCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Create devices using 'good' defaults
    if (mCreateInfo.createDevices) {
        uint32_t count = GetGpuCount();
        for (uint32_t i = 0; i < count; ++i) {
            grfx::GpuPtr gpu;
            ppxres = GetGpu(i, &gpu);
            if (Failed(ppxres)) {
                DestroyApiObjects();
                return ppxres;
            }

            grfx::DeviceCreateInfo deviceCreateInfo = {};
            deviceCreateInfo.pGpu                   = gpu;
            deviceCreateInfo.graphicsQueueCount     = std::min<uint32_t>(1, gpu->GetGraphicsQueueCount());
            deviceCreateInfo.computeQueueCount      = std::min<uint32_t>(1, gpu->GetComputeQueueCount());
            deviceCreateInfo.transferQueueCount     = std::min<uint32_t>(1, gpu->GetTransferQueueCount());
            PPX_ASSERT_MSG((deviceCreateInfo.graphicsQueueCount > 0), "Must have at least one graphics queue");

            grfx::DevicePtr device;
            ppxres = CreateDevice(&deviceCreateInfo, &device);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "Failed creating device: " << gpu->GetDeviceName());
                DestroyApiObjects();
                return ppxres;
            }
        }
    }

    return ppx::SUCCESS;
}

void Instance::Destroy()
{
    DestroyAllObjects(mDevices);
    DestroyAllObjects(mGpus);
    DestroyApiObjects();

    PPX_LOG_INFO("Destroyed instance");
}

template <
    typename ObjectT,
    typename CreateInfoT,
    typename ContainerT>
Result Instance::CreateObject(const CreateInfoT* pCreateInfo, ContainerT& container, ObjectT** ppObject)
{
    // Allocate object
    ObjectT* pObject = nullptr;
    Result   ppxres  = AllocateObject(&pObject);
    if (Failed(ppxres)) {
        return ppxres;
    }
    // Set parent
    pObject->SetParent(this);
    // Create internal objects
    ppxres = pObject->Create(pCreateInfo);
    if (Failed(ppxres)) {
        pObject->Destroy();
        delete pObject;
        pObject = nullptr;
        return ppxres;
    }
    // Store
    container.push_back(ObjPtr<ObjectT>(pObject));
    // Assign
    *ppObject = pObject;
    // Success
    return ppx::SUCCESS;
}

template <
    typename ObjectT,
    typename ContainerT>
void Instance::DestroyObject(ContainerT& container, const ObjectT* pObject)
{
    // Make sure object is in container
    auto it = std::find_if(
        std::begin(container),
        std::end(container),
        [pObject](const ObjPtr<ObjectT>& elem) -> bool {
            bool res = (elem == pObject);
            return res; });
    if (it == std::end(container)) {
        return;
    }
    // Copy pointer
    ObjPtr<ObjectT> object = *it;
    // Remove object pointer from container
    RemoveElement(object, container);
    // Destroy internal objects
    object->Destroy();
    // Delete allocation
    ObjectT* ptr = object.Get();
    delete ptr;
}

template <typename ObjectT>
void Instance::DestroyAllObjects(std::vector<ObjPtr<ObjectT>>& container)
{
    size_t n = container.size();
    for (size_t i = 0; i < n; ++i) {
        // Get object pointer
        ObjPtr<ObjectT> object = container[i];
        // Call destroy
        object->Destroy();
        // Delete allocation
        ObjectT* ptr = object.Get();
        delete ptr;
    }
    // Clear container
    container.clear();
}

Result Instance::GetGpu(uint32_t index, grfx::Gpu** ppGpu) const
{
    PPX_ASSERT_NULL_ARG(ppGpu);
    if (!IsIndexInRange(index, mGpus)) {
        PPX_ASSERT_MSG(false, "index out of range");
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppGpu = mGpus[index];
    return ppx::SUCCESS;
}

Result Instance::GetDevice(uint32_t index, grfx::Device** ppDevice) const
{
    PPX_ASSERT_NULL_ARG(ppDevice);
    if (!IsIndexInRange(index, mDevices)) {
        PPX_ASSERT_MSG(false, "index out of range");
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppDevice = mDevices[index];
    return ppx::SUCCESS;
}

Result Instance::CreateDevice(const grfx::DeviceCreateInfo* pCreateInfo, grfx::Device** ppDevice)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo);
    PPX_ASSERT_NULL_ARG(ppDevice);
    Result ppxres = CreateObject(pCreateInfo, mDevices, ppDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

void Instance::DestroyDevice(const grfx::Device* pDevice)
{
    PPX_ASSERT_NULL_ARG(pDevice);
    DestroyObject(mDevices, pDevice);
}

Result Instance::CreateSurface(const grfx::SurfaceCreateInfo* pCreateInfo, grfx::Surface** ppSurface)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo);
    PPX_ASSERT_NULL_ARG(ppSurface);
    Result ppxres = CreateObject(pCreateInfo, mSurfaces, ppSurface);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

void Instance::DestroySurface(const grfx::Surface* pSurface)
{
    PPX_ASSERT_NULL_ARG(pSurface);
    DestroyObject(mSurfaces, pSurface);
}

Result Instance::CreateGpu(const grfx::internal::GpuCreateInfo* pCreateInfo, grfx::Gpu** ppGpu)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo);
    PPX_ASSERT_NULL_ARG(ppGpu);
    Result ppxres = CreateObject(pCreateInfo, mGpus, ppGpu);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return ppx::SUCCESS;
}

void Instance::DestroyGpu(const grfx::Gpu* pGpu)
{
    PPX_ASSERT_NULL_ARG(pGpu);
    DestroyObject(mGpus, pGpu);
}

Result CreateInstance(const grfx::InstanceCreateInfo* pCreateInfo, grfx::Instance** ppInstance)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo);
    PPX_ASSERT_NULL_ARG(ppInstance);
    if (sInstance) {
        return ppx::ERROR_SINGLE_INIT_ONLY;
    }

    grfx::Instance* pObject = nullptr;
    switch (pCreateInfo->api) {
        default: {
            PPX_ASSERT_MSG(false, "Unsupported API");
            return ppx::ERROR_UNSUPPORTED_API;
        } break;

#if defined(PPX_D3D11)
        case grfx::API_DX_11_0:
        case grfx::API_DX_11_1: {
            pObject = new dx11::Instance();
            if (IsNull(pObject)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }
        } break;
#endif // defiend(PPX_D3D11)

#if defined(PPX_D3D12)
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1: {
            pObject = new dx12::Instance();
            if (IsNull(pObject)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }
        } break;
#endif // defiend(PPX_D3D12)

#if defined(PPX_VULKAN)
        case grfx::API_VK_1_1:
        case grfx::API_VK_1_2: {
            pObject = new vk::Instance();
            if (IsNull(pObject)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }
        } break;
#endif // defined(PPX_VULKAN)
    }

    Result ppxres = pObject->Create(pCreateInfo);
    if (Failed(ppxres)) {
        delete pObject;
        pObject = nullptr;
        return ppxres;
    }

    sInstance   = pObject;
    *ppInstance = pObject;

    return ppx::SUCCESS;
}

void DestroyInstance(const grfx::Instance* pInstance)
{
    PPX_ASSERT_NULL_ARG(pInstance);
    if (sInstance.Get() != pInstance) {
        PPX_ASSERT_MSG(sInstance.Get() != pInstance, "Invalid argument value for pInstance");
        return;
    }
    // Destroy internal objects
    sInstance->Destroy();
    // Delete allocation
    grfx::Instance* ptr = sInstance.Get();
    delete ptr;
    // Reset static var
    sInstance.Reset();
}

} // namespace grfx
} // namespace ppx
