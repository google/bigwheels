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

#ifndef ppx_scene_config_h
#define ppx_scene_config_h

#include "ppx/config.h"
#include "ppx/math_config.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_mesh.h"
#include "ppx/grfx/grfx_pipeline.h"
#include "ppx/grfx/grfx_render_pass.h"
#include "ppx/bounding_volume.h"

#include <filesystem>
#include <unordered_map>

namespace ppx {
namespace scene {

class Image;
class Loader;
class Material;
class MaterialFactory;
class Mesh;
class MeshData;
class Node;
class Renderer;
class ResourceManager;
class Sampler;
class Scene;
class Texture;

using ImageRef    = std::shared_ptr<scene::Image>;
using MaterialRef = std::shared_ptr<scene::Material>;
using MeshRef     = std::shared_ptr<scene::Mesh>;
using MeshDataRef = std::shared_ptr<scene::MeshData>;
using NodeRef     = std::shared_ptr<scene::Node>;
using SamplerRef  = std::shared_ptr<scene::Sampler>;
using TextureRef  = std::shared_ptr<scene::Texture>;

enum LightType
{
    LIGHT_TYPE_UNDEFINED   = 0,
    LIGHT_TYPE_DIRECTIONAL = 1,
    LIGHT_TYPE_POINT       = 2,
    LIGHT_TYPE_SPOT        = 3,
};

const uint32_t kVertexAttributeBinding          = 1;
const uint32_t kVertexAttributeTexCoordLocation = 1;
const uint32_t kVertexAttributeNormalLocation   = 2;
const uint32_t kVertexAttributeTangentLocation  = 3;
const uint32_t kVertexAttributeColorLocation    = 4;

template <
    typename ObjectT,
    typename ObjectRefT = std::shared_ptr<ObjectT>>
ObjectRefT MakeRef(ObjectT* pObject)
{
    return ObjectRefT(pObject);
}

// -------------------------------------------------------------------------------------------------
// Helper Structs
// -------------------------------------------------------------------------------------------------
struct VertexAttributeFlags
{
    union
    {
        struct
        {
            bool texCoords : 1;
            bool normals   : 1;
            bool tangents  : 1;
            bool colors    : 1;
        } bits;
        uint32_t mask = 0;
    };

    VertexAttributeFlags(uint32_t initialMask = 0)
        : mask(initialMask) {}

    ~VertexAttributeFlags() {}

    static VertexAttributeFlags None()
    {
        VertexAttributeFlags flags = {};
        flags.bits.texCoords       = false;
        flags.bits.normals         = false;
        flags.bits.tangents        = false;
        flags.bits.colors          = false;
        return flags;
    }

    static VertexAttributeFlags EnableAll()
    {
        VertexAttributeFlags flags = {};
        flags.bits.texCoords       = true;
        flags.bits.normals         = true;
        flags.bits.tangents        = true;
        flags.bits.colors          = true;
        return flags;
    }

    VertexAttributeFlags& TexCoords(bool enable = true)
    {
        this->bits.texCoords = enable;
        return *this;
    }

    VertexAttributeFlags& Normals(bool enable = true)
    {
        this->bits.normals = enable;
        return *this;
    }

    VertexAttributeFlags& Tangents(bool enable = true)
    {
        this->bits.tangents = enable;
        return *this;
    }

    VertexAttributeFlags& VertexColors(bool enable = true)
    {
        this->bits.colors = enable;
        return *this;
    }

    VertexAttributeFlags& operator&=(const VertexAttributeFlags& rhs)
    {
        this->mask &= rhs.mask;
        return *this;
    }

    VertexAttributeFlags& operator|=(const VertexAttributeFlags& rhs)
    {
        this->mask |= rhs.mask;
        return *this;
    }

    grfx::VertexBinding GetVertexBinding() const
    {
        grfx::VertexBinding binding = grfx::VertexBinding(1, grfx::VERTEX_INPUT_RATE_VERTEX);

        uint32_t offset = 0;
        if (this->bits.texCoords) {
            binding.AppendAttribute(grfx::VertexAttribute{"TEXCOORD", kVertexAttributeTexCoordLocation, grfx::FORMAT_R32G32_FLOAT, kVertexAttributeBinding, offset, grfx::VERTEX_INPUT_RATE_VERTEX});
            offset += 8;
        }
        if (this->bits.normals) {
            binding.AppendAttribute(grfx::VertexAttribute{"NORMAL", kVertexAttributeNormalLocation, grfx::FORMAT_R32G32B32_FLOAT, kVertexAttributeBinding, offset, grfx::VERTEX_INPUT_RATE_VERTEX});
            offset += 12;
        }
        if (this->bits.tangents) {
            binding.AppendAttribute(grfx::VertexAttribute{"TANGENT", kVertexAttributeTangentLocation, grfx::FORMAT_R32G32B32A32_FLOAT, kVertexAttributeBinding, offset, grfx::VERTEX_INPUT_RATE_VERTEX});
            offset += 16;
        }
        if (this->bits.colors) {
            binding.AppendAttribute(grfx::VertexAttribute{"COLOR", kVertexAttributeColorLocation, grfx::FORMAT_R32G32B32_FLOAT, kVertexAttributeBinding, offset, grfx::VERTEX_INPUT_RATE_VERTEX});
        }

        return binding;
    }
};

inline scene::VertexAttributeFlags operator&(const scene::VertexAttributeFlags& a, const scene::VertexAttributeFlags& b)
{
    return scene::VertexAttributeFlags(a.mask & b.mask);
}

inline scene::VertexAttributeFlags operator|(const scene::VertexAttributeFlags& a, const scene::VertexAttributeFlags& b)
{
    return scene::VertexAttributeFlags(a.mask | b.mask);
}

} // namespace scene
} // namespace ppx

#endif // ppx_scene_config_h
