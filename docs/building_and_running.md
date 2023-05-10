# Building BigWheels
The recommended build system is CMake (version 3.20 and above). This repo supports both in-tree, and out-of-tree builds.
The recommended generator is `Visual Studio 2019` for Windows and `Ninja` for Linux, but others are also supported.

Binaries are written to `<build-dir>/bin` and libraries to `<build-dir>/lib`.

Shaders are also written to `<build-dir>`, but prefixed with their path and the format. For example:

- For SPIR-V, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/spv/my_shader.spv`.
- For DXIL, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/dxil/my_shader.dxil`.

Binaries have prefixes indicating the target graphics API and shader format:
 * **dx12** - D3D12 with SM 6.5
   * Shaders are compiled with DXC/DXIL.
 * **vk** - Vulkan
   * Shaders are compiled with DXC/SPIR-V

Build instructions vary slightly depending on the host and target platforms.

## Linux
```
git clone --recursive https://github.com/google/BigWheels
cmake . -GNinja
ninja
```

Built binaries are written to `bin/`.

## Windows
```
git clone --recursive https://github.com/google/BigWheels
cd BigWheels
cmake -B build -G "Visual Studio 16 2019" -A x64
```

Open `build\BigWheels.sln` and build.

Built binaries are written to `build\bin`.

**Note: there is an outstanding [issue](https://github.com/google/bigwheels/issues/97) around duplicate targets in VS solutions which may cause build failures when building many shader targets in parallel. As a temporary workaround, you can re-trigger the build and it will eventually work.**

## Android (on Windows)

Use Android Studio to open the BigWheels folder and build it.
A custom DXC_PATH can be set through the env of a `.properties` file.

You can select the sample and target device by navigating to `Build -> Select Build Variant` within Android Studio.

## Android (on Linux)

Install the Android SDK and NDK with CMake support.
(Can be done through the SDK manager of Android Studio).

```
git clone --recursive https://github.com/google/BigWheels
```

### With Android Studio

See `Android (on Windows)`.

### Command line - Mobile Android

The Android build will also require DXC. DXC can either be retrieved from
the Vulkan SDK, or provided manually (same as the linux build).
Make sure the Android SDK path is in your env

```bash
export ANDROID_HOME=/path/to/android/sdk
```

Multiple targets are available, This example will assume the `triangle`
target is built. Below the target naming scheme will be explained.

To use Vulkan's SDK DXC version:

```bash
./gradlew buildMobileTriangleDebug
```

To provide the DXC path:

```bash
./gradlew buildMobileTriangleDebug -PDXC_PATH=some/path/to/dxc
```

To build & install the application through ADB

```
./gradlew installMobileTriangleDebug -PDXC_PATH=some/path/to/dxc
```

The target names are composed using this pattern:
<buildType><device><sampleName>[xr]<flavor>

buildType:
  - assemble: build and package the APK without installing
  - build: only build the artefacts
  - install: build and package the APK and install with ADB.

device:
  - Mobile : an Android phone
  - Openxr : an openXR device
  - Quest  : Oculus Quest

sampleName:
  - Triangle    : 01_triangle
  - Cube        : 04_cube
  ...

The `xr` suffix can be added on some samples (see projects/ directory)

flavor:
  - Debug
  - Release

Example:

To build Fishtornado, XR, for OpenXR:

```
./gradlew buildOpenxrFishtornadoxrDebug
```

## OpenXR
OpenXR support can be enabled by adding `-DPPX_BUILD_XR=1` flag.
For example, for Windows build:
```
cmake -B build -G "Visual Studio 16 2019" -A x64 -DPPX_BUILD_XR=1
```

# Shader Compilation
Shader binaries are generated during project build. Since BigWheels can target multiple graphics APIs, we compile shaders
for each API depending on the need. API support depends on the system nature and configuration:
    - For Linux, only SPIR-V is generated.
    - For Windows, both DXIL and SPIR-V are generated.

To request a specific API, flags can be passed to Cmake:
 - `PPX_D3D12`: DirectX 12 support.
 - `PPX_VULKAN`: Vulkan support.

**All targets require DXC.**

## DXC
The build system will look for `dxc.exe` or `dxc` in the Vulkan SDK bin directory.
The DXC path can also be provided using `-DDXC_PATH=<path to DXC executable>`.
