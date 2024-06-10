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

#include "ppx/graphics_util.h"

#include <algorithm>

#include "ppx/generate_mip_shader_VK.h"
#include "ppx/generate_mip_shader_DX.h"
#include "ppx/bitmap.h"
#include "ppx/fs.h"
#include "ppx/mipmap.h"
#include "ppx/timer.h"
#include "ppx/grfx/grfx_buffer.h"
#include "ppx/grfx/grfx_command.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_queue.h"
#include "ppx/grfx/grfx_util.h"
#include "ppx/grfx/grfx_scope.h"
#include "gli/gli.hpp"

namespace ppx {
namespace grfx_util {

namespace {

// Start planar image helper functions

// Gets the height of a single plane, in terms of number of pixels represented.
// This doesn't directly correlate to the number of bits / bytes for the plane's
// height. The value returned can be used in a copy-image-to-buffer command.
// plane: The plane to get the height for (containing information about the
//        color components represented in the plane).
// subsampling: The type of subsampling applied to chroma values for the image
//              (e.g. 444, 422, 420).
// imageHeight: The height of the image, in pixels, with no subsampling applied.
uint32_t GetPlaneHeightInPixels(
    const grfx::FormatPlaneDesc::Plane& plane,
    grfx::FormatChromaSubsampling       subsampling,
    uint32_t                            imageHeight)
{
    bool hasColSubsampling = (subsampling == grfx::FORMAT_CHROMA_SUBSAMPLING_420);
    bool hasChromaValue    = false;
    bool hasLumaValue      = false;
    for (const grfx::FormatPlaneDesc::Member& member : plane.members) {
        if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_CHROMA) {
            hasChromaValue = true;
        }
        else if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_LUMA) {
            hasLumaValue = true;
        }
        else {
            PPX_LOG_WARN("Member " << member.component << "has unknown chroma type.");
        }
    }

    if (hasColSubsampling && hasChromaValue) {
        // Note: you never have subsampling on the height axis of the image in
        // a plane if luma values are present, since luma values usually aren't
        // subsampled. You might have subsampling on the width axis, but that
        // would essentially mean you get two luma values, and one of each
        // chroma value, in a block of four.
        if (hasLumaValue) {
            PPX_LOG_WARN(
                "Frame size will be inaccurate, there is vertical subsampling "
                "with both chroma and luma values present on a single plane, "
                "which is not supported!");
        }

        // If we're subsampling at 4:2:0, the image will have half its height.
        return imageHeight / 2;
    }
    return imageHeight;
}

// Gets the width of a single plane, in terms of number of pixels represented.
// This doesn't directly correlate to the number of bits / bytes for the plane's
// height. The value returned can be used in a copy-image-to-buffer command.
// plane: The plane to get the width for (containing information about the
//        color components represented in the plane).
// subsampling: The type of subsampling applied to chroma values for the image
//              (e.g. 444, 422, 420).
// imageWidth: The width of the image, in pixels, with no subsampling applied.
uint32_t GetPlaneWidthInPixels(
    const grfx::FormatPlaneDesc::Plane& plane,
    grfx::FormatChromaSubsampling       subsampling,
    uint32_t                            imageWidth)
{
    bool hasRowSubsampling = (subsampling == grfx::FORMAT_CHROMA_SUBSAMPLING_420) ||
                             (subsampling == grfx::FORMAT_CHROMA_SUBSAMPLING_422);
    bool hasChromaValue = false;
    for (const grfx::FormatPlaneDesc::Member& member : plane.members) {
        if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_CHROMA) {
            hasChromaValue = true;
            break;
        }
    }

    if (hasRowSubsampling && hasChromaValue) {
        // Note: even if the layer has a luma value, generally, in the case of
        // buffer copies, the width is treated as a half width, if we're
        // subsampling at 4:2:0 or 4:2:2, and are looking at a plane with chroma
        // values.
        return imageWidth / 2;
    }
    return imageWidth;
}

// Gets the size of an image plane in bytes.
// plane: The plane to get information for. (Contains information about the
//        color components represented by this plane, and their bit counts).
// subsampling: The type of chroma subsampling applied to this image (e.g.
//              444, 422, 420).
// width: The width of the image, in pixels, with no subsampling applied.
// height: The height of the image, in pixels, with no subsampling applied.
uint32_t GetPlaneSizeInBytes(
    const grfx::FormatPlaneDesc::Plane& plane,
    grfx::FormatChromaSubsampling       subsampling,
    uint32_t                            width,
    uint32_t                            height)
{
    bool     hasColSubsampling = (subsampling == grfx::FORMAT_CHROMA_SUBSAMPLING_420);
    bool     hasRowSubsampling = hasColSubsampling || (subsampling == grfx::FORMAT_CHROMA_SUBSAMPLING_422);
    bool     hasChromaValue    = false;
    bool     hasLumaValue      = false;
    uint32_t rowBitFactor      = 0;
    for (const grfx::FormatPlaneDesc::Member& member : plane.members) {
        if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_CHROMA) {
            hasChromaValue = true;
        }
        else if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_LUMA) {
            hasLumaValue = true;
        }
        else {
            PPX_LOG_WARN("Member " << member.component << "has unknown chroma type.");
        }

        // We only subsample chroma values.
        if (member.type == grfx::FORMAT_PLANE_CHROMA_TYPE_CHROMA && hasRowSubsampling) {
            rowBitFactor += member.bitCount / 2;
        }
        else {
            rowBitFactor += member.bitCount;
        }
    }

    if (hasColSubsampling && hasChromaValue) {
        // Note: you never have subsampling on the height axis of the image in
        // a plane if luma values are present, since luma values usually aren't
        // subsampled. You might have subsampling on the width axis, but that
        // would essentially mean you get two luma values, and one of each
        // chroma value, in a block of four.
        if (hasLumaValue) {
            PPX_LOG_WARN(
                "Frame size will be inaccurate, there is vertical subsampling "
                "with both chroma and luma values present on a single plane, "
                "which is not supported!");
        }

        return (width * rowBitFactor * (height / 2)) / 8;
    }

    // No subsampling for height, OR this plane is of luma values (which are
    // not subsampled).
    return (width * rowBitFactor * height) / 8;
}

// Gets the total size of a planar image in bytes, by calculating the size of
// each plane individually.
// formatDesc: Information about the image format, such as the components
//             represented, etc.
// planeDesc: Information about the components in the current image plane.
// width: The width of the image, in pixels, with no subsampling applied.
// height: The height of the image, in pixels, with no subsampling applied.
uint32_t GetPlanarImageSizeInBytes(
    const grfx::FormatDesc&      formatDesc,
    const grfx::FormatPlaneDesc& planeDesc,
    uint32_t                     width,
    uint32_t                     height)
{
    grfx::FormatChromaSubsampling subsampling = formatDesc.chromaSubsampling;

    uint32_t imageSize = 0;
    for (const grfx::FormatPlaneDesc::Plane& plane : planeDesc.planes) {
        imageSize += GetPlaneSizeInBytes(plane, subsampling, width, height);
    }
    return imageSize;
}

// End planar image helper functions

} // namespace

