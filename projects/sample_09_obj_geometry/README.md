# Object geometry buffer layouts

Displays four spinning colored 3D models on screen using different geometry buffer layouts. Builds upon the [Basic Geometry Buffer Layouts](../sample_08_basic_geometry/README.md) project.

The mesh's geometry data is loaded from disk, but is laid out differently in GPU memory for each model drawn on screen:

- 32-bit indices. Vertices interleaved with vertex colors in the same buffer.
- No indices. Vertices interleaved with vertex colors in the same buffer.
- 32-bit indices. Vertices in a separate buffer from vertex colors.
- No indices. Vertices in a separate buffer from vertex colors.

## Shaders

Shader              | Purpose for this project
------------------- | ----------------------------------------------
`VertexColors.hlsl` | Transform and draw a cube using vertex colors.
