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

#ifndef ppx_grfx_image_h
#define ppx_grfx_image_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

//! @struct ImageCreateInfo
//!
//!
struct ImageCreateInfo
{
    grfx::ImageType              type                      = grfx::IMAGE_TYPE_2D;
    uint32_t                     width                     = 0;
    uint32_t                     height                    = 0;
    uint32_t                     depth                     = 0;
    grfx::Format                 format                    = grfx::FORMAT_UNDEFINED;
    grfx::SampleCount            sampleCount               = grfx::SAMPLE_COUNT_1;
    uint32_t                     mipLevelCount             = 1;
    uint32_t                     arrayLayerCount           = 1;
    grfx::ImageUsageFlags        usageFlags                = grfx::ImageUsageFlags::SampledImage();
    grfx::MemoryUsage            memoryUsage               = grfx::MEMORY_USAGE_GPU_ONLY;  // D3D12 will fail on any other memory usage
    grfx::ResourceState          initialState              = grfx::RESOURCE_STATE_GENERAL; // This may not be the best choice
    grfx::RenderTargetClearValue RTVClearValue             = {0, 0, 0, 0};                 // Optimized RTV clear value
    grfx::DepthStencilClearValue DSVClearValue             = {1.0f, 0xFF};                 // Optimized DSV clear value
    void*                        pApiObject                = nullptr;                      // [OPTIONAL] For external images such as swapchain images
    grfx::Ownership              ownership                 = grfx::OWNERSHIP_REFERENCE;
    bool                         concurrentMultiQueueUsage = false;

    // Returns a create info for sampled image
    static ImageCreateInfo SampledImage2D(
        uint32_t          width,
        uint32_t          height,
        grfx::Format      format,
        grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1,
        grfx::MemoryUsage memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY);

    // Returns a create info for sampled image and depth stencil target
    static ImageCreateInfo DepthStencilTarget(
        uint32_t          width,
        uint32_t          height,
        grfx::Format      format,
        grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1);

    // Returns a create info for sampled image and render target
    static ImageCreateInfo RenderTarget2D(
        uint32_t          width,
        uint32_t          height,
        grfx::Format      format,
        grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1);
};

//! @class Image
//!
//!
class Image
    : public grfx::DeviceObject<grfx::ImageCreateInfo>
{
public:
    Image() {}
    virtual ~Image() {}

    grfx::ImageType                     GetType() const { return mCreateInfo.type; }
    uint32_t                            GetWidth() const { return mCreateInfo.width; }
    uint32_t                            GetHeight() const { return mCreateInfo.height; }
    uint32_t                            GetDepth() const { return mCreateInfo.depth; }
    grfx::Format                        GetFormat() const { return mCreateInfo.format; }
    grfx::SampleCount                   GetSampleCount() const { return mCreateInfo.sampleCount; }
    uint32_t                            GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
    uint32_t                            GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
    const grfx::ImageUsageFlags&        GetUsageFlags() const { return mCreateInfo.usageFlags; }
    grfx::MemoryUsage                   GetMemoryUsage() const { return mCreateInfo.memoryUsage; }
    grfx::ResourceState                 GetInitialState() const { return mCreateInfo.initialState; }
    const grfx::RenderTargetClearValue& GetRTVClearValue() const { return mCreateInfo.RTVClearValue; }
    const grfx::DepthStencilClearValue& GetDSVClearValue() const { return mCreateInfo.DSVClearValue; }
    bool                                GetConcurrentMultiQueueUsageEnabled() const { return mCreateInfo.concurrentMultiQueueUsage; }

    // Convenience functions
    grfx::ImageViewType GuessImageViewType(bool isCube = false) const;

    virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) = 0;
    virtual void   UnmapMemory()                                      = 0;

protected:
    virtual Result Create(const grfx::ImageCreateInfo* pCreateInfo) override;
    friend class grfx::Device;
};

// -------------------------------------------------------------------------------------------------

namespace internal {

class ImageResourceView
{
public:
    ImageResourceView() {}
    virtual ~ImageResourceView() {}
};

} // namespace internal

//! @class ImageView
//!
//! This class exists to genericize descriptor updates for Vulkan's 'image' based resources.
//!
class ImageView
{
public:
    ImageView() {}
    virtual ~ImageView() {}

