// Copyright 2023 Google LLC
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

#include "ppx/scene/scene_pipeline_args.h"
#include "ppx/scene/scene_material.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_instance.h"
#include "ppx/application.h"
#include "ppx/graphics_util.h"

namespace ppx {
namespace scene {

void CopyMaterialTextureParams(
    const std::unordered_map<const scene::Sampler*, uint32_t>& samplersIndexMap,
    const std::unordered_map<const scene::Image*, uint32_t>&   imagesIndexMap,
    const scene::TextureView&                                  srcTextureView,
    scene::MaterialTextureParams&                              dstTextureParams)
{
    // Set default values
    dstTextureParams.samplerIndex      = UINT32_MAX;
    dstTextureParams.textureIndex      = UINT32_MAX;
    dstTextureParams.texCoordTransform = srcTextureView.GetTexCoordTransform();

    // Bail if we don't have a texture
    if (IsNull(srcTextureView.GetTexture())) {
        return;
    }

    auto itSampler = samplersIndexMap.find(srcTextureView.GetTexture()->GetSampler());
    auto itImage   = imagesIndexMap.find(srcTextureView.GetTexture()->GetImage());
    if ((itSampler != samplersIndexMap.end()) && (itImage != imagesIndexMap.end())) {
        dstTextureParams.samplerIndex = itSampler->second;
        dstTextureParams.textureIndex = itImage->second;
    }
}

// -------------------------------------------------------------------------------------------------
// MaterialPipelineArgs
// -------------------------------------------------------------------------------------------------
MaterialPipelineArgs::MaterialPipelineArgs()
{
}

MaterialPipelineArgs::~MaterialPipelineArgs()
{
    if (mDescriptorSet) {
        auto pDevice = mDescriptorSet->GetDevice();
        pDevice->FreeDescriptorSet(mDescriptorSet.Get());
        mDescriptorSet.Reset();
    }

    if (mDescriptorSetLayout) {
        auto pDevice = mDescriptorSetLayout->GetDevice();
        pDevice->DestroyDescriptorSetLayout(mDescriptorSetLayout.Get());
        mDescriptorSetLayout.Reset();
    }

    if (mDescriptorPool) {
        auto pDevice = mDescriptorPool->GetDevice();
        pDevice->DestroyDescriptorPool(mDescriptorPool.Get());
        mDescriptorPool.Reset();
    }

    if (mCpuConstantParamsBuffer) {
        if (!IsNull(mConstantParamsMappedAddress)) {
            mCpuConstantParamsBuffer->UnmapMemory();
        }

        auto pDevice = mCpuConstantParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mCpuConstantParamsBuffer.Get());
        mCpuConstantParamsBuffer.Reset();
    }

    if (mGpuConstantParamsBuffer) {
        auto pDevice = mGpuConstantParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mGpuConstantParamsBuffer.Get());
        mGpuConstantParamsBuffer.Reset();
    }

    if (mCpuInstanceParamsBuffer) {
        if (!IsNull(mInstanceParamsMappedAddress)) {
            mCpuInstanceParamsBuffer->UnmapMemory();
        }

        auto pDevice = mCpuInstanceParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mCpuInstanceParamsBuffer.Get());
        mCpuInstanceParamsBuffer.Reset();
    }

    if (mGpuInstanceParamsBuffer) {
        auto pDevice = mGpuInstanceParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mGpuInstanceParamsBuffer.Get());
        mGpuInstanceParamsBuffer.Reset();
    }

    if (mCpuMateriaParamsBuffer) {
        if (!IsNull(mMaterialParamsMappedAddress)) {
            mCpuMateriaParamsBuffer->UnmapMemory();
        }

        auto pDevice = mCpuMateriaParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mCpuMateriaParamsBuffer.Get());
        mCpuMateriaParamsBuffer.Reset();
    }

    if (mGpuMateriaParamsBuffer) {
        auto pDevice = mGpuMateriaParamsBuffer->GetDevice();
        pDevice->DestroyBuffer(mGpuMateriaParamsBuffer.Get());
        mGpuMateriaParamsBuffer.Reset();
    }
}

