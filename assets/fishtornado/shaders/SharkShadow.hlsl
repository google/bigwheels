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

#if defined(PPX_D3D11) // -----------------------------------------------------
cbuffer Scene : register(RENDER_SCENE_DATA_REGISTER)
{
    SceneData Scene;
};

cbuffer Model : register(RENDER_MODEL_DATA_REGISTER)
{
    ModelData Model;
};
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData> Scene : register(RENDER_SCENE_DATA_REGISTER, SCENE_SPACE);
ConstantBuffer<ModelData> Model : register(RENDER_MODEL_DATA_REGISTER, MODEL_SPACE);
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

float4 vsmain(float4 position : POSITION) : SV_POSITION
{
    float currZ         = position.z;
    float prevZ         = currZ - 1.0;
    float currVal       = getSinVal(currZ * 0.03);
    float prevVal       = getSinVal(prevZ * 0.03);
    
    float3 currCenter   = float3(currVal, 0.0, currZ);
    float3 prevCenter   = float3(prevVal, 0.0, prevZ);
    float3 pathDir      = normalize(currCenter - prevCenter);
    float3 pathUp       = float3(0.0, 1.0, 0.0);
    float3 pathNormal   = cross(pathDir, pathUp);
    
    float3x3 basis      = float3x3(pathNormal, pathUp, pathDir);
    float3 at           = float3(basis[0][0], basis[1][0], basis[2][0]);
    float3 bt           = float3(basis[0][1], basis[1][1], basis[2][1]);
    float3 ct           = float3(basis[0][2], basis[1][2], basis[2][2]);
    float3x3 trans      = float3x3(at, bt, -ct);
    
    float3 spoke   = float3(position.xy, 0.0);
    float3 vertPos = currCenter + mul(basis, spoke);

    float4x4 mvpMatrix = mul(Scene.shadowViewProjectionMatrix, Model.modelMatrix);
         
    float4 result = (float4)0;
    result = mul(mvpMatrix, float4(vertPos, 1.0));
    return result;
}


