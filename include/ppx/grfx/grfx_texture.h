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

#ifndef ppx_grfx_texture_h
#define ppx_grfx_texture_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct TextureCreateInfo
//!
//!
struct TextureCreateInfo
{
    grfx::Image*                  pImage                       = nullptr;
    grfx::ImageType               imageType                    = grfx::IMAGE_TYPE_2D;
    uint32_t                      width                        = 0;
    uint32_t                      height                       = 0;
    uint32_t                      depth                        = 0;
    grfx::Format                  imageFormat                  = grfx::FORMAT_UNDEFINED;
    grfx::SampleCount             sampleCount                  = grfx::SAMPLE_COUNT_1;
    uint32_t                      mipLevelCount                = 1;
    uint32_t                      arrayLayerCount              = 1;
    grfx::ImageUsageFlags         usageFlags                   = grfx::ImageUsageFlags::SampledImage();
    grfx::MemoryUsage             memoryUsage                  = grfx::MEMORY_USAGE_GPU_ONLY;
    grfx::ResourceState           initialState                 = grfx::RESOURCE_STATE_GENERAL;    // This may not be the best choice
    grfx::RenderTargetClearValue  RTVClearValue                = {0, 0, 0, 0};                    // Optimized RTV clear value
    grfx::DepthStencilClearValue  DSVClearValue                = {1.0f, 0xFF};                    // Optimized DSV clear value
    grfx::ImageViewType           sampledImageViewType         = grfx::IMAGE_VIEW_TYPE_UNDEFINED; // Guesses from image if UNDEFINED
    grfx::Format                  sampledImageViewFormat       = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
    grfx::SamplerYcbcrConversion* pSampledImageYcbcrConversion = nullptr;                         // Leave null if not Ycbcr, or not using sampled image.
    grfx::Format                  renderTargetViewFormat       = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
    grfx::Format                  depthStencilViewFormat       = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
    grfx::Format                  storageImageViewFormat       = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
    grfx::Ownership               ownership                    = grfx::OWNERSHIP_REFERENCE;
    bool                          concurrentMultiQueueUsage    = false;
    grfx::ImageCreateFlags        imageCreateFlags             = {};
};

//! @class Texture
//!
//!
class Texture
    : public grfx::DeviceObject<grfx::TextureCreateInfo>
{
public:
    Texture() {}
    virtual ~Texture() {}

    grfx::ImageType              GetImageType() const;
    uint32_t                     GetWidth() const;
    uint32_t                     GetHeight() const;
    uint32_t                     GetDepth() const;
    grfx::Format                 GetImageFormat() const;
    grfx::SampleCount            GetSampleCount() const;
    uint32_t                     GetMipLevelCount() const;
    uint32_t                     GetArrayLayerCount() const;
    const grfx::ImageUsageFlags& GetUsageFlags() const;
    grfx::MemoryUsage            GetMemoryUsage() const;

    grfx::Format GetSampledImageViewFormat() const;
    grfx::Format GetRenderTargetViewFormat() const;
    grfx::Format GetDepthStencilViewFormat() const;
    grfx::Format GetStorageImageViewFormat() const;

    grfx::ImagePtr            GetImage() const { return mImage; }
    grfx::SampledImageViewPtr GetSampledImageView() const { return mSampledImageView; }
    grfx::RenderTargetViewPtr GetRenderTargetView() const { return mRenderTargetView; }
    grfx::DepthStencilViewPtr GetDepthStencilView() const { return mDepthStencilView; }
    grfx::StorageImageViewPtr GetStorageImageView() const { return mStorageImageView; }

protected:
    virtual Result Create(const grfx::TextureCreateInfo* pCreateInfo) override;
    friend class grfx::Device;

    virtual Result CreateApiObjects(const grfx::TextureCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    grfx::ImagePtr            mImage;
    grfx::SampledImageViewPtr mSampledImageView;
    grfx::RenderTargetViewPtr mRenderTargetView;
    grfx::DepthStencilViewPtr mDepthStencilView;
    grfx::StorageImageViewPtr mStorageImageView;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_texture_h
