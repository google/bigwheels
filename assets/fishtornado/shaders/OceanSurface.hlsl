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

#if defined(PPX_D3D11) // -----------------------------------------------------
cbuffer Scene : register(RENDER_SCENE_DATA_REGISTER)
{
    SceneData Scene;
};
cbuffer Model : register(RENDER_MODEL_DATA_REGISTER)
{
    ModelData Model;
};
cbuffer Material : register(RENDER_MATERIAL_DATA_REGISTER)
{
    MaterialData Material;
};
Texture2D              ShadowTexture : register(RENDER_SHADOW_TEXTURE_REGISTER);
SamplerComparisonState ShadowSampler : register(RENDER_SHADOW_SAMPLER_REGISTER);
Texture2D              AlbedoTexture : register(RENDER_ALBEDO_TEXTURE_REGISTER);
Texture2D              RoughnessTexture : register(RENDER_ROUGHNESS_TEXTURE_REGISTER);
Texture2D              NormalMapTexture : register(RENDER_NORMAL_MAP_TEXTURE_REGISTER);
SamplerState           ClampedSampler : register(RENDER_CLAMPED_SAMPLER_REGISTER);
Texture2DArray         CausticsTexture : register(RENDER_CAUSTICS_TEXTURE_REGISTER);
SamplerState           RepeatSampler : register(RENDER_REPEAT_SAMPLER_REGISTER);
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData>    Scene : register(RENDER_SCENE_DATA_REGISTER, SCENE_SPACE);
Texture2D                    ShadowTexture : register(RENDER_SHADOW_TEXTURE_REGISTER, SCENE_SPACE);
ConstantBuffer<ModelData>    Model : register(RENDER_MODEL_DATA_REGISTER, MODEL_SPACE);
ConstantBuffer<MaterialData> Material : register(RENDER_MATERIAL_DATA_REGISTER, MATERIAL_SPACE);
Texture2D                    AlbedoTexture : register(RENDER_ALBEDO_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2D                    RoughnessTexture : register(RENDER_ROUGHNESS_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2D                    NormalMapTexture : register(RENDER_NORMAL_MAP_TEXTURE_REGISTER, MATERIAL_SPACE);
Texture2DArray               CausticsTexture : register(RENDER_CAUSTICS_TEXTURE_REGISTER, MATERIAL_SPACE);
SamplerState                 ClampedSampler : register(RENDER_CLAMPED_SAMPLER_REGISTER, MATERIAL_SPACE);
SamplerState                 RepeatSampler : register(RENDER_REPEAT_SAMPLER_REGISTER, MATERIAL_SPACE);
#endif

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

static const float3 SURFACE_COL = float3(159.0, 224.0, 230.0) / 255.0;

float4 psmain(VSOutput input) : SV_TARGET
{
    // NOTE: This math makes no attempt to be physical accurate :)
    // 
    float3 P  = input.positionWS.xyz;
    float3 V  = normalize(Scene.eyePosition.xyz - input.positionWS);
    float3 Lp = Scene.lightPosition;
    float3 L =  normalize(Lp - P);

    float  t  = 0.008 * Scene.time;
    float3 N1 = NormalMapTexture.Sample(RepeatSampler, input.texCoord * 0.50 - t * 0.07).rbg * 2.0 - 1.0;
    float3 N2 = NormalMapTexture.Sample(RepeatSampler, input.texCoord * 1.40 - t * 0.10).rbg * 2.0 - 1.0;
    float3 N3 = NormalMapTexture.Sample(RepeatSampler, input.texCoord * 2.50 + t * 0.08).rbg * 2.0 - 1.0;
    float3 N  = normalize(N1 + N2 + N3);

    float specular = Phong(N, L, V, 10.0);
    float diffuse  = saturate(dot(N, L));
    float rim      = pow(1.0 - diffuse, 10.0);
   
    float3 color = (specular * 5.0 + pow(diffuse, 10.0) + rim * 10.0 * SURFACE_COL) * 4.0; //(specular * 0.8 + pow(diffuse, 10.0) + rim * 5.0 * SURFACE_COL) * 2.0;
    color = lerp(Scene.fogColor, color, input.fogAmount);

    return float4(color, 1);
}
