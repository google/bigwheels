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

VSOutput vsmain(float4 position : POSITION, float3 color : COLOR)
{
    float4x4 mvp = mul(Scene.viewProjectionMatrix, Model.modelMatrix);    
    
    VSOutput result   = (VSOutput)0;
    result.position   = mul(mvp, position);
    result.color      = color;
    
    return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    return float4(input.color, 1);
}
