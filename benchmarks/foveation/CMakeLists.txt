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
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(foveation)

add_vk_sample(
    NAME ${PROJECT_NAME}
    SOURCES
      "main.cpp"
    DEPENDENCIES
      "shader_foveation_benchmark"
      "shader_foveation_benchmark_shading_rate_khr"
      "shader_foveation_benchmark_frag_size_ext_vs"
      "shader_foveation_benchmark_frag_size_ext_ps"
      "shader_fullscreen_triangle_combined")
