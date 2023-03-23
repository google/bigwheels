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

#include "ppx/grfx/grfx_draw_pass.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_render_pass.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// internal
// -------------------------------------------------------------------------------------------------
namespace internal {

DrawPassCreateInfo::DrawPassCreateInfo(const grfx::DrawPassCreateInfo& obj)
{
    this->version           = CREATE_INFO_VERSION_1;
    this->width             = obj.width;
    this->height            = obj.height;
    this->renderTargetCount = obj.renderTargetCount;

    // Formats
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->V1.renderTargetFormats[i] = obj.renderTargetFormats[i];
    }
    this->V1.depthStencilFormat = obj.depthStencilFormat;

    // Sample count
    this->V1.sampleCount = obj.sampleCount;

    // Usage flags
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->V1.renderTargetUsageFlags[i] = obj.renderTargetUsageFlags[i] | grfx::IMAGE_USAGE_COLOR_ATTACHMENT;
    }
    this->V1.depthStencilUsageFlags = obj.depthStencilUsageFlags | grfx::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;

    // Clear values
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->V1.renderTargetInitialStates[i] = obj.renderTargetInitialStates[i];
    }
    this->V1.depthStencilInitialState = obj.depthStencilInitialState;

    // Clear values
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
    }
    this->depthStencilClearValue = obj.depthStencilClearValue;
}

DrawPassCreateInfo::DrawPassCreateInfo(const grfx::DrawPassCreateInfo2& obj)
{
    this->version           = CREATE_INFO_VERSION_2;
    this->width             = obj.width;
    this->height            = obj.height;
    this->renderTargetCount = obj.renderTargetCount;
    this->depthStencilState = obj.depthStencilState;

    // Images
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->V2.pRenderTargetImages[i] = obj.pRenderTargetImages[i];
    }
    this->V2.pDepthStencilImage = obj.pDepthStencilImage;

    // Clear values
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
    }
    this->depthStencilClearValue = obj.depthStencilClearValue;
}

DrawPassCreateInfo::DrawPassCreateInfo(const grfx::DrawPassCreateInfo3& obj)
{
    this->version           = CREATE_INFO_VERSION_3;
    this->width             = obj.width;
    this->height            = obj.height;
    this->renderTargetCount = obj.renderTargetCount;
    this->depthStencilState = obj.depthStencilState;

    // Textures
    for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
        this->V3.pRenderTargetTextures[i] = obj.pRenderTargetTextures[i];
    }
    this->V3.pDepthStencilTexture = obj.pDepthStencilTexture;
}

} // namespace internal

