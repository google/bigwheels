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
project(benchmarks)

add_subdirectory(draw_call)
add_subdirectory(compute_operations)
add_subdirectory(headless_compute)
add_subdirectory(primitive_assembly)
add_subdirectory(render_target)
add_subdirectory(texture_load)
add_subdirectory(texture_sample)
add_subdirectory(texture_transfer_cpu_to_gpu)
add_subdirectory(overdraw)
