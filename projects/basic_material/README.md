# Shading models

Displays a collection of meshes with a variety of shading models.

The mesh being rendered, its textures and the shading technique can be selected and changed at runtime through the ImGui interface. The following shading models are supported:

- [Gouraud](https://en.wikipedia.org/wiki/Gouraud_shading)
- [Phong](https://en.wikipedia.org/wiki/Phong_shading)
- [Blinn-Phong](https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model)
- [Physically-Based Rendering (PBR)](https://en.wikipedia.org/wiki/Physically_based_rendering)

The PBR shading model is the most complex technique of the four, as it aims to model the physical flow of light and achieve photorealism. The ImGui interface exposes a number of options that can be turned off and on to control specific PBR behavior, such as enabling reflections, normal maps, and so on.

## Shaders

Shader              | Purpose for this project
------------------- | ------------------------------------------------------------
`VertexShader.hlsl` | (Vertex shader only) Transform the model for shading.
`Gouraud.hlsl`      | (Pixel shader only) Draw the model with Gouraud shading.
`Phong.hlsl`        | (Pixel shader only) Draw the model with Phong shading.
`BlinnPhong.hlsl`   | (Pixel shader only) Draw the model with Blinn-Phong shading.
`PBR.hlsl`          | (Pixel shader only) Draw the model with PBR shading.
