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

// 
// Use DX for Vulkan:
//   dxc -spirv -fvk-use-dx-layout -T vs_6_0 -E vsmain -Fo CubeMap.vs.spv CubeMap.hlsl
//   dxc -spirv -fvk-use-dx-layout -T ps_6_0 -E psmain -Fo CubeMap.ps.spv CubeMap.hlsl
//

struct TransformData
{
    float4x4 MVPMatrix;
    float4x4 ModelMatrix;
    float3x3 NormalMatrix;
    float3   EyePos;
};

ConstantBuffer<TransformData> Transform : register(b0);
TextureCube                   Tex0      : register(t1);
SamplerState                  Sampler0  : register(s2);

struct VSOutput {
	float4 Position   : SV_POSITION;
	float3 PositionWS : POSITIONWS;
    float3 NormalWS   : NORMALWS;
};

VSOutput vsmain(float4 Position : POSITION, float3 Normal : NORMAL)
{
	VSOutput result;
	result.Position = mul(Transform.MVPMatrix, Position);
    result.PositionWS = mul(Transform.ModelMatrix, Position).xyz;
    result.NormalWS = mul(Transform.NormalMatrix, Normal);
	return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.NormalWS);
    float3 I = normalize(input.PositionWS - Transform.EyePos);
    float3 R = reflect(I, N);
    return Tex0.Sample(Sampler0, R);
}