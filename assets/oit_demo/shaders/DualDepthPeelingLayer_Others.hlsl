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
Texture2D    DepthBoundsTexture : register(CUSTOM_TEXTURE_1_REGISTER);

struct PSOutput
{
    float4 frontLayer  : SV_TARGET0;
    float4 backLayer   : SV_TARGET1;
    float2 depthBounds : SV_TARGET2;
};

PSOutput psmain(VSOutput input)
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

    const float2 depthBounds = DepthBoundsTexture.Sample(NearestSampler, uv).rg;
    const float frontLayerDepth = -depthBounds.r;
    const float backLayerDepth = depthBounds.g;

    // Skip out-of-bounds fragments
    clip(fragmentDepth > backLayerDepth || fragmentDepth < frontLayerDepth ? -1.0f : 1.0f);

    PSOutput output = (PSOutput)0;

    // Shade the front layer
    if(fragmentDepth == frontLayerDepth)
    {
        output.frontLayer  = float4(input.color, g_Globals.meshOpacity);
        output.backLayer   = (float4)0.0f;
        output.depthBounds = float2(-(fragmentDepth + FLOAT_EPSILON), fragmentDepth);
        return output;
    }

    // Shade the back layer
    if(fragmentDepth == backLayerDepth)
    {
        output.frontLayer  = (float4)0.0f;
        output.backLayer   = float4(input.color, g_Globals.meshOpacity);
        output.depthBounds = float2(-fragmentDepth, fragmentDepth - FLOAT_EPSILON);
        return output;
    }

    // No shading, just write the depth bounds
    output.frontLayer  = (float4)0.0f;
    output.backLayer   = (float4)0.0f;
    output.depthBounds = float2(-fragmentDepth, fragmentDepth);
    return output;
}
