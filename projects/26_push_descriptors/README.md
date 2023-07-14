# Push Descriptor 

Example of how to use the push descriptor command buffer functions for grfx::Buffer, grfx::ImageView, and grfx::Sampler objects in Vulkan. 
This sample is Vulkan only since D3D12 only supports buffer based root descriptors.

The novelty of this feature for Vulkan is that an application can make certain draw calls and dispatches without needing a descriptor set.
This means not having to deal creating a descriptor pool and allocating a descriptor set.

Draws 3 cubes using three different uniform buffers and textures that are pushed to the command buffer for each draw call.
Sampler is pushed ahead of time before the first draw call since it's common to all three draw cals.
Each uniform buffer contains the transform matrix and a texture selection index for the draw call.

## Shaders

Shader                         | Purpose for this project
------------------------------ | -------------------------------------------------------------------------------------------------------
`PushDescriptorsTexture.hlsl`  | Draws the selected texture
