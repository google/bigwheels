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

#if defined(PPX_D3D11)
#include <d3d11_4.h>
#define XR_USE_GRAPHICS_API_D3D11
#endif // defined(PPX_D3D11)

#if defined(PPX_D3D12)
#include <d3d12.h>
#define XR_USE_GRAPHICS_API_D3D12
#endif // defined(PPX_D3D12)

#if defined(PPX_VULKAN)
#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN
#endif // defined(PPX_VULKAN)

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <ppx/grfx/grfx_config.h>

#define CHECK_XR_CALL(cmd) \
    PPX_ASSERT_MSG(cmd == XR_SUCCESS, "XR call failed!");

namespace ppx {

enum struct XrRefSpace
{
    XR_VIEW,
    XR_LOCAL,
    XR_STAGE,
};

//! @class XrSettings
//!
//!
struct XrComponentCreateInfo
{
    grfx::Api               api             = grfx::API_UNDEFINED; // Direct3D or Vulkan.
    std::string             appName         = "";
    grfx::Format            colorFormat     = grfx::FORMAT_B8G8R8A8_SRGB;
    grfx::Format            depthFormat     = grfx::FORMAT_D32_FLOAT;
    XrRefSpace              refSpaceType    = XrRefSpace::XR_STAGE;
    XrViewConfigurationType viewConfigType  = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrVector3f              quadLayerPos    = {0, 0, 0};
    XrExtent2Df             quadLayerSize   = {1.f, 1.f};
    bool                    enableDebug     = false;
    bool                    enableQuadLayer = false;
};

//! @class XrComponent
class XrComponent
{
public:
    void InitializeBeforeGrfxDeviceInit(const XrComponentCreateInfo& createInfo);
    void InitializeAfterGrfxDeviceInit(const grfx::InstancePtr pGrfxInstance);
    void Destroy();

    void PollEvents(bool& exitRenderLoop);

    void BeginFrame(const std::vector<grfx::SwapchainPtr>& swapchains, uint32_t layerProjStartIndex, uint32_t layerQuadStartIndex);
    void EndFrame();

    grfx::Format GetColorFormat() const { return mCreateInfo.colorFormat; }
    grfx::Format GetDepthFormat() const { return mCreateInfo.depthFormat; }

    // This is a hack that assumes both views have the same width/height/sample count
    uint32_t GetWidth() const
    {
        if (mConfigViews.empty())
            return 0;
        return mConfigViews[0].recommendedImageRectWidth;
    }
    uint32_t GetHeight() const
    {
        if (mConfigViews.empty())
            return 0;
        return mConfigViews[0].recommendedImageRectHeight;
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
    glm::mat4  GetViewMatrixForCurrentView() const;
    glm::mat4  GetProjectionMatrixForCurrentView(float nearZ, float farZ) const;
    XrPosef    GetCurrentPose() const;

    bool IsSessionRunning() const { return mIsSessionRunning; }
    bool ShouldRender() const { return mShouldRender; }

private:
    const XrEventDataBaseHeader* TryReadNextEvent();
    void                         HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool& exitRenderLoop);

    XrInstance mInstance = XR_NULL_HANDLE;
    XrSystemId mSystemId = XR_NULL_SYSTEM_ID;
    XrSession  mSession  = XR_NULL_HANDLE;

    std::vector<XrViewConfigurationView>          mConfigViews;
    std::vector<XrCompositionLayerProjectionView> mCompositionLayerProjectionViews;
    std::vector<XrView>                           mViews;

    XrSpace                  mRefSpace           = XR_NULL_HANDLE;
    XrSpace                  mUISpace            = XR_NULL_HANDLE;
    XrSessionState           mSessionState       = XR_SESSION_STATE_UNKNOWN;
    XrEnvironmentBlendMode   mBlend              = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;
    XrDebugUtilsMessengerEXT mDebugUtilMessenger = XR_NULL_HANDLE;
    bool                     mIsSessionRunning   = false;
    bool                     mShouldRender       = false;

    XrFrameState mFrameState = {
        .type = XR_TYPE_FRAME_STATE,
    };

    XrEventDataBuffer            mEventDataBuffer;
    XrCompositionLayerProjection mCompositionLayerProjection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    XrCompositionLayerQuad       mCompositionLayerQuad       = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    uint32_t                     mCurrentViewIndex           = 0;

    XrComponentCreateInfo mCreateInfo = {};
};

} // namespace ppx

#endif // PPX_BUILD_XR
#endif // PPX_XR_COMPONENT_H