    const grfx::internal::ImageResourceView* GetResourceView() const { return mResourceView.get(); }

protected:
    void SetResourceView(std::unique_ptr<internal::ImageResourceView>&& view)
    {
        mResourceView = std::move(view);
    }

private:
    std::unique_ptr<internal::ImageResourceView> mResourceView;
};

// -------------------------------------------------------------------------------------------------

//! @struct SamplerCreateInfo
//!
//!
struct SamplerCreateInfo
{
    grfx::Filter             magFilter        = grfx::FILTER_NEAREST;
    grfx::Filter             minFilter        = grfx::FILTER_NEAREST;
    grfx::SamplerMipmapMode  mipmapMode       = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
    grfx::SamplerAddressMode addressModeU     = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    grfx::SamplerAddressMode addressModeV     = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    grfx::SamplerAddressMode addressModeW     = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    float                    mipLodBias       = 0.0f;
    bool                     anisotropyEnable = false;
    float                    maxAnisotropy    = 0.0f;
    bool                     compareEnable    = false;
    grfx::CompareOp          compareOp        = grfx::COMPARE_OP_NEVER;
    float                    minLod           = 0.0f;
    float                    maxLod           = 1.0f;
    grfx::BorderColor        borderColor      = grfx::BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    grfx::Ownership          ownership        = grfx::OWNERSHIP_REFERENCE;
};

//! @class Sampler
//!
//!
class Sampler
    : public grfx::DeviceObject<grfx::SamplerCreateInfo>
{
public:
    Sampler() {}
    virtual ~Sampler() {}
};

// -------------------------------------------------------------------------------------------------

//! @struct DepthStencilViewCreateInfo
//!
//!
struct DepthStencilViewCreateInfo
{
    grfx::Image*            pImage          = nullptr;
    grfx::ImageViewType     imageViewType   = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
    grfx::Format            format          = grfx::FORMAT_UNDEFINED;
    uint32_t                mipLevel        = 0;
    uint32_t                mipLevelCount   = 0;
    uint32_t                arrayLayer      = 0;
    uint32_t                arrayLayerCount = 0;
    grfx::ComponentMapping  components      = {};
    grfx::AttachmentLoadOp  depthLoadOp     = ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp depthStoreOp    = ATTACHMENT_STORE_OP_STORE;
    grfx::AttachmentLoadOp  stencilLoadOp   = ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp stencilStoreOp  = ATTACHMENT_STORE_OP_STORE;
    grfx::Ownership         ownership       = grfx::OWNERSHIP_REFERENCE;

    static grfx::DepthStencilViewCreateInfo GuessFromImage(grfx::Image* pImage);
};

//! @class DepthStencilView
//!
//!
class DepthStencilView
    : public grfx::DeviceObject<grfx::DepthStencilViewCreateInfo>,
      public grfx::ImageView
{
public:
    DepthStencilView() {}
    virtual ~DepthStencilView() {}

    grfx::ImagePtr          GetImage() const { return mCreateInfo.pImage; }
    grfx::Format            GetFormat() const { return mCreateInfo.format; }
    grfx::AttachmentLoadOp  GetDepthLoadOp() const { return mCreateInfo.depthLoadOp; }
    grfx::AttachmentStoreOp GetDepthStoreOp() const { return mCreateInfo.depthStoreOp; }
    grfx::AttachmentLoadOp  GetStencilLoadOp() const { return mCreateInfo.stencilLoadOp; }
    grfx::AttachmentStoreOp GetStencilStoreOp() const { return mCreateInfo.stencilStoreOp; }
};

// -------------------------------------------------------------------------------------------------

//! @struct RenderTargetViewCreateInfo
//!
//!
struct RenderTargetViewCreateInfo
{
    grfx::Image*            pImage          = nullptr;
    grfx::ImageViewType     imageViewType   = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
    grfx::Format            format          = grfx::FORMAT_UNDEFINED;
    grfx::SampleCount       sampleCount     = grfx::SAMPLE_COUNT_1;
    uint32_t                mipLevel        = 0;
    uint32_t                mipLevelCount   = 0;
    uint32_t                arrayLayer      = 0;
    uint32_t                arrayLayerCount = 0;
    grfx::ComponentMapping  components      = {};
    grfx::AttachmentLoadOp  loadOp          = ATTACHMENT_LOAD_OP_LOAD;
    grfx::AttachmentStoreOp storeOp         = ATTACHMENT_STORE_OP_STORE;
    grfx::Ownership         ownership       = grfx::OWNERSHIP_REFERENCE;

