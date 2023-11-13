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

#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_buffer.h"
#include "ppx/grfx/vk/vk_command.h"
#include "ppx/grfx/vk/vk_descriptor.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_image.h"
#include "ppx/grfx/vk/vk_instance.h"
#include "ppx/grfx/vk/vk_pipeline.h"
#include "ppx/grfx/vk/vk_queue.h"
#include "ppx/grfx/vk/vk_query.h"
#include "ppx/grfx/vk/vk_render_pass.h"
#include "ppx/grfx/vk/vk_shader.h"
#include "ppx/grfx/vk/vk_swapchain.h"
#include "ppx/grfx/vk/vk_sync.h"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#include "vk_mem_alloc.h"
#include <unordered_set>

namespace ppx {
namespace grfx {
namespace vk {

PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR = nullptr;

Result Device::ConfigureQueueInfo(const grfx::DeviceCreateInfo* pCreateInfo, std::vector<float>& queuePriorities, std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos)
{
    VkPhysicalDevicePtr gpu = ToApi(pCreateInfo->pGpu)->GetVkGpu();

    // Queue priorities
    {
        uint32_t maxQueueCount = std::max(pCreateInfo->pGpu->GetGraphicsQueueCount(), pCreateInfo->pGpu->GetComputeQueueCount());
        maxQueueCount          = std::max(maxQueueCount, pCreateInfo->pGpu->GetTransferQueueCount());

        for (uint32_t i = 0; i < maxQueueCount; ++i) {
            queuePriorities.push_back(1.0f);
        }
    }

    // Queue families
    {
        mGraphicsQueueFamilyIndex = ToApi(pCreateInfo->pGpu)->GetGraphicsQueueFamilyIndex();
        mComputeQueueFamilyIndex  = ToApi(pCreateInfo->pGpu)->GetComputeQueueFamilyIndex();
        mTransferQueueFamilyIndex = ToApi(pCreateInfo->pGpu)->GetTransferQueueFamilyIndex();
    }

    // Queues
    {
        std::unordered_set<uint32_t> createdQueues;
        // Graphics
        if (mGraphicsQueueFamilyIndex != PPX_VALUE_IGNORED) {
            VkDeviceQueueCreateInfo vkci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            vkci.queueFamilyIndex        = mGraphicsQueueFamilyIndex;
            vkci.queueCount              = pCreateInfo->pGpu->GetGraphicsQueueCount();
            vkci.pQueuePriorities        = DataPtr(queuePriorities);
            queueCreateInfos.push_back(vkci);
            createdQueues.insert(mGraphicsQueueFamilyIndex);
        }
        // Compute
        if (mComputeQueueFamilyIndex != PPX_VALUE_IGNORED && createdQueues.find(mComputeQueueFamilyIndex) == createdQueues.end()) {
            VkDeviceQueueCreateInfo vkci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            vkci.queueFamilyIndex        = mComputeQueueFamilyIndex;
            vkci.queueCount              = pCreateInfo->pGpu->GetComputeQueueCount();
            vkci.pQueuePriorities        = DataPtr(queuePriorities);
            queueCreateInfos.push_back(vkci);
            createdQueues.insert(mComputeQueueFamilyIndex);
        }
        else if (createdQueues.find(mComputeQueueFamilyIndex) != createdQueues.end()) {
            PPX_LOG_WARN("Graphics queue will be shared with compute queue.");
        }
        // Transfer
        if (mTransferQueueFamilyIndex != PPX_VALUE_IGNORED && createdQueues.find(mTransferQueueFamilyIndex) == createdQueues.end()) {
            VkDeviceQueueCreateInfo vkci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            vkci.queueFamilyIndex        = mTransferQueueFamilyIndex;
            vkci.queueCount              = pCreateInfo->pGpu->GetTransferQueueCount();
            vkci.pQueuePriorities        = DataPtr(queuePriorities);
            queueCreateInfos.push_back(vkci);
            createdQueues.insert(mTransferQueueFamilyIndex);
        }
        else if (createdQueues.find(mTransferQueueFamilyIndex) != createdQueues.end()) {
            PPX_LOG_WARN("Transfer queue will be shared with graphics or compute queue.");
        }
    }

    return ppx::SUCCESS;
}

Result Device::ConfigureExtensions(const grfx::DeviceCreateInfo* pCreateInfo)
{
    VkPhysicalDevicePtr gpu = ToApi(pCreateInfo->pGpu)->GetVkGpu();

    // Enumerate extensions
    uint32_t count = 0;
    VkResult vkres = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateDeviceExtensionProperties(0) failed");
        return ppx::ERROR_API_FAILURE;
    }

