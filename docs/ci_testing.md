# CI and testing
BigWheels uses [GitHub Actions]() for its CI and testing solution.

## Current testing 
The following tests are run as part of every PR submission:
* On Windows: 
  * Full build of BigWheels, all shader targets, and DX12 samples with Visual Studio 2019.
  * Unit tests.
  * Some DX12 samples are run in headless mode using DX12's software renderer.
* On Linux:
  * Full build of BigWheels, SPIR-V shaders, and Vulkan samples.
  * Unit tests.
  * Some Vulkan samples are run with a virtual framebuffer (using `xvfb`) and using Mesa's Lavapipe as the software renderer.
* Code formatting check that ensures code is formatted using `clang-format`.

## Maintenance
Testing workflows are found under the [`.github/workflows`](https://github.com/google/bigwheels/tree/main/.github/workflows) folder in the repository.

We use the freely-available GitHub-hosted runners. There may be the need of updating dependencies or other settings used by the workflows as GitHub updates their runners, or as BigWheels' needs change.

Common maintenance tasks:
* Updating the runner's images: we are currently using `ubuntu-20.04` and `windows-2019`.
* Updating the Vulkan SDK and its components that are downloaded and installed as part of the workflows. This should be a simple change of the version string in the workflows' files:
  * On [Windows](https://github.com/google/bigwheels/blob/main/.github/workflows/windows-build.yml#L24), both the Vulkan SDK and the RT components are used.
  * On [Linux](https://github.com/google/bigwheels/blob/main/.github/workflows/linux-build.yml#L24), only the Vulkan SDK is used.
* Updating the Windows dependencies, mainly the [Windows SDK](https://github.com/google/bigwheels/blob/main/.github/workflows/windows-build.yml#L21).
* Maintaining the CMake build flags used to build BigWheels ([Linux](https://github.com/google/bigwheels/blob/main/.github/workflows/linux-build.yml#L41), [Windows](https://github.com/google/bigwheels/blob/main/.github/workflows/windows-build.yml#L44)).
* Curating the list of samples that are run ([Linux](https://github.com/google/bigwheels/blob/main/.github/workflows/linux-build.yml#L47), [Windows](https://github.com/google/bigwheels/blob/main/.github/workflows/windows-build.yml#L55)). Note that we strive to maintain a healthy balance between test coverage and runtime/resource usage minimization of the CI runners.