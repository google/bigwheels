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

#include "Benchmark.hlsli"

#define PI 3.1415292
#define ROUGHNESS 0.1
#define METALNESS 0.1
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
    const float3   nTS = normalize(input.normalTS);
    const float3   tTS = normalize(input.tangentTS);
    const float3   bTS = normalize(input.bitangentTS);
    const float3x3 TBN = float3x3(tTS.x, bTS.x, nTS.x,
                            tTS.y, bTS.y, nTS.y,
                            tTS.z, bTS.z, nTS.z);

    const float3 V = normalize(Scene.EyePosition.xyz - input.world_position.xyz);
    const float3 normal = float3(0, 0, 1);
    const float3 N = normalize(mul(TBN, normal * 2.0 - 1.0));
    const float4 albedo = float4(1, 1, 1, 1);

    const float3 F0 = lerp(0.04f, albedo.rgb, METALNESS);
    const float Lrad = 4.f;

    const float3 Li    = normalize(Scene.LightPosition.xyz - input.world_position.xyz);
    const float3 E     = normalize(Scene.EyePosition.xyz - input.world_position.xyz);
    const float3 Lo    = E;
    const float  cosLo = saturate(dot(N, Lo));
    const float3 R     = reflect(-Li, N);
    const float3 Lh    = normalize(Li + Lo);
    const float  cosLi = saturate(dot(N, Li));
    const float  cosLh = saturate(dot(N, Lh));

    const float3 F = FresnelSchlick(F0, saturate(dot(Lh, Lo)));
    const float  D = DistributionGGX(cosLh, ROUGHNESS);
    const float  G = GASmithSchlickGGX(cosLi, cosLo, ROUGHNESS);

    const float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), METALNESS);
    const float3 diffuseBRDF = kD * albedo.rgb;
    const float3 specularBRDF = (F * D * G) / max(0.00001, 4.0 * cosLi * cosLo);
    const float3 Co = (diffuseBRDF + specularBRDF) * Lrad * cosLi + Scene.Ambient.rrr * albedo.rgb;

    return float4(Co / (Co + 1.f), 0.5);
}
