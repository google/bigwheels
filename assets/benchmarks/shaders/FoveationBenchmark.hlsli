// Copyright 2024 Google LLC
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

#include "VsOutput.hlsli"

struct Params {
  // Seed for pseudo-random noise generator.
  // This is combined with the fragment location to generate the color.
  uint seed;

  // Number of additional hash computations to perform for each fragment.
  uint extraHashRounds;

  // Per-channel weights of the noise in the output color.
  float3 noiseWeights;

  // Color to mix with the noise.
  float3 color;
};

ConstantBuffer<Params> paramsUniform : register(b0);

// Computes a pseudo-random hash.
uint pcg_hash(uint input) {
  uint state = input * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// Get the output color, which is a mix of pseudo-random noise and a constant
// color.
float4 getColor(Params params, float2 position) {
  uint hashWord = params.seed;
  hashWord = pcg_hash(hashWord ^ uint(position.x));
  hashWord = pcg_hash(hashWord ^ uint(position.y));
  for (int i = 0; i < params.extraHashRounds; ++i) {
    hashWord = pcg_hash(hashWord);
  }
  float3 noise = float3(float(hashWord & 0xFF) / 255.0,
                        float((hashWord >> 8) & 0xFF) / 255.0,
                        float((hashWord >> 16) & 0xFF) / 255.0);
  float3 color = lerp(params.color, noise, params.noiseWeights);
  return float4(color, 1.0);
}

VSOutputPos vsmain(uint vid : SV_VertexID) {
  VSOutputPos result;
  // Position of a triangle covering the entire viewport:
  // {{-1,-1}, {3,-1}, {-1,3}}
  float2 pos = float2(-1.0, -1.0) + float(vid & 1) * float2(4.0, 0.0) +
               float((vid >> 1) & 1) * float2(0.0, 4.0);
  result.position = float4(pos, 0, 1);
  return result;
}
