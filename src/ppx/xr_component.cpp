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
#include "ppx/xr_component.h"
#include "ppx/grfx/grfx_instance.h"
#include <glm/gtc/type_ptr.hpp>

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

#if defined(PPX_D3D11)
        case grfx::API_DX_11_0:
        case grfx::API_DX_11_1:
            graphicsAPIExtension = XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
            break;
#endif // defiend(PPX_D3D11)

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
    if (mCreateInfo.enableDebug) {
        xrInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    // Verify extensions
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    std::vector<XrExtensionProperties> xrExts(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, xrExts.data());
    for (const auto& ext : xrInstanceExtensions) {
        auto it = std::find_if(xrExts.begin(), xrExts.end(), [&](const XrExtensionProperties& e) { return strcmp(e.extensionName, ext) == 0; });
        PPX_ASSERT_MSG((it != xrExts.end()), "The extension is not supported! Also make sure your OpenXR runtime is loaded properly!");
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
    strncpy(instanceCreateInfo.applicationInfo.applicationName, createInfo.appName.c_str(), sizeof(instanceCreateInfo.applicationInfo.applicationName));
    CHECK_XR_CALL(xrCreateInstance(&instanceCreateInfo, &mInstance));
    PPX_ASSERT_MSG(mInstance != nullptr, "XrInstance Creation Failed!");

    // Get System Id
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor      = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    CHECK_XR_CALL(xrGetSystem(mInstance, &systemInfo, &mSystemId));

    // Use the first available Blend Mode
    uint32_t blendCount = 0;
    CHECK_XR_CALL(xrEnumerateEnvironmentBlendModes(mInstance, mSystemId, mCreateInfo.viewConfigType, 1, &blendCount, &mBlend));

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
        debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data) {
            printf("%s: %s\n", msg->functionName, msg->message);
            // Returning XR_TRUE here will force the calling function to fail
            return (XrBool32)XR_FALSE;
        };

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

    XrReferenceSpaceCreateInfo refSpaceCreatInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    const XrPosef              poseIdentity      = {{0, 0, 0, 1}, {0, 0, 0}};
    refSpaceCreatInfo.poseInReferenceSpace       = poseIdentity;
    refSpaceCreatInfo.referenceSpaceType         = refSpaceType;
    xrCreateReferenceSpace(mSession, &refSpaceCreatInfo, &mRefSpace);

    refSpaceCreatInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    xrCreateReferenceSpace(mSession, &refSpaceCreatInfo, &mUISpace);

    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(mInstance, mSystemId, mCreateInfo.viewConfigType, 0, &viewCount, nullptr);
    mConfigViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(mInstance, mSystemId, mCreateInfo.viewConfigType, viewCount, &viewCount, mConfigViews.data());

    mViews.resize(viewCount, {XR_TYPE_VIEW});
}

void XrComponent::Destroy()
{
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

void XrComponent::BeginFrame(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t layerProjStartIndex, uint32_t layerQuadStartIndex)
{
    XrFrameWaitInfo frameWaitInfo = {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    CHECK_XR_CALL(xrWaitFrame(mSession, &frameWaitInfo, &mFrameState));
    mShouldRender = mFrameState.shouldRender;

    // --- Create projection matrices and view matrices for each eye
    XrViewLocateInfo viewLocateInfo = {
        .type                  = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        .displayTime           = mFrameState.predictedDisplayTime,
        .space                 = mRefSpace,
    };

    XrViewState viewState = {
        .type = XR_TYPE_VIEW_STATE,
    };
    uint32_t viewCount = 0;
    CHECK_XR_CALL(xrLocateViews(mSession, &viewLocateInfo, &viewState, (uint32_t)mViews.size(), &viewCount, mViews.data()));

    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
        mShouldRender = false;
    }

    mCompositionLayerProjectionViews.resize(viewCount);
    PPX_ASSERT_MSG(swapchains.size() >= viewCount, "Number of swapchains needs to be larger than or equal to the number of views!");
    for (uint32_t i = 0; i < viewCount; ++i) {
        mCompositionLayerProjectionViews[i]                           = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        mCompositionLayerProjectionViews[i].pose                      = mViews[i].pose;
        mCompositionLayerProjectionViews[i].fov                       = mViews[i].fov;
        mCompositionLayerProjectionViews[i].subImage.swapchain        = swapchains[layerProjStartIndex + i]->GetXrSwapchain();
        mCompositionLayerProjectionViews[i].subImage.imageRect.offset = {0, 0};
        mCompositionLayerProjectionViews[i].subImage.imageRect.extent = {static_cast<int>(GetWidth()), static_cast<int>(GetHeight())};
    }

    // UI composition layer
    if (mCreateInfo.enableQuadLayer) {
        mCompositionLayerQuad.layerFlags                = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        mCompositionLayerQuad.space                     = mUISpace;
        mCompositionLayerQuad.eyeVisibility             = XR_EYE_VISIBILITY_BOTH;
        mCompositionLayerQuad.subImage.swapchain        = swapchains[layerQuadStartIndex]->GetXrSwapchain();
        mCompositionLayerQuad.subImage.imageRect.offset = {0, 0};
        mCompositionLayerQuad.subImage.imageRect.extent = {static_cast<int>(GetWidth()), static_cast<int>(GetHeight())};
        mCompositionLayerQuad.pose                      = {{0, 0, 0, 1}, mCreateInfo.quadLayerPos};
        mCompositionLayerQuad.size                      = mCreateInfo.quadLayerSize;
    }

    // Begin frame
    XrFrameBeginInfo frameBeginInfo = {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

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

void XrComponent::EndFrame()
{
    mCompositionLayerProjection.layerFlags = 0;
    mCompositionLayerProjection.space      = mRefSpace;
    mCompositionLayerProjection.viewCount  = static_cast<uint32_t>(GetViewCount());
    mCompositionLayerProjection.views      = mCompositionLayerProjectionViews.data();
    std::vector<XrCompositionLayerBaseHeader*> layers;
    if (mShouldRender) {
        layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&mCompositionLayerProjection));
        if (mCreateInfo.enableQuadLayer) {
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&mCompositionLayerQuad));
        }
    }

    XrFrameEndInfo frameEndInfo = {
        .type                 = XR_TYPE_FRAME_END_INFO,
        .displayTime          = mFrameState.predictedDisplayTime,
        .environmentBlendMode = mBlend,
        .layerCount           = static_cast<uint32_t>(layers.size()),
        .layers               = layers.data(),
    };

    CHECK_XR_CALL(xrEndFrame(mSession, &frameEndInfo));
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

glm::mat4 XrComponent::GetProjectionMatrixForCurrentView(float nearZ, float farZ) const
{
    PPX_ASSERT_MSG((mCurrentViewIndex < mViews.size()), "Invalid view index!");
    const XrView& view = mViews[mCurrentViewIndex];
    const XrFovf& fov  = view.fov;

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

XrPosef XrComponent::GetCurrentPose() const
{
    PPX_ASSERT_MSG((mCurrentViewIndex < mViews.size()), "Invalid view index!");
    return mViews[mCurrentViewIndex].pose;
}

} // namespace ppx
#endif
