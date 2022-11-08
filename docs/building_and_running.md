# Building BigWheels
The recommended build system is CMake. This repo supports both in-tree, and out-of-tree builds.
The recommended generator is `Visual Studio 2019` for Windows and `Ninja` for Linux, but others are also supported.

Binaries are written to `<build-dir>/bin` and libraries to `<build-dir>/lib`.

Shaders are also written to `<build-dir>`, but prefixed with their path and the format. For example:

- For SPIR-V, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/spv/my_shader.spv`.
- For DXBC51, `$REPO/assets/shaders/my_shader.hlsl` is compiled to `$BUILD_DIR/assets/shaders/dxbc51/my_shader.dxbc51`.

Binaries have prefixes indicating the target graphics API and shader format:
 * **dx11** - D3D11 with SM 5.0
   * Shaders are compiled with FXC
 * **dx12** - D3D12 with SM 5.1
   * Shaders are compiled with FXC
 * **dxil** - D3D12 with SM 6.0+
   * Shaders are compiled with DXC/DXIL
 * **vk** - Vulkan
   * Shaders are compiled with DXC/SPIR-V

Build instructions vary slightly depending on the host and target platforms.

## Linux
```
git clone --recursive https://github.com/googlestadia/BigWheels
cmake . -GNinja
ninja
```

Built binaries are written to `bin/`.

## Windows
```
git clone --recursive https://github.com/googlestadia/BigWheels
cd BigWheels
cmake -B build -G "Visual Studio 16 2019" -A x64
```

Open `build\BigWheels.sln` and build

Built binaries are written to `build\bin`.

## GGP (on Windows)
```
git clone --recursive https://github.com/googlestadia/BigWheels
cd BigWheels
cmake -B build-ggp -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="C:\\Program Files\\GGP SDK\\cmake\\ggp.cmake" -DPPX_GGP=1
```
Open `build-ggp/BigWheels.sln` and build

Built binaries are written to `build-ggp\bin\vk_*`.

**NOTE:** GGP supplied Vulkan headers and libraries are used for building *but* the build system will look for the DXC
executable in the Vulkan SDK directory.

## GGP (on Linux)
```
git clone --recursive https://github.com/googlestadia/BigWheels
cd BigWheels
cmake . -GNinja -DCMAKE_TOOLCHAIN_FILE=$PATH_TO_GGP_SDK/cmake/ggp.cmake -DPPX_GGP=1
ninja
```

Built binaries are written to `bin/vk_*`.

### Running on GGP
Push the `assets` folder to the instance before running. Since shaders are compiled per project, they must be built and pushed *before* running the application. Then push and run the application. For example, on Linux:
```
ggp ssh put -r assets
ggp ssh sync -R bin/vk_01_triangle
ggp run --cmd "bin/vk_01_triangle"
```

You can use the `tools/ggp-run.py` script to automate pushing and running applications on GGP. For example:
```
python tools/ggp-run.py bin/vk_01_triangle --binary_args "--resolution 1920x1080"
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
    - For GGP and Linux, only SPIR-V is generated.
    - For Windows, DXBC, DXIL, and SPIR-V is generated.

To request a specific API, flags can be passed to Cmake:
 - `PPX_D3D12`: DirectX 12 support.
 - `PPX_D3D11`: DirectX 11 support.
 - `PPX_VULKAN`: Vulkan support.

All targets require DXC. Additionally, DirectX 11 and DirectX 12 require FXC.

## DXC
The build system will look for `dxc.exe` or `dxc` in the Vulkan SDK bin directory.
The DXC path can also be provided using `-DDXC_PATH=<path to DXC executable>`.
**DXC is REQUIRED.**

## FXC
The build system will look for `fxc.exe` in the Windows SDK version that CMake selects.
The FXC path can also be provided using `-DFXC_PATH=<path to FXC executable>`.