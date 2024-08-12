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
generate_rules_for_shader("shader_benchmarks_compute_filter_3"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/ComputeFilter3.hlsl"
    STAGES "cs")

generate_rules_for_shader("shader_benchmarks_compute_filter_5"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/ComputeFilter5.hlsl"
    STAGES "cs")

generate_rules_for_shader("shader_benchmarks_compute_filter_7"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/ComputeFilter7.hlsl"
    STAGES "cs")

generate_rules_for_shader("shader_benchmarks_passthrough_pos"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/PassThroughPos.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmarks_compute_buffer_increment"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/ComputeBufferIncrement.hlsl"
    STAGES "cs")

generate_rules_for_shader("shader_benchmarks_texture_sample"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/TextureSample.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmarks_texture_sample_explicit_early_z"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/TextureSample_ExplicitEarlyZ.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmarks_multiple_rt"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/MultipleRT.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmarks_texture_load"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/TextureLoad.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmarks_texture_load_4_textures"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/TextureLoad4Textures.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmark_vs_simple"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_VsSimple.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark.hlsli"
    STAGES "vs")

generate_rules_for_shader("shader_benchmark_vs_alu_bound"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_VsAluBound.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark.hlsli"
    STAGES "vs")

generate_rules_for_shader("shader_benchmark_ps_simple"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_PsSimple.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_benchmark_ps_alu_bound"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_PsAluBound.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_benchmark_ps_mem_bound"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_PsMemBound.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_benchmark_skybox"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_SkyBox.hlsl"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_benchmark_vs_simple_quads"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_VsSimpleQuads.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_Quad.hlsli"
    STAGES "vs")

generate_rules_for_shader("shader_benchmark_random_noise"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_RandomNoise.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_Quad.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_benchmark_solid_color"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_SolidColor.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_Quad.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_benchmark_texture"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_Texture.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/Benchmark_Quad.hlsli"
    STAGES "ps")

generate_rules_for_shader("shader_foveation_benchmark"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/FoveationBenchmark.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/VsOutput.hlsli"
    STAGES "vs" "ps")

generate_rules_for_shader("shader_foveation_benchmark_shading_rate_khr"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/FoveationBenchmarkShadingRateKHR.hlsl"
    INCLUDES "${PPX_DIR}/assets/benchmarks/shaders/VsOutput.hlsli"
    STAGES "ps")

generate_rules_for_glsl_shader("shader_foveation_benchmark_frag_size_ext_vs"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/FoveationBenchmarkFragSizeEXT.vert"
    STAGE "vs")

generate_rules_for_glsl_shader("shader_foveation_benchmark_frag_size_ext_ps"
    SOURCE "${PPX_DIR}/assets/benchmarks/shaders/FoveationBenchmarkFragSizeEXT.frag"
    STAGE "ps")
