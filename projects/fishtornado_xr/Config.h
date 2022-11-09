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

#ifndef CONFIG_H
#define CONFIG_H

// b#
#define RENDER_SCENE_DATA_REGISTER    0
#define RENDER_MODEL_DATA_REGISTER    3
#define RENDER_MATERIAL_DATA_REGISTER 4
// s#
#define RENDER_SHADOW_SAMPLER_REGISTER 2
// t#
#define RENDER_SHADOW_TEXTURE_REGISTER            1
#define RENDER_ALBEDO_TEXTURE_REGISTER            5
#define RENDER_ROUGHNESS_TEXTURE_REGISTER         6
#define RENDER_NORMAL_MAP_TEXTURE_REGISTER        7
#define RENDER_CAUSTICS_TEXTURE_REGISTER          8
#define RENDER_CLAMPED_SAMPLER_REGISTER           9
#define RENDER_REPEAT_SAMPLER_REGISTER            10
#define RENDER_FLOCKING_DATA_REGISTER             11
#define RENDER_PREVIOUS_POSITION_TEXTURE_REGISTER 12
#define RENDER_CURRENT_POSITION_TEXTURE_REGISTER  13
#define RENDER_PREVIOUS_VELOCITY_TEXTURE_REGISTER 14
#define RENDER_CURRENT_VELOCITY_TEXTURE_REGISTER  15
// u#
#define RENDER_OUTPUT_POSITION_TEXTURE_REGISTER 16
#define RENDER_OUTPUT_VELOCITY_TEXTURE_REGISTER 17

#endif //CONFIG_H