grfx::Format ToGrfxFormat(Bitmap::Format value)
{
    // clang-format off
    switch (value) {
        default: break;
        case Bitmap::FORMAT_R_UINT8     : return grfx::FORMAT_R8_UNORM; break;
        case Bitmap::FORMAT_RG_UINT8    : return grfx::FORMAT_R8G8_UNORM; break;
        case Bitmap::FORMAT_RGB_UINT8   : return grfx::FORMAT_R8G8B8_UNORM; break;
        case Bitmap::FORMAT_RGBA_UINT8  : return grfx::FORMAT_R8G8B8A8_UNORM; break;
        case Bitmap::FORMAT_R_UINT16    : return grfx::FORMAT_R16_UNORM; break;
        case Bitmap::FORMAT_RG_UINT16   : return grfx::FORMAT_R16G16_UNORM; break;
        case Bitmap::FORMAT_RGB_UINT16  : return grfx::FORMAT_R16G16B16_UNORM; break;
        case Bitmap::FORMAT_RGBA_UINT16 : return grfx::FORMAT_R16G16B16A16_UNORM; break;
        //case Bitmap::FORMAT_R_UINT32    : return grfx::FORMAT_R32_UNORM; break;
        //case Bitmap::FORMAT_RG_UINT32   : return grfx::FORMAT_R32G32_UNORM; break;
        //case Bitmap::FORMAT_RGB_UINT32  : return grfx::FORMAT_R32G32B32_UNORM; break;
        //case Bitmap::FORMAT_RGBA_UINT32 : return grfx::FORMAT_R32G32B32A32_UNORM; break;
        case Bitmap::FORMAT_R_FLOAT     : return grfx::FORMAT_R32_FLOAT; break;
        case Bitmap::FORMAT_RG_FLOAT    : return grfx::FORMAT_R32G32_FLOAT; break;
        case Bitmap::FORMAT_RGB_FLOAT   : return grfx::FORMAT_R32G32B32_FLOAT; break;
        case Bitmap::FORMAT_RGBA_FLOAT  : return grfx::FORMAT_R32G32B32A32_FLOAT; break;
    }
    // clang-format on
    return grfx::FORMAT_UNDEFINED;
}

grfx::Format ToGrfxFormat(gli::format value)
{
    // clang-format off
    switch (value) {
        case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8          : return grfx::FORMAT_BC1_RGB_UNORM;
        case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8           : return grfx::FORMAT_BC1_RGB_SRGB;
        case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8         : return grfx::FORMAT_BC1_RGBA_UNORM;
        case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8          : return grfx::FORMAT_BC1_RGBA_SRGB;
        case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16         : return grfx::FORMAT_BC2_SRGB;
        case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16        : return grfx::FORMAT_BC2_UNORM;
        case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16         : return grfx::FORMAT_BC3_SRGB;
        case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16        : return grfx::FORMAT_BC3_UNORM;
        case gli::FORMAT_R_ATI1N_UNORM_BLOCK8           : return grfx::FORMAT_BC4_UNORM;
        case gli::FORMAT_R_ATI1N_SNORM_BLOCK8           : return grfx::FORMAT_BC4_SNORM;
        case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16         : return grfx::FORMAT_BC5_UNORM;
        case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16         : return grfx::FORMAT_BC5_SNORM;
        case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16          : return grfx::FORMAT_BC6H_UFLOAT;
        case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16          : return grfx::FORMAT_BC6H_SFLOAT;
        case gli::FORMAT_RGBA_BP_UNORM_BLOCK16          : return grfx::FORMAT_BC7_UNORM;
        case gli::FORMAT_RGBA_BP_SRGB_BLOCK16           : return grfx::FORMAT_BC7_SRGB;
        default:
            return grfx::FORMAT_UNDEFINED;
    }
    // clang-format on
}

// -------------------------------------------------------------------------------------------------

Result CopyBitmapToImage(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image*        pImage,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    grfx::ResourceState stateBefore,
    grfx::ResourceState stateAfter)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pBitmap);
    PPX_ASSERT_NULL_ARG(pImage);

    Result ppxres = ppx::ERROR_FAILED;

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // This is the number of bytes we're going to copy per row.
    uint32_t rowCopySize = pBitmap->GetWidth() * pBitmap->GetPixelStride();

    // When copying from a buffer to a image/texture, D3D12 requires that the rows
    // stored in the source buffer (aka staging buffer) are aligned to 256 bytes.
    // Vulkan does not have this requirement. So for the staging buffer, we want
    // to enforce the alignment for D3D12 but not for Vulkan.
    //
    uint32_t apiRowStrideAligement = grfx::IsDx12(pQueue->GetDevice()->GetApi()) ? PPX_D3D12_TEXTURE_DATA_PITCH_ALIGNMENT : 1;
    // The staging buffer's row stride alignemnt needs to be based off the bitmap's
    // width (i.e. the number of bytes we're going to copy) and not the bitmap's row
    // stride. The bitmap's may be padded beyond width * pixel stride.
    //
    uint32_t stagingBufferRowStride = RoundUp<uint32_t>(rowCopySize, apiRowStrideAligement);

    // Create staging buffer
    grfx::BufferPtr stagingBuffer;
    {
        uint64_t bufferSize = stagingBufferRowStride * pBitmap->GetHeight();

        grfx::BufferCreateInfo ci      = {};
        ci.size                        = bufferSize;
        ci.usageFlags.bits.transferSrc = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

        ppxres = pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(stagingBuffer);

        // Map and copy to staging buffer
        void* pBufferAddress = nullptr;
        ppxres               = stagingBuffer->MapMemory(0, &pBufferAddress);
        if (Failed(ppxres)) {
            return ppxres;
        }

        const char*    pSrc         = pBitmap->GetData();
        char*          pDst         = static_cast<char*>(pBufferAddress);
        const uint32_t srcRowStride = pBitmap->GetRowStride();
        const uint32_t dstRowStride = stagingBufferRowStride;
        for (uint32_t y = 0; y < pBitmap->GetHeight(); ++y) {
            memcpy(pDst, pSrc, rowCopySize);
            pSrc += srcRowStride;
            pDst += dstRowStride;
        }

        stagingBuffer->UnmapMemory();
    }

    // Copy info
    grfx::BufferToImageCopyInfo copyInfo = {};
    copyInfo.srcBuffer.imageWidth        = pBitmap->GetWidth();
    copyInfo.srcBuffer.imageHeight       = pBitmap->GetHeight();
    copyInfo.srcBuffer.imageRowStride    = stagingBufferRowStride;
    copyInfo.srcBuffer.footprintOffset   = 0;
    copyInfo.srcBuffer.footprintWidth    = pBitmap->GetWidth();
    copyInfo.srcBuffer.footprintHeight   = pBitmap->GetHeight();
    copyInfo.srcBuffer.footprintDepth    = 1;
    copyInfo.dstImage.mipLevel           = mipLevel;
    copyInfo.dstImage.arrayLayer         = arrayLayer;
    copyInfo.dstImage.arrayLayerCount    = 1;
    copyInfo.dstImage.x                  = 0;
    copyInfo.dstImage.y                  = 0;
    copyInfo.dstImage.z                  = 0;
    copyInfo.dstImage.width              = pBitmap->GetWidth();
    copyInfo.dstImage.height             = pBitmap->GetHeight();
    copyInfo.dstImage.depth              = 1;

    // Copy to GPU image
    ppxres = pQueue->CopyBufferToImage(
        std::vector<grfx::BufferToImageCopyInfo>{copyInfo},
        stagingBuffer,
        pImage,
        mipLevel,
        1,
        arrayLayer,
        1,
        stateBefore,
        stateAfter);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateImageFromBitmap(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image**       ppImage,
    const ImageOptions& options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pBitmap);
    PPX_ASSERT_NULL_ARG(ppImage);

    Result ppxres = ppx::ERROR_FAILED;

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Cap mip level count
    uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
    uint32_t mipLevelCount    = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

    // Create target image
    grfx::ImagePtr targetImage;
    {
        grfx::ImageCreateInfo ci       = {};
        ci.type                        = grfx::IMAGE_TYPE_2D;
        ci.width                       = pBitmap->GetWidth();
        ci.height                      = pBitmap->GetHeight();
        ci.depth                       = 1;
        ci.format                      = ToGrfxFormat(pBitmap->GetFormat());
        ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount               = mipLevelCount;
        ci.arrayLayerCount             = 1;
        ci.usageFlags.bits.transferDst = true;
        ci.usageFlags.bits.sampled     = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.initialState                = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        ci.usageFlags.flags |= options.mAdditionalUsage;

        ppxres = pQueue->GetDevice()->CreateImage(&ci, &targetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetImage);
    }

    // Since this mipmap is temporary, it's safe to use the static pool.
    Mipmap mipmap = Mipmap(*pBitmap, mipLevelCount, /* useStaticPool= */ true);
    if (!mipmap.IsOk()) {
        return ppx::ERROR_FAILED;
    }

    // Copy mips to image
    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        const Bitmap* pMip = mipmap.GetMip(mipLevel);

        ppxres = CopyBitmapToImage(
            pQueue,
            pMip,
            targetImage,
            mipLevel,
            0,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetImage->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppImage = targetImage;

    return ppx::SUCCESS;
}

