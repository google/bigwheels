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
project(projects)

if (!PPX_ANDROID)
  add_subdirectory(sample_00_ppx_info)
endif ()
add_subdirectory(sample_01_triangle)
add_subdirectory(sample_02_triangle_spinning)
add_subdirectory(sample_03_square_textured)
add_subdirectory(sample_04_cube)
add_subdirectory(sample_05_cube_textured)
add_subdirectory(sample_06_compute_fill)
add_subdirectory(sample_07_draw_indexed)
add_subdirectory(sample_08_basic_geometry)
add_subdirectory(sample_09_obj_geometry)
add_subdirectory(sample_10_cube_map)
add_subdirectory(sample_11_compressed_texture)
add_subdirectory(sample_12_shadows)
add_subdirectory(sample_13_normal_map)
add_subdirectory(input)
add_subdirectory(basic_material)
add_subdirectory(gbuffer)
add_subdirectory(primitives)
add_subdirectory(arcball_camera)
add_subdirectory(camera_fit_scene)
add_subdirectory(camera_motion)
add_subdirectory(text_draw)
add_subdirectory(image_filter)
add_subdirectory(async_compute)
add_subdirectory(push_constants)
add_subdirectory(push_descriptors_buffers)
add_subdirectory(push_descriptors)
add_subdirectory(mipmap_demo)
add_subdirectory(gltf)
add_subdirectory(dynamic_rendering)
add_subdirectory(alloc)
add_subdirectory(fishtornado)
add_subdirectory(fluid_simulation)
add_subdirectory(oit_demo)

if (PPX_BUILD_XR)
add_subdirectory(sample_04_cube_xr)
add_subdirectory(fishtornado_xr)
endif()
