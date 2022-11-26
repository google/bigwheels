# Copyright 2017 Pavel Dobryakov
# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

set(INCLUDE_FILES "${PPX_DIR}/assets/fluid_simulation/shaders/config.hlsli")

generate_rules_for_shader("shader_fluidsim_advection"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/advection.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_bloom_blur"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/bloom_blur.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_bloom_blur_additive"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/bloom_blur_additive.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_bloom_final"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/bloom_final.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_bloom_prefilter"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/bloom_prefilter.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_blur"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/blur.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_checkerboard"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/checkerboard.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_clear"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/clear.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_color"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/color.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_copy"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/copy.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_curl"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/curl.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_display"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/display.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_divergence"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/divergence.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_gradient_subtract"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/gradient_subtract.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_pressure"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/pressure.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_splat"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/splat.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_sunrays"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/sunrays.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_sunrays_mask"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/sunrays_mask.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_vorticity"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/vorticity.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "cs")

generate_rules_for_shader("shader_fluidsim_blur_vert"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/blur_vert.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs")

generate_rules_for_shader("shader_fluidsim_base"
    SOURCE "${PPX_DIR}/assets/fluid_simulation/shaders/base.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "vs")

generate_group_rule_for_shader(
    "shader_fluid_simulation"
    CHILDREN
    "shader_fluidsim_advection"
    "shader_fluidsim_base"
    "shader_fluidsim_bloom_blur"
    "shader_fluidsim_bloom_blur_additive"
    "shader_fluidsim_bloom_final"
    "shader_fluidsim_bloom_prefilter"
    "shader_fluidsim_blur"
    "shader_fluidsim_blur_vert"
    "shader_fluidsim_checkerboard"
    "shader_fluidsim_clear"
    "shader_fluidsim_color"
    "shader_fluidsim_copy"
    "shader_fluidsim_curl"
    "shader_fluidsim_display"
    "shader_fluidsim_divergence"
    "shader_fluidsim_gradient_subtract"
    "shader_fluidsim_pressure"
    "shader_fluidsim_splat"
    "shader_fluidsim_sunrays"
    "shader_fluidsim_sunrays_mask"
    "shader_fluidsim_vorticity"
)