Result CreateImageFromBitmapGpu(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Image**       ppImage,
    const ImageOptions& options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pBitmap);
    PPX_ASSERT_NULL_ARG(ppImage);

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    Result ppxres = ppx::ERROR_FAILED;

    // Cap mip level count
    uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
    uint32_t mipLevelCount    = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

    // Create target image
    grfx::ImagePtr targetImage;
    {
        grfx::ImageCreateInfo ci       = {};
        ci.type                        = grfx::IMAGE_TYPE_2D;
        ci.width                       = pBitmap->GetWidth();
        ci.height                      = pBitmap->GetHeight();
        ci.depth                       = 1;
        ci.format                      = ToGrfxFormat(pBitmap->GetFormat());
        ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount               = mipLevelCount;
        ci.arrayLayerCount             = 1;
        ci.usageFlags.bits.transferDst = true;
        ci.usageFlags.bits.transferSrc = true; // For CS
        ci.usageFlags.bits.sampled     = true;
        ci.usageFlags.bits.storage     = true; // For CS
        ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.initialState                = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        ci.usageFlags.flags |= options.mAdditionalUsage;

        ppxres = pQueue->GetDevice()->CreateImage(&ci, &targetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetImage);
    }

    // Copy first level mip into image
    ppxres = CopyBitmapToImage(
        pQueue,
        pBitmap,
        targetImage,
        0,
        0,
        grfx::RESOURCE_STATE_SHADER_RESOURCE,
        grfx::RESOURCE_STATE_SHADER_RESOURCE);

    if (Failed(ppxres)) {
        return ppxres;
    }

    // Transition image mips from 1 to rest to general layout
    {
        // Create a command buffer
        grfx::CommandBufferPtr cmdBuffer;
        PPX_CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
        // Record command buffer
        PPX_CHECKED_CALL(cmdBuffer->Begin());
        cmdBuffer->TransitionImageLayout(targetImage, 1, mipLevelCount - 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        PPX_CHECKED_CALL(cmdBuffer->End());
        // Submitt to queue
        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &cmdBuffer;
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;
        submitInfo.pFence               = nullptr;
        PPX_CHECKED_CALL(pQueue->Submit(&submitInfo));
    }

    // Requiered to setup compute shader
    grfx::ShaderModulePtr        computeShader;
    grfx::PipelineInterfacePtr   computePipelineInterface;
    grfx::ComputePipelinePtr     computePipeline;
    grfx::DescriptorSetLayoutPtr computeDescriptorSetLayout;
    grfx::DescriptorPoolPtr      descriptorPool;
    grfx::DescriptorSetPtr       computeDescriptorSet;
    grfx::BufferPtr              uniformBuffer;
    grfx::SamplerPtr             sampler;

    { // Uniform buffer
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateBuffer(&bufferCreateInfo, &uniformBuffer));
    }

    { // Sampler
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateSampler(&samplerCreateInfo, &sampler));
    }

    { // Descriptors
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.storageImage                   = 1;
        poolCreateInfo.uniformBuffer                  = 1;
        poolCreateInfo.sampledImage                   = 1;
        poolCreateInfo.sampler                        = 1;

        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateDescriptorPool(&poolCreateInfo, &descriptorPool));

        { // Shader inputs
            grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE));
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(3, grfx::DESCRIPTOR_TYPE_SAMPLER));

            PPX_CHECKED_CALL(pQueue->GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &computeDescriptorSetLayout));

            PPX_CHECKED_CALL(pQueue->GetDevice()->AllocateDescriptorSet(descriptorPool, computeDescriptorSetLayout, &computeDescriptorSet));

            grfx::WriteDescriptor write = {};
            write.binding               = 1;
            write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.bufferOffset          = 0;
            write.bufferRange           = PPX_WHOLE_SIZE;
            write.pBuffer               = uniformBuffer;
            PPX_CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));

            write          = {};
            write.binding  = 3;
            write.type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
            write.pSampler = sampler;
            PPX_CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
        }
    }

    // Compute pipeline
    {
        grfx::Api         api = pQueue->GetDevice()->GetApi();
        std::vector<char> bytecode;
        switch (api) {
            case grfx::API_VK_1_1:
            case grfx::API_VK_1_2:
                bytecode = {std::begin(GenerateMipShaderVK), std::end(GenerateMipShaderVK)};
                break;
            case grfx::API_DX_12_0:
            case grfx::API_DX_12_1:
                bytecode = {std::begin(GenerateMipShaderDX), std::end(GenerateMipShaderDX)};
                break;
            default:
                PPX_ASSERT_MSG(false, "CS shader for this API is not present");
        }

        PPX_ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateShaderModule(&shaderCreateInfo, &computeShader));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = computeDescriptorSetLayout;
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreatePipelineInterface(&piCreateInfo, &computePipelineInterface));

        grfx::ComputePipelineCreateInfo cpCreateInfo = {};
        cpCreateInfo.CS                              = {computeShader.Get(), "CSMain"};
        cpCreateInfo.pPipelineInterface              = computePipelineInterface;
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateComputePipeline(&cpCreateInfo, &computePipeline));
    }

    // Prepare data for CS
    int srcCurrentWidth  = pBitmap->GetWidth();
    int srcCurrentHeight = pBitmap->GetHeight();

    // For the uniform (constant) data
    struct alignas(16) ShaderConstantData
    {
        float texel_size[2]; // 1.0 / srcTex.Dimensions
        int   src_mip_level;
        // Case to filter according the parity of the dimensions in the src texture.
        // Must be one of 0, 1, 2 or 3
        // See CSMain function bellow
        int dimension_case;
        // Ignored for now, if we want to use a different filter strategy. Current one is bi-linear filter
        int filter_option;
    };

    // Generate the rest of the mips
    for (uint32_t i = 1; i < mipLevelCount; ++i) {
        grfx::StorageImageViewPtr storageImageView;
        grfx::SampledImageViewPtr sampledImageView;

        { // Pass uniform data into shader
            ShaderConstantData constants;
            // Calculate current texel size
            constants.texel_size[0] = 1.0f / float(srcCurrentWidth);
            constants.texel_size[1] = 1.0f / float(srcCurrentHeight);
            // Calculate current dimension case
            // If width is even
            if ((srcCurrentWidth % 2) == 0) {
                // Test the height
                constants.dimension_case = (srcCurrentHeight % 2) == 0 ? 0 : 1;
            }
            else { // width is odd
                // Test the height
                constants.dimension_case = (srcCurrentHeight % 2) == 0 ? 2 : 3;
            }
            constants.src_mip_level = i - 1; // We calculate mip level i with level i - 1
            constants.filter_option = 1;     // Ignored for now, defaults to bilinear
            void* pData             = nullptr;
            PPX_CHECKED_CALL(uniformBuffer->MapMemory(0, &pData));
            memcpy(pData, &constants, sizeof(constants));
            uniformBuffer->UnmapMemory();
        }

        { // Storage Image view
            grfx::StorageImageViewCreateInfo storageViewCreateInfo = grfx::StorageImageViewCreateInfo::GuessFromImage(targetImage);
            storageViewCreateInfo.mipLevel                         = i;
            storageViewCreateInfo.mipLevelCount                    = 1;

            PPX_CHECKED_CALL(pQueue->GetDevice()->CreateStorageImageView(&storageViewCreateInfo, &storageImageView));

            grfx::WriteDescriptor write = {};
            write.binding               = 0;
            write.type                  = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageView            = storageImageView;
            PPX_CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
        }

        { // Sampler Image View
            grfx::SampledImageViewCreateInfo sampledViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(targetImage);
            sampledViewCreateInfo.mipLevel                         = i - 1;
            sampledViewCreateInfo.mipLevelCount                    = 1;

            PPX_CHECKED_CALL(pQueue->GetDevice()->CreateSampledImageView(&sampledViewCreateInfo, &sampledImageView));

            grfx::WriteDescriptor write = {};
            write.binding               = 2;
            write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView            = sampledImageView;
            PPX_CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
        }

        { // Create a command buffer
            grfx::CommandBufferPtr cmdBuffer;
            PPX_CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
            // Record command buffer
            PPX_CHECKED_CALL(cmdBuffer->Begin());
            cmdBuffer->BindComputeDescriptorSets(computePipelineInterface, 1, &computeDescriptorSet);
            cmdBuffer->BindComputePipeline(computePipeline);
            // Update width and height for the next iteration
            srcCurrentWidth  = srcCurrentWidth > 1 ? srcCurrentWidth / 2 : 1;
            srcCurrentHeight = srcCurrentHeight > 1 ? srcCurrentHeight / 2 : 1;
            // Launch the CS once per dst size (which is half of src size)
            cmdBuffer->Dispatch(srcCurrentWidth, srcCurrentHeight, 1);
            PPX_CHECKED_CALL(cmdBuffer->End());
            // Submitt to queue
            grfx::SubmitInfo submitInfo     = {};
            submitInfo.commandBufferCount   = 1;
            submitInfo.ppCommandBuffers     = &cmdBuffer;
            submitInfo.waitSemaphoreCount   = 0;
            submitInfo.ppWaitSemaphores     = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.ppSignalSemaphores   = nullptr;
            submitInfo.pFence               = nullptr;

            PPX_CHECKED_CALL(pQueue->Submit(&submitInfo));
            PPX_CHECKED_CALL(pQueue->WaitIdle());
        }

        { // Transition i-th mip back to shader resource
            // Create a command buffer
            grfx::CommandBufferPtr cmdBuffer;
            PPX_CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
            // Record into command buffer
            PPX_CHECKED_CALL(cmdBuffer->Begin());
            cmdBuffer->TransitionImageLayout(targetImage, i, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
            PPX_CHECKED_CALL(cmdBuffer->End());
            // Submitt to queue
            grfx::SubmitInfo submitInfo     = {};
            submitInfo.commandBufferCount   = 1;
            submitInfo.ppCommandBuffers     = &cmdBuffer;
            submitInfo.waitSemaphoreCount   = 0;
            submitInfo.ppWaitSemaphores     = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.ppSignalSemaphores   = nullptr;
            submitInfo.pFence               = nullptr;
            PPX_CHECKED_CALL(pQueue->Submit(&submitInfo));
            PPX_CHECKED_CALL(pQueue->WaitIdle());
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetImage->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppImage = targetImage;

    return ppx::SUCCESS;
}

bool IsDDSFile(const std::filesystem::path& path)
{
    return (std::strstr(path.string().c_str(), ".dds") != nullptr || std::strstr(path.string().c_str(), ".ktx") != nullptr);
}

struct MipLevel
{
    uint32_t width;
    uint32_t height;
    uint32_t bufferWidth;
    uint32_t bufferHeight;
    uint32_t srcRowStride;
    uint32_t dstRowStride;
    size_t   offset;
};

Result CreateImageFromCompressedImage(
    grfx::Queue*        pQueue,
    const gli::texture& image,
    grfx::Image**       ppImage,
    const ImageOptions& options)
{
    Result ppxres;

    PPX_LOG_INFO("Target type: " << grfx::ToString(image.target()) << "\n");
    PPX_LOG_INFO("Format: " << grfx::ToString(image.format()) << "\n");
    PPX_LOG_INFO("Swizzles: " << image.swizzles()[0] << ", " << image.swizzles()[1] << ", " << image.swizzles()[2] << ", " << image.swizzles()[3] << "\n");
    PPX_LOG_INFO("Layer information:\n"
                 << "\tBase layer: " << image.base_layer() << "\n"
                 << "\tMax layer: " << image.max_layer() << "\n"
                 << "\t# of layers: " << image.layers() << "\n");
    PPX_LOG_INFO("Face information:\n"
                 << "\tBase face: " << image.base_face() << "\n"
                 << "\tMax face: " << image.max_face() << "\n"
                 << "\t# of faces: " << image.faces() << "\n");
    PPX_LOG_INFO("Level information:\n"
                 << "\tBase level: " << image.base_level() << "\n"
                 << "\tMax level: " << image.max_level() << "\n"
                 << "\t# of levels: " << image.levels() << "\n");
    PPX_LOG_INFO("Image extents by level:\n");
    for (gli::texture::size_type level = 0; level < image.levels(); level++) {
        PPX_LOG_INFO("\textent(level == " << level << "): [" << image.extent(level)[0] << ", " << image.extent(level)[1] << ", " << image.extent(level)[2] << "]\n");
    }
    PPX_LOG_INFO("Total image size (bytes): " << image.size() << "\n");
    PPX_LOG_INFO("Image size by level:\n");
    for (gli::texture::size_type i = 0; i < image.levels(); i++) {
        PPX_LOG_INFO("\tsize(level == " << i << "): " << image.size(i) << "\n");
    }
    PPX_LOG_INFO("Image data pointer: " << image.data() << "\n");

    PPX_ASSERT_MSG((image.target() == gli::TARGET_2D), "Expecting a 2D DDS image.");

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Cap mip level count
    const grfx::Format format           = ToGrfxFormat(image.format());
    const uint32_t     maxMipLevelCount = std::min<uint32_t>(options.mMipLevelCount, static_cast<uint32_t>(image.levels()));
    const uint32_t     imageWidth       = static_cast<uint32_t>(image.extent(0)[0]);
    const uint32_t     imageHeight      = static_cast<uint32_t>(image.extent(0)[1]);

    // Row stride and texture offset alignment to handle DX's requirements
    const uint32_t rowStrideAlignment = grfx::IsDx12(pQueue->GetDevice()->GetApi()) ? PPX_D3D12_TEXTURE_DATA_PITCH_ALIGNMENT : 1;
    const uint32_t offsetAlignment    = grfx::IsDx12(pQueue->GetDevice()->GetApi()) ? PPX_D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT : 1;
    const uint32_t bytesPerTexel      = grfx::GetFormatDescription(format)->bytesPerTexel;
    const uint32_t blockWidth         = grfx::GetFormatDescription(format)->blockWidth;

    // Create staging buffer
    grfx::BufferPtr stagingBuffer;
    PPX_LOG_INFO("Storage size for image: " << image.size() << " bytes\n");
    PPX_LOG_INFO("Is image compressed: " << (gli::is_compressed(image.format()) ? "YES" : "NO"));

    grfx::BufferCreateInfo ci      = {};
    ci.size                        = 0;
    ci.usageFlags.bits.transferSrc = true;
    ci.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

    // Compute each mipmap level size and alignments.
    // This step filters out levels too small to match minimal alignment.
    std::vector<MipLevel> levelSizes;
    for (gli::texture::size_type level = 0; level < maxMipLevelCount; level++) {
        MipLevel ls;
        ls.width  = static_cast<uint32_t>(image.extent(level)[0]);
        ls.height = static_cast<uint32_t>(image.extent(level)[1]);
        // Stop when mipmaps are becoming too small to respect the format alignment.
        // The DXT* format documentation says texture sizes must be a multiple of 4.
        // For some reason, tools like imagemagick can generate mipmaps with a size < 4.
        // We need to ignore those.
        if (ls.width < blockWidth || ls.height < blockWidth) {
            break;
        }

        // If the DDS file contains textures which size is not a multiple of 4, something is wrong.
        // Since imagemagick can create invalid mipmap levels, I'd assume it can also create invalid
        // textures with non-multiple-of-4 sizes. Asserting to catch those.
        if (ls.width % blockWidth != 0 || ls.height % blockWidth != 0) {
            PPX_LOG_ERROR("Compressed textures width & height must be a multiple of the block size.");
            return ERROR_IMAGE_INVALID_FORMAT;
        }

        // Compute pitch for this format.
        // See https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
        const uint32_t blockRowByteSize = (bytesPerTexel * blockWidth) / (blockWidth * blockWidth);
        const uint32_t rowStride        = (ls.width * blockRowByteSize);

        ls.bufferWidth  = ls.width;
        ls.bufferHeight = ls.height;
        ls.srcRowStride = rowStride;
        ls.dstRowStride = RoundUp<uint32_t>(ls.srcRowStride, rowStrideAlignment);

        ls.offset = ci.size;
        ci.size += (image.size(level) / ls.srcRowStride) * ls.dstRowStride;
        ci.size = RoundUp<uint64_t>(ci.size, offsetAlignment);
        levelSizes.emplace_back(std::move(ls));
    }
    const uint32_t mipmapLevelCount = CountU32(levelSizes);
    PPX_ASSERT_MSG(mipmapLevelCount > 0, "Requested texture size too small for the chosen format.");

    ppxres = pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer);
    if (Failed(ppxres)) {
        return ppxres;
    }
    SCOPED_DESTROYER.AddObject(stagingBuffer);

    // Map and copy to staging buffer
    void* pBufferAddress = nullptr;
    ppxres               = stagingBuffer->MapMemory(0, &pBufferAddress);
    if (Failed(ppxres)) {
        return ppxres;
    }

    for (size_t level = 0; level < mipmapLevelCount; level++) {
        auto& ls = levelSizes[level];

        const char* pSrc = static_cast<const char*>(image.data(0, 0, level));
        char*       pDst = static_cast<char*>(pBufferAddress) + ls.offset;
        for (uint32_t row = 0; row * ls.srcRowStride < image.size(level); row++) {
            const char* pSrcRow = pSrc + row * ls.srcRowStride;
            char*       pDstRow = pDst + row * ls.dstRowStride;
            memcpy(pDstRow, pSrcRow, ls.srcRowStride);
        }
    }

    stagingBuffer->UnmapMemory();

    // Create target image
    grfx::ImagePtr targetImage;
    {
        grfx::ImageCreateInfo ci       = {};
        ci.type                        = grfx::IMAGE_TYPE_2D;
        ci.width                       = imageWidth;
        ci.height                      = imageHeight;
        ci.depth                       = 1;
        ci.format                      = format;
        ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount               = mipmapLevelCount;
        ci.arrayLayerCount             = 1;
        ci.usageFlags.bits.transferDst = true;
        ci.usageFlags.bits.sampled     = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;

        ci.usageFlags.flags |= options.mAdditionalUsage;

        ppxres = pQueue->GetDevice()->CreateImage(&ci, &targetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetImage);
    }

    std::vector<grfx::BufferToImageCopyInfo> copyInfos(mipmapLevelCount);
    for (gli::texture::size_type level = 0; level < mipmapLevelCount; level++) {
        auto& ls       = levelSizes[level];
        auto& copyInfo = copyInfos[level];

        // Copy info
        copyInfo.srcBuffer.imageWidth      = ls.bufferWidth;
        copyInfo.srcBuffer.imageHeight     = ls.bufferHeight;
        copyInfo.srcBuffer.imageRowStride  = ls.dstRowStride;
        copyInfo.srcBuffer.footprintOffset = ls.offset;
        copyInfo.srcBuffer.footprintWidth  = ls.bufferWidth;
        copyInfo.srcBuffer.footprintHeight = ls.bufferHeight;
        copyInfo.srcBuffer.footprintDepth  = 1;
        copyInfo.dstImage.mipLevel         = static_cast<uint32_t>(level);
        copyInfo.dstImage.arrayLayer       = 0;
        copyInfo.dstImage.arrayLayerCount  = 1;
        copyInfo.dstImage.x                = 0;
        copyInfo.dstImage.y                = 0;
        copyInfo.dstImage.z                = 0;
        copyInfo.dstImage.width            = ls.width;
        copyInfo.dstImage.height           = ls.height;
        copyInfo.dstImage.depth            = 1;
    }

    // Copy to GPU image
    ppxres = pQueue->CopyBufferToImage(
        copyInfos,
        stagingBuffer,
        targetImage,
        PPX_ALL_SUBRESOURCES,
        grfx::RESOURCE_STATE_UNDEFINED,
        grfx::RESOURCE_STATE_SHADER_RESOURCE);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Change ownership to reference so object doesn't get destroyed
    targetImage->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppImage = targetImage;

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateImageFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Image**                ppImage,
    const ImageOptions&          options,
    bool                         useGpu)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppImage);

    ScopedTimer timer("Image creation from file '" + path.string() + "'");

    Result ppxres;
    if (Bitmap::IsBitmapFile(path)) {
        // Load bitmap
        Bitmap bitmap;
        ppxres = Bitmap::LoadFile(path, &bitmap);
        if (Failed(ppxres)) {
            return ppxres;
        }

        if (useGpu) {
            ppxres = CreateImageFromBitmapGpu(pQueue, &bitmap, ppImage, options);
        }
        else {
            ppxres = CreateImageFromBitmap(pQueue, &bitmap, ppImage, options);
        }

        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    else if (IsDDSFile(path)) {
        // Generate a bitmap out of a DDS
        gli::texture image = gli::load(path.string().c_str());
        if (image.empty()) {
            return Result::ERROR_IMAGE_FILE_LOAD_FAILED;
        }
        PPX_LOG_INFO("Successfully loaded compressed image: " << path);
        ppxres = CreateImageFromCompressedImage(pQueue, image, ppImage, options);
    }
    else {
        ppxres = Result::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    if (ppxres != Result::SUCCESS) {
        PPX_LOG_INFO("Failed to create image from image file: " << path);
    }
    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CopyBitmapToTexture(
    grfx::Queue*        pQueue,
    const Bitmap*       pBitmap,
    grfx::Texture*      pTexture,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    grfx::ResourceState stateBefore,
    grfx::ResourceState stateAfter)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pBitmap);
    PPX_ASSERT_NULL_ARG(pTexture);

    Result ppxres = CopyBitmapToImage(
        pQueue,
        pBitmap,
        pTexture->GetImage(),
        mipLevel,
        arrayLayer,
        stateBefore,
        stateAfter);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateTextureFromBitmap(
    grfx::Queue*          pQueue,
    const Bitmap*         pBitmap,
    grfx::Texture**       ppTexture,
    const TextureOptions& options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pBitmap);
    PPX_ASSERT_NULL_ARG(ppTexture);

    Result ppxres = ppx::ERROR_FAILED;

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Cap mip level count
    uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
    uint32_t mipLevelCount    = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

    // Create target texture
    grfx::TexturePtr targetTexture;
    {
        grfx::TextureCreateInfo ci      = {};
        ci.pImage                       = nullptr;
        ci.imageType                    = grfx::IMAGE_TYPE_2D;
        ci.width                        = pBitmap->GetWidth();
        ci.height                       = pBitmap->GetHeight();
        ci.depth                        = 1;
        ci.imageFormat                  = ToGrfxFormat(pBitmap->GetFormat());
        ci.sampleCount                  = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount                = mipLevelCount;
        ci.arrayLayerCount              = 1;
        ci.usageFlags.bits.transferDst  = true;
        ci.usageFlags.bits.sampled      = true;
        ci.memoryUsage                  = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.initialState                 = options.mInitialState;
        ci.RTVClearValue                = {{0, 0, 0, 0}};
        ci.DSVClearValue                = {1.0f, 0xFF};
        ci.sampledImageViewType         = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
        ci.sampledImageViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.pSampledImageYcbcrConversion = options.mYcbcrConversion;
        ci.renderTargetViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.depthStencilViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.storageImageViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.ownership                    = grfx::OWNERSHIP_REFERENCE;

        ci.usageFlags.flags |= options.mAdditionalUsage;

        ppxres = pQueue->GetDevice()->CreateTexture(&ci, &targetTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetTexture);
    }

    // Since this mipmap is temporary, it's safe to use the static pool.
    Mipmap mipmap = Mipmap(*pBitmap, mipLevelCount, /* useStaticPool= */ true);
    if (!mipmap.IsOk()) {
        return ppx::ERROR_FAILED;
    }

    // Copy mips to texture
    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        const Bitmap* pMip = mipmap.GetMip(mipLevel);

        ppxres = CopyBitmapToTexture(
            pQueue,
            pMip,
            targetTexture,
            mipLevel,
            0,
            options.mInitialState,
            options.mInitialState);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetTexture->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppTexture = targetTexture;

    return ppx::SUCCESS;
}

