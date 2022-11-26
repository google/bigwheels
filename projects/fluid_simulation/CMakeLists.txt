# Copyright 2017 Pavel Dobryakov
# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(fluid_simulation)

add_samples_for_all_apis(
    NAME ${PROJECT_NAME}
    SOURCES
    "main.cpp"
    "shaders.h"
    "shaders.cpp"
    "sim.h"
    "sim.cpp"
    SHADER_DEPENDENCIES
    "shader_static_texture"
    "shader_fluid_simulation")
