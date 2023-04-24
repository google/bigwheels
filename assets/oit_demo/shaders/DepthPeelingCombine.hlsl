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

SamplerState NearestSampler                            : register(CUSTOM_SAMPLER_0_REGISTER);
Texture2D    LayerTextures[DEPTH_PEELING_LAYERS_COUNT] : register(CUSTOM_TEXTURE_0_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for(int i = g_Globals.depthPeelingBackLayerIndex; i >= g_Globals.depthPeelingFrontLayerIndex; --i)
    {
        MergeColor(color, LayerTextures[i].Sample(NearestSampler, input.uv));
    }
    color.a = 1.0f - color.a;
    return color;
}
