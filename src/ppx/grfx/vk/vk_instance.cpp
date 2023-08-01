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

#include "ppx/grfx/vk/vk_instance.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_profiler_fn_wrapper.h"
#include "ppx/grfx/vk/vk_swapchain.h"

namespace ppx {
namespace grfx {
namespace vk {

// -------------------------------------------------------------------------------------------------
// Default DebugUtilsMessengerCallback
// -------------------------------------------------------------------------------------------------
static VkBool32 VKAPI_PTR DebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
    // Ignore these messages because they're nonsense
    // clang-format off
    if (
        (pCallbackData->messageIdNumber == 0x3d492883) // vkCreateShaderModule(): The SPIR-V Extension (SPV_GOOGLE_hlsl_functionality1) was declared, but none of the requirements were met to use it. The Vulkan spec states: If pCode declares any of the SPIR-V extensions listed in the SPIR-V Environment appendix, one of the corresponding requirements must be satisfied
    ) {
        return VK_FALSE;
    }
    // clang-format on

    // clang-format off
    // Severity
    std::string severity = "<UNKNOWN MESSAGE SEVERITY>";
    switch (messageSeverity) {
        default: break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT : severity = "VERBOSE"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    : severity = "INFO"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : severity = "WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   : severity = "ERROR"; break;
    }
    // clang-format on

    // Type
    std::stringstream ssType;
    ssType << "[";
    {
        uint32_t type_count = 0;
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
            ssType << "GENERAL";
            ++type_count;
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            if (type_count > 0) {
                ssType << ", ";
            }
            ssType << "VALIDATION";
            ++type_count;
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            if (type_count > 0) {
                ssType << ", ";
            }
            ssType << "PERFORMANCE";
        }
    }
    ssType << "]";
    std::string type = ssType.str();
    if (type.empty()) {
        type = "<UNKNOWN MESSAGE TYPE>";
    }

    // clang-format off
    std::stringstream ss;
    ss << "\n";
    ss << "*** VULKAN VALIDATION " << severity << " MESSAGE ***" << "\n";
    ss << "Severity : " << severity << "\n";
    ss << "Type     : " << type << "\n";
    // clang-format on

    if (pCallbackData->objectCount > 0) {
        ss << "Objects  : ";
        for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
            auto object_name_info = pCallbackData->pObjects[i];

            std::string name = (object_name_info.pObjectName != nullptr)
                                   ? object_name_info.pObjectName
                                   : "<UNNAMED OBJECT>";
            if (i > 0) {
                ss << "           ";
            }
            ss << "[" << i << "]"
               << ": " << name << "\n";
        }
    }

    ss << "Message  : " << pCallbackData->pMessage;
    ss << std::endl;

    bool isError      = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    bool isValidation = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT);
    if (isError && isValidation) {
        PPX_LOG_RAW(ss.str().c_str());
        assert(false);
    }
    else {
        PPX_LOG_RAW(ss.str().c_str());
    }

    return VK_FALSE;
}

