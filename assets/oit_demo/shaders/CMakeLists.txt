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

set(INCLUDE_FILES
    "${PPX_DIR}/assets/oit_demo/shaders/Common.hlsli")

generate_rules_for_shader(
    "opaque"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/Opaque.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "unsorted_over"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/UnsortedOver.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_sum"
	SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedSum.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_gather"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageGather.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_combine"
	SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageCombine.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_with_coverage_gather"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageWithCoverageGather.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_with_coverage_combine"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageWithCoverageCombine.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "composite"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/Composite.hlsl"
    INCLUDES ${INCLUDE_FILES}
    STAGES "ps" "vs")

generate_group_rule_for_shader(
    "shader_oit_demo"
    CHILDREN
    "opaque"
    "unsorted_over"
    "weighted_sum"
    "composite"
)

