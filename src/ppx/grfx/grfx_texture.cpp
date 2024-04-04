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

#include "ppx/grfx/grfx_texture.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_image.h"

namespace ppx {
namespace grfx {

Result Texture::Create(const grfx::TextureCreateInfo* pCreateInfo)
{
    // Copy in case view types and formats are specified:
    //   - if an image is supplied, then the next section
    //     will overwrite all the image related fields with
    //     values from the supplied image.
    //   - if an image is NOT supplied, then nothing gets
    //     overwritten.
    //
    mCreateInfo = *pCreateInfo;

    if (!IsNull(pCreateInfo->pImage)) {
        mImage                                = pCreateInfo->pImage;
        mCreateInfo.imageType                 = mImage->GetType();
        mCreateInfo.width                     = mImage->GetWidth();
        mCreateInfo.height                    = mImage->GetHeight();
        mCreateInfo.depth                     = mImage->GetDepth();
        mCreateInfo.imageFormat               = mImage->GetFormat();
        mCreateInfo.sampleCount               = mImage->GetSampleCount();
        mCreateInfo.mipLevelCount             = mImage->GetMipLevelCount();
        mCreateInfo.arrayLayerCount           = mImage->GetArrayLayerCount();
        mCreateInfo.usageFlags                = mImage->GetUsageFlags();
        mCreateInfo.memoryUsage               = mImage->GetMemoryUsage();
        mCreateInfo.initialState              = mImage->GetInitialState();
        mCreateInfo.RTVClearValue             = mImage->GetRTVClearValue();
        mCreateInfo.DSVClearValue             = mImage->GetDSVClearValue();
        mCreateInfo.concurrentMultiQueueUsage = mImage->GetConcurrentMultiQueueUsageEnabled();
    }

    // Yes, mCreateInfo will self overwrite in the following function call.
    //
    Result ppxres = grfx::DeviceObject<grfx::TextureCreateInfo>::Create(pCreateInfo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

Result Texture::CreateApiObjects(const grfx::TextureCreateInfo* pCreateInfo)
{
    // Image
    if (IsNull(pCreateInfo->pImage)) {
        if (pCreateInfo->usageFlags.bits.colorAttachment && pCreateInfo->usageFlags.bits.depthStencilAttachment) {
            PPX_ASSERT_MSG(false, "texture cannot be both color attachment and depth stencil attachment");
            return ppx::ERROR_INVALID_CREATE_ARGUMENT;
        }

        grfx::ImageCreateInfo ci     = {};
        ci.type                      = pCreateInfo->imageType;
        ci.width                     = pCreateInfo->width;
        ci.height                    = pCreateInfo->height;
        ci.depth                     = pCreateInfo->depth;
        ci.format                    = pCreateInfo->imageFormat;
        ci.sampleCount               = pCreateInfo->sampleCount;
        ci.mipLevelCount             = pCreateInfo->mipLevelCount;
        ci.arrayLayerCount           = pCreateInfo->arrayLayerCount;
        ci.usageFlags                = pCreateInfo->usageFlags;
        ci.memoryUsage               = pCreateInfo->memoryUsage;
        ci.initialState              = pCreateInfo->initialState;
        ci.RTVClearValue             = pCreateInfo->RTVClearValue;
        ci.DSVClearValue             = pCreateInfo->DSVClearValue;
        ci.pApiObject                = nullptr;
        ci.ownership                 = pCreateInfo->ownership;
        ci.concurrentMultiQueueUsage = pCreateInfo->concurrentMultiQueueUsage;
        ci.createFlags               = pCreateInfo->imageCreateFlags;

        Result ppxres = GetDevice()->CreateImage(&ci, &mImage);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "texture create image failed");
            return ppxres;
        }
    }

    if (pCreateInfo->usageFlags.bits.sampled) {
        grfx::SampledImageViewCreateInfo ci = grfx::SampledImageViewCreateInfo::GuessFromImage(mImage);
        if (pCreateInfo->sampledImageViewType != grfx::IMAGE_VIEW_TYPE_UNDEFINED) {
            ci.imageViewType = pCreateInfo->sampledImageViewType;
        }
        ci.ycbcrConversion = pCreateInfo->sampledImageYcbcrConversion;

        Result ppxres = GetDevice()->CreateSampledImageView(&ci, &mSampledImageView);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "texture create sampled image view failed");
            return ppxres;
        }
    }

