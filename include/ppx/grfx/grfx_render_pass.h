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

#ifndef ppx_grfx_render_pass_h
#define ppx_grfx_render_pass_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct RenderPassCreateInfo
//!
//! Use this if the RTVs and/or the DSV exists.
//!
struct RenderPassCreateInfo
{
    uint32_t                     width                                           = 0;
    uint32_t                     height                                          = 0;
    uint32_t                     renderTargetCount                               = 0;
    grfx::RenderTargetView*      pRenderTargetViews[PPX_MAX_RENDER_TARGETS]      = {};
    grfx::DepthStencilView*      pDepthStencilView                               = nullptr;
    grfx::ResourceState          depthStencilState                               = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS] = {};
    grfx::DepthStencilClearValue depthStencilClearValue                          = {};
    grfx::Ownership              ownership                                       = grfx::OWNERSHIP_REFERENCE;

    // If `pShadingRatePattern` is not null, then the pipeline targeting this
    // RenderPass must use the same shading rate mode
    // (`GraphicsPipelineCreateInfo.shadingRateMode`).
    grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

    void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
};

//! @struct RenderPassCreateInfo2
//!
//! Use this version if the format(s) are know but images and
//! views need creation.
//!
//! RTVs, DSV, and backing images will be created using the
//! criteria provided in this struct.
//!
struct RenderPassCreateInfo2
{
    uint32_t                     width                                             = 0;
    uint32_t                     height                                            = 0;
    grfx::SampleCount            sampleCount                                       = grfx::SAMPLE_COUNT_1;
    uint32_t                     renderTargetCount                                 = 0;
    grfx::Format                 renderTargetFormats[PPX_MAX_RENDER_TARGETS]       = {};
    grfx::Format                 depthStencilFormat                                = grfx::FORMAT_UNDEFINED;
    grfx::ImageUsageFlags        renderTargetUsageFlags[PPX_MAX_RENDER_TARGETS]    = {};
    grfx::ImageUsageFlags        depthStencilUsageFlags                            = {};
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS]   = {};
    grfx::DepthStencilClearValue depthStencilClearValue                            = {};
    grfx::AttachmentLoadOp       renderTargetLoadOps[PPX_MAX_RENDER_TARGETS]       = {grfx::ATTACHMENT_LOAD_OP_LOAD};
    grfx::AttachmentStoreOp      renderTargetStoreOps[PPX_MAX_RENDER_TARGETS]      = {grfx::ATTACHMENT_STORE_OP_STORE};
    grfx::AttachmentLoadOp       depthLoadOp                                       = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp      depthStoreOp                                      = grfx::ATTACHMENT_STORE_OP_STORE;
    grfx::AttachmentLoadOp       stencilLoadOp                                     = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp      stencilStoreOp                                    = grfx::ATTACHMENT_STORE_OP_STORE;
    grfx::ResourceState          renderTargetInitialStates[PPX_MAX_RENDER_TARGETS] = {grfx::RESOURCE_STATE_UNDEFINED};
    grfx::ResourceState          depthStencilInitialState                          = grfx::RESOURCE_STATE_UNDEFINED;
    grfx::Ownership              ownership                                         = grfx::OWNERSHIP_REFERENCE;

    // If `pShadingRatePattern` is not null, then the pipeline targeting this
    // RenderPass must use the same shading rate mode
    // (`GraphicsPipelineCreateInfo.shadingRateMode`).
    grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

    void SetAllRenderTargetUsageFlags(const grfx::ImageUsageFlags& flags);
    void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
    void SetAllRenderTargetLoadOp(grfx::AttachmentLoadOp op);
    void SetAllRenderTargetStoreOp(grfx::AttachmentStoreOp op);
    void SetAllRenderTargetToClear();
};