Result CreateTextureFromMipmap(
    grfx::Queue*          pQueue,
    const Mipmap*         pMipmap,
    grfx::Texture**       ppTexture,
    const TextureOptions& options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pMipmap);
    PPX_ASSERT_NULL_ARG(ppTexture);

    Result ppxres = ppx::ERROR_FAILED;

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Cap mip level count
    auto pMip0 = pMipmap->GetMip(0);

    // Create target texture
    grfx::TexturePtr targetTexture;
    {
        grfx::TextureCreateInfo ci      = {};
        ci.pImage                       = nullptr;
        ci.imageType                    = grfx::IMAGE_TYPE_2D;
        ci.width                        = pMip0->GetWidth();
        ci.height                       = pMip0->GetHeight();
        ci.depth                        = 1;
        ci.imageFormat                  = ToGrfxFormat(pMip0->GetFormat());
        ci.sampleCount                  = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount                = pMipmap->GetLevelCount();
        ci.arrayLayerCount              = 1;
        ci.usageFlags.bits.transferDst  = true;
        ci.usageFlags.bits.sampled      = true;
        ci.memoryUsage                  = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.initialState                 = options.mInitialState;
        ci.RTVClearValue                = {{0, 0, 0, 0}};
        ci.DSVClearValue                = {1.0f, 0xFF};
        ci.sampledImageViewType         = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
        ci.sampledImageViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.pSampledImageYcbcrConversion = options.mYcbcrConversion;
        ci.renderTargetViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.depthStencilViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.storageImageViewFormat       = grfx::FORMAT_UNDEFINED;
        ci.ownership                    = grfx::OWNERSHIP_REFERENCE;

        ci.usageFlags.flags |= options.mAdditionalUsage;

        ppxres = pQueue->GetDevice()->CreateTexture(&ci, &targetTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetTexture);
    }

    // Copy mips to texture
    for (uint32_t mipLevel = 0; mipLevel < pMipmap->GetLevelCount(); ++mipLevel) {
        const Bitmap* pMip = pMipmap->GetMip(mipLevel);

        ppxres = CopyBitmapToTexture(
            pQueue,
            pMip,
            targetTexture,
            mipLevel,
            0,
            options.mInitialState,
            options.mInitialState);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetTexture->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppTexture = targetTexture;

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateTextureFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Texture**              ppTexture,
    const TextureOptions&        options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppTexture);

    ScopedTimer timer("Texture creation from image file '" + path.string() + "'");

    // Load bitmap
    Bitmap bitmap;
    Result ppxres = Bitmap::LoadFile(path, &bitmap);
    if (Failed(ppxres)) {
        return ppxres;
    }
    return CreateTextureFromBitmap(pQueue, &bitmap, ppTexture, options);
}