// -------------------------------------------------------------------------------------------------
// DrawPass
// -------------------------------------------------------------------------------------------------
Result DrawPass::CreateTexturesV1(const grfx::internal::DrawPassCreateInfo* pCreateInfo)
{
    // Create textures
    {
        // Create render target textures
        for (uint32_t i = 0; i < pCreateInfo->renderTargetCount; ++i) {
            grfx::TextureCreateInfo ci = {};
            ci.pImage                  = nullptr;
            ci.imageType               = grfx::IMAGE_TYPE_2D;
            ci.width                   = pCreateInfo->width;
            ci.height                  = pCreateInfo->height;
            ci.depth                   = 1;
            ci.imageFormat             = pCreateInfo->V1.renderTargetFormats[i];
            ci.sampleCount             = pCreateInfo->V1.sampleCount;
            ci.mipLevelCount           = 1;
            ci.arrayLayerCount         = 1;
            ci.usageFlags              = pCreateInfo->V1.renderTargetUsageFlags[i];
            ci.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
            ci.initialState            = pCreateInfo->V1.renderTargetInitialStates[i];
            ci.RTVClearValue           = pCreateInfo->renderTargetClearValues[i];
            ci.sampledImageViewType    = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
            ci.sampledImageViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.renderTargetViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.depthStencilViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.storageImageViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.ownership               = grfx::OWNERSHIP_EXCLUSIVE;

            grfx::TexturePtr texture;
            Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "render target texture create failed");
                return ppxres;
            }

            mRenderTargetTextures.push_back(texture);
        }

        // DSV image
        if (pCreateInfo->V1.depthStencilFormat != grfx::FORMAT_UNDEFINED) {
            grfx::TextureCreateInfo ci = {};
            ci.pImage                  = nullptr;
            ci.imageType               = grfx::IMAGE_TYPE_2D;
            ci.width                   = pCreateInfo->width;
            ci.height                  = pCreateInfo->height;
            ci.depth                   = 1;
            ci.imageFormat             = pCreateInfo->V1.depthStencilFormat;
            ci.sampleCount             = pCreateInfo->V1.sampleCount;
            ci.mipLevelCount           = 1;
            ci.arrayLayerCount         = 1;
            ci.usageFlags              = pCreateInfo->V1.depthStencilUsageFlags;
            ci.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
            ci.initialState            = pCreateInfo->V1.depthStencilInitialState;
            ci.DSVClearValue           = pCreateInfo->depthStencilClearValue;
            ci.sampledImageViewType    = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
            ci.sampledImageViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.renderTargetViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.depthStencilViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.storageImageViewFormat  = grfx::FORMAT_UNDEFINED;
            ci.ownership               = grfx::OWNERSHIP_EXCLUSIVE;

            grfx::TexturePtr texture;
            Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "depth stencil texture create failed");
                return ppxres;
            }

            mDepthStencilTexture = texture;
        }
    }
    return ppx::SUCCESS;
}

Result DrawPass::CreateTexturesV2(const grfx::internal::DrawPassCreateInfo* pCreateInfo)
{
    // Create textures
    {
        // Create render target textures
        for (uint32_t i = 0; i < pCreateInfo->renderTargetCount; ++i) {
            grfx::TextureCreateInfo ci = {};
            ci.pImage                  = pCreateInfo->V2.pRenderTargetImages[i];

            grfx::TexturePtr texture;
            Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "render target texture create failed");
                return ppxres;
            }

            mRenderTargetTextures.push_back(texture);
        }

        // DSV image
        if (!IsNull(pCreateInfo->V2.pDepthStencilImage)) {
            grfx::TextureCreateInfo ci = {};
            ci.pImage                  = pCreateInfo->V2.pDepthStencilImage;

            grfx::TexturePtr texture;
            Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "dpeth stencil texture create failed");
                return ppxres;
            }

            mDepthStencilTexture = texture;
        }
    }
    return ppx::SUCCESS;
}

Result DrawPass::CreateTexturesV3(const grfx::internal::DrawPassCreateInfo* pCreateInfo)
{
    // Create textures
    {
        // Create render target textures
        for (uint32_t i = 0; i < pCreateInfo->renderTargetCount; ++i) {
            grfx::TexturePtr texture = pCreateInfo->V3.pRenderTargetTextures[i];
            mRenderTargetTextures.push_back(texture);
        }

        // DSV image
        if (!IsNull(pCreateInfo->V3.pDepthStencilTexture)) {
            mDepthStencilTexture = pCreateInfo->V3.pDepthStencilTexture;
        }
    }
    return ppx::SUCCESS;
}

