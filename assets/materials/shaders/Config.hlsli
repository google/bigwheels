// Copyright 2022 Google LLC
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

#ifndef CONFIG_HLSLI
#define CONFIG_HLSLI

#define SCENE_DATA_SPACE         space0
#define MATERIAL_RESOURCES_SPACE space1
#define MATERIAL_DATA_SPACE      space2
#define MODEL_DATA_SPACE         space3

//
// The register numbers are purposely incremental to
// achieve compatibility between D3D12 and Vulkan.
//
#define SCENE_CONSTANTS_REGISTER    b0
#define MATERIAL_CONSTANTS_REGISTER b1
#define MODEL_CONSTANTS_REGISTER    b2

#define CLAMPED_SAMPLER_REGISTER    s3

#define LIGHT_DATA_REGISTER         t4
#define ALBEDO_TEXTURE_REGISTER     t5
#define ROUGHNESS_TEXTURE_REGISTER  t6
#define METALNESS_TEXTURE_REGISTER  t7
#define NORMAL_MAP_TEXTURE_REGISTER t8
#define AMB_OCC_TEXTURE_REGISTER    t9
#define HEIGHT_MAP_TEXTURE_REGISTER t10
#define IRR_MAP_TEXTURE_REGISTER    t11
#define ENV_MAP_TEXTURE_REGISTER    t12
#define BRDF_LUT_TEXTURE_REGISTER   t13

struct SceneData
{
    uint     frameNumber;
    float    time;
    float4x4 viewProjectionMatrix;
    float3   eyePosition;
    uint     lightCount;
    float    ambient;
    float    envLevelCount;
    uint     useBRDFLUT;
};

struct Light
{
    uint   type;
    float3 position;
    float3 color;
    float  intensity;
};

struct MaterialData
{
    float3 F0;
    float3 albedo;
    float  roughness;
    float  metalness;
    float  iblStrength;
    float  envStrength;
    uint   albedoSelect;     // 0 = value, 1 = texture
    uint   roughnessSelect;  // 0 = value, 1 = texture
    uint   metalnessSelect;  // 0 = value, 1 = texture
    uint   normalSelect;     // 0 = attrb, 1 = texture
    uint   iblSelect;        // 0 = white, 1 = texture
    uint   envSelect;        // 0 = none,  1 = texture
};

struct ModelData
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
    float3   debugColor;
};

struct VSInput
{
    float3 position  : POSITION;
    float3 color     : COLOR;
    float3 normal    : NORMAL;
    float2 texCoord  : TEXCOORD;
    float3 tangent   : TANGENT;
    float3 bitangnet : BITANGENT;
};

struct VSOutput
{
    float4 position    : SV_POSITION;
    float3 positionWS  : POSITIONWS;
    float3 color       : COLOR;
    float3 normal      : NORMAL;
    float2 texCoord    : TEXCOORD;
    float3 normalTS    : NORMALTS;
    float3 tangentTS   : TANGENTTS;
    float3 bitangnetTS : BITANGENTTS;
};

#endif // CONFIG_HLSLI