// -------------------------------------------------------------------------------------------------

struct SubImage
{
    uint32_t width        = 0;
    uint32_t height       = 0;
    uint32_t bufferOffset = 0;
};

SubImage CalcSubimageCrossHorizontalLeft(
    uint32_t     subImageIndex,
    uint32_t     imageWidth,
    uint32_t     imageHeight,
    grfx::Format format)
{
    uint32_t cellPixelsX = imageWidth / 4;
    uint32_t cellPixelsY = imageHeight / 3;
    uint32_t cellX       = 0;
    uint32_t cellY       = 0;
    switch (subImageIndex) {
        default: break;

        case 0: {
            cellX = 1;
            cellY = 0;
        } break;

        case 1: {
            cellX = 0;
            cellY = 1;
        } break;

        case 2: {
            cellX = 1;
            cellY = 1;
        } break;

        case 3: {
            cellX = 2;
            cellY = 1;
        } break;

        case 4: {
            cellX = 3;
            cellY = 1;
        } break;

        case 5: {
            cellX = 1;
            cellY = 2;

        } break;
    }

    uint32_t pixelStride  = grfx::GetFormatDescription(format)->bytesPerTexel;
    uint32_t pixelOffsetX = cellX * cellPixelsX * pixelStride;
    uint32_t pixelOffsetY = cellY * cellPixelsY * imageWidth * pixelStride;

    SubImage subImage     = {};
    subImage.width        = cellPixelsX;
    subImage.height       = cellPixelsY;
    subImage.bufferOffset = pixelOffsetX + pixelOffsetY;

    return subImage;
}

