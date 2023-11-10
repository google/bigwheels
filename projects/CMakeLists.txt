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
  add_subdirectory(00_ppx_info)
endif ()
add_subdirectory(01_triangle)
add_subdirectory(02_triangle_spinning)
add_subdirectory(03_square_textured)
add_subdirectory(04_cube)
add_subdirectory(05_cube_textured)
add_subdirectory(06_compute_fill)
add_subdirectory(07_draw_indexed)
add_subdirectory(08_basic_geometry)
add_subdirectory(09_obj_geometry)
add_subdirectory(10_cube_map)
add_subdirectory(11_compressed_texture)
add_subdirectory(12_shadows)
add_subdirectory(13_normal_map)
add_subdirectory(14_input)
add_subdirectory(15_basic_material)
add_subdirectory(16_gbuffer)
add_subdirectory(17_primitives)
add_subdirectory(18_arcball_camera)
add_subdirectory(19_camera_fit_scene)
add_subdirectory(20_camera_motion)
add_subdirectory(21_text_draw)
add_subdirectory(22_image_filter)
add_subdirectory(23_async_compute)
add_subdirectory(24_push_constants)
add_subdirectory(25_push_descriptors_buffers)
add_subdirectory(26_push_descriptors)
add_subdirectory(27_mipmap_demo)
add_subdirectory(28_gltf)
add_subdirectory(alloc)
add_subdirectory(fishtornado)
add_subdirectory(fluid_simulation)
add_subdirectory(oit_demo)


add_subdirectory(04_cube_xr)
if (PPX_BUILD_XR)
add_subdirectory(fishtornado_xr)
endif()