    if (count > 0) {
        std::vector<VkExtensionProperties> properties(count);
        vkres = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, properties.data());
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateDeviceExtensionProperties(1) failed");
            return ppx::ERROR_API_FAILURE;
        }

        for (auto& elem : properties) {
            mFoundExtensions.push_back(elem.extensionName);
        }
        Unique(mFoundExtensions);
    }

    // Swapchains extension
    if (GetInstance()->IsSwapchainEnabled()) {
        mExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    mExtensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

    // Add Vulkan 1.1 extensions:
    //   - VK_EXT_descriptor_indexing (promoted to core in 1.2)
    //   - VK_KHR_timeline_semaphore (promoted to core in 1.2)
    //
    if (GetInstance()->GetApi() == grfx::API_VK_1_1) {
        // VK_EXT_host_query_reset
        mExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);

        // Descriptor indexing
        //
        // 2021/11/15 - Added conditional check for descriptor indexing to accomodate SwiftShader
        if (ElementExists(std::string(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME), mFoundExtensions)) {
            mExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }

        // Timeline semaphore - if present
        if (ElementExists(std::string(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME), mFoundExtensions)) {
            mExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        }
    }

#if defined(PPX_VK_EXTENDED_DYNAMIC_STATE)
    if (ElementExists(std::string(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    }
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

    // Depth clip
    if (ElementExists(std::string(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);
    }

    // Push descriptors
    if (ElementExists(std::string(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    }

    // Add additional extensions and uniquify
    AppendElements(pCreateInfo->vulkanExtensions, mExtensions);
    Unique(mExtensions);

    return ppx::SUCCESS;
}

Result Device::ConfigureFeatures(const grfx::DeviceCreateInfo* pCreateInfo, VkPhysicalDeviceFeatures& features)
{
    vk::Gpu* pGpu = ToApi(pCreateInfo->pGpu);

    VkPhysicalDeviceFeatures foundFeatures = {};
    vkGetPhysicalDeviceFeatures(pGpu->GetVkGpu(), &foundFeatures);

    // Default device features
    //
    // 2021/11/15 - Changed logic to use feature bit from GPU for geo and tess shaders to accomodate
    //              SwiftShader not having support for these shader types.
    //
    features                                      = {};
    features.fullDrawIndexUint32                  = VK_TRUE;
    features.imageCubeArray                       = VK_TRUE;
    features.independentBlend                     = foundFeatures.independentBlend;
    features.pipelineStatisticsQuery              = foundFeatures.pipelineStatisticsQuery;
    features.geometryShader                       = foundFeatures.geometryShader;
    features.tessellationShader                   = foundFeatures.tessellationShader;
    features.fragmentStoresAndAtomics             = foundFeatures.fragmentStoresAndAtomics;
    features.shaderStorageImageReadWithoutFormat  = foundFeatures.shaderStorageImageReadWithoutFormat;
    features.shaderStorageImageWriteWithoutFormat = foundFeatures.shaderStorageImageWriteWithoutFormat;
    features.shaderStorageImageMultisample        = foundFeatures.shaderStorageImageMultisample;
    features.samplerAnisotropy                    = foundFeatures.samplerAnisotropy;

    // Select between default or custom features.
    if (!IsNull(pCreateInfo->pVulkanDeviceFeatures)) {
        const VkPhysicalDeviceFeatures* pFeatures = static_cast<const VkPhysicalDeviceFeatures*>(pCreateInfo->pVulkanDeviceFeatures);
        features                                  = *pFeatures;
    }

    return ppx::SUCCESS;
}

Result Device::CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo)
{
    if (pCreateInfo->graphicsQueueCount > 0) {
        uint32_t queueFamilyIndex = ToApi(pCreateInfo->pGpu)->GetGraphicsQueueFamilyIndex();
        for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->graphicsQueueCount; ++queueIndex) {
            grfx::internal::QueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.queueFamilyIndex                = queueFamilyIndex;
            queueCreateInfo.queueIndex                      = queueIndex;

            grfx::QueuePtr tmpQueue;
            Result         ppxres = CreateGraphicsQueue(&queueCreateInfo, &tmpQueue);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }
    }

    if (pCreateInfo->computeQueueCount > 0) {
        uint32_t queueFamilyIndex = ToApi(pCreateInfo->pGpu)->GetComputeQueueFamilyIndex();
        for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->computeQueueCount; ++queueIndex) {
            grfx::internal::QueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.queueFamilyIndex                = queueFamilyIndex;
            queueCreateInfo.queueIndex                      = queueIndex;

            grfx::QueuePtr tmpQueue;
            Result         ppxres = CreateComputeQueue(&queueCreateInfo, &tmpQueue);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }
    }

    if (pCreateInfo->transferQueueCount > 0) {
        uint32_t queueFamilyIndex = ToApi(pCreateInfo->pGpu)->GetTransferQueueFamilyIndex();
        for (uint32_t queueIndex = 0; queueIndex < pCreateInfo->transferQueueCount; ++queueIndex) {
            grfx::internal::QueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.queueFamilyIndex                = queueFamilyIndex;
            queueCreateInfo.queueIndex                      = queueIndex;

            grfx::QueuePtr tmpQueue;
            Result         ppxres = CreateTransferQueue(&queueCreateInfo, &tmpQueue);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }
    }

    return ppx::SUCCESS;
}

Result Device::CreateApiObjects(const grfx::DeviceCreateInfo* pCreateInfo)
{
    std::vector<float>                   queuePriorities;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    Result ppxres = ConfigureQueueInfo(pCreateInfo, queuePriorities, queueCreateInfos);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = ConfigureExtensions(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = ConfigureFeatures(pCreateInfo, mDeviceFeatures);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // We can't include structs whose extesnions aren't enabled, so do the tracking.
    std::vector<VkBaseOutStructure*> extensionStructs;

    // VK_EXT_descriptor_indexing
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_2) || ElementExists(std::string(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME), mExtensions)) {
        //
        // 2023/10/01 - Just runtimeDescriptorArrays for now - need to survey what Android
        //              usage is like before enabling other freatures.
        //
        descriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing          = VK_FALSE;
        descriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing       = VK_FALSE;
        descriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing       = VK_FALSE;
        descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing         = VK_FALSE;
        descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing          = VK_FALSE;
        descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing         = VK_FALSE;
        descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing          = VK_FALSE;
        descriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing       = VK_FALSE;
        descriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing    = VK_FALSE;
        descriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing    = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind      = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind       = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind       = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind      = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending          = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound                    = VK_FALSE;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount           = VK_FALSE;
        descriptorIndexingFeatures.runtimeDescriptorArray                             = VK_TRUE;

        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&descriptorIndexingFeatures));
    }

    // VK_KHR_timeline_semaphore
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_2) || ElementExists(std::string(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME), mExtensions)) {
        timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&timelineSemaphoreFeatures));
    }

    // VK_EXT_host_query_reset
    VkPhysicalDeviceHostQueryResetFeatures queryResetFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_2) || ElementExists(std::string(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME), mExtensions)) {
        queryResetFeatures.hostQueryReset = VK_TRUE;

        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&queryResetFeatures));
    }

    // Chain pNexts
    for (size_t i = 1; i < extensionStructs.size(); ++i) {
        extensionStructs[i - 1]->pNext = extensionStructs[i];
    }

    // Get C strings
    std::vector<const char*> extensions = GetCStrings(mExtensions);

    VkDeviceCreateInfo vkci      = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    vkci.pNext                   = extensionStructs.empty() ? nullptr : extensionStructs[0];
    vkci.flags                   = 0;
    vkci.queueCreateInfoCount    = CountU32(queueCreateInfos);
    vkci.pQueueCreateInfos       = DataPtr(queueCreateInfos);
    vkci.enabledLayerCount       = 0;
    vkci.ppEnabledLayerNames     = nullptr;
    vkci.enabledExtensionCount   = CountU32(extensions);
    vkci.ppEnabledExtensionNames = DataPtr(extensions);
    vkci.pEnabledFeatures        = &mDeviceFeatures;

    // Log layers and extensions
    {
        PPX_LOG_INFO("Loading " << vkci.enabledExtensionCount << " Vulkan device extensions");
        for (uint32_t i = 0; i < vkci.enabledExtensionCount; ++i) {
            PPX_LOG_INFO("   " << i << " : " << vkci.ppEnabledExtensionNames[i]);
        }
    }

    VkResult vkres;
