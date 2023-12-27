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

#if defined(PPX_BUILD_XR)
#include <queue>
#include <string_view>
#include "ppx/xr_component.h"
#include "ppx/xr_composition_layers.h"
#include "ppx/grfx/grfx_instance.h"
#include <glm/gtc/type_ptr.hpp>

namespace {

constexpr XrPosef kIdentityPose = {{0, 0, 0, 1}, {0, 0, 0}};
constexpr float   kUIZPlane     = -0.5f;

bool IsXrExtensionSupported(const std::vector<XrExtensionProperties>& supportedExts, const std::string& extName)
{
    auto it = std::find_if(supportedExts.begin(), supportedExts.end(), [&](const XrExtensionProperties& e) { return extName == e.extensionName; });
    return it != supportedExts.end();
}

XRAPI_ATTR
static XrBool32 XrDebugUtilsMessengerCallback(XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT /*messageTypes*/, const XrDebugUtilsMessengerCallbackDataEXT* callbackData, void* /*userData*/)
{
    switch (severity) {
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PPX_LOG_ERROR(callbackData->functionName << ": " << callbackData->message);
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PPX_LOG_WARN(callbackData->functionName << ": " << callbackData->message);
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        default:
            PPX_LOG_INFO(callbackData->functionName << ": " << callbackData->message);
            break;
    }
    return XR_FALSE; /* OpenXR spec suggests always returning false. */
}

template <size_t N>
void CharArrayStrCpy(char (&dst)[N], std::string_view src)
{
    PPX_ASSERT_MSG(src.length() < N, "source string too large");
    size_t length = std::min<size_t>(N - 1, src.length());
    memcpy(dst, src.data(), length);
    dst[length] = '\0';
}

ppx::float3 FromXr(const XrVector3f& v)
{
    return ppx::float3(v.x, v.y, v.z);
}

ppx::quat FromXr(const XrQuaternionf& q)
{
    return ppx::quat(q.w, q.x, q.y, q.z);
}

std::optional<XrVector2f> ProjectCursor(XrPosef controller, float uiPlaneZ)
{
    // Assuming ui is on a x-y plane located at z = uiPlaneZ

    const ppx::float3 unitForward = ppx::float3(0, 0, -1);
    const ppx::float3 direction   = glm::rotate(FromXr(controller.orientation), unitForward);
    const ppx::float3 position    = FromXr(controller.position);

    const float       zDistance        = uiPlaneZ - position.z;
    const ppx::float3 uiPlaneDirection = glm::normalize(ppx::float3(0, 0, zDistance));
    // Controller is not pointing forward (or very sideway)
    if (glm::dot(uiPlaneDirection, direction) < 0.001) {
        return std::nullopt;
    }
    float scalar = zDistance / direction.z;
    return XrVector2f{
        position.x + direction.x * scalar,
        position.y + direction.y * scalar,
    };
}

} // namespace

namespace ppx {
void XrComponent::InitializeBeforeGrfxDeviceInit(const XrComponentCreateInfo& createInfo)
{
    mCreateInfo = createInfo;
    // Extensions (Non-Optional)
    const char* graphicsAPIExtension = "";
    switch (createInfo.api) {
        default: {
            PPX_ASSERT_MSG(false, "Unsupported API");
        } break;

#if defined(PPX_D3D12)
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1:
            graphicsAPIExtension = XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
            break;
#endif // defiend(PPX_D3D12)

#if defined(PPX_VULKAN)
        case grfx::API_VK_1_1:
        case grfx::API_VK_1_2:
            graphicsAPIExtension = XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME;
            break;
#endif // defined(PPX_VULKAN)
    }

    std::vector<const char*> xrInstanceExtensions;
    xrInstanceExtensions.push_back(graphicsAPIExtension);
#if defined(PPX_ANDROID)
    XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInitInfoAndroid.applicationVM              = createInfo.androidContext->activity->vm;
    loaderInitInfoAndroid.applicationContext         = createInfo.androidContext->activity->clazz;
    PFN_xrInitializeLoaderKHR pfnInitializeLoaderKHR = nullptr;
    CHECK_XR_CALL(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&pfnInitializeLoaderKHR)));
    PPX_ASSERT_MSG(pfnInitializeLoaderKHR != nullptr, "Cannot get xrInitializeLoaderKHR function pointer!");
    CHECK_XR_CALL(pfnInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid));
    xrInstanceExtensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