ppx::Result MaterialPipelineArgs::InitializeDefaultObjects(grfx::Device* pDevice)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Default sampler
    {
        grfx::SamplerCreateInfo createInfo = {};

        auto ppxres = pDevice->CreateSampler(&createInfo, &mDefaultSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default texture
    {
        ppx::Bitmap bitmap;
        auto        ppxres = ppx::Bitmap::Create(1, 1, ppx::Bitmap::FORMAT_RGBA_UINT8, &bitmap);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // Fill purple
        bitmap.Fill<uint8_t>(0xFF, 0, 0xFF, 0xFF);

        ppxres = grfx_util::CreateTextureFromBitmap(pDevice->GetGraphicsQueue(), &bitmap, &mDefaultTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default BRDF LUT sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        auto ppxres = pDevice->CreateSampler(&createInfo, &mDefaultBRDFLUTSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default BRD LUT texture
    {
        // Favor speed, so use the PNG instad of the HDR
        auto ppxres = ppx::grfx_util::CreateTextureFromFile(
            pDevice->GetGraphicsQueue(),
            ppx::Application::Get()->GetAssetPath("common/textures/ppx/brdf_lut.png"),
            &mDefaultBRDFLUTTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default IBL irradiance sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        auto ppxres = pDevice->CreateSampler(&createInfo, &mDefaultIBLIrradianceSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default IBL environment sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.mipLodBias              = 0.65f;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = 1000.0f;

        auto ppxres = pDevice->CreateSampler(&createInfo, &mDefaultIBLEnvironmentSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Default IBL texture
    {
        ppx::Bitmap bitmap;
        auto        ppxres = ppx::Bitmap::Create(1, 1, ppx::Bitmap::FORMAT_RGBA_UINT8, &bitmap);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // Fill white
        bitmap.Fill<uint8_t>(0xFF, 0xFF, 0xFF, 0xFF);

        ppxres = grfx_util::CreateTextureFromBitmap(pDevice->GetGraphicsQueue(), &bitmap, &mDefaultIBLTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

ppx::Result MaterialPipelineArgs::InitializeDescriptorSet(grfx::Device* pDevice)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Descriptor bindings
    //
    // clang-format off
    std::vector<grfx::DescriptorBinding> bindings = {
        grfx::DescriptorBinding{FRAME_PARAMS_REGISTER,            grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER,       1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{CAMERA_PARAMS_REGISTER,           grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER,       1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{INSTANCE_PARAMS_REGISTER,         grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{MATERIAL_PARAMS_REGISTER,         grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{BRDF_LUT_SAMPLER_REGISTER,        grfx::DESCRIPTOR_TYPE_SAMPLER,              1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{BRDF_LUT_TEXTURE_REGISTER,        grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{IBL_IRRADIANCE_SAMPLER_REGISTER,  grfx::DESCRIPTOR_TYPE_SAMPLER,              1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{IBL_ENVIRONMENT_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER,              1, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{IBL_IRRADIANCE_MAP_REGISTER,      grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_IBL_MAPS, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{IBL_ENVIRONMENT_MAP_REGISTER,     grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_IBL_MAPS, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{MATERIAL_SAMPLERS_REGISTER,       grfx::DESCRIPTOR_TYPE_SAMPLER,              MAX_MATERIAL_SAMPLERS, grfx::SHADER_STAGE_ALL},
        grfx::DescriptorBinding{MATERIAL_TEXTURES_REGISTER,       grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_MATERIAL_TEXTURES, grfx::SHADER_STAGE_ALL},
    };
    // clang-format on

    // Create descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        //
        for (const auto& binding : bindings) {
            // clang-format off
            switch (binding.type) {
                case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER       : createInfo.uniformBuffer += binding.arrayCount; break;
                case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER : createInfo.structuredBuffer += binding.arrayCount; break;
                case grfx::DESCRIPTOR_TYPE_SAMPLER              : createInfo.sampler += binding.arrayCount; break;
                case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE        : createInfo.sampledImage += binding.arrayCount; break;
            }
            // clang-format on
        }

        auto ppxres = pDevice->CreateDescriptorPool(&createInfo, &mDescriptorPool);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Create descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings                            = bindings;

        auto ppxres = pDevice->CreateDescriptorSetLayout(&createInfo, &mDescriptorSetLayout);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Allocate descriptor set
    {
        auto ppxres = pDevice->AllocateDescriptorSet(mDescriptorPool.Get(), mDescriptorSetLayout.Get(), &mDescriptorSet);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

ppx::Result MaterialPipelineArgs::InitializeBuffers(grfx::Device* pDevice)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    static_assert((sizeof(scene::FrameParams) == FRAME_PARAMS_STRUCT_SIZE), "Invalid size for FrameParams");
    static_assert((sizeof(scene::CameraParams) == CAMERA_PARAMS_STRUCT_SIZE), "Invalid size for CameraParams");
    static_assert((sizeof(scene::InstanceParams) == INSTANCE_PARAMS_STRUCT_SIZE), "Invalid size for InstanceParams");
    static_assert((sizeof(scene::MaterialParams) == MATERIAL_PARAMS_STRUCT_SIZE), "Invalid size for MaterialParams");

    // ConstantBuffers
    {
        mFrameParamsPaddedSize  = RoundUp<uint32_t>(FRAME_PARAMS_STRUCT_SIZE, 256);
        mCameraParamsPaddedSize = RoundUp<uint32_t>(CAMERA_PARAMS_STRUCT_SIZE, 256);

        mFrameParamsOffset  = 0;
        mCameraParamsOffset = mFrameParamsPaddedSize;

        mTotalConstantParamsPaddedSize = mFrameParamsPaddedSize + mCameraParamsPaddedSize;

        grfx::BufferCreateInfo createInfo        = {};
        createInfo.size                          = mTotalConstantParamsPaddedSize;
        createInfo.usageFlags.bits.uniformBuffer = true;

        // CPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.usageFlags.bits.transferDst = false;
        //
        auto ppxres = pDevice->CreateBuffer(&createInfo, &mCpuConstantParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // GPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.usageFlags.bits.transferSrc = false;
        createInfo.usageFlags.bits.transferDst = true;
        //
        ppxres = pDevice->CreateBuffer(&createInfo, &mGpuConstantParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Instance params StructuredBuffers
    {
        mTotalInstanceParamsPaddedSize = RoundUp<uint32_t>(MAX_DRAWABLE_INSTANCES * INSTANCE_PARAMS_STRUCT_SIZE, 16);

        grfx::BufferCreateInfo createInfo             = {};
        createInfo.structuredElementStride            = INSTANCE_PARAMS_STRUCT_SIZE;
        createInfo.size                               = mTotalInstanceParamsPaddedSize;
        createInfo.usageFlags.bits.roStructuredBuffer = true;

        // CPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.usageFlags.bits.transferDst = false;
        //
        auto ppxres = pDevice->CreateBuffer(&createInfo, &mCpuInstanceParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // GPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.usageFlags.bits.transferSrc = false;
        createInfo.usageFlags.bits.transferDst = true;
        //
        ppxres = pDevice->CreateBuffer(&createInfo, &mGpuInstanceParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Material params StructuredBuffers
    {
        mTotalMaterialParamsPaddedSize = RoundUp<uint32_t>(MAX_UNIQUE_MATERIALS * MATERIAL_PARAMS_STRUCT_SIZE, 16);

        grfx::BufferCreateInfo createInfo             = {};
        createInfo.structuredElementStride            = MATERIAL_PARAMS_STRUCT_SIZE;
        createInfo.size                               = mTotalMaterialParamsPaddedSize;
        createInfo.usageFlags.bits.roStructuredBuffer = true;

        // CPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.usageFlags.bits.transferDst = false;
        //
        auto ppxres = pDevice->CreateBuffer(&createInfo, &mCpuMateriaParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }

        // GPU buffer
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.usageFlags.bits.transferSrc = false;
        createInfo.usageFlags.bits.transferDst = true;
        //
        ppxres = pDevice->CreateBuffer(&createInfo, &mGpuMateriaParamsBuffer);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

ppx::Result MaterialPipelineArgs::SetDescriptors(grfx::Device* pDevice)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    //
    // FRAME_PARAMS_REGISTER
    // CAMERA_PARAMS_REGISTER
    // INSTANCE_PARAMS_REGISTER
    // MATERIAL_PARAMS_REGISTER
    //
    {
        std::vector<grfx::WriteDescriptor> writes;

        // Frame
        grfx::WriteDescriptor write = {};
        write.binding               = FRAME_PARAMS_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = mFrameParamsOffset;
        write.bufferRange           = mFrameParamsPaddedSize;
        write.pBuffer               = mGpuConstantParamsBuffer;
        writes.push_back(write);

        // Camera
        write              = {};
        write.binding      = CAMERA_PARAMS_REGISTER;
        write.arrayIndex   = 0;
        write.type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset = mCameraParamsOffset;
        write.bufferRange  = mCameraParamsPaddedSize;
        write.pBuffer      = mGpuConstantParamsBuffer;
        writes.push_back(write);

        // Instances
        write                        = {};
        write.binding                = INSTANCE_PARAMS_REGISTER;
        write.arrayIndex             = 0;
        write.type                   = grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
        write.bufferOffset           = 0;
        write.bufferRange            = mTotalInstanceParamsPaddedSize;
        write.structuredElementCount = MAX_DRAWABLE_INSTANCES;
        write.pBuffer                = mGpuInstanceParamsBuffer;
        writes.push_back(write);

        // Materials
        write                        = {};
        write.binding                = MATERIAL_PARAMS_REGISTER;
        write.arrayIndex             = 0;
        write.type                   = grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
        write.bufferOffset           = 0;
        write.bufferRange            = mTotalMaterialParamsPaddedSize;
        write.structuredElementCount = MAX_UNIQUE_MATERIALS;
        write.pBuffer                = mGpuMateriaParamsBuffer;
        writes.push_back(write);

        auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    //
    // BRDF_LUT_SAMPLER_REGISTER
    // BRDF_LUT_TEXTURE_REGISTER
    //
    {
        std::vector<grfx::WriteDescriptor> writes;

        // BRDFLUTSampler
        grfx::WriteDescriptor write = {};
        write.binding               = BRDF_LUT_SAMPLER_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler              = mDefaultBRDFLUTSampler;
        writes.push_back(write);

        // BRDFLUTTexture
        write            = {};
        write.binding    = BRDF_LUT_TEXTURE_REGISTER;
        write.arrayIndex = 0;
        write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView = mDefaultBRDFLUTTexture->GetSampledImageView();
        writes.push_back(write);

        auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    //
    // IBL_IRRADIANCE_SAMPLER_REGISTER
    // IBL_ENVIRONMENT_SAMPLER_REGISTER
    // IBL_IRRADIANCE_MAP_REGISTER
    // IBL_ENVIRONMENT_MAP_REGISTER
    //
    {
        std::vector<grfx::WriteDescriptor> writes;

        // IBLIrradianceSampler
        grfx::WriteDescriptor write = {};
        write.binding               = IBL_IRRADIANCE_SAMPLER_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler              = mDefaultIBLIrradianceSampler;
        writes.push_back(write);

        // IBLEnvironmentSampler
        write            = {};
        write.binding    = IBL_ENVIRONMENT_SAMPLER_REGISTER;
        write.arrayIndex = 0;
        write.type       = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler   = mDefaultIBLEnvironmentSampler;
        writes.push_back(write);

        for (uint32_t i = 0; i < MAX_IBL_MAPS; ++i) {
            // IBLIrradianceMaps
            write            = {};
            write.binding    = IBL_IRRADIANCE_MAP_REGISTER;
            write.arrayIndex = i;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mDefaultIBLTexture->GetSampledImageView();
            writes.push_back(write);

            // IBLEnvironmentMaps
            write            = {};
            write.binding    = IBL_ENVIRONMENT_MAP_REGISTER;
            write.arrayIndex = i;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mDefaultIBLTexture->GetSampledImageView();
            writes.push_back(write);
        }

        auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    //
    // MATERIAL_SAMPLERS_REGISTER
    // MATERIAL_TEXTURES_REGISTER
    //
    {
        std::vector<grfx::WriteDescriptor> writes;

        // MaterialSamplers
        for (uint32_t i = 0; i < MAX_MATERIAL_SAMPLERS; ++i) {
            grfx::WriteDescriptor write = {};
            write.binding               = MATERIAL_SAMPLERS_REGISTER;
            write.arrayIndex            = i;
            write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
            write.pSampler              = mDefaultSampler;
            writes.push_back(write);
        }

        // MaterialTextures
        for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURES; ++i) {
            grfx::WriteDescriptor write = {};
            write.binding               = MATERIAL_TEXTURES_REGISTER;
            write.arrayIndex            = i;
            write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView            = mDefaultTexture->GetSampledImageView();
            writes.push_back(write);
        }

        auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

ppx::Result MaterialPipelineArgs::InitializeResource(grfx::Device* pDevice)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    auto ppxres = this->InitializeDefaultObjects(pDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = this->InitializeDescriptorSet(pDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = this->InitializeBuffers(pDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }

    ppxres = this->SetDescriptors(pDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Get constant params mapped address
    ppxres = mCpuConstantParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mConstantParamsMappedAddress));
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Set frame and camera params addresses
    mFrameParamsAddress  = reinterpret_cast<scene::FrameParams*>(mConstantParamsMappedAddress + mFrameParamsOffset);
    mCameraParamsAddress = reinterpret_cast<scene::CameraParams*>(mConstantParamsMappedAddress + mCameraParamsOffset);

    // Get instance params mapped address
    ppxres = mCpuInstanceParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mInstanceParamsMappedAddress));
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Get instance params mapped address
    ppxres = mCpuMateriaParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mMaterialParamsMappedAddress));
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result MaterialPipelineArgs::Create(grfx::Device* pDevice, scene::MaterialPipelineArgs** ppTargetPipelineArgs)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    scene::MaterialPipelineArgs* pPipelineArgs = new scene::MaterialPipelineArgs();
    if (IsNull(pPipelineArgs)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    auto ppxres = pPipelineArgs->InitializeResource(pDevice);
    if (Failed(ppxres)) {
        return ppxres;
    }

    *ppTargetPipelineArgs = pPipelineArgs;

    return ppx::SUCCESS;
}

void MaterialPipelineArgs::SetCameraParams(const ppx::Camera* pCamera)
{
    PPX_ASSERT_NULL_ARG(pCamera);

    auto pCameraParams = this->GetCameraParams();
    PPX_ASSERT_NULL_ARG(pCameraParams);

    pCameraParams->viewProjectionMatrix = pCamera->GetViewProjectionMatrix();
    pCameraParams->eyePosition          = pCamera->GetEyePosition();
    pCameraParams->nearDepth            = pCamera->GetNearClip();
    pCameraParams->viewDirection        = pCamera->GetViewDirection();
    pCameraParams->farDepth             = pCamera->GetFarClip();
}

scene::InstanceParams* MaterialPipelineArgs::GetInstanceParams(uint32_t index)
{
    if (IsNull(mInstanceParamsMappedAddress) || (index >= MAX_DRAWABLE_INSTANCES)) {
        return nullptr;
    }

    uint32_t offset = index * INSTANCE_PARAMS_STRUCT_SIZE;
    char*    ptr    = mInstanceParamsMappedAddress + offset;
    return reinterpret_cast<scene::InstanceParams*>(ptr);
}

scene::MaterialParams* MaterialPipelineArgs::GetMaterialParams(uint32_t index)
{
    if (IsNull(mMaterialParamsMappedAddress) || (index >= MAX_UNIQUE_MATERIALS)) {
        return nullptr;
    }

    uint32_t offset = index * MATERIAL_PARAMS_STRUCT_SIZE;
    char*    ptr    = mMaterialParamsMappedAddress + offset;
    return reinterpret_cast<scene::MaterialParams*>(ptr);
}

void MaterialPipelineArgs::SetIBLTextures(
    uint32_t                index,
    grfx::SampledImageView* pIrradiance,
    grfx::SampledImageView* pEnvironment)
{
    PPX_ASSERT_NULL_ARG(pIrradiance);
    PPX_ASSERT_NULL_ARG(pEnvironment);

    mDescriptorSet->UpdateSampledImage(IBL_IRRADIANCE_MAP_REGISTER, index, pIrradiance);
    mDescriptorSet->UpdateSampledImage(IBL_ENVIRONMENT_MAP_REGISTER, index, pEnvironment);
}

void MaterialPipelineArgs::SetMaterialSampler(uint32_t index, const scene::Sampler* pSampler)
{
    PPX_ASSERT_NULL_ARG(pSampler);

    mDescriptorSet->UpdateSampler(MATERIAL_SAMPLERS_REGISTER, index, pSampler->GetSampler());
}

void MaterialPipelineArgs::SetMaterialTexture(uint32_t index, const scene::Image* pImage)
{
    PPX_ASSERT_NULL_ARG(pImage);

    mDescriptorSet->UpdateSampledImage(MATERIAL_TEXTURES_REGISTER, index, pImage->GetImageView());
}

void MaterialPipelineArgs::CopyBuffers(grfx::CommandBuffer* pCmd)
{
    PPX_ASSERT_NULL_ARG(pCmd);

    // Constant params buffer
    {
        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.srcBuffer.offset             = 0;
        copyInfo.dstBuffer.offset             = 0;
        copyInfo.size                         = mTotalConstantParamsPaddedSize;

        pCmd->CopyBufferToBuffer(
            &copyInfo,
            mCpuConstantParamsBuffer.Get(),
            mGpuConstantParamsBuffer.Get());
    }

    // Instance params buffer
    {
        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.srcBuffer.offset             = 0;
        copyInfo.dstBuffer.offset             = 0;
        copyInfo.size                         = mTotalInstanceParamsPaddedSize;

        pCmd->CopyBufferToBuffer(
            &copyInfo,
            mCpuInstanceParamsBuffer.Get(),
            mGpuInstanceParamsBuffer.Get());
    }

    // Material params buffer
    {
        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.srcBuffer.offset             = 0;
        copyInfo.dstBuffer.offset             = 0;
        copyInfo.size                         = mTotalMaterialParamsPaddedSize;

        pCmd->CopyBufferToBuffer(
            &copyInfo,
            mCpuMateriaParamsBuffer.Get(),
            mGpuMateriaParamsBuffer.Get());
    }
}

} // namespace scene
} // namespace ppx
