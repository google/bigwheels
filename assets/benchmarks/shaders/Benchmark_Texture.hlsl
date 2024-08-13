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

Texture2D Tex0 : register(t1);  // Slot 0 is used by push constant.
Texture2D Tex1 : register(t2);
Texture2D Tex2 : register(t3);
Texture2D Tex3 : register(t4);
Texture2D Tex4 : register(t5);
Texture2D Tex5 : register(t6);
Texture2D Tex6 : register(t7);
Texture2D Tex7 : register(t8);
Texture2D Tex8 : register(t9);
Texture2D Tex9 : register(t10);

float4 psmain(VSOutputPos input) : SV_TARGET
{
    Texture2D textureArray[3] = {Tex0, Tex1, Tex2};
    uint32_t textureCount = Config.TextureCount;
    float4 color = Tex0.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    color += Tex1.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    if(textureCount <= 2) {
        return color;
    }
    color += Tex2.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    color += Tex3.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    if(textureCount <= 4) {
        return color;
    }
    color += Tex4.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    color += Tex5.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    if (textureCount <= 6) {
      return color;
    }
    color += Tex6.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    color += Tex7.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    if (textureCount <= 8) {
      return color;
    }
    color += Tex8.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);
    color += Tex9.Load(uint3(input.position.x, input.position.y, 0))/float(textureCount);

    return color;
}