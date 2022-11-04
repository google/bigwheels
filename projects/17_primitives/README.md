# Primitives

Displays primitives (cube, sphere, plane) in solid and wireframe mode.

The primitives are created procedurally at startup time.Solid and wireframe rendering are achieved by creating two pipelines that only differ in the fill mode.

## Shaders

Shader              | Purpose for this project
------------------- | -------------------------------
`VertexColors.hlsl` | Transform and draw a primitive.