#if defined(PPX_BUILD_XR)
    if (pCreateInfo->pXrComponent != nullptr) {
#if !defined(PPX_ANDROID)
        // TODO: is this still needed for Air Link?
        // This fixes a validation error with Oculus Quest 2 Runtime
        mDeviceFeatures.samplerAnisotropy             = VK_TRUE;
        mDeviceFeatures.shaderStorageImageMultisample = VK_TRUE;
#endif // !defined(PPX_ANDROID)

        const XrComponent&          xrComponent = *pCreateInfo->pXrComponent;
        XrVulkanDeviceCreateInfoKHR deviceCreateInfo{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
        deviceCreateInfo.systemId                            = xrComponent.GetSystemId();
        deviceCreateInfo.pfnGetInstanceProcAddr              = &vkGetInstanceProcAddr;
        deviceCreateInfo.vulkanCreateInfo                    = &vkci;
        deviceCreateInfo.vulkanPhysicalDevice                = ToApi(GetGpu())->GetVkGpu();
        deviceCreateInfo.vulkanAllocator                     = nullptr;
        PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrCreateVulkanDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanDeviceKHR)));
        PPX_ASSERT_MSG(pfnCreateVulkanDeviceKHR != nullptr, "Cannot get xrCreateVulkanDeviceKHR function pointer!");
        CHECK_XR_CALL(pfnCreateVulkanDeviceKHR(xrComponent.GetInstance(), &deviceCreateInfo, &mDevice, &vkres));
    }
    else
