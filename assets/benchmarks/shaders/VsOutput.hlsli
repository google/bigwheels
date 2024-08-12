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

#ifndef BENCHMARKS_VS_OUTPUT_HLSLI
#define BENCHMARKS_VS_OUTPUT_HLSLI

struct ColorParams {
  float3 Value;
};

struct ConfigParams {
  uint32_t InstCount;
  uint32_t RandomSeed;
  ColorParams Color;
};

#if defined(__spirv__)
[[vk::push_constant]]
#endif
ConstantBuffer<ConfigParams> Config : register(b0);

struct VSOutputPos {
  float4 position : SV_POSITION;
};

#endif // BENCHMARKS_VS_OUTPUT_HLSLI