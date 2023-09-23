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

#ifndef CONFIG_HLSLI
#define CONFIG_HLSLI

#if defined(__spirv__)
#define DECLARE_LOCATION(LOC_NUM) [[vk::location(LOC_NUM)]]
#else
#define DECLARE_LOCATION(LOC_NUM)
#endif

#define INVALID_INDEX 0xFFFFFFFF

#define MAX_UNIQUE_MATERIALS      8192
#define MAX_TEXTURES_PER_MATERIAL 6
#define MAX_MATERIAL_TEXTURES     (MAX_UNIQUE_MATERIALS * MAX_TEXTURES_PER_MATERIAL)

#define MAX_MATERIAL_SAMPLERS     64

#define MAX_IBL_MAPS              16

// -------------------------------------------------------------------------------------------------
// Registers
// -------------------------------------------------------------------------------------------------
#define DRAW_PARAMS_REGISTER                b0
#define FRAME_PARAMS_REGISTER               b1
#define CAMERA_PARAMS_REGISTER              b2

#define INSTANCE_PARAMS_REGISTER            t3
#define MATERIAL_PARAMS_REGISTER            t4

#define BRDF_LUT_SAMPLER_REGISTER           s124
#define BRDF_LUT_TEXTURE_REGISTER           t125

#define IBL_IRRADIANCE_SAMPLER_REGISTER     s126
#define IBL_ENVIRONMENT_SAMPLER_REGISTER    s127
#define IBL_IRRADIANCE_MAP_REGISTER         t128
#define IBL_ENVIRONMENT_MAP_REGISTER        t144

#define MATERIAL_SAMPLERS_REGISTER          s512
#define MATERIAL_TEXTURES_REGISTER          t1024

//// -------------------------------------------------------------------------------------------------
//// Spaces
////
//// Different spaces for each resource type in case we want to have
//// a bindless entry at the end. Vulkan requires this since it
//// does not have resource type namespacing.
////
//// -------------------------------------------------------------------------------------------------
//#define CBV_SPACE             space0 // ConstantBuffer
//#define SRV_UAV_BUFFER_SPACE  space1 // StructuredBuffer, RWStructuredBuffer
//#define SRV_TEXTURE_SPACE     space2 // Texture
//#define UAV_TEXTURE_SPACE     space3 // RWTexture

// -------------------------------------------------------------------------------------------------
// Params Structs
// -------------------------------------------------------------------------------------------------
// size = 8
struct FrameParams
{
    uint32_t frameIndex; // offset = 0
    float    time;       // offset = 4
};

// size = 96
struct CameraParams
{
    float4x4 viewProjectionMatrix; // offset = 0
    float3   eyePosition;          // offset = 64
    float    nearDepth;            // offset = 76
    float3   viewDirection;        // offset = 80
    float    farDepth;             // offset = 92
};

// size = 128
struct InstanceParams
{
    float4x4 modelMatrix;        // offset = 0  (Object space to world space)
    float4x4 inverseModelMatrix; // offset = 64 (World space to object space)
};

// -------------------------------------------------------------------------------------------------
// Vertex Inputs 
//
// Uses fixed location numbers to make it easier on the client
// side Vulkan code if there's gaps due to unused inputs.
//
// The #ifdef statements are required currently because Direct3D
// doesn't seem to dead code eliminate unused inputs. So we have
// to let the shader implementations control which attributes
// they want to enable.
//
// -------------------------------------------------------------------------------------------------
struct StandardVertexInput
{
    DECLARE_LOCATION(0) float3 PositionOS : POSITION;

#if defined(ENABLE_VTX_ATTR_TEXCOORD)
    DECLARE_LOCATION(1) float2 TexCoord : TEXCOORD;
#endif

#if defined(ENABLE_VTX_ATTR_NORMAL)  
    DECLARE_LOCATION(2) float3 Normal : NORMAL;
#endif

#if defined(ENABLE_VTX_ATTR_TANGENT)
    DECLARE_LOCATION(3) float4 Tangent : TANGENT;
#endif

#if defined(ENABLE_VTX_ATTR_COLOR)
    DECLARE_LOCATION(4) float3 Color : COLOR;
#endif
};

// -------------------------------------------------------------------------------------------------
// Vertex Outputs
//
// Uses fixed location numbers to make it easier on the client
// side Vulkan code if there's gaps due to unused inputs.
//
// The #ifdef statements are required currently because Direct3D
// doesn't seem to dead code eliminate unused inputs. So we have
// to let the shader implementations control which attributes
// they want to enable.
//
// -------------------------------------------------------------------------------------------------
struct StandardVertexOutput
{
    DECLARE_LOCATION(0) float4 PositionCS : SV_POSITION;
    DECLARE_LOCATION(1) float4 PositionWS : POSITIONWS;

#if defined(ENABLE_VTX_ATTR_TEXCOORD)
    DECLARE_LOCATION(2) float2 TexCoord : TEXCOORD;
#endif

#if defined(ENABLE_VTX_ATTR_NORMAL)
    DECLARE_LOCATION(3) float3 Normal : NORMAL;
#endif

#if defined(ENABLE_VTX_ATTR_TANGENT)
    DECLARE_LOCATION(4) float4 Tangent : TANGENT;
#endif

#if defined(ENABLE_VTX_ATTR_COLOR)
    DECLARE_LOCATION(5) float3 Color : COLOR;
#endif
};

#endif // CONFIG_HLSLI