#endif
    {
        vkres = vkCreateDevice(ToApi(pCreateInfo->pGpu)->GetVkGpu(), &vkci, nullptr, &mDevice);
    }

    if (vkres != VK_SUCCESS) {
        // clang-format off
        std::stringstream ss;
        ss << "vkCreateInstance failed: " << ToString(vkres);
        if (vkres == VK_ERROR_EXTENSION_NOT_PRESENT) {
            std::vector<std::string> missing = GetNotFound(mExtensions, mFoundExtensions);
            ss << PPX_LOG_ENDL;
            ss << "  " << "Extension(s) not found:" << PPX_LOG_ENDL;
            for (auto& elem : missing) {
                ss << "  " << "  " << elem << PPX_LOG_ENDL;
            }
        }
        // clang-format on

        PPX_ASSERT_MSG(false, ss.str());
        return ppx::ERROR_API_FAILURE;
    }

    //
    // Timeline semaphore and host query reset is in core start in Vulkan 1.2
    //
    // If this is a Vulkan 1.1 device:
    //   - Load vkResetQueryPoolEXT
    //   - Enable timeline semaphore if extension was loaded
    //
    if (GetInstance()->GetApi() == grfx::API_VK_1_1) {
        mFnResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)vkGetDeviceProcAddr(mDevice, "vkResetQueryPoolEXT");
        PPX_ASSERT_MSG(mFnResetQueryPoolEXT != nullptr, "failed to load vkResetQueryPoolEXT");

        mHasTimelineSemaphore = ElementExists(std::string(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME), mExtensions);
    }
    else {
        mHasTimelineSemaphore = true;
    }
    PPX_LOG_INFO("Vulkan timeline semaphore is present: " << mHasTimelineSemaphore);