#endif // defined(PPX_ANDROID)

    for (const auto& extension : createInfo.requiredExtensions) {
        xrInstanceExtensions.push_back(const_cast<char*>(extension.c_str()));
    }

    if (mCreateInfo.enableDebug) {
        xrInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    // Verify required extensions are supported.
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    std::vector<XrExtensionProperties> xrExts(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, xrExts.data());
    for (const auto& ext : xrInstanceExtensions) {
        PPX_ASSERT_MSG(IsXrExtensionSupported(xrExts, ext), "OpenXR extension not supported. Check that your OpenXR runtime is loaded properly.");
    }

    // Optional extensions.
    if (createInfo.depthFormat != grfx::FORMAT_UNDEFINED && UsesDepthSwapchains()) {
        if (IsXrExtensionSupported(xrExts, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME)) {
            xrInstanceExtensions.push_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
            mShouldSubmitDepthInfo = true;
        }
        else {
            PPX_LOG_WARN("XR depth swapchains are enabled but the " XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME
                         " extension is not supported. Depth info will not be submitted to the runtime.");
        }
    }

    if (IsXrExtensionSupported(xrExts, XR_FB_PASSTHROUGH_EXTENSION_NAME)) {
        xrInstanceExtensions.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
        mPassthroughSupported = XR_PASSTHROUGH_OCULUS;
    }

    // Layers (Optional)
    std::vector<const char*> xrRequestedInstanceLayers;
    if (mCreateInfo.enableDebug) {
        // The following environment variables must be set:
        // XR_ENABLE_API_LAYERS=XR_APILAYER_LUNARG_core_validation
        // XR_API_LAYER_PATH= *folder that contains XrApiLayer_core_validation.json*
        xrRequestedInstanceLayers.push_back("XR_APILAYER_LUNARG_core_validation");
    }
    uint32_t layerCount;
    xrEnumerateApiLayerProperties(0, &layerCount, nullptr);
    std::vector<XrApiLayerProperties> xrSupportedLayers(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
    xrEnumerateApiLayerProperties(layerCount, &layerCount, xrSupportedLayers.data());

    std::vector<const char*> xrInstanceLayers;

    for (const auto& layer : xrRequestedInstanceLayers) {
        auto it = std::find_if(xrSupportedLayers.begin(), xrSupportedLayers.end(), [&](const XrApiLayerProperties& l) { return strcmp(l.layerName, layer) == 0; });
        if (it != xrSupportedLayers.end()) {
            xrInstanceLayers.push_back(layer);
        }
    }

    // Create XrInstance
    XrInstanceCreateInfo instanceCreateInfo       = {XR_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.enabledExtensionCount      = (uint32_t)xrInstanceExtensions.size();
    instanceCreateInfo.enabledExtensionNames      = xrInstanceExtensions.data();
    instanceCreateInfo.enabledApiLayerCount       = (uint32_t)xrInstanceLayers.size();
    instanceCreateInfo.enabledApiLayerNames       = xrInstanceLayers.data();
    instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

#if defined(PPX_ANDROID)
    // Android Info
    XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    instanceCreateInfoAndroid.next                           = nullptr;
    instanceCreateInfoAndroid.applicationVM                  = createInfo.androidContext->activity->vm;
    instanceCreateInfoAndroid.applicationActivity            = createInfo.androidContext->activity->clazz;

    instanceCreateInfo.next = &instanceCreateInfoAndroid;
#endif // defined(PPX_ANDROID)

    CharArrayStrCpy(instanceCreateInfo.applicationInfo.applicationName, createInfo.appName.c_str());
    CHECK_XR_CALL(xrCreateInstance(&instanceCreateInfo, &mInstance));
    PPX_ASSERT_MSG(mInstance != XR_NULL_HANDLE, "XrInstance Creation Failed!");

    // Get System Id
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor      = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    CHECK_XR_CALL(xrGetSystem(mInstance, &systemInfo, &mSystemId));

    // Get all supported blend modes.
    uint32_t blendCount = 0;
    CHECK_XR_CALL(xrEnumerateEnvironmentBlendModes(mInstance, mSystemId, mCreateInfo.viewConfigType, 0, &blendCount, nullptr));
    mBlendModes.reserve(blendCount);
    mBlendModes.resize(blendCount, XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM);
    CHECK_XR_CALL(xrEnumerateEnvironmentBlendModes(mInstance, mSystemId, mCreateInfo.viewConfigType, blendCount, &blendCount, mBlendModes.data()));

    // Check to see if passthrough is supported through OpenXR blend modes.
    for (XrEnvironmentBlendMode blendMode : mBlendModes) {
        if (blendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) {
            mPassthroughSupported = XR_PASSTHROUGH_BLEND_MODE;
            break;
        }
    }

    // Create Debug Utils Messenger
    if (mCreateInfo.enableDebug) {
        XrDebugUtilsMessengerCreateInfoEXT debug_info = {XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug_info.messageTypes =
            XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
        debug_info.messageSeverities =
            XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.userCallback = XrDebugUtilsMessengerCallback;

        PFN_xrCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&pfnCreateDebugUtilsMessengerEXT)));
        PPX_ASSERT_MSG(pfnCreateDebugUtilsMessengerEXT != nullptr, "Cannot get xrCreateDebugUtilsMessengerEXT function pointer!");
        CHECK_XR_CALL(pfnCreateDebugUtilsMessengerEXT(mInstance, &debug_info, &mDebugUtilMessenger));
    }
}

