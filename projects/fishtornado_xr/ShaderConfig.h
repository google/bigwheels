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

#ifndef SHADER_CONFIG_H
#define SHADER_CONFIG_H

#include "ppx/math_config.h"
using namespace ppx;

// -------------------------------------------------------------------------------------------------
// Scene
// -------------------------------------------------------------------------------------------------

namespace hlsl {

PPX_HLSL_PACK_BEGIN();
struct SceneData
{
    hlsl_float<4>     time;
    hlsl_float3<12>   eyePosition;
    hlsl_float4x4<64> viewMatrix;
    hlsl_float4x4<64> projectionMatrix;
    hlsl_float4x4<64> viewProjectionMatrix;
    hlsl_float<4>     fogNearDistance;
    hlsl_float<4>     fogFarDistance;
    hlsl_float<8>     fogPower;
    hlsl_float3<16>   fogColor;
    hlsl_float3<16>   lightPosition;
    hlsl_float3<16>   ambient;
    hlsl_float4x4<64> shadowViewProjectionMatrix;
    hlsl_float2<8>    shadowTextureDim;
    hlsl_uint<4>      usePCF;
};
PPX_HLSL_PACK_END();

} // namespace hlsl

// -------------------------------------------------------------------------------------------------
// Model
// -------------------------------------------------------------------------------------------------

namespace hlsl {

PPX_HLSL_PACK_BEGIN();
struct ModelData
{
    hlsl_float4x4<64> modelMatrix;
    hlsl_float4x4<64> mormalMatrix;
};
PPX_HLSL_PACK_END();

PPX_HLSL_PACK_BEGIN();
struct BeamModelData
{
    hlsl_float4x4<64> modelMatrix[2];
};
PPX_HLSL_PACK_END();

} // namespace hlsl

// -------------------------------------------------------------------------------------------------
// Flocking
// -------------------------------------------------------------------------------------------------

namespace hlsl {

PPX_HLSL_PACK_BEGIN();
struct FlockingData
{
    hlsl_int<4>     resX;
    hlsl_int<4>     resY;
    hlsl_float<4>   minThresh;
    hlsl_float<4>   maxThresh;
    hlsl_float<4>   minSpeed;
    hlsl_float<4>   maxSpeed;
    hlsl_float<4>   zoneRadius;
    hlsl_float<4>   time;
    hlsl_float<4>   timeDelta;
    hlsl_float3<12> predPos;
    hlsl_float3<12> camPos;
};
PPX_HLSL_PACK_END();

} // namespace hlsl

#endif // SHADER_CONFIG_H
