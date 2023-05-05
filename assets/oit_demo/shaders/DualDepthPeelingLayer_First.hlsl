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
#include "TransparencyVS.hlsli"

SamplerState NearestSampler     : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    OpaqueDepthTexture : register(CUSTOM_TEXTURE_0_REGISTER);

float2 psmain(VSOutput input) : SV_TARGET
{
    float2 textureDimension = (float2)0;
    OpaqueDepthTexture.GetDimensions(textureDimension.x, textureDimension.y);
    const float2 uv = input.position.xy / textureDimension;

    const float fragmentDepth = input.position.z;

    // Test against opaque depth
    {
        const float opaqueDepth = OpaqueDepthTexture.Sample(NearestSampler, uv).r;
        clip(fragmentDepth < opaqueDepth ? 1.0f : -1.0f);
    }

    return float2(-fragmentDepth, fragmentDepth);
}