#if defined(PPX_VK_EXTENDED_DYNAMIC_STATE)
    mExtendedDynamicStateAvailable = ElementExists(std::string(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), mFoundExtensions));
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

    // Depth clip enabled
    mHasUnrestrictedDepthRange = ElementExists(std::string(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME), mExtensions);

    // Get maxPushDescriptors property and load function
    if (ElementExists(std::string(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME), mExtensions)) {
        VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR};

        VkPhysicalDeviceProperties2 properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        properties.pNext                       = &pushDescriptorProperties;

        vkGetPhysicalDeviceProperties2(ToApi(pCreateInfo->pGpu)->GetVkGpu(), &properties);

        mMaxPushDescriptors = pushDescriptorProperties.maxPushDescriptors;
        PPX_LOG_INFO("Vulkan maxPushDescriptors: " << mMaxPushDescriptors);

        CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(mDevice, "vkCmdPushDescriptorSetKHR");
    }

    // VMA
    {
        VmaAllocatorCreateInfo vmaCreateInfo = {};
        vmaCreateInfo.physicalDevice         = ToApi(pCreateInfo->pGpu)->GetVkGpu();
        vmaCreateInfo.device                 = mDevice;
        vmaCreateInfo.instance               = ToApi(GetInstance())->GetVkInstance();

        vkres = vmaCreateAllocator(&vmaCreateInfo, &mVmaAllocator);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vmaCreateAllocator failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }

    // Create queues
    ppxres = CreateQueues(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Device::DestroyApiObjects()
{
    if (mVmaAllocator) {
        vmaDestroyAllocator(mVmaAllocator);
        mVmaAllocator.Reset();
    }

    if (mDevice) {
        vkDestroyDevice(mDevice, nullptr);
        mDevice.Reset();
    }
}

Result Device::AllocateObject(grfx::Buffer** ppObject)
{
    vk::Buffer* pObject = new vk::Buffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandBuffer** ppObject)
{
    vk::CommandBuffer* pObject = new vk::CommandBuffer();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::CommandPool** ppObject)
{
    vk::CommandPool* pObject = new vk::CommandPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ComputePipeline** ppObject)
{
    vk::ComputePipeline* pObject = new vk::ComputePipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DepthStencilView** ppObject)
{
    vk::DepthStencilView* pObject = new vk::DepthStencilView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorPool** ppObject)
{
    vk::DescriptorPool* pObject = new vk::DescriptorPool();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSet** ppObject)
{
    vk::DescriptorSet* pObject = new vk::DescriptorSet();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::DescriptorSetLayout** ppObject)
{
    vk::DescriptorSetLayout* pObject = new vk::DescriptorSetLayout();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Fence** ppObject)
{
    vk::Fence* pObject = new vk::Fence();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::GraphicsPipeline** ppObject)
{
    vk::GraphicsPipeline* pObject = new vk::GraphicsPipeline();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Image** ppObject)
{
    vk::Image* pObject = new vk::Image();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::PipelineInterface** ppObject)
{
    vk::PipelineInterface* pObject = new vk::PipelineInterface();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Queue** ppObject)
{
    vk::Queue* pObject = new vk::Queue();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Query** ppObject)
{
    vk::Query* pObject = new vk::Query();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderPass** ppObject)
{
    vk::RenderPass* pObject = new vk::RenderPass();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::RenderTargetView** ppObject)
{
    vk::RenderTargetView* pObject = new vk::RenderTargetView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::SampledImageView** ppObject)
{
    vk::SampledImageView* pObject = new vk::SampledImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Sampler** ppObject)
{
    vk::Sampler* pObject = new vk::Sampler();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Semaphore** ppObject)
{
    vk::Semaphore* pObject = new vk::Semaphore();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::ShaderModule** ppObject)
{
    vk::ShaderModule* pObject = new vk::ShaderModule();
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
    vk::StorageImageView* pObject = new vk::StorageImageView();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::AllocateObject(grfx::Swapchain** ppObject)
{
    vk::Swapchain* pObject = new vk::Swapchain();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
}

Result Device::WaitIdle()
{
    VkResult vkres = vkDeviceWaitIdle(mDevice);
    if (vkres != VK_SUCCESS) {
        return ppx::ERROR_API_FAILURE;
    }
    return ppx::SUCCESS;
}

bool Device::PipelineStatsAvailable() const
{
    return mDeviceFeatures.pipelineStatisticsQuery;
}

bool Device::DynamicRenderingSupported() const
{
    return mHasDynamicRendering;
}

bool Device::IndependentBlendingSupported() const
{
    return mDeviceFeatures.independentBlend == VK_TRUE;
}

bool Device::FragmentStoresAndAtomicsSupported() const
{
    return mDeviceFeatures.fragmentStoresAndAtomics == VK_TRUE;
}

void Device::ResetQueryPoolEXT(
    VkQueryPool queryPool,
    uint32_t    firstQuery,
    uint32_t    queryCount) const
{
    mFnResetQueryPoolEXT(mDevice, queryPool, firstQuery, queryCount);
}

std::array<uint32_t, 3> Device::GetAllQueueFamilyIndices() const
{
    return {mGraphicsQueueFamilyIndex, mComputeQueueFamilyIndex, mTransferQueueFamilyIndex};
}

} // namespace vk
} // namespace grfx
} // namespace ppx
