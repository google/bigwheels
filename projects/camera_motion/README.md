# Camera motion

Displays a scene with various meshes on a floor grid, where the camera motion can be controlled by the mouse and keyboard.

Controls:

- Mouse to move the camera's point-of-view.
- `W`,`A`,`S`,`D` to move the camera forward or backwards and strafe left and right.
- Arrow keys to turn the camera up and down and left and right.
- Spacebar to reset the camera's view.
- `1` to choose a perspective camera, `2` to choose an arcball camera.

## Shaders

Shader              | Purpose for this project
------------------- | -----------------------------------------------------------
`VertexColors.hlsl` | Transform and draw the scene's entities and the floor grid.
