# Normal mapping

Displays a spinning textured cube on screen, where the texture is augmented with a normal map. Builds upon the [Textured Cube](../sample_05_cube_textured/README.md) project.

This project showcases the [normal mapping](https://en.wikipedia.org/wiki/Normal_mapping) technique, where a normal map is used to give the impression of tridimensionality to flat faces depending on the light direction.

## Shaders

Shader              | Purpose for this project
------------------- | --------------------------------------------------------------------
`NormalMap.hlsl`    | Transform and draw a cube, sampling from a texture and a normal map.
`VertexColors.hlsl` | Draw a cube representing the light source.
