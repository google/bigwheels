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
    float3 position : POSITION,
    float3 color   : COLOR,
    float3 normal   : NORMAL,
    float2 uv       : TEXCOORD,
    float3 tangent   : TANGENT,
    float3 binormal   : BINORMAL
    )
{
  VSOutput result;

  result.world_position = mul(Scene.ModelMatrix, float4(position, 1)) ;
  result.position = mul(Scene.CameraViewProjectionMatrix, result.world_position);
  result.uv = uv;
  result.normal      = mul((mul(Scene.ITModelMatrix, Scene.ITModelMatrix)), mul(Scene.ITModelMatrix, float4(normal, 0))).xyz;
  result.normalTS    = mul(Scene.CameraViewProjectionMatrix, mul(Scene.ITModelMatrix, float4(normal, 0))).xyz;
  result.tangentTS   = mul(mul(Scene.ITModelMatrix,Scene.CameraViewProjectionMatrix), float4(tangent, 0)).xyz;
  result.bitangentTS = mul(Scene.ITModelMatrix, float4(binormal, 0)).xyz  * color.xyz + mul(Scene.ITModelMatrix, result.world_position);;

  return result;
}
