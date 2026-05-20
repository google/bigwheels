# gltf_basic_materials

This sample loads a scene with `ppx::scene::GltfLoader` and rendering it.

After building, run `gltf_basic_materials` for the desired API in your build directory. E.g. for Vulkan:

```sh
vk_gltf_basic_materials
```

By default, this will load `//assets/scene_renderer/scenes/tests/gltf_test_basic_materials.glb`. You can change which scene to load using the `--gltf-scene-asset` option:

```sh
vk_gltf_basic_materials --gltf-scene-asset basic/models/altimeter/altimeter.gltf
```

This loads and renders `//assets/basic/models/altimeter/altimeter.gltf`. Notice that the provided path must be relative to one of the following directories:

* `//assets/`
* `//third_party/assets/`
* Any path provided via `--extra-assets-path`

## glTF-Sample-Assets

Since BigWheels does not provide many glTF scenes, it is recommended to test changes against [KhronosGroup/gltf-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets).

First, clone the repository. This does not have to be in the BigWheels checkout:

```sh
git clone https://github.com/KhronosGroup/glTF-Sample-Assets
```

Second, run `gltf_basic_materials` with the desired scene.

```sh
vk_gltf_basic_materials --gltf-scene-asset Box/glTF/Box.gltf --extra-assets-path some/path/to/glTF-Sample-Assets/Models
```

Notice that we provide:

* `--extra-assets-path` which points to the `Models` directory in the location of the glTF-Sample-Assets clone
* `--gltf-scene-asset` to load the Box model.
