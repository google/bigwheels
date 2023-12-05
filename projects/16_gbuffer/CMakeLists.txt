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
project(16_gbuffer)

add_samples_for_all_apis(
    NAME ${PROJECT_NAME}
    SOURCES
    "main.cpp"
    "Entity.h"
    "Entity.cpp"
    "Material.h"
    "Material.cpp"
    "Render.h"
    SHADER_DEPENDENCIES
    "shader_gbuffer"
    "shader_fullscreen_triangle")
