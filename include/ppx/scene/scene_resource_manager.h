// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ppx_scene_resource_manager_h
#define ppx_scene_resource_manager_h

#include "ppx/scene/scene_config.h"

namespace ppx {
namespace scene {

// Resource Manager
//
// This class stores required objects for scenes and standalone meshes.
// It also acts as a cache during scene loading to prevent loading of
// duplicate objects.
//
// The resource manager acts as the external owner of all shared resources
// for scenes and meshes. Required objects can be shared in a variety of
// difference cases:
//   - images and image views can be shared among textures
//   - textures can be shared among materials by way of texture views
//   - materials can be shared among primitive batches
//   - mesh data can be shared among meshes
//   - meshes can be shared among nodes
//
// Both scene::Scene and scene::Mesh will call ResourceManager::DestroyAll()
// in their destructors to destroy all its references to shared objects.
// If afterwards a shared object has an external reference, the code holding
// the reference is responsible for the shared objct.
//
class ResourceManager
{
public:
    ResourceManager();
    virtual ~ResourceManager();

    bool Find(uint64_t objectId, scene::ImageRef& outObject) const;
    bool Find(uint64_t objectId, scene::SamplerRef& outObject) const;
    bool Find(uint64_t objectId, scene::TextureRef& outObject) const;
    bool Find(uint64_t objectId, scene::MaterialRef& outObject) const;
    bool Find(uint64_t objectId, scene::MeshDataRef& outObject) const;
    bool Find(uint64_t objectId, scene::MeshRef& outObject) const;

    // Cache functions assumes ownership of pObject
    ppx::Result Cache(uint64_t objectId, const scene::ImageRef& object);
    ppx::Result Cache(uint64_t objectId, const scene::SamplerRef& object);
    ppx::Result Cache(uint64_t objectId, const scene::TextureRef& object);
    ppx::Result Cache(uint64_t objectId, const scene::MaterialRef& object);
    ppx::Result Cache(uint64_t objectId, const scene::MeshDataRef& object);
    ppx::Result Cache(uint64_t objectId, const scene::MeshRef& object);

    uint32_t GetImageCount() const { return static_cast<uint32_t>(mImages.size()); }
    uint32_t GetSamplerCount() const { return static_cast<uint32_t>(mSamplers.size()); }
    uint32_t GetTextureCount() const { return static_cast<uint32_t>(mTextures.size()); }
    uint32_t GetMaterialCount() const { return static_cast<uint32_t>(mMaterials.size()); }
    uint32_t GetMeshDataCount() const { return static_cast<uint32_t>(mMeshData.size()); }
    uint32_t GetMeshCount() const { return static_cast<uint32_t>(mMeshes.size()); }

    void DestroyAll();

private:
    template <
        typename ObjectT,
        typename ObjectRefT = std::shared_ptr<ObjectT>>
    bool FindObject(
        uint64_t                                        objectId,
        const std::unordered_map<uint64_t, ObjectRefT>& container,
        ObjectRefT&                                     outObject) const
    {
        auto it = container.find(objectId);
        if (it == container.end()) {
            return false;
        }

        outObject = (*it).second;
        return true;
    }

    template <
        typename ObjectT,
        typename ObjectRefT = std::shared_ptr<ObjectT>>
    ppx::Result CacheObject(
        uint64_t                                  objectId,
        const ObjectRefT&                         object,
        std::unordered_map<uint64_t, ObjectRefT>& container)
    {
        // Don't cache NULL objects
        if (!object) {
            return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
        }

        auto it = container.find(objectId);
        if (it != container.end()) {
            return ppx::ERROR_DUPLICATE_ELEMENT;
        }

        container[objectId] = object;

        return ppx::SUCCESS;
    }

private:
    std::unordered_map<uint64_t, scene::ImageRef>    mImages;
    std::unordered_map<uint64_t, scene::SamplerRef>  mSamplers;
    std::unordered_map<uint64_t, scene::TextureRef>  mTextures;
    std::unordered_map<uint64_t, scene::MaterialRef> mMaterials;
    std::unordered_map<uint64_t, scene::MeshDataRef> mMeshData;
    std::unordered_map<uint64_t, scene::MeshRef>     mMeshes;
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_resource_manager_h