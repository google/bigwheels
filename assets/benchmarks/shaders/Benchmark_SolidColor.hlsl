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
    float3 Value;
};

#if defined(__spirv__)
[[vk::push_constant]]
#endif
ConstantBuffer<ColorParams> Color : register(b0);

float4 psmain(VSOutputPos input) : SV_TARGET
{
    return float4(Color.Value, 1.0f);
}