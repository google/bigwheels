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

#ifndef ppx_grfx_draw_pass_h
#define ppx_grfx_draw_pass_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

struct RenderPassBeginInfo;

//! @struct DrawPassCreateInfo
//!
//! Use this version if the format(s) are known but images need creation.
//!
//! Backing images will be created using the criteria provided in this struct.
//!
struct DrawPassCreateInfo
{
    uint32_t                     width                                             = 0;
    uint32_t                     height                                            = 0;
    grfx::SampleCount            sampleCount                                       = grfx::SAMPLE_COUNT_1;
    uint32_t                     renderTargetCount                                 = 0;
    grfx::Format                 renderTargetFormats[PPX_MAX_RENDER_TARGETS]       = {};
    grfx::Format                 depthStencilFormat                                = grfx::FORMAT_UNDEFINED;
    grfx::ImageUsageFlags        renderTargetUsageFlags[PPX_MAX_RENDER_TARGETS]    = {};
    grfx::ImageUsageFlags        depthStencilUsageFlags                            = {};
    grfx::ResourceState          renderTargetInitialStates[PPX_MAX_RENDER_TARGETS] = {grfx::RESOURCE_STATE_RENDER_TARGET};
    grfx::ResourceState          depthStencilInitialState                          = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS]   = {};
    grfx::DepthStencilClearValue depthStencilClearValue                            = {};
    grfx::ShadingRatePattern*    pShadingRatePattern                               = nullptr;
};

//! @struct DrawPassCreateInfo2
//!
//! Use this version if the images exists.
//!
struct DrawPassCreateInfo2
{
    uint32_t                     width                                           = 0;
    uint32_t                     height                                          = 0;
    uint32_t                     renderTargetCount                               = 0;
    grfx::Image*                 pRenderTargetImages[PPX_MAX_RENDER_TARGETS]     = {};
    grfx::Image*                 pDepthStencilImage                              = nullptr;
    grfx::ResourceState          depthStencilState                               = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS] = {};
    grfx::DepthStencilClearValue depthStencilClearValue                          = {};
    grfx::ShadingRatePattern*    pShadingRatePattern                             = nullptr;
};

//! struct DrawPassCreateInfo3
//!
//! Use this version if the textures exists.
//!
struct DrawPassCreateInfo3
{
    uint32_t                  width                                         = 0;
    uint32_t                  height                                        = 0;
    uint32_t                  renderTargetCount                             = 0;
    grfx::Texture*            pRenderTargetTextures[PPX_MAX_RENDER_TARGETS] = {};
    grfx::Texture*            pDepthStencilTexture                          = nullptr;
    grfx::ResourceState       depthStencilState                             = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::ShadingRatePattern* pShadingRatePattern                           = nullptr;
};

namespace internal {

struct DrawPassCreateInfo
{
    enum CreateInfoVersion
    {
        CREATE_INFO_VERSION_UNDEFINED = 0,
        CREATE_INFO_VERSION_1         = 1,
        CREATE_INFO_VERSION_2         = 2,
        CREATE_INFO_VERSION_3         = 3,
    };

    CreateInfoVersion         version             = CREATE_INFO_VERSION_UNDEFINED;
    uint32_t                  width               = 0;
    uint32_t                  height              = 0;
    uint32_t                  renderTargetCount   = 0;
    grfx::ResourceState       depthStencilState   = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    grfx::ShadingRatePattern* pShadingRatePattern = nullptr;

    // Data unique to grfx::DrawPassCreateInfo1
    struct
    {
        grfx::SampleCount     sampleCount                                       = grfx::SAMPLE_COUNT_1;
        grfx::Format          renderTargetFormats[PPX_MAX_RENDER_TARGETS]       = {};
        grfx::Format          depthStencilFormat                                = grfx::FORMAT_UNDEFINED;
        grfx::ImageUsageFlags renderTargetUsageFlags[PPX_MAX_RENDER_TARGETS]    = {};
        grfx::ImageUsageFlags depthStencilUsageFlags                            = {};
        grfx::ResourceState   renderTargetInitialStates[PPX_MAX_RENDER_TARGETS] = {grfx::RESOURCE_STATE_RENDER_TARGET};
        grfx::ResourceState   depthStencilInitialState                          = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
    } V1;

    // Data unique to grfx::DrawPassCreateInfo2
    struct
    {
        grfx::Image* pRenderTargetImages[PPX_MAX_RENDER_TARGETS] = {};
        grfx::Image* pDepthStencilImage                          = nullptr;
    } V2;

    // Data unique to grfx::DrawPassCreateInfo3
    struct
    {
        grfx::Texture* pRenderTargetTextures[PPX_MAX_RENDER_TARGETS] = {};
        grfx::Texture* pDepthStencilTexture                          = nullptr;
    } V3;

    // Clear values
    grfx::RenderTargetClearValue renderTargetClearValues[PPX_MAX_RENDER_TARGETS] = {};
    grfx::DepthStencilClearValue depthStencilClearValue                          = {};

    DrawPassCreateInfo() {}
    DrawPassCreateInfo(const grfx::DrawPassCreateInfo& obj);
    DrawPassCreateInfo(const grfx::DrawPassCreateInfo2& obj);
    DrawPassCreateInfo(const grfx::DrawPassCreateInfo3& obj);
};

} // namespace internal

//! @class DrawPass
//!
//!
class DrawPass
    : public DeviceObject<grfx::internal::DrawPassCreateInfo>
{
public:
    DrawPass() {}
    virtual ~DrawPass() {}

    uint32_t              GetWidth() const { return mCreateInfo.width; }
    uint32_t              GetHeight() const { return mCreateInfo.height; }
    const grfx::Rect&     GetRenderArea() const;
    const grfx::Rect&     GetScissor() const;
    const grfx::Viewport& GetViewport() const;

    uint32_t       GetRenderTargetCount() const { return mCreateInfo.renderTargetCount; }
    Result         GetRenderTargetTexture(uint32_t index, grfx::Texture** ppRenderTarget) const;
    grfx::Texture* GetRenderTargetTexture(uint32_t index) const;
    bool           HasDepthStencil() const { return mDepthStencilTexture ? true : false; }
    Result         GetDepthStencilTexture(grfx::Texture** ppDepthStencil) const;
    grfx::Texture* GetDepthStencilTexture() const;

    void PrepareRenderPassBeginInfo(const grfx::DrawPassClearFlags& clearFlags, grfx::RenderPassBeginInfo* pBeginInfo) const;

protected:
    virtual Result CreateApiObjects(const grfx::internal::DrawPassCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result CreateTexturesV1(const grfx::internal::DrawPassCreateInfo* pCreateInfo);
    Result CreateTexturesV2(const grfx::internal::DrawPassCreateInfo* pCreateInfo);
    Result CreateTexturesV3(const grfx::internal::DrawPassCreateInfo* pCreateInfo);

private:
    grfx::Rect                    mRenderArea = {};
    std::vector<grfx::TexturePtr> mRenderTargetTextures;
    grfx::TexturePtr              mDepthStencilTexture;

    struct Pass
    {
        uint32_t            clearMask = 0;
        grfx::RenderPassPtr renderPass;
    };

    std::vector<Pass> mPasses;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_draw_pass_h
