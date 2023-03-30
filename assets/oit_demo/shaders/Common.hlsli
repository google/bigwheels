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
#define OPAQUE_TEXTURE_REGISTER       SHADER_REGISTER(t, 1)
#define TRANSPARENCY_TEXTURE_REGISTER SHADER_REGISTER(t, 2)
#define CUSTOM_TEXTURE_0_REGISTER     SHADER_REGISTER(t, 3)
#define CUSTOM_TEXTURE_1_REGISTER     SHADER_REGISTER(t, 4)

#define EPSILON 0.0001f

struct ShaderGlobals
{
    float4x4 backgroundMVP;
    float4   backgroundColor;

    float4x4 meshMVP;
    float    meshOpacity;
};

#if defined(IS_SHADER)

#if defined(PPX_D3D11)
#define DECLARE_CONSTANT_BUFFER(type, name, reg) \
    cbuffer type : register(reg)                 \
    {                                            \
        type name;                               \
    };
#else
#define DECLARE_CONSTANT_BUFFER(type, name, reg) \
    ConstantBuffer<type> name : register(reg);
#endif

DECLARE_CONSTANT_BUFFER(ShaderGlobals, g_Globals, SHADER_GLOBALS_REGISTER);

#endif
