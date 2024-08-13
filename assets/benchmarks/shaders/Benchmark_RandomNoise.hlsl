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

#include "Benchmark_Quad.hlsli"

float random(float2 st, uint32_t seed) {
    float underOne = sin(float(seed) + 0.5f);
    float2 randomVector = float2(15.0f + underOne, 15.0f - underOne);
    return frac(cos(dot(st.xy, randomVector))*40000.0f);
}

float4 psmain(VSOutputPos input) : SV_TARGET
{
    float rnd = random(input.position.xy, Config.RandomSeed);
    return float4(rnd, rnd, rnd, 1.0f);
}