void XrComponent::InitializeAfterGrfxDeviceInit(const grfx::InstancePtr pGrfxInstance)
{
    PPX_ASSERT_MSG(!pGrfxInstance.IsNull(), "Invalid Instance!");
    const grfx::Instance& grfxInstance = *pGrfxInstance;
    PPX_ASSERT_MSG(grfxInstance.XrIsGraphicsBindingValid(), "Invalid Graphics Binding!");

    // Create XrSession
    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next                = grfxInstance.XrGetGraphicsBinding();
    sessionInfo.systemId            = mSystemId;
    CHECK_XR_CALL(xrCreateSession(mInstance, &sessionInfo, &mSession));
    PPX_ASSERT_MSG(mSession != XR_NULL_HANDLE, "XrSession creation failed!");

    // Create Reference Space
    XrReferenceSpaceType refSpaceType = XR_REFERENCE_SPACE_TYPE_MAX_ENUM;
    switch (mCreateInfo.refSpaceType) {
        case XrRefSpace::XR_VIEW: refSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW; break;
        case XrRefSpace::XR_LOCAL: refSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; break;
        case XrRefSpace::XR_STAGE: refSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE; break;
        default:
            break;
    }
    PPX_ASSERT_MSG(refSpaceType != XR_REFERENCE_SPACE_TYPE_MAX_ENUM, "Unknown XrReferenceSpaceType!");

    XrReferenceSpaceCreateInfo refSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    refSpaceCreateInfo.poseInReferenceSpace       = kIdentityPose;
    refSpaceCreateInfo.referenceSpaceType         = refSpaceType;
    xrCreateReferenceSpace(mSession, &refSpaceCreateInfo, &mRefSpace);

    refSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    xrCreateReferenceSpace(mSession, &refSpaceCreateInfo, &mUISpace);

    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(mInstance, mSystemId, mCreateInfo.viewConfigType, 0, &viewCount, nullptr);
    mConfigViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(mInstance, mSystemId, mCreateInfo.viewConfigType, viewCount, &viewCount, mConfigViews.data());

    mViews.resize(viewCount, {XR_TYPE_VIEW});

    if (mPassthroughSupported == XR_PASSTHROUGH_OCULUS) {
        XrPassthroughFlagsFB      flags                    = {};
        XrPassthroughCreateInfoFB passthroughInfo          = {XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        passthroughInfo.next                               = nullptr;
        passthroughInfo.flags                              = flags;
        PFN_xrCreatePassthroughFB pfnXrCreatePassthroughFB = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrCreatePassthroughFB", (PFN_xrVoidFunction*)(&pfnXrCreatePassthroughFB)));
        PPX_ASSERT_MSG(pfnXrCreatePassthroughFB != nullptr, "Cannot get xrCreatePassthroughFB function pointer!");
        CHECK_XR_CALL(pfnXrCreatePassthroughFB(mSession, &passthroughInfo, &mPassthrough));
        PPX_ASSERT_MSG(mPassthrough != XR_NULL_HANDLE, "XrPassthroughFB creation failed!");
    }

    InitializeInteractionProfile();
}

XrResult XrComponent::InitializeInteractionProfile()
{
    // Controller support is not essential, so if anything fail, we just return.

    XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    actionSetInfo.priority = 0;
    CharArrayStrCpy(actionSetInfo.actionSetName, "imgui");
    CharArrayStrCpy(actionSetInfo.localizedActionSetName, "ImGui");
    CHECK_XR_CALL_RETURN_ON_FAIL(xrCreateActionSet(mInstance, &actionSetInfo, &mImguiInput));

    XrActionCreateInfo clickActioninfo{XR_TYPE_ACTION_CREATE_INFO};
    clickActioninfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    CharArrayStrCpy(clickActioninfo.actionName, "click");
    CharArrayStrCpy(clickActioninfo.localizedActionName, "Click");
    CHECK_XR_CALL_RETURN_ON_FAIL(xrCreateAction(mImguiInput, &clickActioninfo, &mImguiClickAction));

    XrActionCreateInfo pointingActioninfo{XR_TYPE_ACTION_CREATE_INFO};
    pointingActioninfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    CharArrayStrCpy(pointingActioninfo.actionName, "pointing");
    CharArrayStrCpy(pointingActioninfo.localizedActionName, "Pointing");
    CHECK_XR_CALL_RETURN_ON_FAIL(xrCreateAction(mImguiInput, &pointingActioninfo, &mImguiPointingAction));

    XrPath khrSimpleControllerPath;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrStringToPath(mInstance, "/interaction_profiles/khr/simple_controller", &khrSimpleControllerPath));

    XrPath selectClickPath;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrStringToPath(mInstance, "/user/hand/right/input/select/click", &selectClickPath));
    XrPath pointingPath;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrStringToPath(mInstance, "/user/hand/right/input/aim/pose", &pointingPath));

    XrActionSuggestedBinding bindings[2];
    bindings[0].action  = mImguiClickAction;
    bindings[0].binding = selectClickPath;
    bindings[1].action  = mImguiPointingAction;
    bindings[1].binding = pointingPath;

    XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggestedBindings.interactionProfile     = khrSimpleControllerPath;
    suggestedBindings.suggestedBindings      = bindings;
    suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(std::size(bindings));
    CHECK_XR_CALL_RETURN_ON_FAIL(xrSuggestInteractionProfileBindings(mInstance, &suggestedBindings));

    XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets      = &mImguiInput;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrAttachSessionActionSets(mSession, &attachInfo));

    XrActionSpaceCreateInfo pointingSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    pointingSpaceInfo.action            = mImguiPointingAction;
    pointingSpaceInfo.poseInActionSpace = {{0, 0, 0, 1.0f}, {0, 0, 0}};
    CHECK_XR_CALL_RETURN_ON_FAIL(xrCreateActionSpace(mSession, &pointingSpaceInfo, &mImguiPointingSpace));

    mInteractionProfileInitialized = true;

    return XR_SUCCESS;
}

