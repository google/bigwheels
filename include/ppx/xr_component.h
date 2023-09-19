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

#ifndef PPX_XR_COMPONENT_H
#define PPX_XR_COMPONENT_H

#if defined(PPX_BUILD_XR)

#if defined(PPX_D3D12)
#include <d3d12.h>
#define XR_USE_GRAPHICS_API_D3D12
#endif // defined(PPX_D3D12)

#if defined(PPX_VULKAN)
#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN
#endif // defined(PPX_VULKAN)

#if defined(PPX_ANDROID)
#include <android_native_app_glue.h>
#endif // defined(PPX_ANDROID)

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <ppx/grfx/grfx_config.h>

#include <optional>
#include <queue>
#include <unordered_map>

#include "ppx/xr_composition_layers.h"

#define CHECK_XR_CALL(CMD__)                                                                       \
    {                                                                                              \
        auto RESULT__ = CMD__;                                                                     \
        PPX_ASSERT_MSG(RESULT__ == XR_SUCCESS, "XR call failed with result: " << RESULT__ << "!"); \
    }

namespace ppx {
namespace {

// Strict weak ordering by XrLayerBase zIndex.
struct OrderXrLayers
{
    bool operator()(const XrLayerBase* left, const XrLayerBase* right) const
    {
        return left->zIndex() > right->zIndex();
    }
};

// Type alias for a priority queue that uses the OrderXrLayers compare type.
using XrLayerBaseQueue = std::priority_queue<XrLayerBase*, std::vector<XrLayerBase*>, OrderXrLayers>;

} // namespace

enum struct XrRefSpace
{
    XR_VIEW,
    XR_LOCAL,
    XR_STAGE,
};

enum XrPassthroughSupport
{
    XR_PASSTHROUGH_NONE,
    XR_PASSTHROUGH_BLEND_MODE,
    XR_PASSTHROUGH_OCULUS,
};

struct XrComponentResolution
{
    uint32_t width  = 0;
    uint32_t height = 0;
};

//! @class XrSettings
//!
//!
struct XrComponentCreateInfo
{
    grfx::Api   api     = grfx::API_UNDEFINED; // Direct3D or Vulkan.
    std::string appName = "";
#if defined(PPX_ANDROID)
    android_app* androidContext = nullptr;
    grfx::Format colorFormat    = grfx::FORMAT_R8G8B8A8_SRGB;
#else
    grfx::Format colorFormat = grfx::FORMAT_B8G8R8A8_SRGB;
#endif
    grfx::Format            depthFormat          = grfx::FORMAT_D32_FLOAT;
    XrRefSpace              refSpaceType         = XrRefSpace::XR_STAGE;
    XrViewConfigurationType viewConfigType       = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    bool                    enableDebug          = false;
    bool                    enableQuadLayer      = false;
    bool                    enableDepthSwapchain = false;
    XrComponentResolution   resolution           = {0, 0};
    XrComponentResolution   uiResolution         = {0, 0};
};

// Used to reference OpenXR layers added to XrComponent through XrComponent::AddLayer.
using LayerRef = uint32_t;

//! @class XrComponent
class XrComponent
{
public:
    virtual ~XrComponent() = default;

    void InitializeBeforeGrfxDeviceInit(const XrComponentCreateInfo& createInfo);
    void InitializeAfterGrfxDeviceInit(const grfx::InstancePtr pGrfxInstance);
    void Destroy();

    void PollEvents(bool& exitRenderLoop);

    void BeginFrame();
    void EndFrame(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t layerProjStartIndex, uint32_t layerQuadStartIndex);

    grfx::Format GetColorFormat() const { return mCreateInfo.colorFormat; }
    grfx::Format GetDepthFormat() const { return mCreateInfo.depthFormat; }
    bool         UsesDepthSwapchains() const { return mCreateInfo.enableDepthSwapchain; }

    // This is a hack that assumes both views have the same width/height/sample count
    uint32_t GetWidth() const
    {
        if (mConfigViews.empty())
            return 0;
        if (mCreateInfo.resolution.width > 0)
            return mCreateInfo.resolution.width;
        return mConfigViews[0].recommendedImageRectWidth;
    }
    uint32_t GetHeight() const
    {
        if (mConfigViews.empty())
            return 0;
        if (mCreateInfo.resolution.height > 0)
            return mCreateInfo.resolution.height;
        return mConfigViews[0].recommendedImageRectHeight;
    }
    uint32_t GetUIWidth() const
    {
        return (mCreateInfo.uiResolution.width > 0) ? mCreateInfo.uiResolution.width : GetWidth();
    }
    uint32_t GetUIHeight() const
    {
        return (mCreateInfo.uiResolution.height > 0) ? mCreateInfo.uiResolution.height : GetHeight();
    }
    uint32_t GetSampleCount() const
    {
        if (mConfigViews.empty())
            return 0;
        return mConfigViews[0].recommendedSwapchainSampleCount;
    }
    size_t     GetViewCount() const { return mViews.size(); }
    XrInstance GetInstance() const { return mInstance; }
    XrSystemId GetSystemId() const { return mSystemId; }
    XrSession  GetSession() const { return mSession; }
    void       SetCurrentViewIndex(uint32_t index) { mCurrentViewIndex = index; }
    uint32_t   GetCurrentViewIndex() const { return mCurrentViewIndex; }

