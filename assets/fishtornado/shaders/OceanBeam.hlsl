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

struct BeamModelData
{
    float4x4 modelMatrix[2];
};

#if defined(PPX_D3D11) // -----------------------------------------------------
cbuffer Scene : register(RENDER_SCENE_DATA_REGISTER)
{
    SceneData Scene;
};

cbuffer BeamModelData : register(RENDER_MODEL_DATA_REGISTER)
{
    BeamModelData Model;
};
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData>     Scene : register(RENDER_SCENE_DATA_REGISTER, SCENE_SPACE);
ConstantBuffer<BeamModelData> Model : register(RENDER_MODEL_DATA_REGISTER, MODEL_SPACE);
#endif // -- defined (PPX_D3D11) -----------------------------------------------

// -------------------------------------------------------------------------------------------------

float getSinVal(float z)
{
    float speed = 4.0;
    float t     = (Scene.time * 0.65);
    float s     = sin(z + t) * speed * z;
    return s;
}

// -------------------------------------------------------------------------------------------------

struct BeamVSOutput 
{
    float4 position    : SV_POSITION;
    float3 positionVS  : POSITIONVS;
    float3 normal      : NORMAL;
    float2 texCoord    : TEXCOORD;
};

BeamVSOutput vsmain(float3 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD, uint instanceId : SV_InstanceID)
{
    float3x3 normalMatrix = (float3x3)mul(Scene.viewMatrix, Model.modelMatrix[instanceId]);
    
    float4 positionWS = mul(Model.modelMatrix[instanceId], float4(position, 1.0));
    float4 positionVS = mul(Scene.viewMatrix, positionWS);

    BeamVSOutput result = (BeamVSOutput)0;
    result.position     = mul(Scene.projectionMatrix, positionVS);
    result.positionVS   = positionVS.xyz;
    result.normal       = mul(normalMatrix, normal);
    result.texCoord     = texCoord;

    return result;
}

// -------------------------------------------------------------------------------------------------

static const float3 FOG_COLOR = float3(15.0, 86.0, 107.0) / 255.0;

float4 psmain(BeamVSOutput input) : SV_TARGET
{
    float camDistPer = clamp(length(input.positionVS) / 400.0, 0.0, 1.0);

    float falloff    = sin((1.0 - input.texCoord.y) * 3.14159) * 0.1;
    float eyeDiff    = abs(dot(input.normal, normalize(input.positionVS)));

    float  vDistPer = 0.5;
    float3 rgb = pow(eyeDiff, 1.0) * FOG_COLOR;
    float  a   = camDistPer * falloff * vDistPer;

    return float4(rgb, a);
}
