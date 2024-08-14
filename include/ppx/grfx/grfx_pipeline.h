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

#ifndef ppx_grfx_pipeline_h
#define ppx_grfx_pipeline_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

struct ShaderStageInfo
{
    const grfx::ShaderModule* pModule    = nullptr;
    std::string               entryPoint = "";
};

// -------------------------------------------------------------------------------------------------

//! @struct ComputePipelineCreateInfo
//!
//!
struct ComputePipelineCreateInfo
{
    grfx::ShaderStageInfo          CS                 = {};
    const grfx::PipelineInterface* pPipelineInterface = nullptr;
};

//! @class ComputePipeline
//!
//!
class ComputePipeline
    : public grfx::DeviceObject<grfx::ComputePipelineCreateInfo>
{
public:
    ComputePipeline() {}
    virtual ~ComputePipeline() {}

protected:
    virtual Result Create(const grfx::ComputePipelineCreateInfo* pCreateInfo) override;
    friend class grfx::Device;
};

// -------------------------------------------------------------------------------------------------

struct VertexInputState
{
    uint32_t            bindingCount                      = 0;
    grfx::VertexBinding bindings[PPX_MAX_VERTEX_BINDINGS] = {};
};

struct InputAssemblyState
{
    grfx::PrimitiveTopology topology               = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool                    primitiveRestartEnable = false;
};

struct TessellationState
{
    uint32_t                       patchControlPoints = 0;
    grfx::TessellationDomainOrigin domainOrigin       = grfx::TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
};

struct RasterState
{
    bool              depthClampEnable        = false;
    bool              rasterizeDiscardEnable  = false;
    grfx::PolygonMode polygonMode             = grfx::POLYGON_MODE_FILL;
    grfx::CullMode    cullMode                = grfx::CULL_MODE_NONE;
    grfx::FrontFace   frontFace               = grfx::FRONT_FACE_CCW;
    bool              depthBiasEnable         = false;
    float             depthBiasConstantFactor = 0.0f;
    float             depthBiasClamp          = 0.0f;
    float             depthBiasSlopeFactor    = 0.0f;
    bool              depthClipEnable         = false;
    grfx::SampleCount rasterizationSamples    = grfx::SAMPLE_COUNT_1;
};

struct MultisampleState
{
    bool alphaToCoverageEnable = false;
};

struct StencilOpState
{
    grfx::StencilOp failOp      = grfx::STENCIL_OP_KEEP;
    grfx::StencilOp passOp      = grfx::STENCIL_OP_KEEP;
    grfx::StencilOp depthFailOp = grfx::STENCIL_OP_KEEP;
    grfx::CompareOp compareOp   = grfx::COMPARE_OP_NEVER;
    uint32_t        compareMask = 0;
    uint32_t        writeMask   = 0;
    uint32_t        reference   = 0;
};

struct DepthStencilState
{
    bool                 depthTestEnable       = true;
    bool                 depthWriteEnable      = true;
    grfx::CompareOp      depthCompareOp        = grfx::COMPARE_OP_LESS;
    bool                 depthBoundsTestEnable = false;
    float                minDepthBounds        = 0.0f;
    float                maxDepthBounds        = 1.0f;
    bool                 stencilTestEnable     = false;
    grfx::StencilOpState front                 = {};
    grfx::StencilOpState back                  = {};
};

struct BlendAttachmentState
{
    bool                      blendEnable         = false;
    grfx::BlendFactor         srcColorBlendFactor = grfx::BLEND_FACTOR_ONE;
    grfx::BlendFactor         dstColorBlendFactor = grfx::BLEND_FACTOR_ZERO;
    grfx::BlendOp             colorBlendOp        = grfx::BLEND_OP_ADD;
    grfx::BlendFactor         srcAlphaBlendFactor = grfx::BLEND_FACTOR_ONE;
    grfx::BlendFactor         dstAlphaBlendFactor = grfx::BLEND_FACTOR_ZERO;
    grfx::BlendOp             alphaBlendOp        = grfx::BLEND_OP_ADD;
    grfx::ColorComponentFlags colorWriteMask      = grfx::ColorComponentFlags::RGBA();

    // These are best guesses based on random formulas off of the internet.
    // Correct later when authorative literature is found.
    //
    static grfx::BlendAttachmentState BlendModeAdditive();
    static grfx::BlendAttachmentState BlendModeAlpha();
    static grfx::BlendAttachmentState BlendModeOver();
    static grfx::BlendAttachmentState BlendModeUnder();
    static grfx::BlendAttachmentState BlendModePremultAlpha();
    static grfx::BlendAttachmentState BlendModeDisableOutput();
};

