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

void OITDemoApp::SetupDepthPeeling()
{
    // Layer texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = mTransparencyTexture->GetWidth();
        createInfo.height                          = mTransparencyTexture->GetHeight();
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = grfx::FORMAT_B8G8R8A8_UNORM;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.sampled         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
            PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mDepthPeeling.single.layerTextures[i]));
        }
    }

    // Depth texture
    {
        grfx::TextureCreateInfo createInfo                = {};
        createInfo.imageType                              = grfx::IMAGE_TYPE_2D;
        createInfo.width                                  = mDepthPeeling.single.layerTextures[0]->GetWidth();
        createInfo.height                                 = mDepthPeeling.single.layerTextures[0]->GetHeight();
        createInfo.depth                                  = 1;
        createInfo.imageFormat                            = mOpaquePass->GetDepthStencilTexture()->GetDepthStencilViewFormat();
        createInfo.sampleCount                            = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                          = 1;
        createInfo.arrayLayerCount                        = 1;
        createInfo.usageFlags.bits.transferDst            = true;
        createInfo.usageFlags.bits.depthStencilAttachment = true;
        createInfo.usageFlags.bits.sampled                = true;
        createInfo.memoryUsage                            = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                           = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        for (uint32_t i = 0; i < DEPTH_PEELING_DEPTH_TEXTURES_COUNT; ++i) {
            PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mDepthPeeling.single.depthTextures[i]));
        }
    }

    // Pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mDepthPeeling.single.layerTextures[0]->GetWidth();
        createInfo.height                     = mDepthPeeling.single.layerTextures[0]->GetHeight();
        createInfo.renderTargetCount          = 1;
        createInfo.depthStencilState          = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        createInfo.depthStencilClearValue     = {1.0f, 0xFF};

        for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
            createInfo.pRenderTargetImages[0] = mDepthPeeling.single.layerTextures[i]->GetImage();
            createInfo.pDepthStencilImage     = mDepthPeeling.single.depthTextures[i % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]->GetImage();
            PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mDepthPeeling.single.layerPasses[i]));
        }
    }

    ////////////////////////////////////////
    // Layer
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_SAMPLER_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_1_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDepthPeeling.single.layerDescriptorSetLayout));

        for (uint32_t i = 0; i < DEPTH_PEELING_DEPTH_TEXTURES_COUNT; ++i) {
            PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDepthPeeling.single.layerDescriptorSetLayout, &mDepthPeeling.single.layerDescriptorSets[i]));

            std::array<grfx::WriteDescriptor, 4> writes = {};

            writes[0].binding      = SHADER_GLOBALS_REGISTER;
            writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].bufferOffset = 0;
            writes[0].bufferRange  = PPX_WHOLE_SIZE;
            writes[0].pBuffer      = mShaderGlobalsBuffer;

            writes[1].binding  = CUSTOM_SAMPLER_0_REGISTER;
            writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
            writes[1].pSampler = mNearestSampler;

            writes[2].binding    = CUSTOM_TEXTURE_0_REGISTER;
            writes[2].arrayIndex = 0;
            writes[2].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[2].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

            writes[3].binding    = CUSTOM_TEXTURE_1_REGISTER;
            writes[3].arrayIndex = 0;
            writes[3].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[3].pImageView = mDepthPeeling.single.depthTextures[(i + 1) % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]->GetSampledImageView();

            PPX_CHECKED_CALL(mDepthPeeling.single.layerDescriptorSets[i]->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
        }
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDepthPeeling.single.layerDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mDepthPeeling.single.layerPipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mDepthPeeling.single.layerTextures[0]->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mDepthPeeling.single.depthTextures[0]->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mDepthPeeling.single.layerPipelineInterface;

        grfx::ShaderModulePtr VS, PS;

        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingLayer_First.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingLayer_First.ps", &PS));
        gpCreateInfo.VS = {VS, "vsmain"};
        gpCreateInfo.PS = {PS, "psmain"};
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mDepthPeeling.single.layerPipeline_FirstLayer));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);

        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingLayer_Others.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingLayer_Others.ps", &PS));
        gpCreateInfo.VS = {VS, "vsmain"};
        gpCreateInfo.PS = {PS, "psmain"};
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mDepthPeeling.single.layerPipeline_OtherLayers));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    ////////////////////////////////////////
    // Combine
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_SAMPLER_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, DEPTH_PEELING_LAYERS_COUNT, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDepthPeeling.single.combineDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDepthPeeling.single.combineDescriptorSetLayout, &mDepthPeeling.single.combineDescriptorSet));

        std::array<grfx::WriteDescriptor, 2 + DEPTH_PEELING_LAYERS_COUNT> writes = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding  = CUSTOM_SAMPLER_0_REGISTER;
        writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pSampler = mNearestSampler;

        for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
            writes[2 + i].binding    = CUSTOM_TEXTURE_0_REGISTER;
            writes[2 + i].arrayIndex = i;
            writes[2 + i].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[2 + i].pImageView = mDepthPeeling.single.layerTextures[i]->GetSampledImageView();
        }

        PPX_CHECKED_CALL(mDepthPeeling.single.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDepthPeeling.single.combineDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mDepthPeeling.single.combinePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "DepthPeelingCombine.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS, "vsmain"};
        gpCreateInfo.PS                                 = {PS, "psmain"};
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
        gpCreateInfo.pPipelineInterface                 = mDepthPeeling.single.combinePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mDepthPeeling.single.combinePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::RecordDepthPeeling()
{
    // Layer passes: extract all layers
    for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
        grfx::DrawPassPtr layerPass = mDepthPeeling.single.layerPasses[i];
        mCommandBuffer->TransitionImageLayout(
            layerPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(layerPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

        mCommandBuffer->SetScissors(layerPass->GetScissor());
        mCommandBuffer->SetViewports(layerPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mDepthPeeling.single.layerPipelineInterface, 1, &mDepthPeeling.single.layerDescriptorSets[i % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]);
        mCommandBuffer->BindGraphicsPipeline(i == 0 ? mDepthPeeling.single.layerPipeline_FirstLayer : mDepthPeeling.single.layerPipeline_OtherLayers);
        mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
        mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
        mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            layerPass,
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

        mCommandBuffer->BindGraphicsDescriptorSets(mDepthPeeling.single.combinePipelineInterface, 1, &mDepthPeeling.single.combineDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mDepthPeeling.single.combinePipeline);
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
