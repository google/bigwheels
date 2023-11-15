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

#include "FoveationBenchmark.hlsli"

float4 psmain(VSOutputPos Input, uint shading_rate
              : SV_ShadingRate)
    : SV_TARGET {
  // Decode the shading rate into shading densities.
  float2 shading_density = float2(1.0 / (1U << (shading_rate & 3)),
                                  1.0 / (1U << ((shading_rate >> 2) & 3)));

  // Adjust params so that color G&B channels are shading density, and noise
  // is removed from these channels.
  Params params = paramsUniform;
  params.color.yz = float2(shading_density);
  params.noiseWeights.yz = float2(0.001, 0.001);
  return getColor(params, Input.position.xy);
}