void XrComponent::Destroy()
{
    if (mImguiPointingAction != XR_NULL_HANDLE) {
        xrDestroyAction(mImguiPointingAction);
    }

    if (mImguiPointingSpace != XR_NULL_HANDLE) {
        xrDestroySpace(mImguiPointingSpace);
    }

    if (mImguiClickAction != XR_NULL_HANDLE) {
        xrDestroyAction(mImguiClickAction);
    }

    if (mImguiInput != XR_NULL_HANDLE) {
        xrDestroyActionSet(mImguiInput);
    }

    if (mPassthroughLayer != XR_NULL_HANDLE) {
        PFN_xrDestroyPassthroughLayerFB pfnXrDestroyPassthroughLayerFB = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrDestroyPassthroughLayerFB", (PFN_xrVoidFunction*)(&pfnXrDestroyPassthroughLayerFB)));
        PPX_ASSERT_MSG(pfnXrDestroyPassthroughLayerFB != nullptr, "Cannot get xrDestroyPassthroughLayerFB function pointer!");
        CHECK_XR_CALL(pfnXrDestroyPassthroughLayerFB(mPassthroughLayer));
    }

    if (mPassthrough != XR_NULL_HANDLE) {
        PFN_xrDestroyPassthroughFB pfnXrDestroyPassthroughFB = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrDestroyPassthroughFB", (PFN_xrVoidFunction*)(&pfnXrDestroyPassthroughFB)));
        PPX_ASSERT_MSG(pfnXrDestroyPassthroughFB != nullptr, "Cannot get xrDestroyPassthroughFB function pointer!");
        CHECK_XR_CALL(pfnXrDestroyPassthroughFB(mPassthrough));
    }

    if (mDebugUtilMessenger != XR_NULL_HANDLE) {
        PFN_xrDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXTPtr = nullptr;
        CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&pfnDestroyDebugUtilsMessengerEXTPtr)));
        PPX_ASSERT_MSG(pfnDestroyDebugUtilsMessengerEXTPtr != nullptr, "Cannot get xrDestroyDebugUtilsMessengerEXT function pointer!");
        CHECK_XR_CALL(pfnDestroyDebugUtilsMessengerEXTPtr(mDebugUtilMessenger));
    }

    if (mRefSpace != XR_NULL_HANDLE) {
        xrDestroySpace(mRefSpace);
    }

    if (mUISpace != XR_NULL_HANDLE) {
        xrDestroySpace(mUISpace);
    }

    if (mSession != XR_NULL_HANDLE) {
        xrDestroySession(mSession);
    }

    if (mInstance != XR_NULL_HANDLE) {
        xrDestroyInstance(mInstance);
    }
}

