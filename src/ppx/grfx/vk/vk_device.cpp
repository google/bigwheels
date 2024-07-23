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
#include "ppx/grfx/vk/vk_shading_rate.h"
#include "ppx/grfx/vk/vk_swapchain.h"
#include "ppx/grfx/vk/vk_sync.h"
#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#include "vk_mem_alloc.h"
#include <unordered_set>

namespace ppx {
namespace grfx {
namespace vk {

PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR = nullptr;

#if defined(VK_KHR_dynamic_rendering)
PFN_vkCmdBeginRenderingKHR CmdBeginRenderingKHR = nullptr;
PFN_vkCmdEndRenderingKHR   CmdEndRenderingKHR   = nullptr;
#endif

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

    // Variable rate shading
    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_VRS) {
        PPX_ASSERT_MSG(
            ElementExists(std::string(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME), mFoundExtensions),
            "VRS shading rate requires unsupported extension " << VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);

        PPX_ASSERT_MSG(
            ElementExists(std::string(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME), mFoundExtensions),
            "VRS shading rate requires unsupported extension " << VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    }

    // Fragment density map
    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_FDM) {
        PPX_ASSERT_MSG(
            ElementExists(std::string(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME), mFoundExtensions),
            "FDM shading rate requires unsupported extension " << VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);
        mExtensions.push_back(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);

        // VK_EXT_fragment_density_map2 is required on some drivers to enable subsampled images.
        mExtensions.push_back(VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME);

        // VK_KHR_create_renderpass2 is not required for FDM, but simplifies
        // code to create the RenderPass.
        PPX_ASSERT_MSG(
            ElementExists(std::string(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME), mFoundExtensions),
            "FDM shading rate requires unsupported extension " << VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    }

#if defined(PPX_VK_EXTENDED_DYNAMIC_STATE)
    if (ElementExists(std::string(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    }
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

    // Depth clip
    if (ElementExists(std::string(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
    }

    // MultiView
    if (ElementExists(std::string(VK_KHR_MULTIVIEW_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
    }

    // Push descriptors
    if (ElementExists(std::string(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    }

    // YCbCr color conversion
    if (ElementExists(std::string(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
    }

    // Dynamic rendering - if present. It also requires
    // VK_KHR_depth_stencil_resolve and VK_KHR_create_renderpass2.
#if defined(VK_KHR_dynamic_rendering)
    if (ElementExists(std::string(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME), mFoundExtensions) &&
        ElementExists(std::string(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME), mFoundExtensions) &&
        ElementExists(std::string(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME), mFoundExtensions) &&
        ElementExists(std::string(VK_KHR_MULTIVIEW_EXTENSION_NAME), mFoundExtensions) &&
        ElementExists(std::string(VK_KHR_MAINTENANCE2_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
        mExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
        mHasDynamicRendering = true;
    }
#endif

    // 8 bit index buffer
    if (ElementExists(std::string(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME), mFoundExtensions)) {
        mExtensions.push_back(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME);
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
    // 2024/02/13 - Changed fillModeNonSolid to true to allow use of VK_POLYGON_MODE_LINE.
    // 2021/11/15 - Changed logic to use feature bit from GPU for geo and tess shaders to accomodate
    //              SwiftShader not having support for these shader types.
    //
    features                                      = {};
    features.fillModeNonSolid                     = VK_TRUE;
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

    if (ElementExists(std::string(VK_KHR_MULTIVIEW_EXTENSION_NAME), mExtensions)) {
        mHasMultiView = pCreateInfo->multiView;
    }

    // Select between default or custom features.
    if (!IsNull(pCreateInfo->pVulkanDeviceFeatures)) {
        const VkPhysicalDeviceFeatures* pFeatures = static_cast<const VkPhysicalDeviceFeatures*>(pCreateInfo->pVulkanDeviceFeatures);
        features                                  = *pFeatures;
    }

    // Enable shader resource array dynamic indexing.
    // This can be used to choose a texture within an array based on
    // a push constant, among other things.
    std::vector<std::string_view> missingFeatures;
    if (!foundFeatures.shaderUniformBufferArrayDynamicIndexing) {
        missingFeatures.push_back("shaderUniformBufferArrayDynamicIndexing");
    }
    if (!foundFeatures.shaderSampledImageArrayDynamicIndexing) {
        missingFeatures.push_back("shaderSampledImageArrayDynamicIndexing");
    }
    if (!foundFeatures.shaderStorageBufferArrayDynamicIndexing) {
        missingFeatures.push_back("shaderStorageBufferArrayDynamicIndexing");
    }
    if (!foundFeatures.shaderStorageImageArrayDynamicIndexing) {
        missingFeatures.push_back("shaderStorageImageArrayDynamicIndexing");
    }

    if (!missingFeatures.empty()) {
        std::stringstream ss;
        ss << "Device does not support required features:" << PPX_LOG_ENDL;
        for (const auto& elem : missingFeatures) {
            ss << " " << elem << PPX_LOG_ENDL;
        }
        PPX_ASSERT_MSG(false, ss.str());
        return ppx::ERROR_REQUIRED_FEATURE_UNAVAILABLE;
    }

    features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
    features.shaderSampledImageArrayDynamicIndexing  = VK_TRUE;
    features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
    features.shaderStorageImageArrayDynamicIndexing  = VK_TRUE;

    return ppx::SUCCESS;
}

Result Device::ConfigureDescriptorIndexingFeatures(
    const grfx::DeviceCreateInfo* pCreateInfo, VkPhysicalDeviceDescriptorIndexingFeatures& diFeatures)
{
    vk::Gpu* pGpu = ToApi(pCreateInfo->pGpu);

    VkPhysicalDeviceDescriptorIndexingFeatures foundDiFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    VkPhysicalDeviceFeatures2                  foundFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &foundDiFeatures};
    vkGetPhysicalDeviceFeatures2(pGpu->GetVkGpu(), &foundFeatures);

    //
    // 2023/10/01 - Just runtimeDescriptorArrays for now - need to survey what Android
    //              usage is like before enabling other freatures.
    // 2024/03/12 - Fetch features from the GPU, and enable any features that are
    //              supported. runtimeDescriptorArray was forced to true before, so
    //              that setting was kept.
    //
    diFeatures.shaderInputAttachmentArrayDynamicIndexing          = foundDiFeatures.shaderInputAttachmentArrayDynamicIndexing;
    diFeatures.shaderUniformTexelBufferArrayDynamicIndexing       = foundDiFeatures.shaderUniformTexelBufferArrayDynamicIndexing;
    diFeatures.shaderStorageTexelBufferArrayDynamicIndexing       = foundDiFeatures.shaderStorageTexelBufferArrayDynamicIndexing;
    diFeatures.shaderUniformBufferArrayNonUniformIndexing         = foundDiFeatures.shaderUniformBufferArrayNonUniformIndexing;
    diFeatures.shaderSampledImageArrayNonUniformIndexing          = foundDiFeatures.shaderSampledImageArrayNonUniformIndexing;
    diFeatures.shaderStorageBufferArrayNonUniformIndexing         = foundDiFeatures.shaderStorageBufferArrayNonUniformIndexing;
    diFeatures.shaderStorageImageArrayNonUniformIndexing          = foundDiFeatures.shaderStorageImageArrayNonUniformIndexing;
    diFeatures.shaderInputAttachmentArrayNonUniformIndexing       = foundDiFeatures.shaderInputAttachmentArrayNonUniformIndexing;
    diFeatures.shaderUniformTexelBufferArrayNonUniformIndexing    = foundDiFeatures.shaderUniformTexelBufferArrayNonUniformIndexing;
    diFeatures.shaderStorageTexelBufferArrayNonUniformIndexing    = foundDiFeatures.shaderStorageTexelBufferArrayNonUniformIndexing;
    diFeatures.descriptorBindingUniformBufferUpdateAfterBind      = foundDiFeatures.descriptorBindingUniformBufferUpdateAfterBind;
    diFeatures.descriptorBindingSampledImageUpdateAfterBind       = VK_TRUE;
    diFeatures.descriptorBindingStorageImageUpdateAfterBind       = foundDiFeatures.descriptorBindingStorageImageUpdateAfterBind;
    diFeatures.descriptorBindingStorageBufferUpdateAfterBind      = foundDiFeatures.descriptorBindingStorageBufferUpdateAfterBind;
    diFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = foundDiFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind;
    diFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = foundDiFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind;
    diFeatures.descriptorBindingUpdateUnusedWhilePending          = foundDiFeatures.descriptorBindingUpdateUnusedWhilePending;
    diFeatures.descriptorBindingPartiallyBound                    = foundDiFeatures.descriptorBindingPartiallyBound;
    diFeatures.descriptorBindingVariableDescriptorCount           = foundDiFeatures.descriptorBindingVariableDescriptorCount;
    diFeatures.runtimeDescriptorArray                             = VK_TRUE;

    // Verify that any asserted features were actually found to be
    // supported.
    std::vector<std::string_view> missingFeatures;
    if (!foundDiFeatures.descriptorBindingSampledImageUpdateAfterBind) {
        missingFeatures.push_back("descriptorBindingSampledImageUpdateAfterBind");
    }
    if (!foundDiFeatures.runtimeDescriptorArray) {
        missingFeatures.push_back("runtimeDescriptorArray");
    }

    if (!missingFeatures.empty()) {
        std::stringstream ss;
        ss << "Device does not support required features:" << PPX_LOG_ENDL;
        for (const auto& elem : missingFeatures) {
            ss << " " << elem << PPX_LOG_ENDL;
        }
        PPX_ASSERT_MSG(false, ss.str());
        return ppx::ERROR_REQUIRED_FEATURE_UNAVAILABLE;
    }

    return ppx::SUCCESS;
}

void Device::ConfigureShadingRateCapabilities(const grfx::DeviceCreateInfo* pCreateInfo, grfx::ShadingRateCapabilities* pShadingRateCapabilities)
{
    *pShadingRateCapabilities = {};
    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_NONE) {
        return;
    }

    VkInstance       instance       = ToApi(GetInstance())->GetVkInstance();
    VkPhysicalDevice physicalDevice = ToApi(pCreateInfo->pGpu)->GetVkGpu();

    mFnGetPhysicalDeviceFeatures2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2"));
    PPX_ASSERT_MSG(
        mFnGetPhysicalDeviceFeatures2 != nullptr,
        "ConfigureShadingRateCapabilities: Failed to load vkGetPhysicalDeviceFeatures2");

    mFnGetPhysicalDeviceProperties2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2"));
    PPX_ASSERT_MSG(
        mFnGetPhysicalDeviceProperties2 != nullptr,
        "ConfigureShadingRateCapabilities: Failed to load vkGetPhysicalDeviceProperties2");

    pShadingRateCapabilities->supportedShadingRateMode = pCreateInfo->supportShadingRateMode;

    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_FDM) {
        ConfigureFDMShadingRateCapabilities(physicalDevice, pShadingRateCapabilities);
    }

    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_VRS) {
        ConfigureVRSShadingRateCapabilities(physicalDevice, pShadingRateCapabilities);
    }
}

void Device::ConfigureFDMShadingRateCapabilities(
    VkPhysicalDevice               physicalDevice,
    grfx::ShadingRateCapabilities* pShadingRateCapabilities)
{
    VkPhysicalDeviceFeatures2                     features    = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdmFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
    InsertPNext(features, fdmFeatures);
    mFnGetPhysicalDeviceFeatures2(physicalDevice, &features);

    VkPhysicalDeviceProperties2                     properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceFragmentDensityMapPropertiesEXT fdmProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT};
    InsertPNext(properties, fdmProperties);
    mFnGetPhysicalDeviceProperties2(physicalDevice, &properties);

    PPX_ASSERT_MSG(
        fdmFeatures.fragmentDensityMap == VK_TRUE,
        "FDM shading rate mode was requested, but not supported by the GPU.");

    pShadingRateCapabilities->fdm.supportsNonSubsampledImages = fdmFeatures.fragmentDensityMapNonSubsampledImages;

    pShadingRateCapabilities->fdm.minTexelSize = {
        fdmProperties.minFragmentDensityTexelSize.width,
        fdmProperties.minFragmentDensityTexelSize.height};
    pShadingRateCapabilities->fdm.maxTexelSize = {
        fdmProperties.maxFragmentDensityTexelSize.width,
        fdmProperties.maxFragmentDensityTexelSize.height};
}

void Device::ConfigureVRSShadingRateCapabilities(
    VkPhysicalDevice               physicalDevice,
    grfx::ShadingRateCapabilities* pShadingRateCapabilities)
{
    VkPhysicalDeviceFeatures2                      features    = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR vrsFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
    InsertPNext(features, vrsFeatures);
    mFnGetPhysicalDeviceFeatures2(physicalDevice, &features);

    VkPhysicalDeviceProperties2                      properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR vrsProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR};
    InsertPNext(properties, vrsProperties);
    mFnGetPhysicalDeviceProperties2(physicalDevice, &properties);

    PPX_ASSERT_MSG(
        (vrsFeatures.pipelineFragmentShadingRate == VK_TRUE) &&
            (vrsFeatures.attachmentFragmentShadingRate == VK_TRUE),
        "VRS shading rate mode was requested, but not supported by the GPU.");

    if (!vrsFeatures.pipelineFragmentShadingRate && !vrsFeatures.primitiveFragmentShadingRate && !vrsFeatures.attachmentFragmentShadingRate) {
        return;
    }

    pShadingRateCapabilities->vrs.minTexelSize = {
        vrsProperties.minFragmentShadingRateAttachmentTexelSize.width,
        vrsProperties.minFragmentShadingRateAttachmentTexelSize.height};
    pShadingRateCapabilities->vrs.maxTexelSize = {
        vrsProperties.maxFragmentShadingRateAttachmentTexelSize.width,
        vrsProperties.maxFragmentShadingRateAttachmentTexelSize.height};

    VkInstance instance = ToApi(GetInstance())->GetVkInstance();
    mFnGetPhysicalDeviceFragmentShadingRatesKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR"));
    PPX_ASSERT_MSG(
        mFnGetPhysicalDeviceFragmentShadingRatesKHR != nullptr,
        "ConfigureVRSShadingRateCapabilities: Failed to load vkGetPhysicalDeviceFragmentShadingRatesKHR");

    uint32_t fragmentShadingRateCount = 0;
    VkResult vkres                    = mFnGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, &fragmentShadingRateCount, nullptr);
    PPX_ASSERT_MSG(vkres == VK_SUCCESS, "vkGetPhysicalDeviceFragmentShadingRatesKHR failed");

    std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragmentShadingRates(
        fragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR});
    vkres = mFnGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, &fragmentShadingRateCount, fragmentShadingRates.data());
    PPX_ASSERT_MSG(vkres == VK_SUCCESS, "vkGetPhysicalDeviceFragmentShadingRatesKHR failed");

    for (const auto& rate : fragmentShadingRates) {
        auto& supportedRate           = pShadingRateCapabilities->vrs.supportedRates.emplace_back();
        supportedRate.sampleCountMask = rate.sampleCounts;
        supportedRate.fragmentSize    = {rate.fragmentSize.width, rate.fragmentSize.height};
    }
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
    ConfigureShadingRateCapabilities(pCreateInfo, &mShadingRateCapabilities);

    // We can't include structs whose extesnions aren't enabled, so do the tracking.
    std::vector<VkBaseOutStructure*> extensionStructs;

    // VK_EXT_descriptor_indexing
    mDescriptorIndexingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_2) || ElementExists(std::string(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME), mExtensions)) {
        mHasDescriptorIndexingFeatures = true;
        ConfigureDescriptorIndexingFeatures(pCreateInfo, mDescriptorIndexingFeatures);
        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&mDescriptorIndexingFeatures));
    }

    // VK_EXT_scalar_block_layout
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_2) || ElementExists(std::string(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME), mExtensions)) {
        scalarBlockLayoutFeatures.scalarBlockLayout = VK_TRUE;

        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&scalarBlockLayoutFeatures));
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

