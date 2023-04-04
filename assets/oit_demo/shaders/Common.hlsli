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

#define NEAREST_SAMPLER_REGISTER      SHADER_REGISTER(s, 0)
#define SHADER_GLOBALS_REGISTER       SHADER_REGISTER(b, 1)
#define OPAQUE_TEXTURE_REGISTER       SHADER_REGISTER(t, 2)
#define TRANSPARENCY_TEXTURE_REGISTER SHADER_REGISTER(t, 3)
#define CUSTOM_TEXTURE_0_REGISTER     SHADER_REGISTER(t, 4)
#define CUSTOM_TEXTURE_1_REGISTER     SHADER_REGISTER(t, 5)

#define EPSILON 0.0001f

struct ShaderGlobals
{
    float4x4 backgroundMVP;
    float4   backgroundColor;

    float4x4 meshMVP;
    float    meshOpacity;
};

#if defined(IS_SHADER)

ConstantBuffer<ShaderGlobals> g_Globals : register(SHADER_GLOBALS_REGISTER);

#endif
