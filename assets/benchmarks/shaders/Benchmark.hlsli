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

struct SceneData
{
    float4x4 ModelMatrix;                // Transforms object space to world space.
    float4x4 ITModelMatrix;              // Inverse transpose of the ModelMatrix.
    float4   Ambient;                    // Object's ambient intensity.
    float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix.
    float4   LightPosition;              // Light's position.
    float4   EyePosition;                // Eye (camera) position.
};

ConstantBuffer<SceneData> Scene : register(b0);
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
  float3 normalTS    : NORMALTS;
  float3 tangentTS   : TANGENTTS;
  float3 bitangentTS : BITANGENTTS;
};
