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

Texture2D Tex[10]  : register(t1);  // Slot 0 is used by push constant.
RWStructuredBuffer<float> dataBuffer : register(u11);

float4 psmain(VSOutputPos input) : SV_TARGET
{
    uint32_t textureCount = Config.TextureCount;
    float4 color = {0.0f, 0.0f, 0.0f, 0.0f};
    for(uint32_t i = 0; i < textureCount; i++)
    {
        color += Tex[i].Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    }
    if (!any(color))
        dataBuffer[0] = color.r;
    return color;
}