    if (pCreateInfo->usageFlags.bits.colorAttachment) {
        grfx::RenderTargetViewCreateInfo ci = grfx::RenderTargetViewCreateInfo::GuessFromImage(mImage);
        if (pCreateInfo->renderTargetViewFormat != grfx::FORMAT_UNDEFINED) {
            ci.format = pCreateInfo->renderTargetViewFormat;
        }

        Result ppxres = GetDevice()->CreateRenderTargetView(&ci, &mRenderTargetView);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "texture create render target view failed");
            return ppxres;
        }
    }

    if (pCreateInfo->usageFlags.bits.depthStencilAttachment) {
        grfx::DepthStencilViewCreateInfo ci = grfx::DepthStencilViewCreateInfo::GuessFromImage(mImage);
        if (pCreateInfo->depthStencilViewFormat != grfx::FORMAT_UNDEFINED) {
            ci.format = pCreateInfo->depthStencilViewFormat;
        }

        Result ppxres = GetDevice()->CreateDepthStencilView(&ci, &mDepthStencilView);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "texture create depth stencil view failed");
            return ppxres;
        }
    }

    if (pCreateInfo->usageFlags.bits.storage) {
        grfx::StorageImageViewCreateInfo ci = grfx::StorageImageViewCreateInfo::GuessFromImage(mImage);
        if (pCreateInfo->storageImageViewFormat != grfx::FORMAT_UNDEFINED) {
            ci.format = pCreateInfo->storageImageViewFormat;
        }

        Result ppxres = GetDevice()->CreateStorageImageView(&ci, &mStorageImageView);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "texture create storage image view failed");
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

void Texture::DestroyApiObjects()
{
    if (mSampledImageView && (mSampledImageView->GetOwnership() != grfx::OWNERSHIP_REFERENCE)) {
        GetDevice()->DestroySampledImageView(mSampledImageView);
        mSampledImageView.Reset();
    }

    if (mRenderTargetView && (mRenderTargetView->GetOwnership() != grfx::OWNERSHIP_REFERENCE)) {
        GetDevice()->DestroyRenderTargetView(mRenderTargetView);
        mRenderTargetView.Reset();
    }

    if (mDepthStencilView && (mDepthStencilView->GetOwnership() != grfx::OWNERSHIP_REFERENCE)) {
        GetDevice()->DestroyDepthStencilView(mDepthStencilView);
        mDepthStencilView.Reset();
    }

    if (mStorageImageView && (mStorageImageView->GetOwnership() != grfx::OWNERSHIP_REFERENCE)) {
        GetDevice()->DestroyStorageImageView(mStorageImageView);
        mStorageImageView.Reset();
    }

    if (mImage && (mImage->GetOwnership() != grfx::OWNERSHIP_REFERENCE)) {
        GetDevice()->DestroyImage(mImage);
        mImage.Reset();
    }
}

grfx::ImageType Texture::GetImageType() const
{
    return mImage->GetType();
}

uint32_t Texture::GetWidth() const
{
    return mImage->GetWidth();
}

uint32_t Texture::GetHeight() const
{
    return mImage->GetHeight();
}

uint32_t Texture::GetDepth() const
{
    return mImage->GetDepth();
}

grfx::Format Texture::GetImageFormat() const
{
    return mImage->GetFormat();
}

grfx::SampleCount Texture::GetSampleCount() const
{
    return mImage->GetSampleCount();
}

uint32_t Texture::GetMipLevelCount() const
{
    return mImage->GetMipLevelCount();
}

uint32_t Texture::GetArrayLayerCount() const
{
    return mImage->GetArrayLayerCount();
}

const grfx::ImageUsageFlags& Texture::GetUsageFlags() const
{
    return mImage->GetUsageFlags();
}

grfx::MemoryUsage Texture::GetMemoryUsage() const
{
    return mImage->GetMemoryUsage();
}

grfx::Format Texture::GetSampledImageViewFormat() const
{
    return mSampledImageView ? mSampledImageView->GetFormat() : grfx::FORMAT_UNDEFINED;
}

grfx::Format Texture::GetRenderTargetViewFormat() const
{
    return mRenderTargetView ? mRenderTargetView->GetFormat() : grfx::FORMAT_UNDEFINED;
}

grfx::Format Texture::GetDepthStencilViewFormat() const
{
    return mDepthStencilView ? mDepthStencilView->GetFormat() : grfx::FORMAT_UNDEFINED;
}

grfx::Format Texture::GetStorageImageViewFormat() const
{
    return mStorageImageView ? mStorageImageView->GetFormat() : grfx::FORMAT_UNDEFINED;
}

} // namespace grfx
} // namespace ppx
