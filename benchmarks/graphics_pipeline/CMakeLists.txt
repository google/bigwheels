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
project(graphics_pipeline)

add_samples_for_all_apis(
    NAME ${PROJECT_NAME}
    SOURCES 
      "FreeCamera.h"
      "FreeCamera.cpp"
      "GraphicsBenchmarkApp.h"
      "GraphicsBenchmarkApp.cpp"
      "main.cpp"
      "MultiDimensionalIndexer.h"
      "MultiDimensionalIndexer.cpp"
      "SphereMesh.h"
      "SphereMesh.cpp"
    SHADER_DEPENDENCIES
      "shader_benchmark_vs_simple"
      "shader_benchmark_vs_alu_bound"
      "shader_benchmark_ps_simple"
      "shader_benchmark_ps_alu_bound"
      "shader_benchmark_ps_mem_bound"
      "shader_benchmark_random_noise"
      "shader_benchmark_skybox"
      "shader_benchmark_solid_color"
      "shader_benchmark_texture"
      "shader_benchmark_texture-100"
      "shader_benchmark_texture-62"
      "shader_benchmark_vs_simple_quads"
      "shader_fullscreen_triangle")
