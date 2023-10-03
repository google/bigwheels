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

#include "ppx/scene/scene_mesh.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// MeshData
// -------------------------------------------------------------------------------------------------
MeshData::MeshData(
    const scene::VertexAttributeFlags& availableVertexAttributes,
    grfx::Mesh*                        pGpuMesh)
    : mAvailableVertexAttributes(availableVertexAttributes), mGpuMesh(pGpuMesh)
{
    PPX_ASSERT_MSG(!IsNull(mGpuMesh.Get()), "mGpuMesh is NULL");

    mIndexBufferView.pBuffer   = mGpuMesh->GetIndexBuffer();
    mIndexBufferView.indexType = mGpuMesh->GetIndexType();
    mIndexBufferView.offset    = 0;
    mIndexBufferView.size      = mGpuMesh->GetIndexBuffer()->GetSize();

    const uint32_t kPositionIndex = 0;
    mPositionBufferView.pBuffer   = mGpuMesh->GetVertexBuffer(kPositionIndex);
    mPositionBufferView.stride    = mGpuMesh->GetDerivedVertexBindings()[kPositionIndex].GetStride();
    mPositionBufferView.offset    = 0;
    mPositionBufferView.size      = mGpuMesh->GetVertexBuffer(kPositionIndex)->GetSize();

    if (mAvailableVertexAttributes.mask != 0) {
        const uint32_t kAttributesIndex = 1;
        mAttributeBufferView.pBuffer    = mGpuMesh->GetVertexBuffer(kAttributesIndex);
        mAttributeBufferView.stride     = mGpuMesh->GetDerivedVertexBindings()[kAttributesIndex].GetStride();
        mAttributeBufferView.offset     = 0;
        mAttributeBufferView.size       = mGpuMesh->GetVertexBuffer(kAttributesIndex)->GetSize();
    }
}

MeshData::~MeshData()
{
    if (mGpuMesh) {
        auto pDevice = mGpuMesh->GetDevice();
        pDevice->DestroyMesh(mGpuMesh);
        mGpuMesh.Reset();
    }
}

// -------------------------------------------------------------------------------------------------
// PrimitiveBatch
// -------------------------------------------------------------------------------------------------
PrimitiveBatch::PrimitiveBatch(
    const scene::MaterialRef& material,
    uint32_t                  indexOffset,
    uint32_t                  vertexOffset,
    uint32_t                  indexCount,
    uint32_t                  vertexCount,
    ppx::AABB                 boundingBox)
    : mMaterial(material),
      mIndexOffset(indexOffset),
      mVertexOffset(vertexOffset),
      mIndexCount(indexCount),
      mVertexCount(vertexCount),
      mBoundingBox(boundingBox)
{
}

PrimitiveBatch::~PrimitiveBatch()
{
}

// -------------------------------------------------------------------------------------------------
// Mesh
// -------------------------------------------------------------------------------------------------
Mesh::Mesh(
    const scene::MeshDataRef&            meshData,
    std::vector<scene::PrimitiveBatch>&& batches)
    : mMeshData(meshData),
      mBatches(std::move(batches))
{
    UpdateBoundingBox();
}

Mesh::Mesh(
    std::unique_ptr<scene::ResourceManager>&& resourceManager,
    const scene::MeshDataRef&                 meshData,
    std::vector<scene::PrimitiveBatch>&&      batches)
    : mResourceManager(std::move(resourceManager)),
      mMeshData(meshData),
      mBatches(std::move(batches))
{
    UpdateBoundingBox();
}

Mesh::~Mesh()
{
    if (mResourceManager) {
        mResourceManager->DestroyAll();
        mResourceManager.reset();
    }
}

scene::VertexAttributeFlags Mesh::GetAvailableVertexAttributes() const
{
    return mMeshData ? mMeshData->GetAvailableVertexAttributes() : scene::VertexAttributeFlags::None();
}

void Mesh::AddBatch(const scene::PrimitiveBatch& batch)
{
    mBatches.push_back(batch);
}

void Mesh::UpdateBoundingBox()
{
    mBoundingBox.Set(float3(0));
    for (const auto& batch : mBatches) {
        mBoundingBox.Expand(batch.GetBoundingBox().GetMin());
        mBoundingBox.Expand(batch.GetBoundingBox().GetMax());
    }
}

std::vector<const scene::Material*> Mesh::GetMaterials() const
{
    std::vector<const scene::Material*> materials;
    for (auto& batch : mBatches) {
        if (!IsNull(batch.GetMaterial())) {
            // @TODO: need a better way missing materials
            continue;
        }
        materials.push_back(batch.GetMaterial());
    }
    return materials;
}

} // namespace scene
} // namespace ppx