Result Instance::ConfigureLayersAndExtensions(const grfx::InstanceCreateInfo* pCreateInfo)
{
    // Enumerate layers and extensions
    {
        // Layers
        uint32_t count = 0;
        VkResult vkres = vkEnumerateInstanceLayerProperties(&count, nullptr);
        PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateInstanceLayerProperties(0) failed");
        if (vkres == VK_SUCCESS) {
            std::vector<VkLayerProperties> properties(count);
            vkres = vkEnumerateInstanceLayerProperties(&count, properties.data());
            PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateInstanceLayerProperties(1) failed");
            if (vkres == VK_SUCCESS) {
                for (auto& elem : properties) {
                    mFoundLayers.push_back(elem.layerName);
                }
                Unique(mFoundLayers);
            }
        }

        // Extensions
        std::vector<const char*> layers = GetCStrings(mFoundLayers);
        // Add nullptr for standalone extensions
        layers.push_back(nullptr);
        // Enumerate
        for (auto& layer : layers) {
            count = 0;
            vkres = vkEnumerateInstanceExtensionProperties(layer, &count, nullptr);
            PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateInstanceExtensionProperties(0) failed");
            if (vkres == VK_SUCCESS) {
                std::vector<VkExtensionProperties> properties(count);
                vkres = vkEnumerateInstanceExtensionProperties(layer, &count, properties.data());
                PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumerateInstanceExtensionProperties(1) failed");
                if (vkres == VK_SUCCESS) {
                    for (auto& elem : properties) {
                        mFoundExtensions.push_back(elem.extensionName);
                    }
                    Unique(mFoundExtensions);
                }
            }
        }
    }

    mExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // Debug layer and extension
    if (pCreateInfo->enableDebug) {
        mLayers.push_back(VK_LAYER_KHRONOS_VALIDATION_NAME);
        mExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Swapchain surface extensions
    if (pCreateInfo->enableSwapchain) {
        mExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(PPX_LINUX_XCB)
        mExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(PPX_ANDROID)
        mExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#elif defined(PPX_MSW)
        mExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
    }

    // Add additional layers
    AppendElements(pCreateInfo->vulkanLayers, mLayers);
    AppendElements(pCreateInfo->vulkanExtensions, mExtensions);

    // Uniquify layers and extensios
    Unique(mLayers);
    Unique(mExtensions);

    return ppx::SUCCESS;
}

Result Instance::CreateDebugUtils(const grfx::InstanceCreateInfo* pCreateInfo)
{
    // Debug utils
    if (pCreateInfo->enableDebug) {
        // vkCreateDebugUtilsMessengerEXT
        mFnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
        if (mFnCreateDebugUtilsMessengerEXT == nullptr) {
            PPX_ASSERT_MSG(false, "vkGetInstanceProcAddr failed for vkGetInstanceProcAddr");
            return ppx::ERROR_API_FAILURE;
        }
        // vkDestroyDebugUtilsMessengerEXT
        mFnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (mFnCreateDebugUtilsMessengerEXT == nullptr) {
            PPX_ASSERT_MSG(false, "vkGetInstanceProcAddr failed for vkDestroyDebugUtilsMessengerEXT");
            return ppx::ERROR_API_FAILURE;
        }
        // vkSubmitDebugUtilsMessageEXT
        mFnSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(mInstance, "vkSubmitDebugUtilsMessageEXT");
        if (mFnCreateDebugUtilsMessengerEXT == nullptr) {
            PPX_ASSERT_MSG(false, "vkGetInstanceProcAddr failed for vkSubmitDebugUtilsMessageEXT");
            return ppx::ERROR_API_FAILURE;
        }

        VkDebugUtilsMessengerCreateInfoEXT vkci = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        // vkci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        // vkci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        vkci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        vkci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // vkci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        vkci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        // vkci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        vkci.pfnUserCallback = vk::DebugUtilsMessengerCallback;

        VkResult vkres = mFnCreateDebugUtilsMessengerEXT(mInstance, &vkci, nullptr, &mMessenger);
        if (vkres != VK_SUCCESS) {
            PPX_ASSERT_MSG(false, "vkCreateDebugUtilsMessengerEXT failed: " << ToString(vkres));
            return ppx::ERROR_API_FAILURE;
        }
    }

    return ppx::SUCCESS;
}

Result Instance::EnumerateAndCreateeGpus()
{
    PPX_ASSERT_MSG(!mCreateInfo.useSoftwareRenderer, "A software renderer was requested but it is not available in Vulkan.");

#if defined(PPX_BUILD_XR)
    if (isXREnabled()) {
        const XrComponent&               xrComponent = *mCreateInfo.pXrComponent;
        VkPhysicalDevice                 physicalDevice;
        XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
        deviceGetInfo.systemId                                           = xrComponent.GetSystemId();
        deviceGetInfo.vulkanInstance                                     = mInstance;
        PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2KHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrGetVulkanGraphicsDevice2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsDevice2KHR)));
        PPX_ASSERT_MSG(pfnGetVulkanGraphicsDevice2KHR != nullptr, "Cannot get xrGetVulkanGraphicsDevice2KHR function pointer!");
        CHECK_XR_CALL(pfnGetVulkanGraphicsDevice2KHR(xrComponent.GetInstance(), &deviceGetInfo, &physicalDevice));

        grfx::internal::GpuCreateInfo gpuCreateInfo = {};
        gpuCreateInfo.pApiObject                    = static_cast<void*>(physicalDevice);
        grfx::GpuPtr tmpGpu;
        Result       ppxres = CreateGpu(&gpuCreateInfo, &tmpGpu);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "Failed creating GPU object! ");
            return ppxres;
        }
        PPX_LOG_INFO("   "
                     << "graphics queue count : " << tmpGpu->GetGraphicsQueueCount());
        PPX_LOG_INFO("   "
                     << "compute  queue count : " << tmpGpu->GetComputeQueueCount());
        PPX_LOG_INFO("   "
                     << "transfer queue count : " << tmpGpu->GetTransferQueueCount());

        mXrGraphicsBinding.instance         = mInstance;
        mXrGraphicsBinding.physicalDevice   = physicalDevice;
        mXrGraphicsBinding.device           = VK_NULL_HANDLE;
        mXrGraphicsBinding.queueFamilyIndex = ToApi(tmpGpu)->GetGraphicsQueueFamilyIndex();
        mXrGraphicsBinding.queueIndex       = 0; // TODO(wangra): find a better way to handle this
    }
    else
