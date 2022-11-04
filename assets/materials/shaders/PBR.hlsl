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

//
// Simple PBR implementation based on https://github.com/Nadrin/PBR (MIT license)
// 
//
#include "Config.hlsli"

#if defined (PPX_D3D11) // -----------------------------------------------------
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

Texture2D    AlbedoTex      : register(ALBEDO_TEXTURE_REGISTER);
Texture2D    RoughnessTex   : register(ROUGHNESS_TEXTURE_REGISTER);
Texture2D    MetalnessTex   : register(METALNESS_TEXTURE_REGISTER);
Texture2D    NormalMapTex   : register(NORMAL_MAP_TEXTURE_REGISTER);
Texture2D    IBLTex         : register(IBL_MAP_TEXTURE_REGISTER);
Texture2D    EnvMapTex      : register(ENV_MAP_TEXTURE_REGISTER);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER);
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER, SCENE_DATA_SPACE);

Texture2D    AlbedoTex      : register(ALBEDO_TEXTURE_REGISTER,     MATERIAL_RESOURCES_SPACE);
Texture2D    RoughnessTex   : register(ROUGHNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    MetalnessTex   : register(METALNESS_TEXTURE_REGISTER,  MATERIAL_RESOURCES_SPACE);
Texture2D    NormalMapTex   : register(NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_RESOURCES_SPACE);
Texture2D    IBLTex         : register(IBL_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
Texture2D    EnvMapTex      : register(ENV_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER,    MATERIAL_RESOURCES_SPACE);
#endif // -- defined (PPX_D3D11) -----------------------------------------------

//
// GGX/Towbridge-Reitz normal distribution function with
// Disney's reparametrization of alpha = roughness^2
//
float DistributionGGX(float cosLh, float roughness)
{
    float alpha   = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom   = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

//
// Single term for separable Schlick-GGX below
//
float GASchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

//
// Schlick-GGX approximation of geometric attenuation function using Smith's method
//
float GASmithSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights
    return GASchlickG1(cosLi, k) * GASchlickG1(cosLo, k);
}

//
// Shlick's approximation of the Fresnel factor
//
float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * (float3)pow(1.0 - cosTheta, 5.0);
}

float3 Environment(Texture2D tex, float3 coord, float lod)
{
    float2 uv = CartesianToSphereical(normalize(coord));
    uv.x = saturate(uv.x / (2.0 * PI));
    uv.y = saturate(uv.y / PI);   
    float3 color = tex.SampleLevel(ClampedSampler, uv, lod).rgb;
    return color;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    float3 N  = normalize(input.normal);  
    float3 V  = normalize(Scene.eyePosition.xyz - input.positionWS);
    
    float3 albedo = Material.albedo;
    if (Material.albedoSelect == 1) {
        albedo = AlbedoTex.Sample(ClampedSampler, input.texCoord).rgb;
    }
    
    float roughness = Material.roughness;
    if (Material.roughnessSelect == 1) {
        roughness = RoughnessTex.Sample(ClampedSampler, input.texCoord).r;
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
    
    float3 P     = input.positionWS;
    float3 E     = normalize(Scene.eyePosition - P);
    float3 R     = reflect(-V, N);
    float3 Lo    = E; // Outgoing light direction
    float  cosLo = saturate(dot(N, Lo));

    // The value for F0 is passed in from the application. If in doubt use 0.04 for 
    // dieletric materials and scale beween 0.04 and the albedo based on the metalness
    // value.
    //
    float3 F0 = Material.F0;
    F0        = lerp(F0, albedo, metalness);

    // Calculate direct lighting
    float3 directLighting = (float3)0;
    for (uint i = 0; i < Scene.lightCount; ++i) {
        float3 Li   = normalize(Lights[i].position - P); // Incoming light direction
        float  Lrad = Lights[i].intensity;               // Light radiance
        float3 Lh   = normalize(Li + Lo);                // Half-vector between Li and Lo
        float  cosLi = saturate(dot(N, Li));
        float  cosLh = saturate(dot(N, Lh));

        // Calculate Fresnel term for direct lighting 
        float3 F = FresnelSchlick(F0, saturate(dot(Lh, Lo)));
        // Calculate normal distribution for specular BRDF
        float  D = DistributionGGX(cosLh, roughness);
        // Calculate geometric attenuation for specular BRDF
        float  G = GASmithSchlickGGX(cosLi, cosLo, roughness);

        // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
        // Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
        // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
        //
        float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);

        // clang-format off
        // 
        // Lambert diffuse BRDF
        // 
        // We don't scale by 1/PI for lighting & material units to be more convenient.
        //   See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        //
        // clang-format on
        float3 diffuseBRDF = kD * albedo;

        // Calculate specular BRDF
        float3 specularBRDF = (F * D * G) / max(0.00001, 4.0 * cosLi * cosLo);
    
        // Accumulate light's total contribution
        directLighting += (diffuseBRDF + specularBRDF) * Lrad * cosLi;
    }

    // Calculate indirect lighting
    //
    // There's no attempt here to conserve energy for "correctness". The diffuse 
    // and specular components are calculated independently to strive for a more 
    // appealing look.
    //
    float3 indirectLighting = (float3)1;
    {
        float3 irradiance = (float3)0;
        float3 kS         = (float3)0;
        if (Material.iblSelect == 1) {
            float maxLevel = (Scene.iblLevelCount - 1);
            float lod      = roughness * maxLevel;
            irradiance     = Environment(IBLTex, R, lod) * Material.iblStrength;

            kS = Environment(IBLTex, R,maxLevel) * Material.iblStrength;
        }

        // clang-format off
        // 
        // Calculate Fresnel term for indirect lighting
        // 
        // Since we use prefiltered images and irradiance is coming from many directions
        // use cosLo instead of angle with light's half-vector (cosLh)
        //    See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
        //
        // clang-format on
        float3 F = FresnelSchlick(F0, cosLo);

        // Get diffuse contribution factor (as with direct lighting)
        float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);

        // Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either
        float3 diffuseIBL = kD * albedo * irradiance;

        // Hacky specular approximation
        float3 specularIBL = kS * F0 * roughness;

        indirectLighting = diffuseIBL + specularIBL;
    }

    // Simple environment reflection
    float3 envReflection = (float3)0.0;
    if (Material.envSelect) {
        float  lod    = roughness * (Scene.envLevelCount - 1);
        envReflection = Environment(EnvMapTex, R, lod) * (1 - roughness) * Material.envStrength;
    }

    float3 Co = directLighting + indirectLighting + envReflection + (float3)Scene.ambient;

    return float4(Co, 1);
}
