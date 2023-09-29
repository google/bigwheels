#include "ppx/scene/scene_mesh.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// MeshData
// -------------------------------------------------------------------------------------------------
MeshData::MeshData(
    const scene::VertexAttributeFlags& activeVertexAttributes,
    grfx::Mesh*                        pGpuMesh)
    : mActiveVertexAttributes(activeVertexAttributes), mGpuMesh(pGpuMesh)
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

    if (mActiveVertexAttributes.mask != 0) {
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

scene::VertexAttributeFlags Mesh::GetActiveVertexAttributes() const
{
    return mMeshData ? mMeshData->GetActiveVertexAttributes() : scene::VertexAttributeFlags::None();
}

void Mesh::AddBatch(const scene::PrimitiveBatch& batch)
{
    mBatches.push_back(batch);
}

void Mesh::UpdateBoundingBox()
{
    mBoundingBox.Set(float3(0));
    for (const auto& batch : mBatches) {
        mBoundingBox.Expand(batch.boundingBox.GetMin());
        mBoundingBox.Expand(batch.boundingBox.GetMax());
    }
}

std::vector<scene::Material*> Mesh::GetMaterials() const
{
    std::vector<scene::Material*> materials;
    for (auto& batch : mBatches) {
        if (!batch.material) {
            // @TODO: need a better way missing materials
            continue;
        }
        materials.push_back(batch.material.get());
    }
    return materials;
}

} // namespace scene
} // namespace ppx
