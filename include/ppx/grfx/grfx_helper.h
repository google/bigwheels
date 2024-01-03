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

#ifndef ppx_grfx_helper_h
#define ppx_grfx_helper_h

#include "ppx/config.h"
#include "ppx/grfx/grfx_constants.h"
#include "ppx/grfx/grfx_enums.h"
#include "ppx/grfx/grfx_format.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ppx {
namespace grfx {

struct BufferUsageFlags
{
    union
    {
        struct
        {
            bool transferSrc                    : 1;
            bool transferDst                    : 1;
            bool uniformTexelBuffer             : 1;
            bool storageTexelBuffer             : 1;
            bool uniformBuffer                  : 1;
            bool rawStorageBuffer               : 1;
            bool roStructuredBuffer             : 1;
            bool rwStructuredBuffer             : 1;
            bool indexBuffer                    : 1;
            bool vertexBuffer                   : 1;
            bool indirectBuffer                 : 1;
            bool conditionalRendering           : 1;
            bool rayTracing                     : 1;
            bool transformFeedbackBuffer        : 1;
            bool transformFeedbackCounterBuffer : 1;
            bool shaderDeviceAddress            : 1;
        } bits;
        uint32_t flags;
    };

    BufferUsageFlags()
        : flags(0) {}

    BufferUsageFlags(uint32_t _flags)
        : flags(_flags) {}

    BufferUsageFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }
};

// -------------------------------------------------------------------------------------------------

struct ColorComponentFlags
{
    union
    {
        struct
        {
            bool r : 1;
            bool g : 1;
            bool b : 1;
            bool a : 1;
        } bits;
        uint32_t flags;
    };

    ColorComponentFlags()
        : flags(0) {}

    ColorComponentFlags(uint32_t _flags)
        : flags(_flags) {}

    ColorComponentFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }

    static ColorComponentFlags RGBA();
};

// -------------------------------------------------------------------------------------------------

struct DescriptorSetLayoutFlags
{
    union
    {
        struct
        {
            bool pushable : 1;

        } bits;
        uint32_t flags;
    };

    DescriptorSetLayoutFlags()
        : flags(0) {}

    DescriptorSetLayoutFlags(uint32_t flags_)
        : flags(flags_) {}

    DescriptorSetLayoutFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }
};

// -------------------------------------------------------------------------------------------------

struct DrawPassClearFlags
{
    union
    {
        struct
        {
            bool clearRenderTargets : 1;
            bool clearDepth         : 1;
            bool clearStencil       : 1;

        } bits;
        uint32_t flags;
    };

    DrawPassClearFlags()
        : flags(0) {}

    DrawPassClearFlags(uint32_t flags_)
        : flags(flags_) {}

    DrawPassClearFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }
};

// -------------------------------------------------------------------------------------------------

struct BeginRenderingFlags
{
    union
    {
        struct
        {
            bool suspending : 1;
            bool resuming   : 1;
        } bits;
        uint32_t flags;
    };

    BeginRenderingFlags()
        : flags(0) {}

    BeginRenderingFlags(uint32_t flags_)
        : flags(flags_) {}

    BeginRenderingFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }
};

// -------------------------------------------------------------------------------------------------

struct ImageUsageFlags
{
    union
    {
        struct
        {
            bool transferSrc                   : 1;
            bool transferDst                   : 1;
            bool sampled                       : 1;
            bool storage                       : 1;
            bool colorAttachment               : 1;
            bool depthStencilAttachment        : 1;
            bool transientAttachment           : 1;
            bool inputAttachment               : 1;
            bool fragmentDensityMap            : 1;
            bool fragmentShadingRateAttachment : 1;
        } bits;
        uint32_t flags;
    };

    ImageUsageFlags()
        : flags(0) {}

    ImageUsageFlags(uint32_t flags_)
        : flags(flags_) {}

    ImageUsageFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    ImageUsageFlags& operator|=(const ImageUsageFlags& rhs)
    {
        this->flags |= rhs.flags;
        return *this;
    }

    ImageUsageFlags& operator|=(uint32_t rhs)
    {
        this->flags |= rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }

    static ImageUsageFlags SampledImage();
};

// -------------------------------------------------------------------------------------------------