Result CreateIBLTexturesFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Texture**              ppIrradianceTexture,
    grfx::Texture**              ppEnvironmentTexture)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppIrradianceTexture);
    PPX_ASSERT_NULL_ARG(ppEnvironmentTexture);

    auto fileBytes = ppx::fs::load_file(path);
    if (!fileBytes.has_value()) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    auto is = std::istringstream(std::string(fileBytes.value().data(), fileBytes.value().size()));
    if (!is.good()) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    std::filesystem::path irrFile;
    is >> irrFile;

    std::filesystem::path envFile;
    is >> envFile;

    uint32_t baseWidth = 0;
    is >> baseWidth;

    uint32_t baseHeight = 0;
    is >> baseHeight;

    uint32_t levelCount = 0;
    is >> levelCount;

    if (irrFile.empty() || envFile.empty() || (baseWidth == 0) || (baseHeight == 0) || (levelCount == 0)) {
        return ppx::ERROR_IMAGE_FILE_LOAD_FAILED;
    }

    // Create irradiance texture - does not require mip maps
    std::filesystem::path irrFilePath = path.parent_path() / irrFile;
    Result                ppxres;
    {
        ScopedTimer timer("Texture creation from file '" + irrFilePath.string() + "'");
        ppxres = CreateTextureFromFile(pQueue, irrFilePath, ppIrradianceTexture);
    }
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Load IBL environment map - this is stored as a bitmap on disk
    std::filesystem::path envFilePath = path.parent_path() / envFile;
    ScopedTimer           timer("Texture creation from mipmap file '" + envFilePath.string() + "'");
    Mipmap                mipmap = {};
    ppxres                       = Mipmap::LoadFile(envFilePath, baseWidth, baseHeight, &mipmap, levelCount);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Create environment texture
    return CreateTextureFromMipmap(pQueue, &mipmap, ppEnvironmentTexture);
}

