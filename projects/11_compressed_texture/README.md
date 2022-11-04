# Compressed textures

Displays eight spinning textured cubes on screen, each using a texture with a different compression format. Builds upon the [Textured Cube](../05_cube_textured/README.md) project.

Each of the eight cubes is textured with a [DDS](https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide) texture that uses a different [block compression](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11) format:

- **BC1** (three color channels with 0-1 bits of alpha)
- **BC2** (three color channels with 4 bits of alpha)
- **BC3** (three color channels with 8 bits of alpha)
- **BC4** (one color channel)
- **BC5** (two color channels)
- **BC6H** (three color channels in 16-bit floating point for HDR)
- **BC6H_SF** (three color channels in signed 16-bit floating point for HDR)
- **BC7** (three color channels with 0 to 8 bits of alpha)

## Shaders

Shader         | Purpose for this project
-------------- | ---------------------------------------------------
`Texture.hlsl` | Transform and draw a cube, sampling from a texture.