//! @struct RenderPassCreateInfo3
//!
//! Use this if the images exists but views need creation.
//!
struct RenderPassCreateInfo3
{
    uint32_t                     width                                           = 0;
    uint32_t                     height                                          = 0;
    uint32_t                     renderTargetCount                               = 0;
    grfx::Image*                 pRenderTargetImages[PPX_MAX_RENDER_TARGETS]     = {};
    grfx::Image*                 pDepthStencilImage                              = nullptr;
    grfx::ResourceState          depthStencilState                               = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS] = {};
    grfx::DepthStencilClearValue depthStencilClearValue                          = {};
    grfx::AttachmentLoadOp       renderTargetLoadOps[PPX_MAX_RENDER_TARGETS]     = {grfx::ATTACHMENT_LOAD_OP_LOAD};
    grfx::AttachmentStoreOp      renderTargetStoreOps[PPX_MAX_RENDER_TARGETS]    = {grfx::ATTACHMENT_STORE_OP_STORE};
    grfx::AttachmentLoadOp       depthLoadOp                                     = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp      depthStoreOp                                    = grfx::ATTACHMENT_STORE_OP_STORE;
    grfx::AttachmentLoadOp       stencilLoadOp                                   = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp      stencilStoreOp                                  = grfx::ATTACHMENT_STORE_OP_STORE;
    grfx::Ownership              ownership                                       = grfx::OWNERSHIP_REFERENCE;

    // If `pShadingRatePattern` is not null, then the pipeline targeting this
    // RenderPass must use the same shading rate mode
    // (`GraphicsPipelineCreateInfo.shadingRateMode`).
    grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

    void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
    void SetAllRenderTargetLoadOp(grfx::AttachmentLoadOp op);
    void SetAllRenderTargetStoreOp(grfx::AttachmentStoreOp op);
    void SetAllRenderTargetToClear();
};

namespace internal {

struct RenderPassCreateInfo
{
    enum CreateInfoVersion
    {
        CREATE_INFO_VERSION_UNDEFINED = 0,
        CREATE_INFO_VERSION_1         = 1,
        CREATE_INFO_VERSION_2         = 2,
        CREATE_INFO_VERSION_3         = 3,
    };

    grfx::Ownership           ownership           = grfx::OWNERSHIP_REFERENCE;
    CreateInfoVersion         version             = CREATE_INFO_VERSION_UNDEFINED;
    uint32_t                  width               = 0;
    uint32_t                  height              = 0;
    uint32_t                  renderTargetCount   = 0;
    grfx::ResourceState       depthStencilState   = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::ShadingRatePattern* pShadingRatePattern = nullptr;

    // Data unique to grfx::RenderPassCreateInfo
    struct
    {
        grfx::RenderTargetView* pRenderTargetViews[PPX_MAX_RENDER_TARGETS] = {};
        grfx::DepthStencilView* pDepthStencilView                          = nullptr;
    } V1;

    // Data unique to grfx::RenderPassCreateInfo2
    struct
    {
        grfx::SampleCount     sampleCount                                       = grfx::SAMPLE_COUNT_1;
        grfx::Format          renderTargetFormats[PPX_MAX_RENDER_TARGETS]       = {};
        grfx::Format          depthStencilFormat                                = grfx::FORMAT_UNDEFINED;
        grfx::ImageUsageFlags renderTargetUsageFlags[PPX_MAX_RENDER_TARGETS]    = {};
        grfx::ImageUsageFlags depthStencilUsageFlags                            = {};
        grfx::ResourceState   renderTargetInitialStates[PPX_MAX_RENDER_TARGETS] = {grfx::RESOURCE_STATE_UNDEFINED};
        grfx::ResourceState   depthStencilInitialState                          = grfx::RESOURCE_STATE_UNDEFINED;
    } V2;

    // Data unique to grfx::RenderPassCreateInfo3
    struct
    {
        grfx::Image* pRenderTargetImages[PPX_MAX_RENDER_TARGETS] = {};
        grfx::Image* pDepthStencilImage                          = nullptr;
    } V3;

    // Clear values
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS] = {};
    grfx::DepthStencilClearValue depthStencilClearValue                          = {};

