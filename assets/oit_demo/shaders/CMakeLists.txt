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

set(COMMON_INCLUDE_FILES
    "${PPX_DIR}/assets/oit_demo/shaders/Common.hlsli")

set(TRANSPARENCY_INCLUDE_FILES
    ${COMMON_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/TransparencyVS.hlsli")

set(FULLSCREEN_INCLUDE_FILES
    ${COMMON_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/FullscreenVS.hlsli")

################################################################################

generate_rules_for_shader(
    "opaque"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/Opaque.hlsl"
    INCLUDES ${COMMON_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "unsorted_over"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/UnsortedOver.hlsl"
    INCLUDES ${TRANSPARENCY_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_sum"
	SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedSum.hlsl"
    INCLUDES ${TRANSPARENCY_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_fragment_count_gather"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageFragmentCountGather.hlsl"
    INCLUDES
    ${TRANSPARENCY_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageGather.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_fragment_count_combine"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageFragmentCountCombine.hlsl"
    INCLUDES
    ${FULLSCREEN_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageCombine.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_exact_coverage_gather"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageExactCoverageGather.hlsl"
    INCLUDES
    ${TRANSPARENCY_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageGather.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "weighted_average_exact_coverage_combine"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageExactCoverageCombine.hlsl"
    INCLUDES
    ${FULLSCREEN_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/WeightedAverageCombine.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "composite"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/Composite.hlsl"
    INCLUDES ${FULLSCREEN_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "depth_peeling_layer_first"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/DepthPeelingLayer_First.hlsl"
    INCLUDES
    ${TRANSPARENCY_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/DepthPeelingLayer.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "depth_peeling_layer_others"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/DepthPeelingLayer_Others.hlsl"
    INCLUDES
    ${TRANSPARENCY_INCLUDE_FILES}
    "${PPX_DIR}/assets/oit_demo/shaders/DepthPeelingLayer.hlsli"
    STAGES "ps" "vs")

generate_rules_for_shader(
    "depth_peeling_combine"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/DepthPeelingCombine.hlsl"
    INCLUDES ${FULLSCREEN_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "buffer_buckets_gather"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/BufferBucketsGather.hlsl"
    INCLUDES ${TRANSPARENCY_INCLUDE_FILES}
    STAGES "ps" "vs")

generate_rules_for_shader(
    "buffer_buckets_combine"
    SOURCE "${PPX_DIR}/assets/oit_demo/shaders/BufferBucketsCombine.hlsl"
    INCLUDES ${FULLSCREEN_INCLUDE_FILES}
    STAGES "ps" "vs")

################################################################################

generate_group_rule_for_shader(
    "shader_oit_demo"
    CHILDREN
    "opaque"
    "unsorted_over"
    "weighted_sum"
    "weighted_average_fragment_count_gather"
    "weighted_average_fragment_count_combine"
    "weighted_average_exact_coverage_gather"
    "weighted_average_exact_coverage_combine"
    "depth_peeling_layer_first"
    "depth_peeling_layer_others"
    "depth_peeling_combine"
    "buffer_buckets_gather"
    "buffer_buckets_combine"
    "composite"
)