void XrComponent::BeginPassthrough()
{
    switch (mPassthroughSupported) {
        case XR_PASSTHROUGH_OCULUS: {
            PFN_xrPassthroughStartFB pfnXrPassthroughStartFB = nullptr;
            CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrPassthroughStartFB", (PFN_xrVoidFunction*)(&pfnXrPassthroughStartFB)));
            PPX_ASSERT_MSG(pfnXrPassthroughStartFB != nullptr, "Cannot get xrPassthroughStartFB function pointer!");
            CHECK_XR_CALL(pfnXrPassthroughStartFB(mPassthrough));

            if (!mPassthroughLayer) {
                XrPassthroughFlagsFB           layerFlags                    = {};
                XrPassthroughLayerCreateInfoFB passthroughLayerInfo          = {XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
                passthroughLayerInfo.next                                    = nullptr;
                passthroughLayerInfo.passthrough                             = mPassthrough;
                passthroughLayerInfo.flags                                   = layerFlags;
                passthroughLayerInfo.purpose                                 = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
                PFN_xrCreatePassthroughLayerFB pfnXrCreatePassthroughLayerFB = nullptr;
                CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrCreatePassthroughLayerFB", (PFN_xrVoidFunction*)(&pfnXrCreatePassthroughLayerFB)));
                PPX_ASSERT_MSG(pfnXrCreatePassthroughLayerFB != nullptr, "Cannot get xrCreatePassthroughLayerFB function pointer!");
                CHECK_XR_CALL(pfnXrCreatePassthroughLayerFB(mSession, &passthroughLayerInfo, &mPassthroughLayer));
                PPX_ASSERT_MSG(mPassthroughLayer != XR_NULL_HANDLE, "XrPassthroughLayerFB creation failed!");
            }

            PFN_xrPassthroughLayerResumeFB pfnXrPassthroughLayerResumeFB = nullptr;
            CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrPassthroughLayerResumeFB", (PFN_xrVoidFunction*)(&pfnXrPassthroughLayerResumeFB)));
            PPX_ASSERT_MSG(pfnXrPassthroughLayerResumeFB != nullptr, "Cannot get xrPassthroughLayerResumeFB function pointer!");
            CHECK_XR_CALL(pfnXrPassthroughLayerResumeFB(mPassthroughLayer));
            mPassthroughEnabled = true;
            return;
        }
        case XR_PASSTHROUGH_BLEND_MODE: {
            mPassthroughEnabled = true;
            return;
        }
        case XR_PASSTHROUGH_NONE:
        default:
            return;
    }
}

void XrComponent::EndPassthrough()
{
    switch (mPassthroughSupported) {
        case XR_PASSTHROUGH_OCULUS: {
            PFN_xrPassthroughLayerPauseFB pfnXrPassthroughLayerPauseFB = nullptr;
            CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrPassthroughLayerPauseFB", (PFN_xrVoidFunction*)(&pfnXrPassthroughLayerPauseFB)));
            PPX_ASSERT_MSG(pfnXrPassthroughLayerPauseFB != nullptr, "Cannot get xrPassthroughLayerPauseFB function pointer!");
            CHECK_XR_CALL(pfnXrPassthroughLayerPauseFB(mPassthroughLayer));

            PFN_xrPassthroughPauseFB pfnXrPassthroughPauseFB = nullptr;
            CHECK_XR_CALL(xrGetInstanceProcAddr(mInstance, "xrPassthroughPauseFB", (PFN_xrVoidFunction*)(&pfnXrPassthroughPauseFB)));
            PPX_ASSERT_MSG(pfnXrPassthroughPauseFB != nullptr, "Cannot get xrPassthroughPauseFB function pointer!");
            CHECK_XR_CALL(pfnXrPassthroughPauseFB(mPassthrough));
            mPassthroughEnabled = false;
            return;
        }
        case XR_PASSTHROUGH_BLEND_MODE: {
            mPassthroughEnabled = false;
            return;
        }
        case XR_PASSTHROUGH_NONE:
        default:
            return;
    }
}

void XrComponent::TogglePassthrough()
{
    if (mPassthroughSupported == XR_PASSTHROUGH_NONE) {
        return;
    }
    if (!mPassthroughEnabled) {
        BeginPassthrough();
        return;
    }
    EndPassthrough();
}

// Return event if one is available, otherwise return null.
const XrEventDataBaseHeader* XrComponent::TryReadNextEvent()
{
    // It is sufficient to clear the just the XrEventDataBuffer header to
    // XR_TYPE_EVENT_DATA_BUFFER
    XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&mEventDataBuffer);
    *baseHeader                       = {XR_TYPE_EVENT_DATA_BUFFER};
    const XrResult xr                 = xrPollEvent(mInstance, &mEventDataBuffer);
    if (xr == XR_SUCCESS) {
        return baseHeader;
    }
    if (xr == XR_EVENT_UNAVAILABLE) {
        return nullptr;
    }
    PPX_ASSERT_MSG(false, "Unknown event!");
    return nullptr;
}

void XrComponent::PollEvents(bool& exitRenderLoop)
{
    exitRenderLoop = false;
    // Process all pending messages.
    while (const XrEventDataBaseHeader* event = TryReadNextEvent()) {
        switch (event->type) {
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                exitRenderLoop = true;
                return;
            }
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
                HandleSessionStateChangedEvent(sessionStateChangedEvent, exitRenderLoop);
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            default:
                break;
        }
    }
    if (mInteractionProfileInitialized) {
        PollActions();
    }
}

