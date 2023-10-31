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

#include "VsOutput.hlsli"

struct ColorParams
{
    uint32_t Seed;
    float3 Value;
};

#if defined(__spirv__)
[[vk::push_constant]]
#endif
ConstantBuffer<ColorParams> Color : register(b0);

// returns zigzag value between (0.5 ~ 1.0) in steps of 0.1 between consecutive seeds
// this is chosen to try and reduce flashing effect when the slider is dragged slowly
// but so that color difference is still visible to the eye between consecutive quads
float intensity(uint32_t seed)
{
    //     index:   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0...
    // intensity: 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0...
    float index = seed % 10;
    float weight = step(index, 4.5f); // weight==0: 0~4; weight==1: 5~9
    return (1.0f - weight)*(index / 10.0f) + weight*(1.0f - (index / 10.f));
}

float4 psmain(VSOutputPos input) : SV_TARGET
{
    float3 dimmedColor = intensity(Color.Seed) * Color.Value;
    return float4(dimmedColor, 1.0f);
}