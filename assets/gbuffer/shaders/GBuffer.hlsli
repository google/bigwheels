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

#ifndef GBUFFER_HLSLI
#define GBUFFER_HLSLI

#define GBUFFER_SPACE space1

#define GBUFFER_CONSTANTS_REGISTER b3

// 
// The register numbers are purposely incremental to
// achieve compatibility between D3D12 and Vulkan.
//
#define GBUFFER_RT0_REGISTER     t16
#define GBUFFER_RT1_REGISTER     t17
#define GBUFFER_RT2_REGISTER     t18
#define GBUFFER_RT3_REGISTER     t19
#define GBUFFER_ENV_REGISTER     t20
#define GBUFFER_IBL_REGISTER     t21

#define GBUFFER_SAMPLER_REGISTER s6

// GBuffer Attributes
#define GBUFFER_POSITION     0
#define GBUFFER_NORMAL       1
#define GBUFFER_ALBEDO       2
#define GBUFFER_F0           3
#define GBUFFER_ROUGHNESS    4
#define GBUFFER_METALNESS    5
#define GBUFFER_AMB_OCC      6
#define GBUFFER_IBL_STRENGTH 7
#define GBUFFER_ENV_STRENGTH 8


struct GBuffer
{
    float3 position;
    float3 normal;
    float3 albedo;
    float  F0;
    float  roughness;
    float  metalness;
    float  ambOcc;
    float  iblStrength;
    float  envStrength;
};

struct PackedGBuffer
{
    float4 rt0 : SV_TARGET0;
    float4 rt1 : SV_TARGET1;
    float4 rt2 : SV_TARGET2;
    float4 rt3 : SV_TARGET3;
};

PackedGBuffer PackGBuffer(GBuffer gbuffer)
{
    PackedGBuffer packed = (PackedGBuffer)0;
    packed.rt0 = float4(gbuffer.position, gbuffer.roughness);
    packed.rt1 = float4(gbuffer.normal, gbuffer.metalness);
    packed.rt2 = float4(gbuffer.albedo, gbuffer.F0);
    packed.rt3 = float4(gbuffer.ambOcc, gbuffer.iblStrength, gbuffer.envStrength, 0);
    return packed;
}

GBuffer UnpackGBuffer(PackedGBuffer packed)
{
    GBuffer gbuffer = (GBuffer)0;
    gbuffer.position    = packed.rt0.xyz;
    gbuffer.normal      = packed.rt1.xyz;
    gbuffer.albedo      = packed.rt2.xyz;
    gbuffer.F0          = packed.rt2.w;
    gbuffer.roughness   = packed.rt0.w;
    gbuffer.metalness   = packed.rt1.w;
    gbuffer.ambOcc      = packed.rt3.r;
    gbuffer.iblStrength = packed.rt3.g;
    gbuffer.envStrength = packed.rt3.b;
    return gbuffer;
};

#endif // GBUFFER_HLSLI