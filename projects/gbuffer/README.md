# Deferred shading (gbuffer)

Displays six spheres using the deferred shading technique.

This project implements [deferred shading](https://en.wikipedia.org/wiki/Deferred_shading) using a [gbuffer](https://en.wikipedia.org/wiki/Glossary_of_computer_graphics#g-buffer), a technique where geometries are first rendered to off-screen render targets which are used to store geometry information such as position, color, normals and other attributes to be used by the shading model. The off-screen targets are then used as textures in a later pass to compose the final image, with the addition of light information.

The gbuffer in this example is packed to contain the following attributes:

- Position
- Normal
- Albedo color
- F0 (Fresnel Reflectance)
- Roughness
- Metalness
- Ambient occlusion
- IBL strength
- Environment strength

The gbuffer is then unpacked and used in a later lighting pass to compose the final image, taking into account scene lights. The final image is then drawn ("blitted") into the swapchain image.

For debug purposes and to aid visualization, the ImGui interface offers an option to draw single attributes from the gbuffer.

## Shaders

Shader                        | Purpose for this project
----------------------------- | -------------------------------------------------------------
`DeferredRender.hlsl`       | Draw model and pack gbuffer.
`DeferredLight.hlsl`        | Unpack gbuffer and draw composed image.
`FullScreenTriangle.hlsl`   | Draw final image to swapchain.
`DrawGBufferAttribute.hlsl` | Draw a single attribute from the gbuffer, for debug purposes.
