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

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * (float3)pow(1.0f - cosTheta, 5.0f);
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
    float  ao = 0.4f;    
    
	float3 albedo   = gbuffer.albedo;    
    float roughness = gbuffer.roughness;    
    float metalness = gbuffer.metalness;
    
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    //
    // float3 F0 = (float3)0.04;
    //
    float3 F0 = gbuffer.F0;
    F0        = lerp(F0, albedo, metalness);
    
    float3 Lo = (float3)0.0;    
	for (uint i = 0; i < Scene.lightCount; ++i) {    
        float3 Lp    = Lights[i].position;
        float3 L     = normalize(Lp - P);
        float3 H     = normalize(V + L);

        // calculate per-light radiance
        //float  distance    = length(lightPositions[i] - WorldPos);
        //float  attenuation = 1.0 / (distance * distance);
        float3 radiance = (float3)1.0; //lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, roughness);
        float3 F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 nominator   = NDF * G * F;
        float  denominator = 4 * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.00001; // 0.001 to prevent divide by zero.
        float3 specular    = nominator / denominator;

        // kS is equal to Fresnel
        float3 kS = F;

        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = 1.0 - kS;

        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalness;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}
    
    // ambient lighting (we now use IBL as the ambient term)
    float3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;
    
    float3 irradiance = (float3)1.0; 
    if (enableIBL == 1) {    
        float iblStrength = gbuffer.iblStrength;
        float lod = roughness * (Scene.iblLevelCount - 1.0);
        irradiance = Environment(IBLTex, R, lod) * iblStrength;
    }
    
    float3 diffuse    = irradiance * albedo +  Scene.ambient;
    float3 ambient    = (kD * diffuse) * ao;    
        
    // Environment reflection
    float3 reflection = (float3)0.0;
    if (enableEnv) {
        float envStrength = gbuffer.envStrength;
        float lod = roughness * (Scene.envLevelCount - 1.0);
        reflection = Environment(EnvMapTex, R, lod) * envStrength;
    }    
    
    // Final color
    float3 color = ambient + Lo + (kS * reflection * (1.0 - roughness));
    
    // Faux HDR tonemapping
    color = color / (color + float3(1, 1, 1));
    
    return color;
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