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

#ifndef GBUFFER_H
#define GBUFFER_H

// b#
#define SCENE_CONSTANTS_REGISTER    0
#define MATERIAL_CONSTANTS_REGISTER 1
#define MODEL_CONSTANTS_REGISTER    2
#define GBUFFER_CONSTANTS_REGISTER  3

// s#
#define CLAMPED_SAMPLER_REGISTER 4

// t#
#define LIGHT_DATA_REGISTER                  5
#define MATERIAL_ALBEDO_TEXTURE_REGISTER     6  // DeferredRender only
#define MATERIAL_ROUGHNESS_TEXTURE_REGISTER  7  // DeferredRender only
#define MATERIAL_METALNESS_TEXTURE_REGISTER  8  // DeferredRender only
#define MATERIAL_NORMAL_MAP_TEXTURE_REGISTER 9  // DeferredRender only
#define MATERIAL_AMB_OCC_TEXTURE_REGISTER    10 // DeferredRender only
#define MATERIAL_HEIGHT_MAP_TEXTURE_REGISTER 11 // DeferredRender only
#define MATERIAL_IBL_MAP_TEXTURE_REGISTER    12
#define MATERIAL_ENV_MAP_TEXTURE_REGISTER    13

// t#
#define GBUFFER_RT0_REGISTER 16 // DeferredLight only
#define GBUFFER_RT1_REGISTER 17 // DeferredLight only
#define GBUFFER_RT2_REGISTER 18 // DeferredLight only
#define GBUFFER_RT3_REGISTER 19 // DeferredLight only
#define GBUFFER_ENV_REGISTER 20 // DeferredLight only
#define GBUFFER_IBL_REGISTER 21 // DeferredLight only

// s#
#define GBUFFER_SAMPLER_REGISTER 6 // DeferredLight only

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

#endif // GBUFFER_H