XrResult XrComponent::PollActions()
{
    if (mSessionState != XR_SESSION_STATE_FOCUSED) {
        return XR_SESSION_NOT_FOCUSED;
    }
    XrActiveActionSet activeActionSet{mImguiInput, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets      = &activeActionSet;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrSyncActions(mSession, &syncInfo));

    XrActionStateBoolean clickActionState    = {XR_TYPE_ACTION_STATE_BOOLEAN};
    XrActionStatePose    pointingActionState = {XR_TYPE_ACTION_STATE_POSE};

    XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = mImguiClickAction;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrGetActionStateBoolean(mSession, &getInfo, &clickActionState));
    getInfo.action = mImguiPointingAction;
    CHECK_XR_CALL_RETURN_ON_FAIL(xrGetActionStatePose(mSession, &getInfo, &pointingActionState));

    if (clickActionState.isActive) {
        mImguiClickState = clickActionState.currentState;
    }
    if (pointingActionState.isActive) {
        XrSpaceLocation pointingLocation{XR_TYPE_SPACE_LOCATION};
        CHECK_XR_CALL_RETURN_ON_FAIL(xrLocateSpace(mImguiPointingSpace, mUISpace, mImguiActionTime, &pointingLocation));
        mImguiPointingState = pointingLocation.pose;
    }
    return XR_SUCCESS;
}

std::optional<XrVector2f> XrComponent::GetUICursor() const
{
    if (!mImguiPointingState.has_value()) {
        return std::nullopt;
    }
    return ProjectCursor(mImguiPointingState.value(), kUIZPlane);
}

void XrComponent::HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool& exitRenderLoop)
{
    const XrSessionState oldState = mSessionState;
    mSessionState                 = stateChangedEvent.state;

    if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != mSession)) {
        PPX_ASSERT_MSG(false, "XrEventDataSessionStateChanged for unknown session");
        return;
    }

    switch (mSessionState) {
        case XR_SESSION_STATE_READY: {
            PPX_ASSERT_MSG(mSession != XR_NULL_HANDLE, "Session is not created!");
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = mCreateInfo.viewConfigType;
            CHECK_XR_CALL(xrBeginSession(mSession, &sessionBeginInfo));
            mIsSessionRunning = true;
            break;
        }
        case XR_SESSION_STATE_STOPPING: {
            PPX_ASSERT_MSG(mSession != XR_NULL_HANDLE, "Session is not created!");
            mIsSessionRunning = false;
            CHECK_XR_CALL(xrEndSession(mSession))
            break;
        }
        case XR_SESSION_STATE_EXITING: {
            exitRenderLoop = true;
            break;
        }
        case XR_SESSION_STATE_LOSS_PENDING: {
            exitRenderLoop = true;
            break;
        }
        default:
            break;
    }
}

void XrComponent::BeginFrame()
{
    XrFrameWaitInfo frameWaitInfo = {XR_TYPE_FRAME_WAIT_INFO};

    CHECK_XR_CALL(xrWaitFrame(mSession, &frameWaitInfo, &mFrameState));
    mShouldRender = mFrameState.shouldRender;

    mImguiActionTime = mFrameState.predictedDisplayTime;

    // Reset near and far plane values for this frame.
    mNearPlaneForFrame = std::nullopt;
    mFarPlaneForFrame  = std::nullopt;

    // Create projection matrices and view matrices for each eye.

    XrViewLocateInfo viewLocateInfo = {
        XR_TYPE_VIEW_LOCATE_INFO,                  // type
        nullptr,                                   // next
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, // viewConfigurationType
        mFrameState.predictedDisplayTime,          // displayTime
        mRefSpace,                                 // space
    };

    XrViewState viewState = {XR_TYPE_VIEW_STATE};

    uint32_t viewCount = 0;
    CHECK_XR_CALL(xrLocateViews(mSession, &viewLocateInfo, &viewState, (uint32_t)mViews.size(), &viewCount, mViews.data()));

    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
        mShouldRender = false;
    }

    // Begin frame.
    XrFrameBeginInfo frameBeginInfo = {XR_TYPE_FRAME_BEGIN_INFO};

    XrResult result = xrBeginFrame(mSession, &frameBeginInfo);
    if (result != XR_SUCCESS) {
        switch (result) {
            case XR_SESSION_LOSS_PENDING:
            case XR_FRAME_DISCARDED:
                mShouldRender = false;
                break;

            default:
                PPX_ASSERT_MSG(false, "xrBeginFrame failed!");
                break;
        }
    }
}

