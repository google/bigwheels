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
Texture2D    IrrMapTex      : register(IRR_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
Texture2D    EnvMapTex      : register(ENV_MAP_TEXTURE_REGISTER,    MATERIAL_RESOURCES_SPACE);
Texture2D    BRDFLUTTex     : register(BRDF_LUT_TEXTURE_REGISTER,   MATERIAL_RESOURCES_SPACE);
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

float3 SampleIBLTexture(Texture2D tex, float3 dir, float lod)
{
    float2 uv = CartesianToSphereical(normalize(dir));
    uv.x = saturate(uv.x / (2.0 * PI));
    uv.y = saturate(uv.y / PI);   
    float3 color = tex.SampleLevel(ClampedSampler, uv, lod).rgb;
    return color;
}

float3 GetEnvDFGPolynomial(float3 specularColor, float gloss, float NoV)
{
    float x = gloss;
    float y = NoV;
    
    float b1 = -0.1688;
    float b2 = 1.895;
    float b3 = 0.9903;
    float b4 = -4.853;
    float b5 = 8.404;
    float b6 = -5.069;
    float bias = saturate( min( b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y ) );
    
    float d0 = 0.6045;
    float d1 = 1.699;
    float d2 = -0.5228;
    float d3 = -3.603;
    float d4 = 1.404;
    float d5 = 0.1939;
    float d6 = 2.661;
    float delta = saturate( d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x );
    float scale = delta - bias;
    
    bias *= saturate( 50.0 * specularColor.y );
    return specularColor * scale + bias;
}

float3 GetEnvDFGKaris(float3 specularColor, float roughness, float NoV)
{
    const float4 c0 = {-1, -0.0275, -0.572, 0.022};
    const float4 c1 = {1, 0.0425, 1.04, -0.04};
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
    return specularColor * AB.x + AB.y;
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

    // Calculate indirect lighting
    float3 indirectLighting = 0;
    {
        float3 F = Fresnel_SchlickRoughness(NoV, F0, roughness);
        float3 kD = (1.0 - F) * (1.0 - metalness);

        // Diffuse BRDF
        float3 irradiance = SampleIBLTexture(IrrMapTex, N, 0);
        float3 diffuse = irradiance * diffuseColor * Lambert();
        float3 diffuseBRDF = kD * diffuse;

        // Specular BRDF - normally a LUT is used for the BRDF integration
        // but to keep things fast for mobile we'll use an approximation.
        float  lod = Scene.envLevelCount * roughness;
        float3 prefilteredColor = SampleIBLTexture(EnvMapTex, R, lod);
        prefilteredColor = min(prefilteredColor, float3(10.0, 10.0, 10.0));
        // Use split-sum BRDF integration or approximation
        float3 specular = 0;
        if (Scene.useBRDFLUT) {
            float2 uv = float2(saturate(roughness), saturate(NoV));
            float2 envBRDF = BRDFLUTTex.Sample(ClampedSampler, uv).rg;
            specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
        }
        else {
            specular = F * GetEnvDFGKaris(prefilteredColor, roughness, NoV);
        }
        float3 dielectricSpecular = (1.0 - metalness) * specularReflectance * specular; // non-metal specular
        float3 metalSpecular = metalness * specular;
        float3 specularBRDF = dielectricSpecular + metalSpecular;

        // Combine diffuse and specular BRDFs
        indirectLighting += diffuseBRDF + specularBRDF;
    }

    // Final color
    float3 finalColor = directLighting + indirectLighting;

    // Reapply gamma
    finalColor = pow(finalColor, 1 / 2.2);

    return float4(finalColor, 1);
}
