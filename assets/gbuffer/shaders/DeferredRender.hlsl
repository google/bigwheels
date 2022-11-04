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


#include "GBuffer.hlsli"
#include "Config.hlsli"

#if defined(PPX_D3D11) // -----------------------------------------------------
cbuffer Scene : register(SCENE_CONSTANTS_REGISTER)
{
    SceneData Scene;
};

cbuffer Material : register(MATERIAL_CONSTANTS_REGISTER)
{
    MaterialData Material;
};

cbuffer Model : register(MODEL_CONSTANTS_REGISTER)
{
    ModelData Model;
};

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER);

Texture2D    AlbedoTex      : register(MATERIAL_ALBEDO_TEXTURE_REGISTER);
Texture2D    RoughnessTex   : register(MATERIAL_ROUGHNESS_TEXTURE_REGISTER);
Texture2D    MetalnessTex   : register(MATERIAL_METALNESS_TEXTURE_REGISTER);
Texture2D    NormalMapTex   : register(MATERIAL_NORMAL_MAP_TEXTURE_REGISTER);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER);

#else // --- D3D12 / Vulkan ----------------------------------------------------

ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
StructuredBuffer<Light>      Lights   : register(LIGHT_DATA_REGISTER,         SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);

Texture2D    AlbedoTex      : register(MATERIAL_ALBEDO_TEXTURE_REGISTER,     MATERIAL_RESOURCES_SPACE);
Texture2D    RoughnessTex   : register(MATERIAL_ROUGHNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    MetalnessTex   : register(MATERIAL_METALNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    NormalMapTex   : register(MATERIAL_NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_RESOURCES_SPACE);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER,             MATERIAL_RESOURCES_SPACE);

#endif // -- defined (PPX_D3D11) -----------------------------------------------

PackedGBuffer psmain(VSOutput input)
{
    GBuffer gbuffer  = (GBuffer)0;
    gbuffer.position = input.positionWS;
    
    float3 normal = normalize(input.normal);
    if (Material.normalSelect == 1) {
        float3   nTS = normalize(input.normalTS);
        float3   tTS = normalize(input.tangentTS);
        float3   bTS = normalize(input.bitangnetTS);
        float3x3 TBN = float3x3(tTS.x, bTS.x, nTS.x,
                                tTS.y, bTS.y, nTS.y,
                                tTS.z, bTS.z, nTS.z);
        
        // Read normal map value in tangent space
        normal = (NormalMapTex.Sample(ClampedSampler, input.texCoord).rgb * 2.0f) - 1.0;

        // Transform from tangent space to world space
        normal = mul(TBN, normal);
        
        // Normalize the transformed normal
        normal = normalize(normal);        
    }    
    gbuffer.normal = normal;
    
	gbuffer.albedo = Material.albedo;
	if (Material.albedoSelect == 1) {
		gbuffer.albedo = AlbedoTex.Sample(ClampedSampler, input.texCoord).rgb;
	}
    
    gbuffer.F0 = Material.F0;
    
    gbuffer.roughness = Material.roughness;
    if (Material.roughnessSelect == 1) {
        gbuffer.roughness = RoughnessTex.Sample(ClampedSampler, input.texCoord).r;
    }
    
    gbuffer.metalness = Material.metalness;
    if (Material.metalnessSelect == 1) {
        gbuffer.metalness = MetalnessTex.Sample(ClampedSampler, input.texCoord).r;
    }
    
    gbuffer.ambOcc      = 1;    
    gbuffer.iblStrength = Material.iblStrength;
    gbuffer.envStrength = Material.envStrength;
        
    PackedGBuffer packed = PackGBuffer(gbuffer);
    return packed;
}