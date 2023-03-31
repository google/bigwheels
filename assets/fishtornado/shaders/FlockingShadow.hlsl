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

ConstantBuffer<SceneData>    Scene           : register(RENDER_SCENE_DATA_REGISTER, SCENE_SPACE);
ConstantBuffer<ModelData>    Model           : register(RENDER_MODEL_DATA_REGISTER, MODEL_SPACE);
ConstantBuffer<FlockingData> Flocking        : register(RENDER_FLOCKING_DATA_REGISTER, FLOCKING_SPACE);
Texture2D<float4>            PrevPositionTex : register(RENDER_PREVIOUS_POSITION_TEXTURE_REGISTER, FLOCKING_SPACE);
Texture2D<float4>            CurrPositionTex : register(RENDER_CURRENT_POSITION_TEXTURE_REGISTER, FLOCKING_SPACE);
Texture2D<float4>            CurrVelocityTex : register(RENDER_CURRENT_VELOCITY_TEXTURE_REGISTER, FLOCKING_SPACE);

// -------------------------------------------------------------------------------------------------

float getSinVal(float z, float randPer)
{
    //float t = (Scene.time * 0.75 + randPer * 26.0);
    float t = (Scene.time * 1.5 + randPer * 32.0);
    float s = sin(z + t) * z;
    return s;
}

// -------------------------------------------------------------------------------------------------

float4 vsmain(float4 position : POSITION, uint instanceId : SV_InstanceID) : SV_POSITION
{
    // Texture data
    uint2  xy        = uint2((instanceId % Flocking.resX), (instanceId / Flocking.resY));
    float4 prevPos   = PrevPositionTex[xy];
    float4 currPos   = CurrPositionTex[xy];
    float4 currVel   = CurrVelocityTex[xy];
    float  leader    = currPos.a;

    float currZ = position.z;
    float prevZ = currZ - 1.0;
    float currVal = getSinVal(currZ * 0.2, leader + float(instanceId));
    float prevVal = getSinVal(prevZ * 0.2, leader + float(instanceId));

    float3 currCenter = float3(currVal, 0.0, currZ);
    float3 prevCenter = float3(prevVal, 0.0, prevZ);
    float3 pathDir    = normalize(currCenter - prevCenter);
    float3 pathUp     = float3(0.0, 1.0, 0.0);
    float3 pathNormal = normalize(cross(pathUp, pathDir));
    float3x3 basis    = float3x3(pathNormal, pathUp, pathDir);

    float3 spoke   = float3(position.xy, 0.0);
    float3 vertPos = currCenter + mul(basis, spoke);

    // Calculate matrix
    float3   worldUp = float3(0, 1, 0);
    //float3   dir     = normalize(currPos.xyz - prevPos.xyz); 
    float3   dir     = normalize(currVel.xyz);
    float3   right   = normalize(cross(dir, worldUp));
    float3   up      = normalize(cross(right, dir));
    float3x3 m       = float3x3(right, up, dir);

    // Set final vertex
    vertPos = mul(m, vertPos);
    vertPos.xyz += currPos.xyz;
   
    // NOTE: modelMatrix is an identity matrix for flocking
    //
    float4x4 mvpMatrix = mul(Scene.shadowViewProjectionMatrix, Model.modelMatrix);
    
    float4 result = (float4)0;
    result = mul(mvpMatrix, float4(vertPos, 1.0));
    return result;
}