struct Range
{
    uint32_t start = 0;
    uint32_t end   = 0;
};

// -------------------------------------------------------------------------------------------------

struct ShaderStageFlags
{
    union
    {
        struct
        {
            bool VS : 1;
            bool HS : 1;
            bool DS : 1;
            bool GS : 1;
            bool PS : 1;
            bool CS : 1;

        } bits;
        uint32_t flags;
    };

    ShaderStageFlags()
        : flags(0) {}

    ShaderStageFlags(uint32_t _flags)
        : flags(_flags) {}

    ShaderStageFlags& operator=(uint32_t rhs)
    {
        this->flags = rhs;
        return *this;
    }

    operator uint32_t() const
    {
        return flags;
    }

    static ShaderStageFlags SampledImage();
};

// -------------------------------------------------------------------------------------------------

struct VertexAttribute
{
    std::string           semanticName = "";                              // Semantic name (no effect in Vulkan currently)
    uint32_t              location     = 0;                               // @TODO: Find a way to handle between DX and VK
    grfx::Format          format       = grfx::FORMAT_UNDEFINED;          //
    uint32_t              binding      = 0;                               // Valid range is [0, 15]
    uint32_t              offset       = PPX_APPEND_OFFSET_ALIGNED;       // Use PPX_APPEND_OFFSET_ALIGNED to auto calculate offsets
    grfx::VertexInputRate inputRate    = grfx::VERTEX_INPUT_RATE_VERTEX;  //
    grfx::VertexSemantic  semantic     = grfx::VERTEX_SEMANTIC_UNDEFINED; // [OPTIONAL]
};

// -------------------------------------------------------------------------------------------------

//! @class VertexBinding
//!
//! Storage class for binding number, vertex data stride, and vertex attributes for a vertex buffer
//! binding.
//!
//! ** WARNING **
//! Adding an attribute updates the stride information based on the current set of attributes.
//! If a custom stride is required, add all the attributes first then call
//! VertexBinding::SetStride() to set the stride.
//!
class VertexBinding
{
public:
    VertexBinding() {}

    VertexBinding(uint32_t binding, grfx::VertexInputRate inputRate)
        : mBinding(binding), mInputRate(inputRate) {}

    VertexBinding(const VertexAttribute& attribute)
        : mBinding(attribute.binding), mInputRate(attribute.inputRate)
    {
        AppendAttribute(attribute);
    }

    ~VertexBinding() {}

    uint32_t              GetBinding() const { return mBinding; }
    void                  SetBinding(uint32_t binding);
    const uint32_t&       GetStride() const { return mStride; } // Return a reference to member var so apps can take address of it
    void                  SetStride(uint32_t stride);
    grfx::VertexInputRate GetInputRate() const { return mInputRate; }
    uint32_t              GetAttributeCount() const { return static_cast<uint32_t>(mAttributes.size()); }
    Result                GetAttribute(uint32_t index, const grfx::VertexAttribute** ppAttribute) const;
    uint32_t              GetAttributeIndex(grfx::VertexSemantic semantic) const;
    VertexBinding&        AppendAttribute(const grfx::VertexAttribute& attribute);

    VertexBinding& operator+=(const grfx::VertexAttribute& rhs);

private:
    static const grfx::VertexInputRate kInvalidVertexInputRate = static_cast<grfx::VertexInputRate>(~0);

    uint32_t                           mBinding   = 0;
    uint32_t                           mStride    = 0;
    grfx::VertexInputRate              mInputRate = kInvalidVertexInputRate;
    std::vector<grfx::VertexAttribute> mAttributes;
};

// -------------------------------------------------------------------------------------------------

class VertexDescription
{
public:
    VertexDescription() {}
    ~VertexDescription() {}

    uint32_t                   GetBindingCount() const { return CountU32(mBindings); }
    Result                     GetBinding(uint32_t index, const grfx::VertexBinding** ppBinding) const;
    const grfx::VertexBinding* GetBinding(uint32_t index) const;
    uint32_t                   GetBindingIndex(uint32_t binding) const;
    Result                     AppendBinding(const grfx::VertexBinding& binding);

private:
    std::vector<VertexBinding> mBindings;
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_helper_h
