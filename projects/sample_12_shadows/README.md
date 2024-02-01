# Shadows

Displays a cube and a sphere on a plane with dynamic shadows cast from an orbiting light source.

The main technique used is [shadow mapping](https://en.wikipedia.org/wiki/Shadow_mapping), where the scene's depth information is used to create a shadow map that can be used to compute shadows in a later rendering pass. Shadows' hard edges are smoothed using Percentage-Closer Filtering ([PCF](https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing)).

This project showcases a more complex rendering pipeline than the previous ones. Of note is the use of a separate render pass. The frame is built in the following stages:

1. The cube and sphere are drawn in a render pass that writes solely to the depth buffer (shadow map creation).
2. The cube and sphere are then rendered into the scene, along with shadows computed using the shadow map created in step 1.
3. Finally, to aid the readability of the scene, the orbiting light source is drawn as a cube.

## Shaders

Shader               | Purpose for this project
-------------------- | ----------------------------------------------------------------
`Depth.hlsl`         | (Vertex shader only) Write transformed position to depth buffer.
`DiffuseShadow.hlsl` | Compute PCF shadows and draw meshes with shadows.
`VertexColors.hlsl`  | Draw a cube representing the light source.