void XrComponent::EndFrame(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t layerProjStartIndex, uint32_t layerQuadStartIndex)
{
    size_t viewCount = mViews.size();
    PPX_ASSERT_MSG(swapchains.size() >= viewCount, "Number of swapchains needs to be larger than or equal to the number of views!");

    // Priority queue to order XrLayerBases according to their zIndex.
    XrLayerBaseQueue layerQueue;

    XrProjectionLayer projectionLayer;
    XrQuadLayer       imGuiLayer;
    // Used when mPassthroughSupported == XR_PASSTHROUGH_OCULUS
    XrPassthroughFbLayer passthroughFbLayer;

    // Set the order the default layers should be rendered in.
    // Higher numbers are rendered later, and space is purposefully provided
    // between each layer to allow for rendering additional content between the three default layers, such as a quad above the main projection, but below ImGui.
    passthroughFbLayer.SetZIndex(100);
    projectionLayer.SetZIndex(200);
    imGuiLayer.SetZIndex(300);

    // Populate data into the layers, and add them into the queue as needed.
    ConditionallyPopulatePassthroughFbLayer(layerQueue, passthroughFbLayer);
    ConditionallyPopulateProjectionLayer(swapchains, layerProjStartIndex, layerQueue, projectionLayer);
    ConditionallyPopulateImGuiLayer(swapchains, layerQuadStartIndex, layerQueue, imGuiLayer);

    // Add any additional owned layers to the queue.
    for (const auto& [ref, layer] : mLayers) {
        layerQueue.push(layer.get());
    }

    // Create an ordered vector of layers cast to the base struct.
    std::vector<XrCompositionLayerBaseHeader*> layers;
    for (; !layerQueue.empty(); layerQueue.pop()) {
        layers.push_back(layerQueue.top()->basePtr());
    }

    // Get the blend mode.
    XrEnvironmentBlendMode blendMode = mBlendModes[0];
    if (mShouldRender && mPassthroughEnabled && (mPassthroughSupported == XR_PASSTHROUGH_BLEND_MODE)) {
        blendMode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
    }

    // Submit layers and end frame.
    XrFrameEndInfo frameEndInfo = {
        XR_TYPE_FRAME_END_INFO,               // type
        nullptr,                              // next
        mFrameState.predictedDisplayTime,     // displayTime
        blendMode,                            // environmentBlendMode
        static_cast<uint32_t>(layers.size()), // layerCount
        layers.data(),                        // layers
    };

    CHECK_XR_CALL(xrEndFrame(mSession, &frameEndInfo));
}

void XrComponent::ConditionallyPopulateProjectionLayer(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t startIndex, XrLayerBaseQueue& layerQueue, XrProjectionLayer& projectionLayer)
{
    const size_t viewCount = mViews.size();
    PPX_ASSERT_MSG(swapchains.size() >= (viewCount + startIndex), "Number of swapchains needs to be larger than or equal to the number of views!");

    if (!mShouldRender) {
        return;
    }

    // Projection and (optional) depth info layer from color+depth swapchains.
    for (size_t i = 0; i < viewCount; ++i) {
        XrCompositionLayerProjectionView view = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        view.pose                             = mViews[i].pose;
        view.fov                              = mViews[i].fov;
        view.subImage.swapchain               = swapchains[startIndex + i]->GetXrColorSwapchain();
        view.subImage.imageRect.offset        = {0, 0};
        view.subImage.imageRect.extent        = {static_cast<int>(GetWidth()), static_cast<int>(GetHeight())};

        if (mShouldSubmitDepthInfo && (swapchains[startIndex + i]->GetXrDepthSwapchain() != XR_NULL_HANDLE)) {
            PPX_ASSERT_MSG(mNearPlaneForFrame.has_value() && mFarPlaneForFrame.has_value(), "Depth info layer cannot be submitted because near and far plane values are not set. "
                                                                                            "Call GetProjectionMatrixForCurrentViewAndSetFrustumPlanes to set per-frame values.");
            XrCompositionLayerDepthInfoKHR depthInfo = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
            depthInfo.minDepth                       = 0.0f;
            depthInfo.maxDepth                       = 1.0f;
            depthInfo.nearZ                          = *mNearPlaneForFrame;
            depthInfo.farZ                           = *mFarPlaneForFrame;
            depthInfo.subImage.swapchain             = swapchains[startIndex + i]->GetXrDepthSwapchain();
            depthInfo.subImage.imageRect.offset      = {0, 0};
            depthInfo.subImage.imageRect.extent      = {static_cast<int>(GetWidth()), static_cast<int>(GetHeight())};

            projectionLayer.AddView(view, depthInfo);
        }
        else {
            projectionLayer.AddView(view);
        }
    }

    projectionLayer.layer().type       = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    projectionLayer.layer().layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    projectionLayer.layer().space      = mRefSpace;

    layerQueue.push(&projectionLayer);
}

void XrComponent::ConditionallyPopulateImGuiLayer(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t index, XrLayerBaseQueue& layerQueue, XrQuadLayer& quadLayer)
{
    if (!mShouldRender) {
        return;
    }
    if (!mCreateInfo.enableQuadLayer) {
        return;
    }

    quadLayer.layer().type                      = XR_TYPE_COMPOSITION_LAYER_QUAD;
    quadLayer.layer().layerFlags                = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    quadLayer.layer().space                     = mUISpace;
    quadLayer.layer().eyeVisibility             = XR_EYE_VISIBILITY_BOTH;
    quadLayer.layer().subImage.swapchain        = swapchains[index]->GetXrColorSwapchain();
    quadLayer.layer().subImage.imageRect.offset = {0, 0};
    quadLayer.layer().subImage.imageRect.extent = {static_cast<int>(GetUIWidth()), static_cast<int>(GetUIHeight())};
    quadLayer.layer().pose                      = {{0, 0, 0, 1}, {0, 0, kUIZPlane}};
    quadLayer.layer().size                      = {1, 1};

    layerQueue.push(&quadLayer);
}

