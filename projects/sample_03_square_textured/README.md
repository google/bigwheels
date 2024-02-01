# Textured square

Displays a spinning textured square on screen. Builds upon the [Spinning Triangle](../sample_02_triangle_spinning/README.md) project.

The texture is loaded from disk at application setup time and sampled in the shader.

## Shaders

Shader              | Purpose for this project
------------------- | --------------------------------------------------
`Texture.hlsl` | Transform and draw a square, sampling from a texture.
