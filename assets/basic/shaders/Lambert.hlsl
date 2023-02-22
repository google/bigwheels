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

//
// Keep things easy for now and use 16-byte aligned types
//
struct SceneData
{
    float4x4 ModelMatrix;  // Transforms object space to world space
    float4   Ambient;      // Object's ambient intensity
    float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
    float4   LightPosition; // Light's position
    float4   EyePosition; // Eye (camera) position
};

// ConstantBuffer was addd in SM5.1 for D3D12
#if defined(PPX_D3D11)
cbuffer Scene : register(b0)
{
    SceneData Scene;
};
#else
ConstantBuffer<SceneData> Scene : register(b0);
#endif // defined(PPX_D3D11)

Texture2D                 AlbedoTexture         : register(t1);
SamplerState              AlbedoSampler         : register(s2);
Texture2D                 NormalMap             : register(t3);
SamplerState              NormalMapSampler      : register(s4);
Texture2D                 MetalRoughness        : register(t5);
SamplerState              MetalRoughnessSampler : register(s6);

struct VSOutput {
  float4 world_position : POSITION;
  float4 position       : SV_POSITION;
  float2 uv             : TEXCOORD;
  float3 normal         : NORMAL;
};

VSOutput vsmain(
    float4 position : POSITION,
    float2 uv       : TEXCOORD,
    float3 normal   : NORMAL)
{
  VSOutput result;

  result.world_position = mul(Scene.ModelMatrix, position);
  result.position = mul(Scene.CameraViewProjectionMatrix, result.world_position);
  result.uv = uv;
  result.normal = mul(Scene.ModelMatrix, float4(normal, 0)).xyz;

  return result;
}

#define PI 3.1415292

//
// GGX/Towbridge-Reitz normal distribution function with
// Disney's reparametrization of alpha = roughness^2
//
float DistributionGGX(float cosLh, float roughness)
{
    const float alpha   = roughness * roughness;
    const float alphaSq = alpha * alpha;
    const float denom   = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
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
    const float r = roughness + 1.0;
    const float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights
    return GASchlickG1(cosLi, k) * GASchlickG1(cosLo, k);
}

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * (float3)pow(1.0 - cosTheta, 5.0);
}

float4 psmain(VSOutput input) : SV_TARGET
{
    const float3 V = normalize(Scene.EyePosition.xyz - input.world_position.xyz);
    const float3 N = normalize(input.normal);

    // Read albedo texture value
    const float3 albedo = AlbedoTexture.Sample(AlbedoSampler, input.uv).rgb;
    const float roughness = MetalRoughness.Sample(MetalRoughnessSampler, input.uv).r;
    const float metalness = MetalRoughness.Sample(MetalRoughnessSampler, input.uv).g;
    const float3 F0 = lerp(0.4f, albedo, metalness);
    const float Lrad = 5.f;               // Light radiance

    // Light
    const float3 Li = normalize(Scene.LightPosition.xyz - input.world_position.xyz);
    const float3 E = normalize(Scene.EyePosition.xyz - input.world_position.xyz);
    const float3 Lo    = E; // Outgoing light direction
    const float  cosLo = saturate(dot(N, Lo));
    const float3 R = reflect(-Li, N);
    const float3 Lh = normalize(Li + Lo);                // Half-vector between Li and Lo
    const float  cosLi = saturate(dot(N, Li));
    const float  cosLh = saturate(dot(N, Lh));

    const float3 F = FresnelSchlick(F0, saturate(dot(Lh, Lo)));
    const float  D = DistributionGGX(cosLh, roughness);
    const float  G = GASmithSchlickGGX(cosLi, cosLo, roughness);
    const float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);
    const float3 diffuseBRDF = kD * albedo;
    const float3 specularBRDF = (F * D * G) / max(0.00001, 4.0 * cosLi * cosLo);
    const float3 Co = (diffuseBRDF + specularBRDF) * Lrad * cosLi;

    return float4(Co, 1);
}

