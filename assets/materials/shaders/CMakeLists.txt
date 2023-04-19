# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
set(INCLUDE_FILES "${PPX_DIR}/assets/materials/shaders/Config.hlsli")

generate_rules_for_shader("shader_mtl_vertex_shader"
    SOURCE "${PPX_DIR}/assets/materials/shaders/VertexShader.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs")

generate_rules_for_shader("shader_mtl_gouraud"
    SOURCE "${PPX_DIR}/assets/materials/shaders/Gouraud.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps")

generate_rules_for_shader("shader_mtl_phong"
    SOURCE "${PPX_DIR}/assets/materials/shaders/Phong.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps")

generate_rules_for_shader("shader_mtl_blinn_phong"
    SOURCE "${PPX_DIR}/assets/materials/shaders/BlinnPhong.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps")

generate_rules_for_shader("shader_mtl_pbr"
    SOURCE "${PPX_DIR}/assets/materials/shaders/PBR.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps")

generate_rules_for_shader("shader_mtl_env_draw_vertex_shader"
    SOURCE "${PPX_DIR}/assets/materials/shaders/EnvDraw.hlsl"
    STAGES "vs")

generate_rules_for_shader("shader_mtl_env_draw_pixel_shader"
    SOURCE "${PPX_DIR}/assets/materials/shaders/EnvDraw.hlsl"
    STAGES "ps")
