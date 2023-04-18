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

float Distribution_GGX(float3 N, float3 H, float roughness)
{
    float NoH    = saturate(dot(N, H));
    float NoH2   = NoH * NoH;
    float alpha2 = max(roughness * roughness, EPSILON);
    float A      = NoH2 * (alpha2 - 1) + 1;
	return alpha2 / (PI * A * A);
}

float Geometry_SchlickBeckman(float NoV, float k)
{
	return NoV / (NoV * (1 - k) + k);
}

float Geometry_Smiths(float3 N, float3 V, float3 L,  float roughness)
{    
    float k   = pow(roughness + 1, 2) / 8.0; 
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));    
    float G1  = Geometry_SchlickBeckman(NoV, k);
    float G2  = Geometry_SchlickBeckman(NoL, k);
    return G1 * G2;
}

float3 Fresnel_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 r = (float3)(1 - roughness);
    return F0 + (max(r, F0) - F0) * pow(1 - cosTheta, 5);
}

float Lambert() 
{
    return 1.0 / PI;
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
    float3 P = input.positionWS;    
    float3 N = normalize(input.normal);  
    float3 V = normalize(Scene.eyePosition.xyz - input.positionWS);
    
    float3 albedo = Material.albedo;
    if (Material.albedoSelect == 1) {
        albedo = AlbedoTex.Sample(ClampedSampler, input.texCoord).rgb;
        // Remove gamma
        albedo = pow(albedo, 2.2);        
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

    // Remap
    float3 diffuseColor = (1.0 - metalness) * albedo;
    roughness = roughness * roughness;

    // Calculate F0
    float  specularReflectance = 0.5;
    float3 F0 = 0.16 * specularReflectance * specularReflectance * (1 - metalness) + albedo * metalness;    

    // Reflection and cosine angle bewteen N and V 
    float3 R = reflect(-V, N);
    float  NoV = saturate(dot(N, V));

    // Calculate direct lighting
    float3 directLighting = 0;
	for (uint i = 0; i < Scene.lightCount; ++i) {    
        // Light variables - world space
        float3 L   = normalize(Lights[i].position - P);
        float3 H   = normalize(L + V);
        float3 Lc  = Lights[i].color;
        float  Ls  = Lights[i].intensity;
        float3 radiance = Lc * Ls;
        float  NoL = saturate(dot(N, L));

        // Specular BRDF
        float  cosTheta = saturate(dot(H, V));        
        float  D = Distribution_GGX(N, H, roughness);
        float3 F = Fresnel_SchlickRoughness(cosTheta, F0, roughness);
        float  G = Geometry_Smiths(N, V, L, roughness);
        float  Vis = G / max(0.0001, (4.0 * NoV * NoL));       
        float3 specular = (D * F * Vis);
        float3 dielectricSpecular = (1.0 - metalness) * specularReflectance * specular; // non-metal specular
        float3 metalSpecular = metalness * specular;
        float3 specularBRDF = dielectricSpecular + metalSpecular;

        // Fresnel refelectance for diffuse 
        float3 kD = (1.0 - F) * (1.0 - metalness);

        // Diffuse BRDF
        float3 diffuse = diffuseColor * Lambert();
        float3 diffuseBRDF = kD * diffuse;

        // Combine diffuse and specular BRDFs
        float3 finalBRDF = (diffuseBRDF + specularBRDF) * radiance * NoL;

        directLighting += finalBRDF;
    }

    // TODO: Calculate indirect lighting
    float3 indirectLighting = 0;

    // Final color
    float3 finalColor = directLighting + indirectLighting;

    // Reapply gamma
    finalColor = pow(finalColor, 1 / 2.2);    

    return float4(finalColor, 1);  
}