Result DrawPass::CreateApiObjects(const grfx::internal::DrawPassCreateInfo* pCreateInfo)
{
    mRenderArea = {0, 0, pCreateInfo->width, pCreateInfo->height};

    // Create backing resources
    switch (pCreateInfo->version) {
        default: return ppx::ERROR_INVALID_CREATE_ARGUMENT; break;

        case grfx::internal::DrawPassCreateInfo::CREATE_INFO_VERSION_1: {
            Result ppxres = CreateTexturesV1(pCreateInfo);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "create textures(V1) failed");
                return ppxres;
            }
        } break;

        case grfx::internal::DrawPassCreateInfo::CREATE_INFO_VERSION_2: {
            Result ppxres = CreateTexturesV2(pCreateInfo);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "create textures(V2) failed");
                return ppxres;
            }
        } break;

        case grfx::internal::DrawPassCreateInfo::CREATE_INFO_VERSION_3: {
            Result ppxres = CreateTexturesV3(pCreateInfo);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "create textures(V3) failed");
                return ppxres;
            }
        } break;
    }

    // Create render passes
    for (uint32_t clearMask = 0; clearMask <= static_cast<uint32_t>(DRAW_PASS_CLEAR_FLAG_CLEAR_ALL); ++clearMask) {
        grfx::AttachmentLoadOp renderTargetLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
        grfx::AttachmentLoadOp depthLoadOp        = grfx::ATTACHMENT_LOAD_OP_LOAD;
        grfx::AttachmentLoadOp stencilLoadOp      = grfx::ATTACHMENT_LOAD_OP_LOAD;

        if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS) != 0) {
            renderTargetLoadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        }
        if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH) != 0) {
            if (GetFormatDescription(mDepthStencilTexture->GetImageFormat())->aspect & FORMAT_ASPECT_DEPTH) {
                depthLoadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
            }
        }
        if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL) != 0) {
            if (GetFormatDescription(mDepthStencilTexture->GetImageFormat())->aspect & FORMAT_ASPECT_STENCIL) {
                stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
            }
        }

        // If the the depth/stencil state has READ for either depth or stecil
        // then skip creating any LOAD_OP_CLEAR render passes for it.
        // Not skipping will result in API errors.
        //
        bool skip = false;
        if (depthLoadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            switch (pCreateInfo->depthStencilState) {
                default: break;
                case grfx::RESOURCE_STATE_DEPTH_STENCIL_READ:
                case grfx::RESOURCE_STATE_DEPTH_READ_STENCIL_WRITE: {
                    skip = true;
                } break;
            }
        }
        if (stencilLoadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
            switch (pCreateInfo->depthStencilState) {
                default: break;
                case grfx::RESOURCE_STATE_DEPTH_STENCIL_READ:
                case grfx::RESOURCE_STATE_DEPTH_WRITE_STENCIL_READ: {
                    skip = true;
                } break;
            }
        }
        if (skip) {
            continue;
        }

        grfx::RenderPassCreateInfo3 rpCreateInfo = {};
        rpCreateInfo.width                       = pCreateInfo->width;
        rpCreateInfo.height                      = pCreateInfo->height;
        rpCreateInfo.renderTargetCount           = pCreateInfo->renderTargetCount;
        rpCreateInfo.depthStencilState           = pCreateInfo->depthStencilState;

        for (uint32_t i = 0; i < rpCreateInfo.renderTargetCount; ++i) {
            if (!mRenderTargetTextures[i]) {
                continue;
            }
            rpCreateInfo.pRenderTargetImages[i]     = mRenderTargetTextures[i]->GetImage();
            rpCreateInfo.renderTargetClearValues[i] = mRenderTargetTextures[i]->GetImage()->GetRTVClearValue();
            rpCreateInfo.renderTargetLoadOps[i]     = renderTargetLoadOp;
            rpCreateInfo.renderTargetStoreOps[i]    = grfx::ATTACHMENT_STORE_OP_STORE;
        }

        if (mDepthStencilTexture) {
            rpCreateInfo.pDepthStencilImage     = mDepthStencilTexture->GetImage();
            rpCreateInfo.depthStencilClearValue = mDepthStencilTexture->GetImage()->GetDSVClearValue();
            rpCreateInfo.depthLoadOp            = depthLoadOp;
            rpCreateInfo.depthStoreOp           = grfx::ATTACHMENT_STORE_OP_STORE;
            rpCreateInfo.stencilLoadOp          = stencilLoadOp;
            rpCreateInfo.stencilStoreOp         = grfx::ATTACHMENT_STORE_OP_STORE;
        }

        Pass pass      = {};
        pass.clearMask = clearMask;

        Result ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &pass.renderPass);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "create render pass failed for clearMask=" << clearMask);
            return ppxres;
        }

        mPasses.push_back(pass);
    }

    return ppx::SUCCESS;
}