// -------------------------------------------------------------------------------------------------

Result CreateCubeMapFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    const CubeMapCreateInfo*     pCreateInfo,
    grfx::Image**                ppImage,
    const grfx::ImageUsageFlags& additionalImageUsage)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppImage);
    ScopedTimer timer("Cubemap creation from file '" + path.string() + "'");

    // Load bitmap
    Bitmap bitmap;
    Result ppxres = Bitmap::LoadFile(path, &bitmap);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Create staging buffer
    grfx::BufferPtr stagingBuffer;
    {
        uint64_t bitmapFootprintSize = bitmap.GetFootprintSize();

        grfx::BufferCreateInfo ci      = {};
        ci.size                        = bitmapFootprintSize;
        ci.usageFlags.bits.transferSrc = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

        ppxres = pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(stagingBuffer);

        // Map and copy to staging buffer
        void* pBufferAddress = nullptr;
        ppxres               = stagingBuffer->MapMemory(0, &pBufferAddress);
        if (Failed(ppxres)) {
            return ppxres;
        }
        std::memcpy(pBufferAddress, bitmap.GetData(), bitmapFootprintSize);
        stagingBuffer->UnmapMemory();
    }

    // Target format
    grfx::Format targetFormat = grfx::FORMAT_R8G8B8A8_UNORM;

    PPX_ASSERT_MSG(bitmap.GetWidth() * 3 == bitmap.GetHeight() * 4, "cubemap texture dimension must be a multiple of 4x3");
    // Calculate subImage to use for target image dimensions
    SubImage tmpSubImage = CalcSubimageCrossHorizontalLeft(0, bitmap.GetWidth(), bitmap.GetHeight(), targetFormat);

    PPX_ASSERT_MSG(tmpSubImage.width == tmpSubImage.height, "cubemap face width != height");
    // Create target image
    grfx::ImagePtr targetImage;
    {
        grfx::ImageCreateInfo ci       = {};
        ci.type                        = grfx::IMAGE_TYPE_CUBE;
        ci.width                       = tmpSubImage.width;
        ci.height                      = tmpSubImage.height;
        ci.depth                       = 1;
        ci.format                      = targetFormat;
        ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount               = 1;
        ci.arrayLayerCount             = 6;
        ci.usageFlags.bits.transferDst = true;
        ci.usageFlags.bits.sampled     = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;

        ci.usageFlags.flags |= additionalImageUsage.flags;

        ppxres = pQueue->GetDevice()->CreateImage(&ci, &targetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetImage);
    }

    // Copy to GPU image
    //
    {
        uint32_t faces[6] = {
            pCreateInfo->posX,
            pCreateInfo->negX,
            pCreateInfo->posY,
            pCreateInfo->negY,
            pCreateInfo->posZ,
            pCreateInfo->negZ,
        };

        std::vector<grfx::BufferToImageCopyInfo> copyInfos(6);
        for (uint32_t arrayLayer = 0; arrayLayer < 6; ++arrayLayer) {
            uint32_t subImageIndex = faces[arrayLayer];
            SubImage subImage      = CalcSubimageCrossHorizontalLeft(subImageIndex, bitmap.GetWidth(), bitmap.GetHeight(), targetFormat);

            // Copy info
            grfx::BufferToImageCopyInfo& copyInfo = copyInfos[arrayLayer];
            copyInfo.srcBuffer.imageWidth         = bitmap.GetWidth();
            copyInfo.srcBuffer.imageHeight        = bitmap.GetHeight();
            copyInfo.srcBuffer.imageRowStride     = bitmap.GetRowStride();
            copyInfo.srcBuffer.footprintOffset    = subImage.bufferOffset;
            copyInfo.srcBuffer.footprintWidth     = subImage.width;
            copyInfo.srcBuffer.footprintHeight    = subImage.height;
            copyInfo.srcBuffer.footprintDepth     = 1;
            copyInfo.dstImage.mipLevel            = 0;
            copyInfo.dstImage.arrayLayer          = arrayLayer;
            copyInfo.dstImage.arrayLayerCount     = 1;
            copyInfo.dstImage.x                   = 0;
            copyInfo.dstImage.y                   = 0;
            copyInfo.dstImage.z                   = 0;
            copyInfo.dstImage.width               = subImage.width;
            copyInfo.dstImage.height              = subImage.height;
            copyInfo.dstImage.depth               = 1;
        }

        ppxres = pQueue->CopyBufferToImage(
            copyInfos,
            stagingBuffer,
            targetImage,
            PPX_ALL_SUBRESOURCES,
            grfx::RESOURCE_STATE_UNDEFINED,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetImage->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppImage = targetImage;

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateMeshFromGeometry(
    grfx::Queue*    pQueue,
    const Geometry* pGeometry,
    grfx::Mesh**    ppMesh)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pGeometry);
    PPX_ASSERT_NULL_ARG(ppMesh);

    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

    // Create staging buffer
    grfx::BufferPtr stagingBuffer;
    {
        uint32_t biggestBufferSize = pGeometry->GetLargestBufferSize();

        grfx::BufferCreateInfo ci      = {};
        ci.size                        = biggestBufferSize;
        ci.usageFlags.bits.transferSrc = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

        Result ppxres = pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(stagingBuffer);
    }

    // Create target mesh
    grfx::MeshPtr targetMesh;
    {
        grfx::MeshCreateInfo ci = grfx::MeshCreateInfo(*pGeometry);

        Result ppxres = pQueue->GetDevice()->CreateMesh(&ci, &targetMesh);
        if (Failed(ppxres)) {
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetMesh);
    }

    // Copy geometry data to mesh
    {
        // Copy info
        grfx::BufferToBufferCopyInfo copyInfo = {};

        // Index buffer
        if (pGeometry->GetIndexType() != grfx::INDEX_TYPE_UNDEFINED) {
            const Geometry::Buffer* pGeoBuffer = pGeometry->GetIndexBuffer();
            PPX_ASSERT_NULL_ARG(pGeoBuffer);

            uint32_t geoBufferSize = pGeoBuffer->GetSize();

            Result ppxres = stagingBuffer->CopyFromSource(geoBufferSize, pGeoBuffer->GetData());
            if (Failed(ppxres)) {
                return ppxres;
            }

            copyInfo.size = geoBufferSize;

            // Copy to GPU buffer
            ppxres = pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetMesh->GetIndexBuffer(), grfx::RESOURCE_STATE_INDEX_BUFFER, grfx::RESOURCE_STATE_INDEX_BUFFER);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }

        // Vertex buffers
        uint32_t vertexBufferCount = pGeometry->GetVertexBufferCount();
        for (uint32_t i = 0; i < vertexBufferCount; ++i) {
            const Geometry::Buffer* pGeoBuffer = pGeometry->GetVertexBuffer(i);
            PPX_ASSERT_NULL_ARG(pGeoBuffer);

            uint32_t geoBufferSize = pGeoBuffer->GetSize();

            Result ppxres = stagingBuffer->CopyFromSource(geoBufferSize, pGeoBuffer->GetData());
            if (Failed(ppxres)) {
                return ppxres;
            }

            copyInfo.size = geoBufferSize;

            grfx::BufferPtr targetBuffer = targetMesh->GetVertexBuffer(i);

            // Copy to GPU buffer
            ppxres = pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetBuffer, grfx::RESOURCE_STATE_VERTEX_BUFFER, grfx::RESOURCE_STATE_VERTEX_BUFFER);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }
    }

    // Change ownership to reference so object doesn't get destroyed
    targetMesh->SetOwnership(grfx::OWNERSHIP_REFERENCE);

    // Assign output
    *ppMesh = targetMesh;

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateMeshFromTriMesh(
    grfx::Queue*   pQueue,
    const TriMesh* pTriMesh,
    grfx::Mesh**   ppMesh)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pTriMesh);
    PPX_ASSERT_NULL_ARG(ppMesh);

    Result ppxres = ppx::ERROR_FAILED;

    Geometry geo;
    ppxres = Geometry::Create(*pTriMesh, &geo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = CreateMeshFromGeometry(pQueue, &geo, ppMesh);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateMeshFromWireMesh(
    grfx::Queue*    pQueue,
    const WireMesh* pWireMesh,
    grfx::Mesh**    ppMesh)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(pWireMesh);
    PPX_ASSERT_NULL_ARG(ppMesh);

    Result ppxres = ppx::ERROR_FAILED;

    Geometry geo;
    ppxres = Geometry::Create(*pWireMesh, &geo);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = CreateMeshFromGeometry(pQueue, &geo, ppMesh);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result CreateMeshFromFile(
    grfx::Queue*                 pQueue,
    const std::filesystem::path& path,
    grfx::Mesh**                 ppMesh,
    const TriMeshOptions&        options)
{
    PPX_ASSERT_NULL_ARG(pQueue);
    PPX_ASSERT_NULL_ARG(ppMesh);

    TriMesh mesh = TriMesh::CreateFromOBJ(path, options);

    Result ppxres = CreateMeshFromTriMesh(pQueue, &mesh, ppMesh);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------

Result LoadFramesFromRawVideo(
    const std::filesystem::path&    path,
    grfx::Format                    format,
    uint32_t                        width,
    uint32_t                        height,
    std::vector<std::vector<char>>* pFrames)
{
    PPX_ASSERT_NULL_ARG(pFrames);

    const grfx::FormatDesc* formatDesc = grfx::GetFormatDescription(format);
    if (formatDesc == nullptr) {
        PPX_LOG_ERROR("Failed to fetch information for texture format " << format);
        return ppx::ERROR_FAILED;
    }

    uint32_t frameSize = 0; // As measured in bytes, not pixels.
    if (formatDesc->isPlanar) {
        std::optional<grfx::FormatPlaneDesc> formatPlanes = grfx::GetFormatPlaneDescription(format);
        PPX_ASSERT_MSG(formatPlanes.has_value(), "No planes found for format " << format);
        frameSize = GetPlanarImageSizeInBytes(*formatDesc, *formatPlanes, width, height);
    }
    else {
        frameSize = formatDesc->bytesPerTexel * width * height;
    }

    ppx::fs::File file;
    if (!file.Open(path)) {
        PPX_ASSERT_MSG(false, "Cannot open the file!");
        return ppx::ERROR_FAILED;
    }
    const size_t fileSize = file.GetLength();

    std::vector<char> buffer(frameSize);
    size_t            totalRead = 0;
    while (totalRead < fileSize) {
        const size_t bytesRead = file.Read(buffer.data(), frameSize);
        if (bytesRead < frameSize) {
            // If we didn't read as many bytes as we expected to, and we haven't
            // reached the end of the file, this is an error.
            if (totalRead + bytesRead < fileSize) {
                PPX_ASSERT_MSG(
                    false,
                    "Unable to load video frame; expected " << frameSize << " but read " << bytesRead << "bytes (previously read " << totalRead << ").");
                return ppx::ERROR_FAILED;
            }
            // Otherwise, fill the rest of the buffer with 0s.
            else {
                PPX_LOG_WARN("Read " << bytesRead << " bytes for the last frame of the video at " << path << "; filling the rest of the frame with 0s.");
                std::fill(buffer.begin() + bytesRead, buffer.end(), 0);
            }
        }
        pFrames->push_back(std::move(buffer));
        totalRead += bytesRead;
    }

    return ppx::SUCCESS;
}

} // namespace grfx_util
} // namespace ppx
