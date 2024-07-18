# Overview
BigWheels is a cross-platform, API agnostic framework to build graphics applications.

Supported platforms:
* Windows
* Linux
* Android

Supported graphics APIs:
* DirectX 12
* Vulkan

*Note: This is not an officially supported Google product.*

# Requirements
 * Software
   * Linux
     * C++17 compliant compiler (clang 10+ or gcc 9+)
     * Visual Studio Code or CLI
     * Recent Vulkan SDK
       * 1.3.216.0 or later
   * Windows
     * Visual Studio 2019 or 2022, or Visual Studio Code
     * Recent Vulkan SDK
       * 1.3.216.0 or later
     * Recent version of Windows SDK
       * 10.0.22621.0 or later
   * Android
     * Android Studio, 2022.1.1 Patch 2 or later
     * Recent version of NDK
       * 25.1.8937393 or later
 * Hardware (*not an exhaustive list*)
   * AMD
     * Vega GPUs
       * Vega 56
       * Vega 64
     * Navi GPUs
        * Radeon 5700
        * Radeon 5700 XT
        * Radeon 6700 XT
   * NVIDIA
     * 30x0 GPUs
     * 20x0 GPUs
     * 16x0 GPus
     * 10x0 GPUs
       * *This is iffy - some of the later more recent DX12 and Vulkan features many not work.*
    * Intel
       * None tested

# Building and running
See the [documentation](docs/building_and_running.md) for instructions on how to build and run BigWheels and the sample projects.

# Unit Tests
BigWheels uses GoogleTest for its unit testing framework. Unit tests can be optionally disabled from the build with `-DPPX_BUILD_TESTS=OFF`.

To add a new test:
1) Add and write a new test file under `src/test/`, e.g. `src/test/new_test.cpp`.
2) Add the new file name to the list of test sources in `src/test/CMakeLists.txt`.

To build tests (without running them), use `ninja build-tests`. Test binaries are output in `test/`.

To run tests (they will be built if needed), use `ninja run-tests`.

To generate code coverage, consult our additional [documentation](docs/code_coverage.md).

# Benchmarks
BigWheels includes a variety of benchmarks that test graphics fundamentals under the `benchmarks` folder. See the [documentation](docs/benchmarks.md) for benchmarks on how to build and run them.

# Software rendering
If you don't have a compatible GPU, you can use a software renderer to run BigWheels applications. See the [documentation](docs/software_rendering.md) for software rendering options for Vulkan and DirectX.

# CI and testing
We run automated workflows for testing as part of PR submissions. See the [documentation](docs/ci_testing.md) for information on what is tested and common maintenance tasks.

# Contributing & Reviews

See [this document](CONTRIBUTING.md)