    static grfx::RenderTargetViewCreateInfo GuessFromImage(grfx::Image* pImage);
};

//! @class RenderTargetView
//!
//!
class RenderTargetView
    : public grfx::DeviceObject<grfx::RenderTargetViewCreateInfo>,
      public grfx::ImageView
{
public:
    RenderTargetView() {}
    virtual ~RenderTargetView() {}

    grfx::ImagePtr          GetImage() const { return mCreateInfo.pImage; }
    grfx::Format            GetFormat() const { return mCreateInfo.format; }
    grfx::SampleCount       GetSampleCount() const { return mCreateInfo.sampleCount; }
    grfx::AttachmentLoadOp  GetLoadOp() const { return mCreateInfo.loadOp; }
    grfx::AttachmentStoreOp GetStoreOp() const { return mCreateInfo.storeOp; }
};

// -------------------------------------------------------------------------------------------------

//! @struct SampledImageViewCreateInfo
//!
//!
struct SampledImageViewCreateInfo
{
    grfx::Image*           pImage          = nullptr;
    grfx::ImageViewType    imageViewType   = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
    grfx::Format           format          = grfx::FORMAT_UNDEFINED;
    grfx::SampleCount      sampleCount     = grfx::SAMPLE_COUNT_1;
    uint32_t               mipLevel        = 0;
    uint32_t               mipLevelCount   = 0;
    uint32_t               arrayLayer      = 0;
    uint32_t               arrayLayerCount = 0;
    grfx::ComponentMapping components      = {};
    grfx::Ownership        ownership       = grfx::OWNERSHIP_REFERENCE;

    static grfx::SampledImageViewCreateInfo GuessFromImage(grfx::Image* pImage);
};

//! @class SampledImageView
//!
//!
class SampledImageView
    : public grfx::DeviceObject<grfx::SampledImageViewCreateInfo>,
      public grfx::ImageView
{
public:
    SampledImageView() {}
    virtual ~SampledImageView() {}

    grfx::ImagePtr                GetImage() const { return mCreateInfo.pImage; }
    grfx::ImageViewType           GetImageViewType() const { return mCreateInfo.imageViewType; }
    grfx::Format                  GetFormat() const { return mCreateInfo.format; }
    grfx::SampleCount             GetSampleCount() const { return mCreateInfo.sampleCount; }
    uint32_t                      GetMipLevel() const { return mCreateInfo.mipLevel; }
    uint32_t                      GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
    uint32_t                      GetArrayLayer() const { return mCreateInfo.arrayLayer; }
    uint32_t                      GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
    const grfx::ComponentMapping& GetComponents() const { return mCreateInfo.components; }
};

// -------------------------------------------------------------------------------------------------

//! @struct StorageImageViewCreateInfo
//!
//!
struct StorageImageViewCreateInfo
{
    grfx::Image*           pImage          = nullptr;
    grfx::ImageViewType    imageViewType   = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
    grfx::Format           format          = grfx::FORMAT_UNDEFINED;
    grfx::SampleCount      sampleCount     = grfx::SAMPLE_COUNT_1;
    uint32_t               mipLevel        = 0;
    uint32_t               mipLevelCount   = 0;
    uint32_t               arrayLayer      = 0;
    uint32_t               arrayLayerCount = 0;
    grfx::ComponentMapping components      = {};
    grfx::Ownership        ownership       = grfx::OWNERSHIP_REFERENCE;

    static grfx::StorageImageViewCreateInfo GuessFromImage(grfx::Image* pImage);
};

//! @class StorageImageView
//!
//!
class StorageImageView
    : public grfx::DeviceObject<grfx::StorageImageViewCreateInfo>,
      public grfx::ImageView
{
public:
    StorageImageView() {}
    virtual ~StorageImageView() {}

    grfx::ImagePtr      GetImage() const { return mCreateInfo.pImage; }
    grfx::ImageViewType GetImageViewType() const { return mCreateInfo.imageViewType; }
    grfx::Format        GetFormat() const { return mCreateInfo.format; }
    grfx::SampleCount   GetSampleCount() const { return mCreateInfo.sampleCount; }
    uint32_t            GetMipLevel() const { return mCreateInfo.mipLevel; }
    uint32_t            GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
    uint32_t            GetArrayLayer() const { return mCreateInfo.arrayLayer; }
    uint32_t            GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_image_h
