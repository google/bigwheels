# Push Descriptor Buffers

Example of how to use the push descriptor command buffer functions for grfx::Buffer objects in both D3D12 and Vulkan. 

Draws 3 cubes using three different uniform buffers that are pushed to the command buffer for each draw call.
Each uniform buffer contains the transform matrix and a texture selection index for the draw call.

A descriptor set is used for the textures and sampler descriptors.

## Shaders

Shader                                | Purpose for this project
------------------------------------- | -------------------------------------------------------------------------------------------------------
`PushDescriptorsBuffersTexture.hlsl`  | Draws the selected texture