    // Load/store ops
    grfx::AttachmentLoadOp  renderTargetLoadOps[PPX_MAX_RENDER_TARGETS]  = {grfx::ATTACHMENT_LOAD_OP_LOAD};
    grfx::AttachmentStoreOp renderTargetStoreOps[PPX_MAX_RENDER_TARGETS] = {grfx::ATTACHMENT_STORE_OP_STORE};
    grfx::AttachmentLoadOp  depthLoadOp                                  = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp depthStoreOp                                 = grfx::ATTACHMENT_STORE_OP_STORE;
    grfx::AttachmentLoadOp  stencilLoadOp                                = grfx::ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp stencilStoreOp                               = grfx::ATTACHMENT_STORE_OP_STORE;

    RenderPassCreateInfo() {}
    RenderPassCreateInfo(const grfx::RenderPassCreateInfo& obj);
    RenderPassCreateInfo(const grfx::RenderPassCreateInfo2& obj);
    RenderPassCreateInfo(const grfx::RenderPassCreateInfo3& obj);
};

} // namespace internal

//! @class RenderPass
//!
//!
class RenderPass
    : public grfx::DeviceObject<grfx::internal::RenderPassCreateInfo>
{
public:
    RenderPass() {}
    virtual ~RenderPass() {}

    const grfx::Rect&     GetRenderArea() const { return mRenderArea; }
    const grfx::Rect&     GetScissor() const { return mRenderArea; }
    const grfx::Viewport& GetViewport() const { return mViewport; }

    uint32_t GetRenderTargetCount() const { return mCreateInfo.renderTargetCount; }
    bool     HasDepthStencil() const { return mDepthStencilImage ? true : false; }

    Result GetRenderTargetView(uint32_t index, grfx::RenderTargetView** ppView) const;
    Result GetDepthStencilView(grfx::DepthStencilView** ppView) const;

    Result GetRenderTargetImage(uint32_t index, grfx::Image** ppImage) const;
    Result GetDepthStencilImage(grfx::Image** ppImage) const;

    //! This only applies to grfx::RenderPass objects created using grfx::RenderPassCreateInfo2.
    //! These functions will set 'isExternal' to true resulting in these objects NOT getting
    //! destroyed when the encapsulating grfx::RenderPass object is destroyed.
    //!
    //! Calling these fuctions on grfx::RenderPass objects created using using grfx::RenderPassCreateInfo
    //! will still return a valid object if the index or DSV object exists.
    //!
    Result DisownRenderTargetView(uint32_t index, grfx::RenderTargetView** ppView);
    Result DisownDepthStencilView(grfx::DepthStencilView** ppView);
    Result DisownRenderTargetImage(uint32_t index, grfx::Image** ppImage);
    Result DisownDepthStencilImage(grfx::Image** ppImage);

    // Convenience functions returns empty ptr if index is out of range or DSV object does not exist.
    grfx::RenderTargetViewPtr GetRenderTargetView(uint32_t index) const;
    grfx::DepthStencilViewPtr GetDepthStencilView() const;
    grfx::ImagePtr            GetRenderTargetImage(uint32_t index) const;
    grfx::ImagePtr            GetDepthStencilImage() const;

    // Returns index of pImage otherwise returns UINT32_MAX
    uint32_t GetRenderTargetImageIndex(const grfx::Image* pImage) const;

    // Returns true if render targets or depth stencil contains ATTACHMENT_LOAD_OP_CLEAR
    bool HasLoadOpClear() const { return mHasLoadOpClear; }

protected:
    virtual Result Create(const grfx::internal::RenderPassCreateInfo* pCreateInfo) override;
    virtual void   Destroy() override;
    friend class grfx::Device;

private:
    Result CreateImagesAndViewsV1(const grfx::internal::RenderPassCreateInfo* pCreateInfo);
    Result CreateImagesAndViewsV2(const grfx::internal::RenderPassCreateInfo* pCreateInfo);
    Result CreateImagesAndViewsV3(const grfx::internal::RenderPassCreateInfo* pCreateInfo);

protected:
    grfx::Rect                             mRenderArea = {};
    grfx::Viewport                         mViewport   = {};
    std::vector<grfx::RenderTargetViewPtr> mRenderTargetViews;
    grfx::DepthStencilViewPtr              mDepthStencilView;
    std::vector<grfx::ImagePtr>            mRenderTargetImages;
    grfx::ImagePtr                         mDepthStencilImage;
    bool                                   mHasLoadOpClear = false;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_render_pass_h
