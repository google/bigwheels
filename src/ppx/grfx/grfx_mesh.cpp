// Copyright 2022 Google LLC
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

#include "ppx/grfx/grfx_mesh.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// MeshCreateInfo
// -------------------------------------------------------------------------------------------------
MeshCreateInfo::MeshCreateInfo(const ppx::Geometry& geometry)
{
    this->indexType         = geometry.GetIndexType();
    this->indexCount        = geometry.GetIndexCount();
    this->vertexCount       = geometry.GetVertexCount();
    this->vertexBufferCount = geometry.GetVertexBufferCount();
    this->memoryUsage       = grfx::MEMORY_USAGE_GPU_ONLY;
    std::memset(&this->vertexBuffers, 0, PPX_MAX_VERTEX_BINDINGS * sizeof(grfx::MeshVertexBufferDescription));

    const uint32_t bindingCount = geometry.GetVertexBindingCount();
    for (uint32_t bindingIdx = 0; bindingIdx < bindingCount; ++bindingIdx) {
        const uint32_t attrCount = geometry.GetVertexBinding(bindingIdx)->GetAttributeCount();

        this->vertexBuffers[bindingIdx].attributeCount = attrCount;
        for (uint32_t attrIdx = 0; attrIdx < attrCount; ++attrIdx) {
            const grfx::VertexAttribute* pAttribute = nullptr;
            geometry.GetVertexBinding(bindingIdx)->GetAttribute(attrIdx, &pAttribute);

            this->vertexBuffers[bindingIdx].attributes[attrIdx].format         = pAttribute->format;
            this->vertexBuffers[bindingIdx].attributes[attrIdx].stride         = 0; // Calculated later
            this->vertexBuffers[bindingIdx].attributes[attrIdx].vertexSemantic = pAttribute->semantic;
        }

        this->vertexBuffers[bindingIdx].vertexInputRate = grfx::VERTEX_INPUT_RATE_VERTEX;
    }
}

