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
cbuffer Scene : register(SCENE_CONSTANTS_REGISTER)
{
    SceneData Scene;
};

StructuredBuffer<Light> Lights : register(LIGHT_DATA_REGISTER);

cbuffer Material : register(MATERIAL_CONSTANTS_REGISTER)
{
    MaterialData Material;
};

cbuffer Model : register(MODEL_CONSTANTS_REGISTER)
{
    ModelData Model;
};
#else // --- D3D12 / Vulkan ----------------------------------------------------
ConstantBuffer<SceneData>    Scene    : register(SCENE_CONSTANTS_REGISTER,    SCENE_DATA_SPACE);
StructuredBuffer<Light>      Lights   : register(LIGHT_DATA_REGISTER,         SCENE_DATA_SPACE);
ConstantBuffer<MaterialData> Material : register(MATERIAL_CONSTANTS_REGISTER, MATERIAL_DATA_SPACE);
ConstantBuffer<ModelData>    Model    : register(MODEL_CONSTANTS_REGISTER,    MODEL_DATA_SPACE);
#endif // -- defined (PPX_D3D11) -----------------------------------------------

VSOutput vsmain(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    output.positionWS = mul(Model.modelMatrix, float4(input.position, 1)).xyz;
    output.position   = mul(Scene.viewProjectionMatrix, float4(output.positionWS, 1));
    output.normal     = mul(Model.normalMatrix, float4(input.normal, 0)).xyz;

    output.color    = input.color;
    output.texCoord = input.texCoord;
    
    // TBN
    output.normalTS    = mul(Model.modelMatrix, float4(input.normal, 0)).xyz;
    output.tangentTS   = mul(Model.modelMatrix, float4(input.tangent, 0)).xyz;
    output.bitangnetTS = mul(Model.modelMatrix, float4(input.bitangnet, 0)).xyz;    
    
    return output;
}