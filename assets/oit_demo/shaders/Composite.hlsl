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

Texture2D    OpaqueTexture       : register(OPAQUE_TEXTURE_REGISTER);
Texture2D    TransparencyTexture : register(TRANSPARENCY_TEXTURE_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    const int3 texelCoord = int3(input.position.xy, 0);

    const float3 opaqueColor = OpaqueTexture.Load(texelCoord).rgb;

    const float4 transparencySample = TransparencyTexture.Load(texelCoord);
    const float3 transparencyColor  = transparencySample.rgb;
    const float  coverage           = transparencySample.a;

    const float3 finalColor = transparencyColor + ((1.0f - coverage) * opaqueColor);
    return float4(finalColor, 1.0f);
}
