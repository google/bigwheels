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
#include "Lighting.hlsli"

ConstantBuffer<SceneData>    Scene : register(RENDER_SCENE_DATA_REGISTER, SCENE_SPACE);
Texture2D                    ShadowTexture : register(RENDER_SHADOW_TEXTURE_REGISTER, SCENE_SPACE);
SamplerComparisonState       ShadowSampler : register(RENDER_SHADOW_SAMPLER_REGISTER, SCENE_SPACE);
ConstantBuffer<ModelData>    Model : register(RENDER_MODEL_DATA_REGISTER, MODEL_SPACE);
ConstantBuffer<MaterialData> Material : register(RENDER_MATERIAL_DATA_REGISTER, MATERIAL_SPACE);
Texture2D                    AlbedoTexture : register(RENDER_ALBEDO_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2D                    RoughnessTexture : register(RENDER_ROUGHNESS_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2D                    NormalMapTexture : register(RENDER_NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2DArray               CausticsTexture : register(RENDER_CAUSTICS_TEXTURE_REGISTER, MATERIAL_SPACE);
SamplerState                 ClampedSampler : register(RENDER_CLAMPED_SAMPLER_REGISTER, MATERIAL_SPACE);
SamplerState                 RepeatSampler : register(RENDER_REPEAT_SAMPLER_REGISTER, MATERIAL_SPACE);

// -------------------------------------------------------------------------------------------------

float getSinVal(float z)
{
    float speed = 4.0;
    float t     = (Scene.time * 0.65);
    float s     = sin(z + t) * speed * z;
    return s;
}

// -------------------------------------------------------------------------------------------------

VSOutput vsmain(VSInput input)
{   
    float3x3 normalMatrix = (float3x3)Model.modelMatrix;
    
    float4 positionWS = mul(Model.modelMatrix, float4(input.position, 1.0));
    float4 positionVS = mul(Scene.viewMatrix, positionWS);   
    
    VSOutput result    = (VSOutput)0;
    result.position    = mul(Scene.projectionMatrix, positionVS);
    result.positionWS  = positionWS.xyz;
    result.positionLS  = mul(Scene.shadowViewProjectionMatrix, positionWS);
    result.color       = input.color;
    result.normal      = mul(normalMatrix, input.normal);
    result.texCoord    = input.texCoord;
    result.normalTS    = mul(normalMatrix, input.normal);
    result.tangentTS   = mul(normalMatrix, input.tangent);
    result.bitangnetTS = mul(normalMatrix, input.bitangnet);
    result.fogAmount   = pow(1.0 - min(max(length(positionVS.xyz) - Scene.fogNearDistance, 0.0) / Scene.fogFarDistance, 1.0), Scene.fogPower);

    return result;
}

// -------------------------------------------------------------------------------------------------

float4 psmain(VSOutput input) : SV_TARGET
{
    float3 P  = input.positionWS.xyz;
    float3 N  = normalize(input.normal);
    float3 V  = normalize(Scene.eyePosition.xyz - input.positionWS);
    
    float3   nTS = normalize(input.normalTS);
    float3   tTS = normalize(input.tangentTS);
    float3   bTS = normalize(input.bitangnetTS);
    float3x3 TBN = float3x3(tTS.x, bTS.x, nTS.x,
                            tTS.y, bTS.y, nTS.y,
                            tTS.z, bTS.z, nTS.z);
    N = (NormalMapTexture.Sample(RepeatSampler, input.texCoord).rgb * 2.0f) - 1.0;
    N = mul(TBN, N);
    N = normalize(N);

    float3 Lp = Scene.lightPosition; 
    float3 L =  normalize(Lp - P);
    
    float roughness = RoughnessTexture.Sample(RepeatSampler, input.texCoord).r;
    float hardness  = lerp(1.0, 100.0, 1.0 - saturate(roughness));
    
    float  diffuse  = lerp(0.8, 1.0, Lambert(N, L));
    float  specular = 0.25 * BlinnPhong(N, L, V, roughness);
    
    float2 tc       = input.positionWS.xz / 25.0;
    float3 caustics = 1.25 * CalculateCaustics(Scene.time, tc, CausticsTexture, RepeatSampler);
    
    float  shadow   = CalculateShadow(input.positionLS, Scene.shadowTextureDim, ShadowTexture, ShadowSampler, Scene.usePCF);
    
    float3 color = AlbedoTexture.Sample(RepeatSampler, input.texCoord).rgb;
    color = color * ((float3)(diffuse + specular) * shadow + Scene.ambient + caustics);
    
    color = lerp(Scene.fogColor, color, input.fogAmount);    
    
    return float4(color, 1);
}
