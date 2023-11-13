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
    grfx::Buffer*                      pGpuBuffer)
    : mAvailableVertexAttributes(availableVertexAttributes),
      mGpuBuffer(pGpuBuffer)
{
    // Position
    auto positionBinding = grfx::VertexBinding(0, grfx::VERTEX_INPUT_RATE_VERTEX);
    {
        grfx::VertexAttribute attr = {};
        attr.semanticName          = "POSITION";
        attr.location              = scene::kVertexPositionLocation;
        attr.format                = scene::kVertexPositionFormat;
        attr.binding               = scene::kVertexPositionBinding;
        attr.offset                = PPX_APPEND_OFFSET_ALIGNED;
        attr.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
        attr.semantic              = grfx::VERTEX_SEMANTIC_POSITION;
        positionBinding.AppendAttribute(attr);
    }
    mVertexBindings.push_back(positionBinding);

    // Done if no vertex attributes
    if (availableVertexAttributes.mask == 0) {
        return;
    }

    auto attributeBinding = grfx::VertexBinding(kVertexAttributeBinding, grfx::VERTEX_INPUT_RATE_VERTEX);
    {
        // TexCoord
        if (availableVertexAttributes.bits.texCoords) {
            grfx::VertexAttribute attr = {};
            attr.semanticName          = "TEXCOORD";
            attr.location              = scene::kVertexAttributeTexCoordLocation;
            attr.format                = scene::kVertexAttributeTexCoordFormat;
            attr.binding               = scene::kVertexAttributeBinding;
            attr.offset                = PPX_APPEND_OFFSET_ALIGNED;
            attr.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
            attr.semantic              = grfx::VERTEX_SEMANTIC_TEXCOORD;
            attributeBinding.AppendAttribute(attr);
        }

        // Normal
        if (availableVertexAttributes.bits.normals) {
            grfx::VertexAttribute attr = {};
            attr.semanticName          = "NORMAL";
            attr.location              = scene::kVertexAttributeNormalLocation;
            attr.format                = scene::kVertexAttributeNormalFormat;
            attr.binding               = scene::kVertexAttributeBinding;
            attr.offset                = PPX_APPEND_OFFSET_ALIGNED;
            attr.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
            attr.semantic              = grfx::VERTEX_SEMANTIC_NORMAL;
            attributeBinding.AppendAttribute(attr);
        }

        // Tangent
        if (availableVertexAttributes.bits.tangents) {
            grfx::VertexAttribute attr = {};
            attr.semanticName          = "TANGENT";
            attr.location              = scene::kVertexAttributeTangentLocation;
            attr.format                = scene::kVertexAttributeTagentFormat;
            attr.binding               = scene::kVertexAttributeBinding;
            attr.offset                = PPX_APPEND_OFFSET_ALIGNED;
            attr.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
            attr.semantic              = grfx::VERTEX_SEMANTIC_TANGENT;
            attributeBinding.AppendAttribute(attr);
        }

        // Color
        if (availableVertexAttributes.bits.colors) {
            grfx::VertexAttribute attr = {};
            attr.semanticName          = "COLOR";
            attr.location              = scene::kVertexAttributeColorLocation;
            attr.format                = scene::kVertexAttributeColorFormat;
            attr.binding               = scene::kVertexAttributeBinding;
            attr.offset                = PPX_APPEND_OFFSET_ALIGNED;
            attr.inputRate             = grfx::VERTEX_INPUT_RATE_VERTEX;
            attr.semantic              = grfx::VERTEX_SEMANTIC_COLOR;
            attributeBinding.AppendAttribute(attr);
        }
    }
    mVertexBindings.push_back(attributeBinding);
}

MeshData::~MeshData()
{
    if (mGpuBuffer) {
        auto pDevice = mGpuBuffer->GetDevice();
        pDevice->DestroyBuffer(mGpuBuffer);
    }
}

// -------------------------------------------------------------------------------------------------
// PrimitiveBatch
// -------------------------------------------------------------------------------------------------
PrimitiveBatch::PrimitiveBatch(
    const scene::MaterialRef&     material,
    const grfx::IndexBufferView&  indexBufferView,
    const grfx::VertexBufferView& positionBufferView,
    const grfx::VertexBufferView& attributeBufferView,
    uint32_t                      indexCount,
    uint32_t                      vertexCount,
    const ppx::AABB&              boundingBox)
    : mMaterial(material),
      mIndexBufferView(indexBufferView),
      mPositionBufferView(positionBufferView),
      mAttributeBufferView(attributeBufferView),
      mIndexCount(indexCount),
      mVertexCount(vertexCount),
      mBoundingBox(boundingBox)
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

std::vector<grfx::VertexBinding> Mesh::GetAvailableVertexBindings() const
{
    std::vector<grfx::VertexBinding> bindings;
    if (mMeshData) {
        bindings = mMeshData->GetAvailableVertexBindings();
    }
    return bindings;
}

void Mesh::AddBatch(const scene::PrimitiveBatch& batch)
{
    mBatches.push_back(batch);
}

void Mesh::UpdateBoundingBox()
{
    if (mBatches.empty()) {
        return;
    }

    mBoundingBox.Set(mBatches[0].GetBoundingBox().GetMin());

    for (const auto& batch : mBatches) {
        mBoundingBox.Expand(batch.GetBoundingBox().GetMin());
        mBoundingBox.Expand(batch.GetBoundingBox().GetMax());
    }
}

std::vector<const scene::Material*> Mesh::GetMaterials() const
{
    std::vector<const scene::Material*> materials;
    for (auto& batch : mBatches) {
        if (IsNull(batch.GetMaterial())) {
            // @TODO: need a better way to handle missing materials
            continue;
        }
        materials.push_back(batch.GetMaterial());
    }
    return materials;
}

} // namespace scene
} // namespace ppx