void XrComponent::ConditionallyPopulatePassthroughFbLayer(XrLayerBaseQueue& layerQueue, XrPassthroughFbLayer& passthroughFbLayer)
{
    if (!mShouldRender) {
        return;
    }
    if (!mPassthroughEnabled) {
        return;
    }
    if (mPassthroughSupported != XR_PASSTHROUGH_OCULUS) {
        return;
    }
    if (mPassthroughLayer == XR_NULL_HANDLE) {
        return;
    }

    passthroughFbLayer.layer().type        = XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB;
    passthroughFbLayer.layer().next        = nullptr;
    passthroughFbLayer.layer().flags       = XrCompositionLayerFlags{};
    passthroughFbLayer.layer().space       = XR_NULL_HANDLE;
    passthroughFbLayer.layer().layerHandle = mPassthroughLayer;

    layerQueue.push(&passthroughFbLayer);
}

LayerRef XrComponent::AddLayer(std::unique_ptr<XrLayerBase> layer)
{
    const LayerRef ref = mNextLayerRef++;
    mLayers.insert(std::make_pair(ref, std::move(layer)));
    return ref;
}

bool XrComponent::RemoveLayer(LayerRef layerRef)
{
    return mLayers.erase(layerRef);
}

glm::mat4 XrComponent::GetViewMatrixForCurrentView() const
{
    PPX_ASSERT_MSG((mCurrentViewIndex < mViews.size()), "Invalid view index!");
    const XrView&  view = mViews[mCurrentViewIndex];
    const XrPosef& pose = view.pose;
    // OpenXR is using right handed system which is the same as Vulkan
    glm::quat quat         = glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
    glm::mat4 rotation     = glm::mat4_cast(quat);
    glm::vec3 position     = glm::vec3(pose.position.x, pose.position.y, pose.position.z);
    glm::mat4 translation  = glm::translate(glm::mat4(1.f), position);
    glm::mat4 view_glm     = translation * rotation;
    glm::mat4 view_glm_inv = glm::inverse(view_glm);
    return view_glm_inv;
}

glm::mat4 XrComponent::GetProjectionMatrixForCurrentViewAndSetFrustumPlanes(float nearZ, float farZ)
{
    PPX_ASSERT_MSG((mCurrentViewIndex < mViews.size()), "Invalid view index!");
    const XrView& view = mViews[mCurrentViewIndex];
    const XrFovf& fov  = view.fov;

    // Save near and far plane values so that they can be referenced
    // in EndFrame(), as part of the depth layer info submission.
    // They can only be set once per frame.
    PPX_ASSERT_MSG(!mNearPlaneForFrame.has_value() || *mNearPlaneForFrame == nearZ, "GetProjectionMatrixForCurrentViewAndSetFrustumPlanes was already called this frame with a different nearZ value.");
    PPX_ASSERT_MSG(!mFarPlaneForFrame.has_value() || *mFarPlaneForFrame == farZ, "GetProjectionMatrixForCurrentViewAndSetFrustumPlanes was already called this frame with a different farZ value.");
    mNearPlaneForFrame = nearZ;
    mFarPlaneForFrame  = farZ;

    const float tan_left  = tanf(fov.angleLeft);
    const float tan_right = tanf(fov.angleRight);

    const float tan_down = tanf(fov.angleDown);
    const float tan_up   = tanf(fov.angleUp);

    const float tan_width  = tan_right - tan_left;
    const float tan_height = (tan_up - tan_down);

    const float a00 = 2 / tan_width;
    const float a11 = 2 / tan_height;

    const float a20 = (tan_right + tan_left) / tan_width;
    const float a21 = (tan_up + tan_down) / tan_height;
    const float a22 = -farZ / (farZ - nearZ);

    const float a32 = -(farZ * nearZ) / (farZ - nearZ);

    // clang-format off
    const float mat[16] = {
        a00,    0,      0,      0,
        0,      a11,    0,      0,
        a20,    a21,    a22,    -1,
        0,      0,      a32,    0,
    };
    // clang-format on

    return glm::make_mat4(mat);
}

XrPosef XrComponent::GetPoseForCurrentView() const
{
    PPX_ASSERT_MSG((mCurrentViewIndex < mViews.size()), "Invalid view index!");
    return mViews[mCurrentViewIndex].pose;
}

} // namespace ppx
#endif
