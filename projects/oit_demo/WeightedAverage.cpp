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

#include "OITDemoApplication.h"

void OITDemoApp::SetupWeightedAverage()
{
    ////////////////////////////////////////
    // Common
    ////////////////////////////////////////

    // Texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = mTransparencyTexture->GetWidth();
        createInfo.height                          = mTransparencyTexture->GetHeight();
        createInfo.depth                           = 1;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.sampled         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        createInfo.imageFormat = grfx::FORMAT_R16G16B16A16_FLOAT;
        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mWeightedAverage.colorTexture));

        createInfo.imageFormat = grfx::FORMAT_R16_FLOAT;
        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mWeightedAverage.extraTexture));
    }

    ////////////////////////////////////////
    // Gather
    ////////////////////////////////////////

    // Pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mWeightedAverage.colorTexture->GetWidth();
        createInfo.height                     = mWeightedAverage.colorTexture->GetHeight();
        createInfo.renderTargetCount          = 2;
        createInfo.pRenderTargetImages[0]     = mWeightedAverage.colorTexture->GetImage();
        createInfo.pRenderTargetImages[1]     = mWeightedAverage.extraTexture->GetImage();
        createInfo.pDepthStencilImage         = mOpaquePass->GetDepthStencilTexture()->GetImage();
        createInfo.depthStencilState          = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        createInfo.depthStencilClearValue     = {1.0f, 0xFF};

        // Count type
        createInfo.renderTargetClearValues[1] = {0, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mWeightedAverage.count.gatherPass));

        // Coverage type
        createInfo.renderTargetClearValues[1] = {1, 1, 1, 1};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mWeightedAverage.coverage.gatherPass));
    }

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mWeightedAverage.gatherDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mWeightedAverage.gatherDescriptorSetLayout, &mWeightedAverage.gatherDescriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = SHADER_GLOBALS_REGISTER;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mShaderGlobalsBuffer;
        PPX_CHECKED_CALL(mWeightedAverage.gatherDescriptorSet->UpdateDescriptors(1, &write));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mWeightedAverage.gatherDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mWeightedAverage.gatherPipelineInterface));

        grfx::ShaderModulePtr            VS, PS;
        grfx::GraphicsPipelineCreateInfo gpCreateInfo   = {};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.inputAssemblyState.topology        = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.rasterState.polygonMode            = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.rasterState.cullMode               = grfx::CULL_MODE_NONE;
        gpCreateInfo.rasterState.frontFace              = grfx::FRONT_FACE_CCW;
        gpCreateInfo.rasterState.rasterizationSamples   = grfx::SAMPLE_COUNT_1;
        gpCreateInfo.depthStencilState.depthTestEnable  = true;
        gpCreateInfo.depthStencilState.depthWriteEnable = false;

        gpCreateInfo.colorBlendState.blendAttachmentCount = 2;

        gpCreateInfo.colorBlendState.blendAttachments[0].blendEnable         = true;
        gpCreateInfo.colorBlendState.blendAttachments[0].srcColorBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].dstColorBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].colorBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[0].srcAlphaBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].dstAlphaBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].alphaBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[0].colorWriteMask      = grfx::ColorComponentFlags::RGBA();

        gpCreateInfo.colorBlendState.blendAttachments[1].blendEnable         = true;
        gpCreateInfo.colorBlendState.blendAttachments[1].colorBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[1].srcAlphaBlendFactor = grfx::BLEND_FACTOR_ZERO;
        gpCreateInfo.colorBlendState.blendAttachments[1].dstAlphaBlendFactor = grfx::BLEND_FACTOR_ZERO;
        gpCreateInfo.colorBlendState.blendAttachments[1].alphaBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[1].colorWriteMask      = grfx::ColorComponentFlags::RGBA();

        gpCreateInfo.outputState.renderTargetCount      = 2;
        gpCreateInfo.outputState.renderTargetFormats[0] = mWeightedAverage.colorTexture->GetImageFormat();
        gpCreateInfo.outputState.renderTargetFormats[1] = mWeightedAverage.extraTexture->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mWeightedAverage.gatherPipelineInterface;

        // Count type
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageFragmentCountGather.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageFragmentCountGather.ps", &PS));
        gpCreateInfo.VS                                                      = {VS, "vsmain"};
        gpCreateInfo.PS                                                      = {PS, "psmain"};
        gpCreateInfo.colorBlendState.blendAttachments[1].srcColorBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[1].dstColorBlendFactor = grfx::BLEND_FACTOR_ONE;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mWeightedAverage.count.gatherPipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);

        // Coverage type
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageExactCoverageGather.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageExactCoverageGather.ps", &PS));
        gpCreateInfo.VS                                                      = {VS, "vsmain"};
        gpCreateInfo.PS                                                      = {PS, "psmain"};
        gpCreateInfo.colorBlendState.blendAttachments[1].srcColorBlendFactor = grfx::BLEND_FACTOR_ZERO;
        gpCreateInfo.colorBlendState.blendAttachments[1].dstColorBlendFactor = grfx::BLEND_FACTOR_SRC_COLOR;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mWeightedAverage.coverage.gatherPipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    ////////////////////////////////////////
    // Combine
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_SAMPLER_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_1_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mWeightedAverage.combineDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mWeightedAverage.combineDescriptorSetLayout, &mWeightedAverage.combineDescriptorSet));

        std::array<grfx::WriteDescriptor, 3> writes = {};

        writes[0].binding  = CUSTOM_SAMPLER_0_REGISTER;
        writes[0].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[0].pSampler = mNearestSampler;

        writes[1].binding    = CUSTOM_TEXTURE_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mWeightedAverage.colorTexture->GetSampledImageView();

        writes[2].binding    = CUSTOM_TEXTURE_1_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2].pImageView = mWeightedAverage.extraTexture->GetSampledImageView();

        PPX_CHECKED_CALL(mWeightedAverage.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mWeightedAverage.combineDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mWeightedAverage.combinePipelineInterface));

        grfx::ShaderModulePtr             VS, PS;
        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.vertexInputState.bindingCount      = 0;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyTexture->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mWeightedAverage.combinePipelineInterface;

        // Count type
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageFragmentCountCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageFragmentCountCombine.ps", &PS));
        gpCreateInfo.VS = {VS, "vsmain"};
        gpCreateInfo.PS = {PS, "psmain"};
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mWeightedAverage.count.combinePipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);

        // Coverage type
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageExactCoverageCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "WeightedAverageExactCoverageCombine.ps", &PS));
        gpCreateInfo.VS = {VS, "vsmain"};
        gpCreateInfo.PS = {PS, "psmain"};
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mWeightedAverage.coverage.combinePipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::RecordWeightedAverage()
{
    grfx::DrawPassPtr         gatherPass;
    grfx::GraphicsPipelinePtr gatherPipeline;
    grfx::GraphicsPipelinePtr combinePipeline;
    switch (mGuiParameters.weightedAverage.type) {
        case WEIGHTED_AVERAGE_TYPE_FRAGMENT_COUNT: {
            gatherPass      = mWeightedAverage.count.gatherPass;
            gatherPipeline  = mWeightedAverage.count.gatherPipeline;
            combinePipeline = mWeightedAverage.count.combinePipeline;
            break;
        }
        case WEIGHTED_AVERAGE_TYPE_EXACT_COVERAGE: {
            gatherPass      = mWeightedAverage.coverage.gatherPass;
            gatherPipeline  = mWeightedAverage.coverage.gatherPipeline;
            combinePipeline = mWeightedAverage.coverage.combinePipeline;
            break;
        }
        default: {
            PPX_ASSERT_MSG(false, "unknown weighted average type");
            break;
        }
    }

    // Gather pass: compute the formula factors for each pixels
    {
        mCommandBuffer->TransitionImageLayout(
            gatherPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(gatherPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

        mCommandBuffer->SetScissors(gatherPass->GetScissor());
        mCommandBuffer->SetViewports(gatherPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mWeightedAverage.gatherPipelineInterface, 1, &mWeightedAverage.gatherDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(gatherPipeline);
        mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
        mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
        mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            gatherPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }

    // Transparency pass: combine the results for each pixels
    {
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(mTransparencyPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

        mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
        mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mWeightedAverage.combinePipelineInterface, 1, &mWeightedAverage.combineDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(combinePipeline);
        mCommandBuffer->Draw(3);

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }
}
