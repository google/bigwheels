# Mipmaps

Display two textures on screen, allowing the selection of each image's mipmap level.

The two textures are shown side-by-side and each image's mipmap level can be chosen separately on the ImGui interface. One texture's mip maps are created on the GPU using a mipmap generation shader while the other's are created on the CPU.

## Shaders

Shader            | Purpose for this project
----------------- | -------------------------------------------------------
`TextureMip.hlsl` | Draw a texture sampling from the selected mipmap level.
