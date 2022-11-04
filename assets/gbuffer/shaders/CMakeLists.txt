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
set(INCLUDE_FILES
    "${PPX_DIR}/assets/gbuffer/shaders/Config.hlsli"
    "${PPX_DIR}/assets/gbuffer/shaders/GBuffer.hlsli")

generate_rules_for_shader("shader_gbuffer_vertex_shader"
    SOURCE "${PPX_DIR}/assets/gbuffer/shaders/VertexShader.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs")

generate_rules_for_shader("shader_gbuffer_deferred_light"
    SOURCE "${PPX_DIR}/assets/gbuffer/shaders/DeferredLight.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs" "ps")

generate_rules_for_shader("shader_gbuffer_draw_attributes"
    SOURCE "${PPX_DIR}/assets/gbuffer/shaders/DrawGBufferAttribute.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs" "ps")

generate_rules_for_shader("shader_gbuffer_deferred_render"
    SOURCE "${PPX_DIR}/assets/gbuffer/shaders/DeferredRender.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps")

generate_group_rule_for_shader(
    "shader_gbuffer"
    CHILDREN
    "shader_gbuffer_vertex_shader"
    "shader_gbuffer_deferred_light"
    "shader_gbuffer_draw_attributes"
    "shader_gbuffer_deferred_render"
)
