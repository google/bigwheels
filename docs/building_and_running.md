# Building BigWheels
The recommended build system is CMake (version 3.20 and above). This repo supports both in-tree, and out-of-tree builds.
The recommended generator is `Visual Studio 2019` for Windows and `Ninja` for Linux, but others are also supported.

Binaries are written to `<build-dir>/bin` and libraries to `<build-dir>/lib`.

Shaders are also written to `<build-dir>`, but prefixed with their path and the format. For example:

- For SPIR-V, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/spv/my_shader.spv`.
- For DXBC51, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/dxbc51/my_shader.dxbc51`.

Binaries have prefixes indicating the target graphics API and shader format:
 * **dx12** - D3D12 with SM 5.1
   * Shaders are compiled with FXC
 * **dxil** - D3D12 with SM 6.0+
   * Shaders are compiled with DXC/DXIL
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

Open `build\BigWheels.sln` and build

Built binaries are written to `build\bin`.

## Android (on Windows or Linux)
```
git clone --recursive https://github.com/google/BigWheels
```

Use Android Studio to open `android` subfolder and build

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
    - For Windows, DXBC, DXIL, and SPIR-V is generated.

To request a specific API, flags can be passed to Cmake:
 - `PPX_D3D12`: DirectX 12 support.
 - `PPX_VULKAN`: Vulkan support.

All targets require DXC. Additionally, DirectX 12 requires FXC.

## DXC
The build system will look for `dxc.exe` or `dxc` in the Vulkan SDK bin directory.
The DXC path can also be provided using `-DDXC_PATH=<path to DXC executable>`.
**DXC is REQUIRED.**

## FXC
The build system will look for `fxc.exe` in the Windows SDK version that CMake selects.
The FXC path can also be provided using `-DFXC_PATH=<path to FXC executable>`.
