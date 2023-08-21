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
    float2 uv       : TEXCOORD,
    float3 normal   : NORMAL,
    float3 tangent   : TANGENT)
{
  VSOutput result;

  const float4x4 jointMat[2] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
  };

  float4 jointWeights = {
    frac(uv.x), 1 - frac(uv.x), 0, 0
  };

  float4 jointIndices = {0, 1, 0, 0};

  // TODO: Hardcode the joint weights and indices for now for testing.
  // Check this later if we get a mesh that acctually contains the
  // skinning data per vertex.
  float4x4 skinMat = jointWeights.x * jointMat[int(jointIndices.x)] +
    jointWeights.y * jointMat[int(jointIndices.y)] +
    jointWeights.z * jointMat[int(jointIndices.z)] +
    jointWeights.w * jointMat[int(jointIndices.w)];

  result.world_position = mul(Scene.ModelMatrix, float4(position, 1));
  result.position = mul(Scene.CameraViewProjectionMatrix, result.world_position);
  result.position = mul(skinMat, result.position);
  result.uv = uv;
  result.normal      = mul(Scene.ITModelMatrix, float4(normal, 0)).xyz;
  result.normalTS    = mul(Scene.ITModelMatrix, float4(normal, 0)).xyz;
  result.tangentTS   = mul(Scene.ITModelMatrix, float4(tangent, 0)).xyz;
  result.bitangentTS = cross(normal, tangent);

  return result;
}
