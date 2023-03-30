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

#define IS_SHADER
#include "Common.hlsli"
#include "FullscreenVS.hlsli"

Texture2D        ColorTexture : register(CUSTOM_TEXTURE_0_REGISTER);
Texture2D<float> CountTexture : register(CUSTOM_TEXTURE_1_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    const int3 texelCoord = int3(input.position.xy, 0);

    const float4 colorInfo    = ColorTexture.Load(texelCoord);
    const float3 colorSum     = colorInfo.rgb;
    const float  alphaSum     = max(colorInfo.a, EPSILON);
    const float3 averageColor = colorSum / alphaSum;

    const uint count     = max(1, (uint)CountTexture.Load(texelCoord));
    const float coverage = 1.0f - pow(max(0.0f, 1.0f - (alphaSum / count)), count);

    return float4(averageColor * coverage, coverage);
}