void DrawPass::DestroyApiObjects()
{
    for (size_t i = 0; i < mPasses.size(); ++i) {
        if (mPasses[i].renderPass) {
            GetDevice()->DestroyRenderPass(mPasses[i].renderPass);
            mPasses[i].renderPass.Reset();
        }
    }
    mPasses.clear();

    for (size_t i = 0; i < mRenderTargetTextures.size(); ++i) {
        if (mRenderTargetTextures[i] && (mRenderTargetTextures[i]->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE)) {
            GetDevice()->DestroyTexture(mRenderTargetTextures[i]);
            mRenderTargetTextures[i].Reset();
        }
    }
    mRenderTargetTextures.clear();

    if (mDepthStencilTexture && (mDepthStencilTexture->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE)) {
        GetDevice()->DestroyTexture(mDepthStencilTexture);
        mDepthStencilTexture.Reset();
    }

    for (size_t i = 0; i < mPasses.size(); ++i) {
        if (mPasses[i].renderPass) {
            GetDevice()->DestroyRenderPass(mPasses[i].renderPass);
            mPasses[i].renderPass.Reset();
        }
    }
    mPasses.clear();
}

const grfx::Rect& DrawPass::GetRenderArea() const
{
    PPX_ASSERT_MSG(mPasses.size() > 0, "no render passes");
    PPX_ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
    return mPasses[0].renderPass->GetRenderArea();
}

const grfx::Rect& DrawPass::GetScissor() const
{
    PPX_ASSERT_MSG(mPasses.size() > 0, "no render passes");
    PPX_ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
    return mPasses[0].renderPass->GetScissor();
}

const grfx::Viewport& DrawPass::GetViewport() const
{
    PPX_ASSERT_MSG(mPasses.size() > 0, "no render passes");
    PPX_ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
    return mPasses[0].renderPass->GetViewport();
}

Result DrawPass::GetRenderTargetTexture(uint32_t index, grfx::Texture** ppRenderTarget) const
{
    if (index >= mCreateInfo.renderTargetCount) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppRenderTarget = mRenderTargetTextures[index];
    return ppx::SUCCESS;
}

grfx::Texture* DrawPass::GetRenderTargetTexture(uint32_t index) const
{
    grfx::Texture* pTexture = nullptr;
    GetRenderTargetTexture(index, &pTexture);
    return pTexture;
}

Result DrawPass::GetDepthStencilTexture(grfx::Texture** ppDepthStencil) const
{
    if (!HasDepthStencil()) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }
    *ppDepthStencil = mDepthStencilTexture;
    return ppx::SUCCESS;
}

grfx::Texture* DrawPass::GetDepthStencilTexture() const
{
    grfx::Texture* pTexture = nullptr;
    GetDepthStencilTexture(&pTexture);
    return pTexture;
}

void DrawPass::PrepareRenderPassBeginInfo(const grfx::DrawPassClearFlags& clearFlags, grfx::RenderPassBeginInfo* pBeginInfo) const
{
    uint32_t clearMask = clearFlags.flags;

    auto it = FindIf(
        mPasses,
        [clearMask](const Pass& elem) -> bool {
            bool isMatch = (elem.clearMask == clearMask);
            return isMatch; });
    if (it == std::end(mPasses)) {
        PPX_ASSERT_MSG(false, "couldn't find matching pass for clearMask=" << clearMask);
        return;
    }

    pBeginInfo->pRenderPass   = it->renderPass;
    pBeginInfo->renderArea    = GetRenderArea();
    pBeginInfo->RTVClearCount = mCreateInfo.renderTargetCount;

    if (clearFlags & grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS) {
        for (uint32_t i = 0; i < mCreateInfo.renderTargetCount; ++i) {
            pBeginInfo->RTVClearValues[i] = mCreateInfo.renderTargetClearValues[i];
        }
    }

    if ((clearFlags & grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH) || (clearFlags & grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL)) {
        pBeginInfo->DSVClearValue = mCreateInfo.depthStencilClearValue;
    }
}

} // namespace grfx
} // namespace ppx
