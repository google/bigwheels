// Copyright 2024 Google LLC
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

#ifndef MATERIAL_INTERFACE_HLSLI
#define MATERIAL_INTERFACE_HLSLI

#include "Config.hlsli"

// -------------------------------------------------------------------------------------------------
// Material Params Structs
// -------------------------------------------------------------------------------------------------
// size = 24
struct MaterialTextureParams
{
    uint     textureIndex;      // offset = 0
    uint     samplerIndex;      // offset = 4
    float2x2 texCoordTransform; // offset = 8
};

// size = 164
struct MaterialParams
{
    float4                baseColorFactor;      // offset = 0
    float                 metallicFactor;       // offset = 16
    float                 roughnessFactor;      // offset = 20
    float                 occlusionStrength;    // offset = 24
    float3                emissiveFactor;       // offset = 28
    float                 emissiveStrength;     // offset = 40
    MaterialTextureParams baseColorTex;         // offset = 44
    MaterialTextureParams metallicRoughnessTex; // offset = 68
    MaterialTextureParams normalTex;            // offset = 92
    MaterialTextureParams occlusionTex;         // offset = 116
    MaterialTextureParams emssiveTex;           // offset = 140
};

// -------------------------------------------------------------------------------------------------
// Draw Arguments
//
// Direct3D: register=b0, space=space0
// Vulkan  : Push Constants Block
// -------------------------------------------------------------------------------------------------
struct DrawParams {
    uint instanceIndex;   // push constant offset = 0
    uint materialIndex;   // push constant offset = 1
    uint iblIndex;        // push constant offset = 2
    uint iblLevelCount;   // push constant offset = 3
    uint dbgVtxAttrIndex; // push constant offset = 4
};

#if defined(__spirv__)
[[vk::push_constant]]
#endif 
ConstantBuffer<DrawParams> Draw : register(DRAW_PARAMS_REGISTER);

// -------------------------------------------------------------------------------------------------
// ConstantBuffers
// -------------------------------------------------------------------------------------------------
ConstantBuffer<FrameParams>  Frame  : register(FRAME_PARAMS_REGISTER);
ConstantBuffer<CameraParams> Camera : register(CAMERA_PARAMS_REGISTER);

// -------------------------------------------------------------------------------------------------
// Models and Materials Arrays
//   - use DrawParams::instanceIndex to index into Instances
//   - use DrawParams::materialIndex to index into Materials
//
// -------------------------------------------------------------------------------------------------
StructuredBuffer<InstanceParams> Instances : register(INSTANCE_PARAMS_REGISTER);
StructuredBuffer<MaterialParams> Materials : register(MATERIAL_PARAMS_REGISTER);

// -------------------------------------------------------------------------------------------------
// BRDF LUT Sampler and Texture
// -------------------------------------------------------------------------------------------------
SamplerState BRDFLUTSampler : register(BRDF_LUT_SAMPLER_REGISTER);
Texture2D    BRDFLUTTexture : register(BRDF_LUT_TEXTURE_REGISTER);

// -------------------------------------------------------------------------------------------------
// IBL Samplers and Textures
// -------------------------------------------------------------------------------------------------
SamplerState IBLIrradianceSampler             : register(IBL_IRRADIANCE_SAMPLER_REGISTER);
SamplerState IBLEnvironmentSampler            : register(IBL_ENVIRONMENT_SAMPLER_REGISTER);
Texture2D    IBLIrradianceMaps[MAX_IBL_MAPS]  : register(IBL_IRRADIANCE_MAP_REGISTER);
Texture2D    IBLEnvironmentMaps[MAX_IBL_MAPS] : register(IBL_ENVIRONMENT_MAP_REGISTER);

// -------------------------------------------------------------------------------------------------
// Material Samplers and Textures
//   - use MaterialTextureParams::samplerIndex to index into MaterialSamplers
//   - use MaterialTextureParams::textureIndex to index into MaterialTextures
// -------------------------------------------------------------------------------------------------
SamplerState MaterialSamplers[MAX_MATERIAL_SAMPLERS] : register(MATERIAL_SAMPLERS_REGISTER);
Texture2D    MaterialTextures[MAX_MATERIAL_TEXTURES] : register(MATERIAL_TEXTURES_REGISTER);

#endif // MATERIAL_INTERFACE_HLSLI
