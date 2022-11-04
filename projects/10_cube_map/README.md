# Cube map

Displays a model that reflects a skybox environment, using a [cube map](https://en.wikipedia.org/wiki/Cube_mapping).

The cube map is used for two purposes:

1. As a surrounding environment map that the model reflects on itself, like a mirror.
2. As the texture for the skybox around the model, showing what the environment looks like.

## Shaders

Shader         | Purpose for this project
-------------- | --------------------------------------------------------------------------------------------------------
`SkyBox.hlsl`  | Draw a skybox, sampling from a cube map.
`CubeMap.hlsl` | Draw a model, texturing it with reflections from a cube map that represents the surrounding environment.
