# Camera re-center

Displays a cube at the center of the scene, where the orbiting (arc-ball) camera can be controlled by the mouse and re-centered on the scene with a key press. Builds upon the [Arcball Camera](../arcball_camera/README.md) project.

Click and drag the left mouse button to orbit the camera, and the right mouse button to pan the camera. Press `F` to re-center the camera on the scene. The centering algorithm uses the bounding box of the scene to compute the best camera position.

## Shaders

Shader              | Purpose for this project
------------------- | ----------------------------
`VertexColors.hlsl` | Transform and draw the cube.