#endif
    {
        uint32_t count = 0;
        VkResult vkres = vkEnumeratePhysicalDevices(mInstance, &count, nullptr);
        PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumeratePhysicalDevices(0) failed");
        if (vkres == VK_SUCCESS) {
            if (count == 0) {
                return ppx::ERROR_NO_GPUS_FOUND;
            }

            std::vector<VkPhysicalDevice> physicalDevices(count);
            vkres = vkEnumeratePhysicalDevices(mInstance, &count, physicalDevices.data());
            PPX_ASSERT_MSG((vkres == VK_SUCCESS), "vkEnumeratePhysicalDevices(1) failed");
            if (vkres == VK_SUCCESS) {
                for (uint32_t i = 0; i < count; ++i) {
                    VkPhysicalDeviceProperties deviceProperties = {};
                    vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
                    PPX_LOG_INFO("Found GPU [" << i << "]: " << deviceProperties.deviceName);

                    grfx::internal::GpuCreateInfo gpuCreateInfo = {};
                    gpuCreateInfo.pApiObject                    = static_cast<void*>(physicalDevices[i]);

                    grfx::GpuPtr tmpGpu;
                    Result       ppxres = CreateGpu(&gpuCreateInfo, &tmpGpu);
                    if (Failed(ppxres)) {
                        PPX_ASSERT_MSG(false, "Failed creating GPU object using " << deviceProperties.deviceName);
                        return ppxres;
                    }
                    PPX_LOG_INFO("   "
                                 << "graphics queue count : " << tmpGpu->GetGraphicsQueueCount());
                    PPX_LOG_INFO("   "
                                 << "compute  queue count : " << tmpGpu->GetComputeQueueCount());
                    PPX_LOG_INFO("   "
                                 << "transfer queue count : " << tmpGpu->GetTransferQueueCount());
                }
            }
        }
    }
    return ppx::SUCCESS;
}