// -------------------------------------------------------------------------------------------------
// Mesh
// -------------------------------------------------------------------------------------------------
Result Mesh::CreateApiObjects(const grfx::MeshCreateInfo* pCreateInfo)
{
    // Bail if both index count and vertex count are 0
    if ((pCreateInfo->indexCount == 0) && (pCreateInfo->vertexCount) == 0) {
        return Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION;
    }
    // Bail if vertexCount is not 0 but vertex buffer count is 0
    if ((pCreateInfo->vertexCount > 0) && (pCreateInfo->vertexBufferCount == 0)) {
        return Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION;
    }

    // Index buffer
    if (pCreateInfo->indexCount > 0) {
        // Bail if index type doesn't make sense
        if ((pCreateInfo->indexType != grfx::INDEX_TYPE_UINT16) && (pCreateInfo->indexType != grfx::INDEX_TYPE_UINT32)) {
            return Result::ERROR_GRFX_INVALID_INDEX_TYPE;
        }

        grfx::BufferCreateInfo createInfo      = {};
        createInfo.size                        = pCreateInfo->indexCount * grfx::IndexTypeSize(pCreateInfo->indexType);
        createInfo.usageFlags.bits.indexBuffer = true;
        createInfo.usageFlags.bits.transferDst = true;
        createInfo.memoryUsage                 = pCreateInfo->memoryUsage;
        createInfo.initialState                = grfx::RESOURCE_STATE_GENERAL;
        createInfo.ownership                   = grfx::OWNERSHIP_REFERENCE;

        auto ppxres = GetDevice()->CreateBuffer(&createInfo, &mIndexBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "create mesh index buffer failed");
            return ppxres;
        }
    }

    // Vertex buffers
    if (pCreateInfo->vertexCount > 0) {
        mVertexBuffers.resize(pCreateInfo->vertexBufferCount);

        // Iterate through all the vertex buffer descriptions and create appropriately sized buffers
        for (uint32_t vbIdx = 0; vbIdx < pCreateInfo->vertexBufferCount; ++vbIdx) {
            // Copy vertex buffer description
            std::memcpy(&mVertexBuffers[vbIdx].second, &pCreateInfo->vertexBuffers[vbIdx], sizeof(MeshVertexBufferDescription));

            auto& vertexBufferDesc = mVertexBuffers[vbIdx].second;
            // Bail if attribute count is 0
            if (vertexBufferDesc.attributeCount == 0) {
                return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT;
            }

            // Calculate vertex stride (if needed) and attribute offsets
            bool calculateVertexStride = (mVertexBuffers[vbIdx].second.stride == 0);
            for (uint32_t attrIdx = 0; attrIdx < vertexBufferDesc.attributeCount; ++attrIdx) {
                auto&        attr   = vertexBufferDesc.attributes[attrIdx];
                grfx::Format format = attr.format;
                if (format == grfx::FORMAT_UNDEFINED) {
                    return Result::ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED;
                }

                auto* pFormatDesc = grfx::GetFormatDescription(format);
                // Bail if the format's size is zero
                if (pFormatDesc->bytesPerTexel == 0) {
                    return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE;
                }
                // Bail if the attribute stride is NOT 0 and less than the format size
                if ((attr.stride != 0) && (attr.stride < pFormatDesc->bytesPerTexel)) {
                    return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE;
                }

                // Calculate stride if needed
                if (attr.stride == 0) {
                    attr.stride = pFormatDesc->bytesPerTexel;
                }

                // Set offset
                attr.offset = mVertexBuffers[vbIdx].second.stride;

                // Increment vertex stride
                if (calculateVertexStride) {
                    mVertexBuffers[vbIdx].second.stride += attr.stride;
                }
            }

            grfx::BufferCreateInfo createInfo       = {};
            createInfo.size                         = pCreateInfo->vertexCount * mVertexBuffers[vbIdx].second.stride;
            createInfo.usageFlags.bits.vertexBuffer = true;
            createInfo.usageFlags.bits.transferDst  = true;
            createInfo.memoryUsage                  = pCreateInfo->memoryUsage;
            createInfo.initialState                 = grfx::RESOURCE_STATE_GENERAL;
            createInfo.ownership                    = grfx::OWNERSHIP_REFERENCE;

            auto ppxres = GetDevice()->CreateBuffer(&createInfo, &mVertexBuffers[vbIdx].first);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "create mesh vertex buffer failed");
                return ppxres;
            }
        }
    }

    // Derived vertex bindings
    {
        uint32_t location = 0;
        for (uint32_t bufferIndex = 0; bufferIndex < GetVertexBufferCount(); ++bufferIndex) {
            auto pBufferDesc = GetVertexBufferDescription(bufferIndex);

            grfx::VertexBinding binding = grfx::VertexBinding(bufferIndex, pBufferDesc->vertexInputRate);
            binding.SetBinding(bufferIndex);

            for (uint32_t attrIdx = 0; attrIdx < pBufferDesc->attributeCount; ++attrIdx) {
                auto& srcAttr = pBufferDesc->attributes[attrIdx];

                std::string semanticName = PPX_SEMANTIC_NAME_CUSTOM;
                // clang-format off
                switch (srcAttr.vertexSemantic) {
                    default: break;
                    case grfx::VERTEX_SEMANTIC_POSITION  : semanticName = PPX_SEMANTIC_NAME_POSITION;  break;
                    case grfx::VERTEX_SEMANTIC_NORMAL    : semanticName = PPX_SEMANTIC_NAME_NORMAL;    break;
                    case grfx::VERTEX_SEMANTIC_COLOR     : semanticName = PPX_SEMANTIC_NAME_COLOR;     break;
                    case grfx::VERTEX_SEMANTIC_TEXCOORD  : semanticName = PPX_SEMANTIC_NAME_TEXCOORD;  break;
                    case grfx::VERTEX_SEMANTIC_TANGENT   : semanticName = PPX_SEMANTIC_NAME_TANGENT;   break;
                    case grfx::VERTEX_SEMANTIC_BITANGENT : semanticName = PPX_SEMANTIC_NAME_BITANGENT; break;
                }
                // clang-format on

                grfx::VertexAttribute attr = {};
                attr.semanticName          = semanticName;
                attr.location              = location;
                attr.format                = srcAttr.format;
                attr.binding               = bufferIndex;
                attr.offset                = srcAttr.offset;
                attr.inputRate             = pBufferDesc->vertexInputRate;
                attr.semantic              = srcAttr.vertexSemantic;

                binding.AppendAttribute(attr);

                ++location;
            }

            binding.SetStride(pBufferDesc->stride);

            mDerivedVertexBindings.push_back(binding);
        }
    }

    return Result::SUCCESS;
}

void Mesh::DestroyApiObjects()
{
    if (mIndexBuffer) {
        GetDevice()->DestroyBuffer(mIndexBuffer);
    }

    for (auto& elem : mVertexBuffers) {
        GetDevice()->DestroyBuffer(elem.first);
    }
    mVertexBuffers.clear();
}

grfx::BufferPtr Mesh::GetVertexBuffer(uint32_t index) const
{
    const uint32_t vertexBufferCount = CountU32(mVertexBuffers);
    if (index >= vertexBufferCount) {
        return nullptr;
    }
    return mVertexBuffers[index].first;
}

const grfx::MeshVertexBufferDescription* Mesh::GetVertexBufferDescription(uint32_t index) const
{
    const uint32_t vertexBufferCount = CountU32(mVertexBuffers);
    if (index >= vertexBufferCount) {
        return nullptr;
    }
    return &mVertexBuffers[index].second;
}

} // namespace grfx
} // namespace ppx