#if defined(VK_KHR_dynamic_rendering)
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR};
    if ((GetInstance()->GetApi() >= grfx::API_VK_1_3) || ElementExists(std::string(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME), mExtensions)) {
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&dynamicRenderingFeatures));
    }
#endif

    PPX_LOG_INFO("Vulkan MultiView is chosen and present: " << mHasMultiView);
    VkPhysicalDeviceMultiviewFeatures physicalDeviceMultiviewFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    if (GetInstance()->GetApi() >= grfx::API_VK_1_1 && mHasMultiView) {
        physicalDeviceMultiviewFeatures.pNext                       = nullptr;
        physicalDeviceMultiviewFeatures.multiview                   = VK_TRUE;
        physicalDeviceMultiviewFeatures.multiviewGeometryShader     = VK_FALSE;
        physicalDeviceMultiviewFeatures.multiviewTessellationShader = VK_FALSE;
        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&physicalDeviceMultiviewFeatures));
    }

    VkPhysicalDeviceFragmentDensityMapFeaturesEXT fragmentDensityMapFeature = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_FDM) {
        fragmentDensityMapFeature.fragmentDensityMap = VK_TRUE;
        if (mShadingRateCapabilities.fdm.supportsNonSubsampledImages) {
            fragmentDensityMapFeature.fragmentDensityMapNonSubsampledImages = VK_TRUE;
        }
        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&fragmentDensityMapFeature));
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeature = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
    if (pCreateInfo->supportShadingRateMode == SHADING_RATE_VRS) {
        fragmentShadingRateFeature.pipelineFragmentShadingRate   = VK_TRUE;
        fragmentShadingRateFeature.attachmentFragmentShadingRate = VK_TRUE;
        extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&fragmentShadingRateFeature));
    }

    // VK_EXT_index_type_uint8
    VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT};
    if (ElementExists(std::string(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME), mExtensions)) {
        VkPhysicalDeviceFeatures2 foundFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexTypeUint8Features};
        vkGetPhysicalDeviceFeatures2(ToApi(pCreateInfo->pGpu)->GetVkGpu(), &foundFeatures);
        if (indexTypeUint8Features.indexTypeUint8 == VK_TRUE) {
            mIndexTypeUint8Supported = true;
            extensionStructs.push_back(reinterpret_cast<VkBaseOutStructure*>(&indexTypeUint8Features));
        }
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
        ss << "vkCreateDevice failed: " << ToString(vkres);
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
    if (mHasTimelineSemaphore) {
        // Load in KHR versions of functions since they'll cover Vulkan 1.1 and later versions
        mFnWaitSemaphores           = (PFN_vkWaitSemaphoresKHR)vkGetDeviceProcAddr(mDevice, "vkWaitSemaphoresKHR");
        mFnSignalSemaphore          = (PFN_vkSignalSemaphoreKHR)vkGetDeviceProcAddr(mDevice, "vkSignalSemaphoreKHR");
        mFnGetSemaphoreCounterValue = (PFN_vkGetSemaphoreCounterValueKHR)vkGetDeviceProcAddr(mDevice, "vkGetSemaphoreCounterValueKHR");
    }
    PPX_LOG_INFO("Vulkan timeline semaphore is present: " << mHasTimelineSemaphore);

