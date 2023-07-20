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

float Lambert(float3 N, float3 L)
{
    float diffuse = saturate(dot(N, L));
    return diffuse;
}

float Phong(float3 N, float3 L, float3 V, float hardness)
{
    float3 R        = reflect(-L, N);
    float  theta    = max(dot(R, V), 0);
    float  specular = pow(theta, hardness);
    return specular;
}

float3 Maxof4(float3 v1, float3 v2, float3 v3, float3 v4)
{
    return float3(max(max(v1.x, v2.x), max(v3.x, v4.x)), 
        max(max(v1.y, v2.y), max(v3.y, v4.y)), 
        max(max(v1.z, v2.z), max(v3.z, v4.z))
        );
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
    
    float2 uv1 = input.uv;
    const float offset = 0.005;
    float2 uv2 = clamp(input.uv + float2(-offset, 0), 0.0, 1.0);
    float2 uv3 = clamp(input.uv + float2(0, -offset), 0.0, 1.0);
    float2 uv4 = clamp(input.uv + float2(0, offset), 0.0, 1.0);
    float2 uv5 = clamp(input.uv + float2(offset, 0), 0.0, 1.0);

    const float3 normal1 = NormalMap.Sample(NormalMapSampler, uv1).rgb;
    const float3 normal2 = NormalMap.Sample(NormalMapSampler, uv2).rgb;
    const float3 normal3 = NormalMap.Sample(NormalMapSampler, uv3).rgb;
    const float3 normal4 = NormalMap.Sample(NormalMapSampler, uv4).rgb;

    float3 normal = Maxof4(normal1, normal2, normal3, normal4);

    const float3 N = normalize(mul(TBN, normal * 2.0 - 1.0));


    const float4 albedo1 = AlbedoTexture.Sample(AlbedoSampler, uv1).rgba;
    const float4 albedo2 = AlbedoTexture.Sample(AlbedoSampler, uv2).rgba;
    const float4 albedo3 = AlbedoTexture.Sample(AlbedoSampler, uv3).rgba;
    const float4 albedo4 = AlbedoTexture.Sample(AlbedoSampler, uv4).rgba;
    const float4 albedo5 = AlbedoTexture.Sample(AlbedoSampler, uv5).rgba;

    const float4 albedo = (albedo1 + albedo2 + albedo3 + albedo4 + albedo5)/5.0;
    
    const float roughness1 = MetalRoughness.Sample(MetalRoughnessSampler, uv1).g;
    const float roughness2 = MetalRoughness.Sample(MetalRoughnessSampler, uv2).g;
    const float roughness3 = MetalRoughness.Sample(MetalRoughnessSampler, uv3).g;
    const float roughness4 = MetalRoughness.Sample(MetalRoughnessSampler, uv4).g;
    const float roughness5 = MetalRoughness.Sample(MetalRoughnessSampler, uv5).g;
    const float roughness = (roughness1 + roughness2 + roughness3 + roughness4 + roughness5)/5.0;

    const float ambientStrength = 0.6;
    const float lightIntensity = 0.8;

    float hardness = lerp(1.0, 100.0, 1.0 - saturate(roughness));
    float diffuse  = 0;
    float specular = 0;

    const float3 lightColor = {0.5, 0.5, 0.5};

    float3 ambient = ambientStrength * lightColor;

    const float3 L    = normalize(Scene.LightPosition.xyz - input.world_position.xyz);
    diffuse += Lambert(N, L);
    specular += Phong(N, L, V, hardness) * lightIntensity;

    float3 result = (ambient + diffuse ) * albedo.rgb + specular;
    
    return float4(result.rgb, 0.5);
}
