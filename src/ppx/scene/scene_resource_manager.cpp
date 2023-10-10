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

#include "ppx/scene/scene_resource_manager.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace scene {

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

bool ResourceManager::Find(uint64_t objectId, scene::SamplerRef& outObject) const
{
    return FindObject<grfx::Sampler>(objectId, mSamplers, outObject);
}

bool ResourceManager::Find(uint64_t objectId, scene::ImageRef& outObject) const
{
    return FindObject<grfx::Image>(objectId, mImages, outObject);
}

bool ResourceManager::Find(uint64_t objectId, scene::TextureRef& outObject) const
{
    return FindObject<scene::Texture>(objectId, mTextures, outObject);
}

bool ResourceManager::Find(uint64_t objectId, scene::MaterialRef& outObject) const
{
    return FindObject<scene::Material>(objectId, mMaterials, outObject);
}

bool ResourceManager::Find(uint64_t objectId, scene::MeshDataRef& outObject) const
{
    return FindObject<scene::MeshData>(objectId, mMeshData, outObject);
}

bool ResourceManager::Find(uint64_t objectId, scene::MeshRef& outObject) const
{
    return FindObject<scene::Mesh>(objectId, mMeshes, outObject);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::SamplerRef& object)
{
    return CacheObject<grfx::Image>(objectId, object, mSamplers);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::ImageRef& object)
{
    return CacheObject<grfx::Image>(objectId, object, mImages);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::TextureRef& object)
{
    return CacheObject<scene::Texture>(objectId, object, mTextures);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::MaterialRef& object)
{
    return CacheObject<scene::Material>(objectId, object, mMaterials);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::MeshDataRef& object)
{
    return CacheObject<scene::MeshData>(objectId, object, mMeshData);
}

ppx::Result ResourceManager::Cache(uint64_t objectId, const scene::MeshRef& object)
{
    return CacheObject<scene::Mesh>(objectId, object, mMeshes);
}

void ResourceManager::DestroyAll()
{
    mImages.clear();
    mSamplers.clear();
    mTextures.clear();
    mMaterials.clear();
    mMeshData.clear();
    mMeshes.clear();
}

std::vector<const scene::Sampler*> ResourceManager::GetSamplers() const
{
    std::vector<const scene::Sampler*> objectPtrs;
    for (auto& it : mSamplers) {
        objectPtrs.push_back(it.second.get());
    }
    return objectPtrs;
}

std::vector<const scene::Image*> ResourceManager::GetImages() const
{
    std::vector<const scene::Image*> objectPtrs;
    for (auto& it : mImages) {
        objectPtrs.push_back(it.second.get());
    }
    return objectPtrs;
}

std::vector<const scene::Texture*> ResourceManager::GetTextures() const
{
    std::vector<const scene::Texture*> objectPtrs;
    for (auto& it : mTextures) {
        objectPtrs.push_back(it.second.get());
    }
    return objectPtrs;
}

std::vector<const scene::Material*> ResourceManager::GetMaterials() const
{
    std::vector<const scene::Material*> objectPtrs;
    for (auto& it : mMaterials) {
        objectPtrs.push_back(it.second.get());
    }
    return objectPtrs;
}

} // namespace scene
} // namespace ppx
