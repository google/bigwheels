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

#include "Material.h"
#include "Render.h"

#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_scope.h"
#include "ppx/application.h"
#include "ppx/graphics_util.h"
using namespace ppx;

#include <map>
#include <filesystem>

const float  F0_Generic         = 0.04f;
const float3 F0_MetalTitanium   = float3(0.542f, 0.497f, 0.449f);
const float3 F0_MetalChromium   = float3(0.549f, 0.556f, 0.554f);
const float3 F0_MetalIron       = float3(0.562f, 0.565f, 0.578f);
const float3 F0_MetalNickel     = float3(0.660f, 0.609f, 0.526f);
const float3 F0_MetalPlatinum   = float3(0.673f, 0.637f, 0.585f);
const float3 F0_MetalCopper     = float3(0.955f, 0.638f, 0.538f);
const float3 F0_MetalPalladium  = float3(0.733f, 0.697f, 0.652f);
const float3 F0_MetalZinc       = float3(0.664f, 0.824f, 0.850f);
const float3 F0_MetalGold       = float3(1.022f, 0.782f, 0.344f);
const float3 F0_MetalAluminum   = float3(0.913f, 0.922f, 0.924f);
const float3 F0_MetalSilver     = float3(0.972f, 0.960f, 0.915f);
const float3 F0_DiletricWater   = float3(0.020f);
const float3 F0_DiletricPlastic = float3(0.040f);
const float3 F0_DiletricGlass   = float3(0.045f);
const float3 F0_DiletricCrystal = float3(0.050f);
const float3 F0_DiletricGem     = float3(0.080f);
const float3 F0_DiletricDiamond = float3(0.150f);

ppx::grfx::TexturePtr             Material::s1x1BlackTexture;
ppx::grfx::TexturePtr             Material::s1x1WhiteTexture;
ppx::grfx::SamplerPtr             Material::sClampedSampler;
ppx::grfx::DescriptorSetLayoutPtr Material::sMaterialResourcesLayout;
ppx::grfx::DescriptorSetLayoutPtr Material::sMaterialDataLayout;

Material Material::sWood;
Material Material::sTiles;

static std::map<std::string, grfx::TexturePtr> mTextureCache;

static Result LoadTexture(grfx::Queue* pQueue, const std::filesystem::path& path, grfx::Texture** ppTexture)
{
    if (!std::filesystem::exists(path)) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    // Try to load from cache
    auto it = mTextureCache.find(path.string());
    if (it != std::end(mTextureCache)) {
        *ppTexture = it->second;
    }
    else {
        grfx_util::TextureOptions textureOptions = grfx_util::TextureOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(pQueue, path, ppTexture, textureOptions));
    }

    return ppx::SUCCESS;
}

ppx::Result Material::Create(ppx::grfx::Queue* pQueue, ppx::grfx::DescriptorPool* pPool, const MaterialCreateInfo* pCreateInfo)
{
    Result ppxres = ppx::ERROR_FAILED;

    grfx::Device* pDevice = pQueue->GetDevice();

    // Material constants temp buffer
    grfx::BufferPtr    tmpCpuMaterialConstants;
    MaterialConstants* pMaterialConstants = nullptr;
    {
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_CONSTANT_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(pDevice->CreateBuffer(&bufferCreateInfo, &tmpCpuMaterialConstants));

        PPX_CHECKED_CALL(tmpCpuMaterialConstants->MapMemory(0, reinterpret_cast<void**>(&pMaterialConstants)));
    }

    // Values
    pMaterialConstants->F0          = pCreateInfo->F0;
    pMaterialConstants->albedo      = pCreateInfo->albedo;
    pMaterialConstants->roughness   = pCreateInfo->roughness;
    pMaterialConstants->metalness   = pCreateInfo->metalness;
    pMaterialConstants->iblStrength = pCreateInfo->iblStrength;
    pMaterialConstants->envStrength = pCreateInfo->envStrength;

    // Albedo texture
    mAlbedoTexture = s1x1WhiteTexture;
    if (!pCreateInfo->albedoTexturePath.empty()) {
        PPX_CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->albedoTexturePath, &mAlbedoTexture));
        pMaterialConstants->albedoSelect = 1;
    }

    // Roughness texture
    mRoughnessTexture = s1x1BlackTexture;
    if (!pCreateInfo->roughnessTexturePath.empty()) {
        PPX_CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->roughnessTexturePath, &mRoughnessTexture));
        pMaterialConstants->roughnessSelect = 1;
    }

    // Metalness texture
    mMetalnessTexture = s1x1BlackTexture;
    if (!pCreateInfo->metalnessTexturePath.empty()) {
        PPX_CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->metalnessTexturePath, &mMetalnessTexture));
        pMaterialConstants->metalnessSelect = 1;
    }

    // Normal map texture
    mNormalMapTexture = s1x1BlackTexture;
    if (!pCreateInfo->normalTexturePath.empty()) {
        PPX_CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->normalTexturePath, &mNormalMapTexture));
        pMaterialConstants->normalSelect = 1;
    }

    // Unmap since we're done
    tmpCpuMaterialConstants->UnmapMemory();

    // Copy material constants to GPU buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = tmpCpuMaterialConstants->GetSize();
        bufferCreateInfo.usageFlags.bits.transferDst   = true;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(pDevice->CreateBuffer(&bufferCreateInfo, &mMaterialConstants));

        grfx::BufferToBufferCopyInfo copyInfo = {tmpCpuMaterialConstants->GetSize()};

        ppxres = pQueue->CopyBufferToBuffer(&copyInfo, tmpCpuMaterialConstants, mMaterialConstants, grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Allocate descriptor sets
    PPX_CHECKED_CALL(pDevice->AllocateDescriptorSet(pPool, sMaterialResourcesLayout, &mMaterialResourcesSet));
    PPX_CHECKED_CALL(pDevice->AllocateDescriptorSet(pPool, sMaterialDataLayout, &mMaterialDataSet));
    mMaterialResourcesSet->SetName("Material Resource");
    mMaterialDataSet->SetName("Material Data");

    // Update material resource descriptors
    {
        grfx::WriteDescriptor writes[5] = {};
        writes[0].binding               = MATERIAL_ALBEDO_TEXTURE_REGISTER;
        writes[0].arrayIndex            = 0;
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageView            = mAlbedoTexture->GetSampledImageView();
        writes[1].binding               = MATERIAL_ROUGHNESS_TEXTURE_REGISTER;
        writes[1].arrayIndex            = 0;
        writes[1].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView            = mRoughnessTexture->GetSampledImageView();
        writes[2].binding               = MATERIAL_METALNESS_TEXTURE_REGISTER;
        writes[2].arrayIndex            = 0;
        writes[2].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2].pImageView            = mMetalnessTexture->GetSampledImageView();
        writes[3].binding               = MATERIAL_NORMAL_MAP_TEXTURE_REGISTER;
        writes[3].arrayIndex            = 0;
        writes[3].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[3].pImageView            = mNormalMapTexture->GetSampledImageView();
        writes[4].binding               = CLAMPED_SAMPLER_REGISTER;
        writes[4].type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[4].pSampler              = sClampedSampler;

        PPX_CHECKED_CALL(mMaterialResourcesSet->UpdateDescriptors(5, writes));
    }

    // Update material data descriptors
    {
        grfx::WriteDescriptor write = {};
        write.binding               = MATERIAL_CONSTANTS_REGISTER;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mMaterialConstants;
        PPX_CHECKED_CALL(mMaterialDataSet->UpdateDescriptors(1, &write));
    }

    return ppx::SUCCESS;
}