    // Computes the projection matrix for the current view given the near and
    // far frustum planes. The values for the frustum planes will be sent to
    // the OpenXR runtime as part of the frame depth info submission, and the
    // caller must ensure that the values do not change within a frame.
    glm::mat4 GetProjectionMatrixForCurrentViewAndSetFrustumPlanes(float nearZ, float farZ);
    glm::mat4 GetViewMatrixForCurrentView() const;
    XrPosef   GetPoseForCurrentView() const;

    bool IsSessionRunning() const { return mIsSessionRunning; }
    bool ShouldRender() const { return mShouldRender; }

    virtual void BeginPassthrough();
    virtual void EndPassthrough();
    virtual void TogglePassthrough();

    // Adds an OpenXR Layer to the layers used to render OpenXR frames. The XrComponent
    // assumes ownership over the given layer, and returns a reference that can be used
    // to remove the layer from future frames as needed.
    LayerRef AddLayer(std::unique_ptr<XrLayerBase> layer);

    // Removes a XrLayerBase from being rendered in future frames. Removing a layer will
    // cause the XrComponent to deinitialize the referenced layer. Returns true if the
    // requested layer was successfully removed from the owned layers, and false otherwise.
    bool RemoveLayer(LayerRef layerRef);

private:
    const XrEventDataBaseHeader* TryReadNextEvent();
    void                         HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool& exitRenderLoop);

    // Methods that populate the OpenXR composition layers with information when they are needed for rendering.
    // Used by XrComponent::EndFrame to support the base application composition layers.
    void ConditionallyPopulateProjectionLayer(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t startIndex, XrLayerBaseQueue& layerQueue, XrProjectionLayer& projectionLayer);
    void ConditionallyPopulateImGuiLayer(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t index, XrLayerBaseQueue& layerQueue, XrQuadLayer& quadLayer);
    void ConditionallyPopulatePassthroughFbLayer(XrLayerBaseQueue& layerQueue, XrPassthroughFbLayer& passthroughFbLayer);

    XrInstance mInstance = XR_NULL_HANDLE;
    XrSystemId mSystemId = XR_NULL_SYSTEM_ID;
    XrSession  mSession  = XR_NULL_HANDLE;

    XrPassthroughSupport mPassthroughSupported = XR_PASSTHROUGH_NONE;
    bool                 mPassthroughEnabled   = false;

    std::vector<XrViewConfigurationView> mConfigViews;
    std::vector<XrView>                  mViews;
    std::vector<XrEnvironmentBlendMode>  mBlendModes;
    uint32_t                             mCurrentViewIndex = 0;

    std::unordered_map<LayerRef, std::unique_ptr<XrLayerBase>> mLayers;
    LayerRef                                                   mNextLayerRef = 0;

    XrSpace                  mRefSpace           = XR_NULL_HANDLE;
    XrSpace                  mUISpace            = XR_NULL_HANDLE;
    XrSessionState           mSessionState       = XR_SESSION_STATE_UNKNOWN;
    XrEnvironmentBlendMode   mBlend              = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;
    XrDebugUtilsMessengerEXT mDebugUtilMessenger = XR_NULL_HANDLE;
    bool                     mIsSessionRunning   = false;
    bool                     mShouldRender       = false;

    std::optional<float> mNearPlaneForFrame     = std::nullopt;
    std::optional<float> mFarPlaneForFrame      = std::nullopt;
    bool                 mShouldSubmitDepthInfo = false;

    XrFrameState mFrameState = {XR_TYPE_FRAME_STATE};

    XrEventDataBuffer mEventDataBuffer;

    XrComponentCreateInfo mCreateInfo = {};

    // Oculus only
    XrPassthroughFB      mPassthrough      = XR_NULL_HANDLE;
    XrPassthroughLayerFB mPassthroughLayer = XR_NULL_HANDLE;
};

} // namespace ppx

#endif // PPX_BUILD_XR
#endif // PPX_XR_COMPONENT_H
