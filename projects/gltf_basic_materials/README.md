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
