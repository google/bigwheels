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

## Prerequisites

### DXC

**All targets require DXC.**

The build system will look for `dxc.exe` or `dxc` in the Vulkan SDK bin directory. To download and install the Vulkan SDK, follow the instructions here:
* Linux: https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html
* Windows: https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html

Alternatively, a DXC path can be provided using `-DDXC_PATH=<path to DXC executable>`.

### Packages

#### Linux

Install the following prerequisite packages before building:

```
sudo apt install libxrandr-dev libxinerama-dev libx11-dev libxcursor-dev libxi-dev libx11-xcb-dev clang mesa-vulkan-drivers
```

## Linux
```
git clone --recursive https://github.com/google/bigwheels
cd bigwheels
cmake -B build . -GNinja
ninja -C build
```

Built binaries are written to `build/bin/`.

## Windows
```
git clone --recursive https://github.com/google/bigwheels
cd bigwheels
cmake -B build -G "Visual Studio 16 2019" -A x64
```

Open `build\BigWheels.sln` and build.

Built binaries are written to `build\bin`.

**Note: there is an outstanding [issue](https://github.com/google/bigwheels/issues/97) around duplicate targets in VS solutions which may cause build failures when building many shader targets in parallel. As a temporary workaround, you can re-trigger the build and it will eventually work.**

## Android (on Windows)

Use Android Studio to open the BigWheels folder and build it.
A custom DXC_PATH can be set through the env of a `.properties` file.

You can select the sample in the "Select Run/Debug Configuring" drop-down
within the top toolbar. You can also select the target device using the
top toolbar.

## Android (on Linux)

Install the Android SDK and NDK with CMake support.
(Can be done through the SDK manager of Android Studio).

```
git clone --recursive https://github.com/google/bigwheels
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

You can list available projects with:

```bash
./gradlew projects
```

You can use command line tools to build, package and install projects.
The example below will assume the `01_triangle` target is built.

Some common commands:

```bash
# Build using Vulkan's SDK DXC version:
./gradlew 01_triangle:build

# Building with a custom DXC path:
./gradlew 01_triangle:build -PDXC_PATH=some/path/to/dxc

# Assemble a debug APK without installing:
./gradlew 01_triangle:assembleDebug

# Build & install the debug application through ADB:
./gradlew 01_triangle:installDebug -PDXC_PATH=some/path/to/dxc
```

To generate APKs for all available samples:

```bash
./gradlew assemble
```

The generated APKs can be found in the
`build/android/<project_name>/outputs/apk/<flavour>` directory.

### Adding a new Android sample

To add a new Android project to the list of available projects,
create a `build.gradle` file in the project directory and define
a `copyTask` that specifies all assets used in the project.
You can follow any existing samples for an example.

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
