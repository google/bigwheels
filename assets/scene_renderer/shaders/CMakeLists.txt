# Copyright 2023 Google LLC
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

set(SRC_DIR    "${PPX_DIR}/assets/scene_renderer/shaders")

list(APPEND INCLUDE_DIRS
    "${SRC_DIR}"
)

list(APPEND INCLUDE_FILES 
    "${SRC_DIR}/Config.hlsli"
)

# ------------------------------------------------------------------------------
# Material Shaders
# ------------------------------------------------------------------------------
set(COMMON_DIR "${PPX_DIR}/assets/common/shaders")

list(APPEND INCLUDE_DIRS
    "${SRC_DIR}"
    "${COMMON_DIR}"
)

list(APPEND INCLUDE_FILES 
    "${SRC_DIR}/MaterialInterface.hlsli"
    "${COMMON_DIR}/ppx/BRDF.hlsli"
    "${COMMON_DIR}/ppx/Math.hlsli"
    "${COMMON_DIR}/ppx/PBR.hlsli"
    "${COMMON_DIR}/ppx/Sampling.hlsli"
)

generate_rules_for_shader("shader_scene_renderer_vertex_material_vertex"
    SOURCE "${SRC_DIR}/MaterialVertex.hlsl"
    INCLUDES ${INCLUDE_FILES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    STAGES "vs")

generate_rules_for_shader("shader_scene_renderer_material_error"
    SOURCE "${SRC_DIR}/ErrorMaterial.hlsl"
    INCLUDES ${INCLUDE_FILES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    STAGES "ps")

generate_rules_for_shader("shader_scene_renderer_material_debug"
    SOURCE "${SRC_DIR}/DebugMaterial.hlsl"
    INCLUDES ${INCLUDE_FILES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    STAGES "ps")

generate_rules_for_shader("shader_scene_renderer_material_unlit"
    SOURCE "${SRC_DIR}/UnlitMaterial.hlsl"
    INCLUDES ${INCLUDE_FILES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    STAGES "ps")

generate_rules_for_shader("shader_scene_renderer_material_standard"
    SOURCE "${SRC_DIR}/StandardMaterial.hlsl"
    INCLUDES ${INCLUDE_FILES} 
    INCLUDE_DIRS ${INCLUDE_DIRS}
    STAGES "ps")
