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

#include "Benchmark.hlsli"

VSOutput vsmain(
    float4 position : POSITION,
    float2 uv       : TEXCOORD,
    float3 normal   : NORMAL,
    float3 tangent   : TANGENT)
{
  VSOutput result;

  result.world_position = mul(Scene.ModelMatrix, position);
  result.position = mul(Scene.CameraViewProjectionMatrix, result.world_position);
  result.uv = uv;
  result.normal      = mul(Scene.ITModelMatrix, float4(normal, 0)).xyz;
  result.normalTS    = mul(Scene.ITModelMatrix, float4(normal, 0)).xyz;
  result.tangentTS   = mul(Scene.ITModelMatrix, float4(tangent, 0)).xyz;
  result.bitangentTS = cross(normal, tangent);

  return result;
}
