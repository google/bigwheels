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

SamplerState NearestSampler    : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    FrontLayerTexture : register(CUSTOM_TEXTURE_0_REGISTER);
Texture2D    BackLayerTexture  : register(CUSTOM_TEXTURE_1_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    const float4 frontColor = FrontLayerTexture.Sample(NearestSampler, input.uv);
    const float4 backColor  = BackLayerTexture.Sample(NearestSampler, input.uv);

    // Under blend the two layers
    const float3 color = (frontColor.rgb * backColor.a) + backColor.rgb;
    const float alpha  = backColor.a * (1.0f - frontColor.a);
    return float4(color, alpha);
}
