# Spinning triangle

Displays a spinning colored triangle on screen. Builds upon the [Static Triangle](../01_triangle/README.md) project.

A uniform buffer is used to upload the updated position of the triangle every frame based on its rotation.

## Shaders

Shader              | Purpose for this project
------------------- | --------------------------------------------------
`VertexColors.hlsl` | Transform and draw a triangle using vertex colors.
