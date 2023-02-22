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

//
// Keep things easy for now and use 16-byte aligned types
//
struct SceneData
{
    float4x4 ModelMatrix;  // Transforms object space to world space
    float4   Ambient;      // Object's ambient intensity
    float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
    float4   LightPosition; // Light's position
    float4   EyePosition; // Eye (camera) position
};

// ConstantBuffer was addd in SM5.1 for D3D12
#if defined(PPX_D3D11)
cbuffer Scene : register(b0)
{
    SceneData Scene;
};
#else
ConstantBuffer<SceneData> Scene : register(b0);
#endif // defined(PPX_D3D11)

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
};

VSOutput vsmain(
    float4 position : POSITION,
    float2 uv       : TEXCOORD,
    float3 normal   : NORMAL)
{
  VSOutput result;

  result.world_position = mul(Scene.ModelMatrix, position);
  result.position = mul(Scene.CameraViewProjectionMatrix, result.world_position);
  result.uv = uv;
  result.normal = mul(Scene.ModelMatrix, float4(normal, 0)).xyz;

  return result;
}


float4 psmain(VSOutput input) : SV_TARGET
{
    const float3 V = normalize(Scene.EyePosition.xyz - input.world_position.xyz);
    const float3 N = normalize(input.normal);


    // Read albedo texture value
    const float3 albedo = AlbedoTexture.Sample(AlbedoSampler, input.uv).rgb;

    // Light
    const float3 L = normalize(Scene.LightPosition.xyz - input.world_position.xyz);
    const float3 R = reflect(-L, N);

    const float diffuse  = saturate(dot(N, L));           // Lambert
    const float specular = pow(saturate(dot(R, V)), 8.0); // Phong
    const float ambient  = Scene.Ambient.x;

    const float3 Co = (diffuse + specular + ambient) * albedo;
    return float4(Co, 1);
}