Result Instance::CreateApiObjects(const grfx::InstanceCreateInfo* pCreateInfo)
{
    // Register profiling functions
    vk::RegisterProfilerFunctions();

    // Configure layers and extensions
    Result ppxres = ConfigureLayersAndExtensions(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Enumerate version
    uint32_t foundVkVersion = 0;
    VkResult vkres          = vkEnumerateInstanceVersion(&foundVkVersion);
    if (vkres != VK_SUCCESS) {
        PPX_ASSERT_MSG(false, "vkEnumerateInstanceVersion failed: " << ToString(vkres));
        return ppx::ERROR_API_FAILURE;
    }

    // Version check
    uint32_t vkVersion = ppx::InvalidValue<uint32_t>();
    switch (pCreateInfo->api) {
        default: break;

        case grfx::API_VK_1_1: {
            vkVersion = VK_MAKE_VERSION(1, 1, 0);
            PPX_LOG_INFO("Using Vulkan 1.1");
        } break;

        case grfx::API_VK_1_2: {
            vkVersion = VK_MAKE_VERSION(1, 2, 0);
            PPX_LOG_INFO("Using Vulkan 1.2");
        } break;
    }
    if ((vkVersion == ppx::InvalidValue<uint32_t>()) || (vkVersion > foundVkVersion)) {
        std::stringstream ss;
        ss << "Unsupported Vulkan version";
        if (vkVersion != ppx::InvalidValue<uint32_t>()) {
            ss << "(wanted Vulkan " << VK_VERSION_MAJOR(vkVersion) << "." << VK_VERSION_MINOR(vkVersion);
            ss << " but got " << VK_VERSION_MAJOR(foundVkVersion) << "." << VK_VERSION_MINOR(foundVkVersion) << ")";
        }
        PPX_ASSERT_MSG(false, ss.str());
        return ppx::ERROR_UNSUPPORTED_API;
    }

    // Get C strings
    std::vector<const char*> layers     = GetCStrings(mLayers);
    std::vector<const char*> extensions = GetCStrings(mExtensions);

    VkApplicationInfo applicationInfo  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName   = pCreateInfo->applicationName.empty() ? "PPX Application" : pCreateInfo->applicationName.c_str();
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName        = pCreateInfo->engineName.empty() ? "PPX Engine" : pCreateInfo->engineName.c_str();
    applicationInfo.engineVersion      = 0;
    applicationInfo.apiVersion         = vkVersion;

    VkInstanceCreateInfo vkci    = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    vkci.flags                   = 0;
    vkci.pApplicationInfo        = &applicationInfo;
    vkci.enabledLayerCount       = CountU32(layers);
    vkci.ppEnabledLayerNames     = DataPtr(layers);
    vkci.enabledExtensionCount   = CountU32(extensions);
    vkci.ppEnabledExtensionNames = DataPtr(extensions);

    // Log layers and extensions
    {
        PPX_LOG_INFO("Loading " << vkci.enabledLayerCount << " Vulkan instance layers");
        for (uint32_t i = 0; i < vkci.enabledLayerCount; ++i) {
            PPX_LOG_INFO("   " << i << " : " << vkci.ppEnabledLayerNames[i]);
        }

        PPX_LOG_INFO("Loading " << vkci.enabledExtensionCount << " Vulkan instance extensions");
        for (uint32_t i = 0; i < vkci.enabledExtensionCount; ++i) {
            PPX_LOG_INFO("   " << i << " : " << vkci.ppEnabledExtensionNames[i]);
        }
    }

#if defined(PPX_BUILD_XR)
    if (isXREnabled()) {
        mXrGraphicsBinding = {XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR, nullptr, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, UINT32_MAX, UINT32_MAX};
        PPX_ASSERT_MSG(pCreateInfo->pXrComponent != nullptr, "XrComponent should not be nullptr!");
        const XrComponent& xrComponent = *pCreateInfo->pXrComponent;
        // TODO(wangra): Can verify versions if required
        XrGraphicsRequirementsVulkan2KHR        graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
        PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrGetVulkanGraphicsRequirements2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirements2KHR)));
        PPX_ASSERT_MSG(pfnGetVulkanGraphicsRequirements2KHR != nullptr, "Cannot get xrGetVulkanGraphicsRequirements2KHR function pointer!");
        CHECK_XR_CALL(pfnGetVulkanGraphicsRequirements2KHR(xrComponent.GetInstance(), xrComponent.GetSystemId(), &graphicsRequirements));

        // Create Vulkan Instance
        XrVulkanInstanceCreateInfoKHR createInfo{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
        createInfo.systemId                                      = xrComponent.GetSystemId();
        createInfo.pfnGetInstanceProcAddr                        = &vkGetInstanceProcAddr;
        createInfo.vulkanCreateInfo                              = &vkci;
        createInfo.vulkanAllocator                               = nullptr;
        PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(xrComponent.GetInstance(), "xrCreateVulkanInstanceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanInstanceKHR)));
        PPX_ASSERT_MSG(pfnCreateVulkanInstanceKHR != nullptr, "Cannot get xrCreateVulkanInstanceKHR function pointer!");
        CHECK_XR_CALL(pfnCreateVulkanInstanceKHR(xrComponent.GetInstance(), &createInfo, &mInstance, &vkres));
    }
    else
#endif
    {
        vkres = vkCreateInstance(&vkci, nullptr, &mInstance);
    }

    if (vkres != VK_SUCCESS) {
        // clang-format off
        std::stringstream ss;
        ss << "vkCreateInstance failed: " << ToString(vkres);
        if (vkres == VK_ERROR_LAYER_NOT_PRESENT) {
            std::vector<std::string> missing = GetNotFound(mLayers, mFoundLayers);
            ss << PPX_LOG_ENDL;
            ss << "  " << "Layer(s) not found:" << PPX_LOG_ENDL;
            for (auto& elem : missing) {
                ss << "  " << "  " << elem << PPX_LOG_ENDL;
            }
        }
        else if (vkres == VK_ERROR_EXTENSION_NOT_PRESENT) {
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

    ppxres = CreateDebugUtils(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = EnumerateAndCreateeGpus();
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void Instance::DestroyApiObjects()
{
    if (mInstance) {
        if (mMessenger) {
            mFnDestroyDebugUtilsMessengerEXT(mInstance, mMessenger, nullptr);
            mMessenger.Reset();
        }

        vkDestroyInstance(mInstance, nullptr);
        mInstance.Reset();
    }
}

Result Instance::AllocateObject(grfx::Device** ppDevice)
{
    vk::Device* pObject = new vk::Device();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppDevice = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Gpu** ppGpu)
{
    vk::Gpu* pObject = new vk::Gpu();
    if (IsNull(pObject)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }
    *ppGpu = pObject;
    return ppx::SUCCESS;
}

Result Instance::AllocateObject(grfx::Surface** ppSurface)
{
    vk::Surface* pObject = new vk::Surface();
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
    return (mXrGraphicsBinding.instance != VK_NULL_HANDLE) && (mXrGraphicsBinding.physicalDevice != VK_NULL_HANDLE) && (mXrGraphicsBinding.device != VK_NULL_HANDLE) && (mXrGraphicsBinding.queueFamilyIndex != UINT32_MAX) && (mXrGraphicsBinding.queueIndex != UINT32_MAX);
}

void Instance::XrUpdateDeviceInGraphicsBinding()
{
    PPX_ASSERT_MSG(mDevices.size() == 1, "there should be 1 valid device for XR!");
    mXrGraphicsBinding.device = ToApi(mDevices[0])->GetVkDevice();
}
#endif
} // namespace vk
} // namespace grfx
} // namespace ppx
