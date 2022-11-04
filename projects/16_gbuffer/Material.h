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

#ifndef MATERIAL_H
#define MATERIAL_H

#include "ppx/math_config.h"
#include "ppx/grfx/grfx_pipeline.h"

#include <filesystem>

using ppx::float3;
using ppx::hlsl_float;
using ppx::hlsl_float2;
using ppx::hlsl_float3;
using ppx::hlsl_float4;
using ppx::hlsl_int;
using ppx::hlsl_int2;
using ppx::hlsl_int3;
using ppx::hlsl_uint;
using ppx::hlsl_uint2;
using ppx::hlsl_uint3;
using ppx::hlsl_uint4;

extern const float3 F0_MetalTitanium;
extern const float3 F0_MetalChromium;
extern const float3 F0_MetalIron;
extern const float3 F0_MetalNickel;
extern const float3 F0_MetalPlatinum;
extern const float3 F0_MetalCopper;
extern const float3 F0_MetalPalladium;
extern const float3 F0_MetalZinc;
extern const float3 F0_MetalGold;
extern const float3 F0_MetalAluminum;
extern const float3 F0_MetalSilver;
extern const float3 F0_DiletricWater;
extern const float3 F0_DiletricPlastic;
extern const float3 F0_DiletricGlass;
extern const float3 F0_DiletricCrystal;
extern const float3 F0_DiletricGem;
extern const float3 F0_DiletricDiamond;

PPX_HLSL_PACK_BEGIN();
struct MaterialConstants
{
    hlsl_float<4>   F0;
    hlsl_float3<12> albedo;
    hlsl_float<4>   roughness;
    hlsl_float<4>   metalness;
    hlsl_float<4>   iblStrength;
    hlsl_float<4>   envStrength;
    hlsl_uint<4>    albedoSelect;
    hlsl_uint<4>    roughnessSelect;
    hlsl_uint<4>    metalnessSelect;
    hlsl_uint<4>    normalSelect;
};
PPX_HLSL_PACK_END();

struct MaterialCreateInfo
{
    float         F0;
    float3        albedo;
    float         roughness;
    float         metalness;
    float         iblStrength;
    float         envStrength;
    std::filesystem::path albedoTexturePath;
    std::filesystem::path roughnessTexturePath;
    std::filesystem::path metalnessTexturePath;
    std::filesystem::path normalTexturePath;
};

class Material
{
public:
    Material() {}
    virtual ~Material() {}

    ppx::Result Create(ppx::grfx::Queue* pQueue, ppx::grfx::DescriptorPool* pPool, const MaterialCreateInfo* pCreateInfo);
    void        Destroy();

    static ppx::Result CreateMaterials(ppx::grfx::Queue* pQueue, ppx::grfx::DescriptorPool* pPool);
    static void        DestroyMaterials();

    static Material* GetMaterialWood() { return &sWood; }
    static Material* GetMaterialTiles() { return &sTiles; }

    static ppx::grfx::DescriptorSetLayout* GetMaterialResourcesLayout() { return sMaterialResourcesLayout.Get(); }
    static ppx::grfx::DescriptorSetLayout* GetMaterialDataLayout() { return sMaterialDataLayout.Get(); }

    ppx::grfx::DescriptorSet* GetMaterialResourceSet() const { return mMaterialResourcesSet.Get(); }
    ppx::grfx::DescriptorSet* GetMaterialDataSet() const { return mMaterialDataSet.Get(); }

private:
    ppx::grfx::BufferPtr        mMaterialConstants;
    ppx::grfx::TexturePtr       mAlbedoTexture;
    ppx::grfx::TexturePtr       mRoughnessTexture;
    ppx::grfx::TexturePtr       mMetalnessTexture;
    ppx::grfx::TexturePtr       mNormalMapTexture;
    ppx::grfx::DescriptorSetPtr mMaterialResourcesSet;
    ppx::grfx::DescriptorSetPtr mMaterialDataSet;

    static ppx::grfx::TexturePtr             s1x1BlackTexture;
    static ppx::grfx::TexturePtr             s1x1WhiteTexture;
    static ppx::grfx::SamplerPtr             sClampedSampler;
    static ppx::grfx::DescriptorSetLayoutPtr sMaterialResourcesLayout;
    static ppx::grfx::DescriptorSetLayoutPtr sMaterialDataLayout;

    static Material sWood;
    static Material sTiles;
};

#endif // MATERIAL_H