struct ColorBlendState
{
    bool                       logicOpEnable                            = false;
    grfx::LogicOp              logicOp                                  = grfx::LOGIC_OP_CLEAR;
    uint32_t                   blendAttachmentCount                     = 0;
    grfx::BlendAttachmentState blendAttachments[PPX_MAX_RENDER_TARGETS] = {};
    float                      blendConstants[4]                        = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct OutputState
{
    uint32_t     renderTargetCount                           = 0;
    grfx::Format renderTargetFormats[PPX_MAX_RENDER_TARGETS] = {grfx::FORMAT_UNDEFINED};
    grfx::Format depthStencilFormat                          = grfx::FORMAT_UNDEFINED;
};

//! @struct GraphicsPipelineCreateInfo
//!
//!
struct GraphicsPipelineCreateInfo
{
    grfx::ShaderStageInfo          VS                 = {};
    grfx::ShaderStageInfo          HS                 = {};
    grfx::ShaderStageInfo          DS                 = {};
    grfx::ShaderStageInfo          GS                 = {};
    grfx::ShaderStageInfo          PS                 = {};
    grfx::VertexInputState         vertexInputState   = {};
    grfx::InputAssemblyState       inputAssemblyState = {};
    grfx::TessellationState        tessellationState  = {};
    grfx::RasterState              rasterState        = {};
    grfx::MultisampleState         multisampleState   = {};
    grfx::DepthStencilState        depthStencilState  = {};
    grfx::ColorBlendState          colorBlendState    = {};
    grfx::OutputState              outputState        = {};
    grfx::ShadingRateMode          shadingRateMode    = grfx::SHADING_RATE_NONE;
    grfx::MultiViewState           multiViewState     = {};
    const grfx::PipelineInterface* pPipelineInterface = nullptr;
    bool                           dynamicRenderPass  = false;
};

struct GraphicsPipelineCreateInfo2
{
    grfx::ShaderStageInfo          VS                                 = {};
    grfx::ShaderStageInfo          PS                                 = {};
    grfx::VertexInputState         vertexInputState                   = {};
    grfx::PrimitiveTopology        topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    grfx::PolygonMode              polygonMode                        = grfx::POLYGON_MODE_FILL;
    grfx::CullMode                 cullMode                           = grfx::CULL_MODE_NONE;
    grfx::FrontFace                frontFace                          = grfx::FRONT_FACE_CCW;
    bool                           depthReadEnable                    = true;
    bool                           depthWriteEnable                   = true;
    grfx::CompareOp                depthCompareOp                     = grfx::COMPARE_OP_LESS;
    grfx::BlendMode                blendModes[PPX_MAX_RENDER_TARGETS] = {grfx::BLEND_MODE_NONE};
    grfx::OutputState              outputState                        = {};
    grfx::ShadingRateMode          shadingRateMode                    = grfx::SHADING_RATE_NONE;
    grfx::MultiViewState           multiViewState                     = {};
    const grfx::PipelineInterface* pPipelineInterface                 = nullptr;
    bool                           dynamicRenderPass                  = false;
};

namespace internal {

void FillOutGraphicsPipelineCreateInfo(
    const grfx::GraphicsPipelineCreateInfo2* pSrcCreateInfo,
    grfx::GraphicsPipelineCreateInfo*        pDstCreateInfo);

} // namespace internal

//! @class GraphicsPipeline
//!
//!
class GraphicsPipeline
    : public grfx::DeviceObject<grfx::GraphicsPipelineCreateInfo>
{
public:
    GraphicsPipeline() {}
    virtual ~GraphicsPipeline() {}

protected:
    virtual Result Create(const grfx::GraphicsPipelineCreateInfo* pCreateInfo) override;
    friend class grfx::Device;
};

// -------------------------------------------------------------------------------------------------

//! @struct PipelineInterfaceCreateInfo
//!
//!
struct PipelineInterfaceCreateInfo
{
    uint32_t setCount = 0;
    struct
    {
        uint32_t                         set     = PPX_VALUE_IGNORED; // Set number
        const grfx::DescriptorSetLayout* pLayout = nullptr;           // Set layout
    } sets[PPX_MAX_BOUND_DESCRIPTOR_SETS] = {};

    // VK: Push constants
    // DX: Root constants
    //
    // Push/root constants are measured in DWORDs (uint32_t) aka 32-bit values.
    //
    // The binding and set for push constants CAN NOT overlap with a binding
    // AND set in sets (the struct immediately above this one). It's okay for
    // push constants to be in an existing set at binding that is not used
    // by an entry in the set layout.
    //
    struct
    {
        uint32_t              count           = 0;                 // Measured in DWORDs, must be less than or equal to PPX_MAX_PUSH_CONSTANTS
        uint32_t              binding         = PPX_VALUE_IGNORED; // D3D12 only, ignored by Vulkan
        uint32_t              set             = PPX_VALUE_IGNORED; // D3D12 only, ignored by Vulkan
        grfx::ShaderStageBits shaderVisiblity = grfx::SHADER_STAGE_ALL;
    } pushConstants;
};

//! @class PipelineInterface
//!
//! VK: Pipeline layout
//! DX: Root signature
//!
class PipelineInterface
    : public grfx::DeviceObject<grfx::PipelineInterfaceCreateInfo>
{
public:
    PipelineInterface() {}
    virtual ~PipelineInterface() {}

    bool                         HasConsecutiveSetNumbers() const { return mHasConsecutiveSetNumbers; }
    const std::vector<uint32_t>& GetSetNumbers() const { return mSetNumbers; }

    const grfx::DescriptorSetLayout* GetSetLayout(uint32_t setNumber) const;

protected:
    virtual Result Create(const grfx::PipelineInterfaceCreateInfo* pCreateInfo) override;
    friend class grfx::Device;

private:
    bool                  mHasConsecutiveSetNumbers = false;
    std::vector<uint32_t> mSetNumbers               = {};
};

} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_pipeline_h
