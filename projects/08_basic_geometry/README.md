# Basic geometry buffer layouts

Displays six spinning colored cubes on screen using different geometry buffer layouts. Builds upon the [Spinning Cube (Indexed Draw)](../07_draw_indexed/README.md) project.

Cube geometry data (vertices, indices and vertex colors) is generated at setup time, but is laid out differently in GPU memory for each cube:

- 16-bit indices. Vertices interleaved with vertex colors in the same buffer.
- 32-bit indices. Vertices interleaved with vertex colors in the same buffer.
- No indices. Vertices interleaved with vertex colors in the same buffer.
- 16-bit indices. Vertices in a separate buffer from vertex colors.
- 32-bit indices. Vertices in a separate buffer from vertex colors.
- No indices. Vertices in a separate buffer from vertex colors.

## Shaders

Shader              | Purpose for this project
------------------- | ----------------------------------------------
`VertexColors.hlsl` | Transform and draw a cube using vertex colors.
