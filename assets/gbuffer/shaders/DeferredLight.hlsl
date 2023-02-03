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

#define CUSTOM_VS_OUTPUT
#include "Config.hlsli"
#include "GBuffer.hlsli"

#if defined(PPX_D3D11) // -----------------------------------------------------
cbuffer Scene : register(SCENE_CONSTANTS_REGISTER)
{
    SceneData Scene;
};

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER);

Texture2D    GBufferRT0 : register(GBUFFER_RT0_REGISTER);
Texture2D    GBufferRT1 : register(GBUFFER_RT1_REGISTER);
Texture2D    GBufferRT2 : register(GBUFFER_RT2_REGISTER);
Texture2D    GBufferRT3 : register(GBUFFER_RT3_REGISTER);
Texture2D    IBLTex     : register(GBUFFER_ENV_REGISTER);
Texture2D    EnvMapTex  : register(GBUFFER_IBL_REGISTER);

SamplerState ClampedSampler : register(CLAMPED_SAMPLER_REGISTER);

cbuffer GBufferData : register(GBUFFER_CONSTANTS_REGISTER)
{
    uint enableIBL;
    uint enableEnv;
    uint debugAttrIndex;
};
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER, SCENE_DATA_SPACE);
StructuredBuffer<Light>      Lights   : register(LIGHT_DATA_REGISTER,      SCENE_DATA_SPACE);

Texture2D    GBufferRT0     : register(GBUFFER_RT0_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT1     : register(GBUFFER_RT1_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT2     : register(GBUFFER_RT2_REGISTER,     GBUFFER_SPACE);
Texture2D    GBufferRT3     : register(GBUFFER_RT3_REGISTER,     GBUFFER_SPACE);
Texture2D    IBLTex         : register(GBUFFER_ENV_REGISTER,     GBUFFER_SPACE);
Texture2D    EnvMapTex      : register(GBUFFER_IBL_REGISTER,     GBUFFER_SPACE);
SamplerState ClampedSampler : register(GBUFFER_SAMPLER_REGISTER, GBUFFER_SPACE);

cbuffer GBufferData : register(GBUFFER_CONSTANTS_REGISTER, GBUFFER_SPACE)
{
    uint enableIBL;
    uint enableEnv;
    uint debugAttrIndex;
};
#endif // -- defined (PPX_D3D11) -----------------------------------------------

// -------------------------------------------------------------------------------------------------

struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(uint id : SV_VertexID)
{
	VSOutput result;
    
    // Clip space position
	result.Position.x = (float)(id / 2) * 4.0 - 1.0;
    result.Position.y = (float)(id % 2) * 4.0 - 1.0;
    result.Position.z = 0.0;
    result.Position.w = 1.0;
    
    // Texture coordinates
	result.TexCoord.x = (float)(id / 2) * 2.0;
    result.TexCoord.y = 1.0 - (float)(id % 2) * 2.0;
    
	return result;
}

// -------------------------------------------------------------------------------------------------

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

float3 PBR(GBuffer gbuffer)
{
    float3 P  = gbuffer.position;
    float3 N  = gbuffer.normal;
    float3 V  = normalize(Scene.eyePosition.xyz - P);
    float3 R  = reflect(-V, N);    
    float3 Lo = V;    
    float  cosLo = saturate(dot(N, Lo));
    //float  ao = 0.4f;    
    
	float3 albedo   = gbuffer.albedo;    
    float roughness = gbuffer.roughness;    
    float metalness = gbuffer.metalness;
    
    // The value for F0 is passed in from the application. If in doubt use 0.04 for 
    // dieletric materials and scale beween 0.04 and the albedo based on the metalness
    // value.
    //
    float3 F0 = gbuffer.F0;
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
        if (enableIBL == 1) {
            float maxLevel = (Scene.iblLevelCount - 1);
            float lod      = roughness * maxLevel;
            irradiance     = Environment(IBLTex, R, lod) * gbuffer.iblStrength;

            kS = Environment(IBLTex, R,maxLevel) * gbuffer.iblStrength;
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
    if (enableEnv) {
        float  lod    = roughness * (Scene.envLevelCount - 1);
        envReflection = Environment(EnvMapTex, R, lod) * (1 - roughness) * gbuffer.envStrength;
    }

    float3 Co = directLighting + indirectLighting + envReflection + (float3)Scene.ambient;
    return Co;    
}

float4 psmain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD) : SV_TARGET
{
    PackedGBuffer packed = (PackedGBuffer) 0;
    packed.rt0 = GBufferRT0.Sample(ClampedSampler, TexCoord);
    packed.rt1 = GBufferRT1.Sample(ClampedSampler, TexCoord);
    packed.rt2 = GBufferRT2.Sample(ClampedSampler, TexCoord);
    packed.rt3 = GBufferRT3.Sample(ClampedSampler, TexCoord);
    
    GBuffer gbuffer = UnpackGBuffer(packed);
    
    float3 color = PBR(gbuffer);

    return float4(color, 1.0);
}