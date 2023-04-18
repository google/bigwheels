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

#if defined(IS_SHADER)
#define SHADER_REGISTER(type, num) type##num
#else
#define SHADER_REGISTER(type, num) num
#endif

#define SHADER_GLOBALS_REGISTER       SHADER_REGISTER(b, 0)
#define CUSTOM_SAMPLER_0_REGISTER     SHADER_REGISTER(s, 1)
#define CUSTOM_SAMPLER_1_REGISTER     SHADER_REGISTER(s, 2)
#define CUSTOM_TEXTURE_0_REGISTER     SHADER_REGISTER(t, 3)
#define CUSTOM_TEXTURE_1_REGISTER     SHADER_REGISTER(t, 4)
#define CUSTOM_TEXTURE_2_REGISTER     SHADER_REGISTER(t, 5)
#define CUSTOM_TEXTURE_3_REGISTER     SHADER_REGISTER(t, 6)
#define CUSTOM_TEXTURE_4_REGISTER     SHADER_REGISTER(t, 7)
#define CUSTOM_TEXTURE_5_REGISTER     SHADER_REGISTER(t, 8)
#define CUSTOM_TEXTURE_6_REGISTER     SHADER_REGISTER(t, 9)
#define CUSTOM_TEXTURE_7_REGISTER     SHADER_REGISTER(t, 10)
#define CUSTOM_UAV_0_REGISTER         SHADER_REGISTER(u, 11)
#define CUSTOM_UAV_1_REGISTER         SHADER_REGISTER(u, 12)
#define CUSTOM_UAV_2_REGISTER         SHADER_REGISTER(u, 13)
#define CUSTOM_UAV_3_REGISTER         SHADER_REGISTER(u, 14)

#define EPSILON 0.0001f

#define DEPTH_PEELING_LAYERS_COUNT              8
#define DEPTH_PEELING_DEPTH_TEXTURES_COUNT      2

#define BUFFER_BUCKETS_SIZE_PER_PIXEL           8

#define BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE  8
#define BUFFER_LISTS_MAX_SIZE                   64
#define BUFFER_LISTS_INVALID_INDEX              0xFFFFFFFFU

struct ShaderGlobals
{
    float4x4 backgroundMVP;
    float4   backgroundColor;
    float4x4 meshMVP;

    float    meshOpacity;
    float    _floatUnused0;
    float    _floatUnused1;
    float    _floatUnused2;

    int      depthPeelingFrontLayerIndex;
    int      depthPeelingBackLayerIndex;
    int      bufferBucketsFragmentsMaxCount;
    int      bufferListsFragmentBufferScale;
    int      bufferListsMaxSize;
    int      _intUnused0;
    int      _intUnused1;
    int      _intUnused2;
};

#if defined(IS_SHADER)

ConstantBuffer<ShaderGlobals> g_Globals : register(SHADER_GLOBALS_REGISTER);

void MergeColor(inout float4 outColor, float4 fragmentColor)
{
    outColor.rgb = lerp(outColor.rgb, fragmentColor.rgb, fragmentColor.a);
    outColor.a *= 1.0f - fragmentColor.a;
}

uint PackColor(float4 color)
{
    const uint4 ci = (uint4)(clamp(color, 0.0f, 1.0f) * 255.0f);
    return (ci.r << 24) | (ci.g << 16) | (ci.b << 8) | ci.a;
}

float4 UnpackColor(uint data)
{
    const float red   = float((data >> 24) & 0xFF) / 255.0f;
    const float green = float((data >> 16) & 0xFF) / 255.0f;
    const float blue  = float((data >> 8) & 0xFF) / 255.0f;
    const float alpha = float(data & 0xFF) / 255.0f;
    return float4(red, green, blue, alpha);
}

#endif
