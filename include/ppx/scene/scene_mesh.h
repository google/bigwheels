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

#ifndef ppx_scene_mesh_h
#define ppx_scene_mesh_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_resource_manager.h"

namespace ppx {
namespace scene {

// Mesh Data
//
// Container for geometry data and the buffer views required by
// a renderer. scene::MeshData objects can be shared among different
// scene::Mesh instances.
//
// It's necessary to separate out the mesh data from the mesh since
// it's possible for a series of meshes to use the same geometry
// data but a different set of scene::PrimitiveBatch descriptions.
//
//
class MeshData
    : public grfx::NamedObjectTrait
{
public:
    MeshData(
        const scene::VertexAttributeFlags& availableVertexAttributes,
        grfx::Mesh*                        pGpuMesh);
    virtual ~MeshData();

    grfx::Mesh*                        GetGpuMesh() const { return mGpuMesh.Get(); }
    const scene::VertexAttributeFlags& GetAvailableVertexAttributes() const { return mAvailableVertexAttributes; }
    const grfx::IndexBufferView&       GetIndexBufferView() const { return mIndexBufferView; }
    const grfx::VertexBufferView&      GetPositionBufferView() const { return mPositionBufferView; }
    const grfx::VertexBufferView&      GetAttributeBufferView() const { return mAttributeBufferView; }

private:
    std::string                 mName                      = "";
    scene::VertexAttributeFlags mAvailableVertexAttributes = {};
    grfx::MeshPtr               mGpuMesh                   = nullptr;
    grfx::IndexBufferView       mIndexBufferView           = {};
    grfx::VertexBufferView      mPositionBufferView        = {};
    grfx::VertexBufferView      mAttributeBufferView       = {};
};

// -------------------------------------------------------------------------------------------------

// Primitive Batch
//
// Contains all information necessary for a draw call. The material
// reference will determine which pipeline gets used. The offsets and
// counts correspond to the graphics API's draw call. Bounding box
// can be used by renderer.
//
class PrimitiveBatch
{
public:
    PrimitiveBatch(
        const scene::MaterialRef& material,
        uint32_t                  indexOffset,
        uint32_t                  vertexOffset,
        uint32_t                  indexCount,
        uint32_t                  vertexCount,
        ppx::AABB                 boundingBox);

    const scene::Material* GetMaterial() const { return mMaterial.get(); }
    uint32_t               GetIndexOffset() const { return mIndexOffset; }
    uint32_t               GetVertexOffset() const { return mVertexOffset; }
    uint32_t               GetIndexCount() const { return mIndexCount; }
    uint32_t               GetVertexCount() const { return mVertexCount; }
    ppx::AABB              GetBoundingBox() const { return mBoundingBox; }

private:
    scene::MaterialRef mMaterial     = nullptr;
    uint32_t           mIndexOffset  = 0;
    uint32_t           mVertexOffset = 0;
    uint32_t           mIndexCount   = 0;
    uint32_t           mVertexCount  = 0;
    ppx::AABB          mBoundingBox  = {};
};

// -------------------------------------------------------------------------------------------------

// Mesh
//
// Contains all the information necessary to render a model:
//   - geometry data reference
//   - primitive batches
//   - material references
//
// If a mesh is loaded standalone, it will use its own resource
// manager for required materials, textures, images, and samplers.
//
// If a mesh is loaded as part of a scene, the scene's resource
// manager will be used instead.
//
class Mesh
    : public grfx::NamedObjectTrait
{
public:
    Mesh(
        const scene::MeshDataRef&            meshData,
        std::vector<scene::PrimitiveBatch>&& batches);
    Mesh(
        std::unique_ptr<scene::ResourceManager>&& resourceManager,
        const scene::MeshDataRef&                 meshData,
        std::vector<scene::PrimitiveBatch>&&      batches);
    virtual ~Mesh();

    bool HasResourceManager() const { return mResourceManager ? true : false; }

    scene::VertexAttributeFlags GetAvailableVertexAttributes() const;

    scene::MeshData*                          GetMeshData() const { return mMeshData.get(); }
    const std::vector<scene::PrimitiveBatch>& GetBatches() const { return mBatches; }

    void AddBatch(const scene::PrimitiveBatch& batch);

    const ppx::AABB& GetBoundingBox() const { return mBoundingBox; }
    void             UpdateBoundingBox();

    std::vector<const scene::Material*> GetMaterials() const;

private:
    std::unique_ptr<scene::ResourceManager> mResourceManager = nullptr;
    scene::MeshDataRef                      mMeshData        = nullptr;
    std::string                             mName            = "";
    std::vector<scene::PrimitiveBatch>      mBatches         = {};
    ppx::AABB                               mBoundingBox     = {};
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_mesh_h