void Material::Destroy()
{
}

ppx::Result Material::CreateMaterials(ppx::grfx::Queue* pQueue, ppx::grfx::DescriptorPool* pPool)
{
    grfx::Device* pDevice = pQueue->GetDevice();

    // Create 1x1 black and white textures
    {
        PPX_CHECKED_CALL(grfx_util::CreateTexture1x1(pQueue, float4(0), &s1x1BlackTexture));
        PPX_CHECKED_CALL(grfx_util::CreateTexture1x1(pQueue, float4(1), &s1x1WhiteTexture));
    }

    // Create sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        PPX_CHECKED_CALL(pDevice->CreateSampler(&createInfo, &sClampedSampler));
    }

    // Material resources layout
    {
        // clang-format off
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_ALBEDO_TEXTURE_REGISTER,     grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_ROUGHNESS_TEXTURE_REGISTER,  grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_METALNESS_TEXTURE_REGISTER,  grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_NORMAL_MAP_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        createInfo.bindings.push_back({grfx::DescriptorBinding{CLAMPED_SAMPLER_REGISTER,             grfx::DESCRIPTOR_TYPE_SAMPLER,       1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        PPX_CHECKED_CALL(pDevice->CreateDescriptorSetLayout(&createInfo, &sMaterialResourcesLayout));
        // clang-format on
    }

    // Material data layout
    {
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_CONSTANTS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        PPX_CHECKED_CALL(pDevice->CreateDescriptorSetLayout(&createInfo, &sMaterialDataLayout));
    }

    // Wood
    {
        MaterialCreateInfo createInfo   = {};
        createInfo.F0                   = F0_Generic;
        createInfo.albedo               = F0_DiletricPlastic;
        createInfo.roughness            = 1;
        createInfo.metalness            = 0;
        createInfo.iblStrength          = 0;
        createInfo.envStrength          = 0.0f;
        createInfo.albedoTexturePath    = Application::Get()->GetAssetPath("materials/textures/wood/albedo.png");
        createInfo.roughnessTexturePath = Application::Get()->GetAssetPath("materials/textures/wood/roughness.png");
        createInfo.metalnessTexturePath = Application::Get()->GetAssetPath("materials/textures/wood/metalness.png");
        createInfo.normalTexturePath    = Application::Get()->GetAssetPath("materials/textures/wood/normal.png");

        PPX_CHECKED_CALL(sWood.Create(pQueue, pPool, &createInfo));
    }

    // Tiles
    {
        MaterialCreateInfo createInfo   = {};
        createInfo.F0                   = F0_Generic;
        createInfo.albedo               = F0_DiletricCrystal;
        createInfo.roughness            = 0.4f;
        createInfo.metalness            = 0;
        createInfo.iblStrength          = 0;
        createInfo.envStrength          = 0.0f;
        createInfo.albedoTexturePath    = Application::Get()->GetAssetPath("materials/textures/tiles/albedo.png");
        createInfo.roughnessTexturePath = Application::Get()->GetAssetPath("materials/textures/tiles/roughness.png");
        createInfo.metalnessTexturePath = Application::Get()->GetAssetPath("materials/textures/tiles/metalness.png");
        createInfo.normalTexturePath    = Application::Get()->GetAssetPath("materials/textures/tiles/normal.png");

        PPX_CHECKED_CALL(sTiles.Create(pQueue, pPool, &createInfo));
    }

    return ppx::SUCCESS;
}

void Material::DestroyMaterials()
{
}
