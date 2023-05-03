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

#include "Config.hlsli"
#include "ppx/PBR.hlsli"

ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER, SCENE_DATA_SPACE);

Texture2D    AlbedoTex      : register(ALBEDO_TEXTURE_REGISTER,     MATERIAL_RESOURCES_SPACE);
Texture2D    RoughnessTex   : register(ROUGHNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    MetalnessTex   : register(METALNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    NormalMapTex   : register(NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_RESOURCES_SPACE);
Texture2D    IrrMapTex      : register(IRR_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
Texture2D    EnvMapTex      : register(ENV_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
Texture2D    BRDFLUTTex     : register(BRDF_LUT_TEXTURE_REGISTER,   MATERIAL_RESOURCES_SPACE);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER,    MATERIAL_RESOURCES_SPACE);

float4 psmain(VSOutput input) : SV_TARGET
{
    float3 P = input.positionWS;
    float3 N = normalize(input.normal);
    float3 V = normalize(Scene.eyePosition.xyz - input.positionWS);

    float3 albedo = Material.albedo;
    if (Material.albedoSelect == 1) {
        albedo = AlbedoTex.Sample(ClampedSampler, input.texCoord).rgb;
        albedo = RemoveGamma(albedo, 2.2);
    }

    float perceptualRoughness = Material.roughness;
    if (Material.roughnessSelect == 1) {
        perceptualRoughness = RoughnessTex.Sample(ClampedSampler, input.texCoord).r;
    }

    float metalness = Material.metalness;
    if (Material.metalnessSelect == 1) {
        metalness = MetalnessTex.Sample(ClampedSampler, input.texCoord).r;
    }

    if (Material.normalSelect == 1) {
        float3   nTS = normalize(input.normalTS);
        float3   tTS = normalize(input.tangentTS);
        float3   bTS = normalize(input.bitangnetTS);
        float3x3 TBN = float3x3(tTS.x, bTS.x, nTS.x,
                                tTS.y, bTS.y, nTS.y,
                                tTS.z, bTS.z, nTS.z);

        // Read normal map value in tangent space
        float3 normal = (NormalMapTex.Sample(ClampedSampler, input.texCoord).rgb * 2.0) - 1.0;

        // Transform from tangent space to world space
        N = mul(TBN, normal);

        // Normalize the transformed normal
        N = normalize(N);
    }

    // Remap
    float3 diffuseColor = CalculateDiffuseColor(albedo, metalness);
    float roughness = CalculateRoughness(perceptualRoughness);
    float dielectricness = CalculateDielectricness(metalness);

    // Calculate F0
    float specularReflectance = 0.5;
    float3 F0 = CalculateF0(albedo, metalness, specularReflectance);

    // Calculate reflection vector and cosine angle bewteen N and V
    float3 R = reflect(-V, N);
    float  NoV = saturate(dot(N, V));

    // Calculate direct lighting
    float3 directLighting = 0;
    for (uint i = 0; i < Scene.lightCount; ++i) {
        float3 L = normalize(Lights[i].position - P);
        float3 Lc = Lights[i].color;
        float  Ls = Lights[i].intensity;

        float3 color = Light_GGX_Direct(
            P,
            N,
            V,
            R,
            L,
            Lc,
            Ls,
            F0,
            diffuseColor,
            roughness,
            metalness,
            dielectricness,
            specularReflectance,
            NoV);

        directLighting += color;
    }

    // Calculate indirect lighting
    float3 indirectLighting = Light_GGX_Indirect(
        Scene.envLevelCount,
        EnvMapTex,
        IrrMapTex,
        ClampedSampler,
        BRDFLUTTex,
        ClampedSampler,
        N,
        V,
        R,
        F0,
        diffuseColor,
        roughness,
        metalness,
        dielectricness,
        specularReflectance,
        NoV);

    // Final color
    float3 finalColor = directLighting + indirectLighting;

    // Reapply gamma
    finalColor = ApplyGamma(finalColor, 2.2);

    return float4(finalColor, 1);
}