#if defined(VK_KHR_dynamic_rendering)
    if (GetInstance()->GetApi() == grfx::API_VK_1_3) {
        mHasDynamicRendering = true;
    }
    else {
        mHasDynamicRendering = ElementExists(std::string(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME), mExtensions);
    }
#endif
    PPX_LOG_INFO("Vulkan dynamic rendering is present: " << mHasDynamicRendering);

#if defined(PPX_VK_EXTENDED_DYNAMIC_STATE)
    mExtendedDynamicStateAvailable = ElementExists(std::string(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), mFoundExtensions));
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

    // Depth clip enabled
    mHasDepthClipEnabled = ElementExists(std::string(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME), mExtensions);

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

#if defined(VK_KHR_dynamic_rendering)
    if (mHasDynamicRendering) {
        CmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(mDevice, "vkCmdBeginRenderingKHR");
        CmdEndRenderingKHR   = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(mDevice, "vkCmdEndRenderingKHR");
    }
#endif

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

Result Device::AllocateObject(grfx::SamplerYcbcrConversion** ppObject)
{
    vk::SamplerYcbcrConversion* pObject = new vk::SamplerYcbcrConversion();
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

Result Device::AllocateObject(grfx::ShadingRatePattern** ppObject)
{
    vk::ShadingRatePattern* pObject = new vk::ShadingRatePattern();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppObject = pObject;
    return ppx::SUCCESS;
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

bool Device::MultiViewSupported() const
{
    return mHasMultiView;
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

bool Device::PartialDescriptorBindingsSupported() const
{
    return mDescriptorIndexingFeatures.descriptorBindingPartiallyBound;
}

bool Device::IndexTypeUint8Supported() const
{
    return mIndexTypeUint8Supported;
}

void Device::ResetQueryPoolEXT(
    VkQueryPool queryPool,
    uint32_t    firstQuery,
    uint32_t    queryCount) const
{
    mFnResetQueryPoolEXT(mDevice, queryPool, firstQuery, queryCount);
}

VkResult Device::WaitSemaphores(const VkSemaphoreWaitInfo* pWaitInfo, uint64_t timeout) const
{
    PPX_ASSERT_NULL_ARG(mFnWaitSemaphores);
    return mFnWaitSemaphores(mDevice, pWaitInfo, timeout);
}

VkResult Device::SignalSemaphore(const VkSemaphoreSignalInfo* pSignalInfo)
{
    PPX_ASSERT_NULL_ARG(mFnSignalSemaphore);
    return mFnSignalSemaphore(mDevice, pSignalInfo);
}

VkResult Device::GetSemaphoreCounterValue(VkSemaphore semaphore, uint64_t* pValue)
{
    PPX_ASSERT_NULL_ARG(mFnGetSemaphoreCounterValue);
    return mFnGetSemaphoreCounterValue(mDevice, semaphore, pValue);
}

std::array<uint32_t, 3> Device::GetAllQueueFamilyIndices() const
{
    return {mGraphicsQueueFamilyIndex, mComputeQueueFamilyIndex, mTransferQueueFamilyIndex};
}

} // namespace vk
} // namespace grfx
} // namespace ppx
