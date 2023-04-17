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

void OITDemoApp::SetupBuffer()
{
    mBuffer.countTextureNeedClear = true;

    // Count texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = mTransparencyTexture->GetWidth();
        createInfo.height                          = mTransparencyTexture->GetHeight();
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = grfx::FORMAT_R32_UINT;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.storage         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mBuffer.countTexture));
    }

    // Fragment texture
    {
        grfx::TextureCreateInfo createInfo = {};
        createInfo.imageType               = grfx::IMAGE_TYPE_2D;
        createInfo.width                   = mBuffer.countTexture->GetWidth();
        createInfo.height                  = mBuffer.countTexture->GetHeight() * BUFFER_BUCKET_SIZE_PER_PIXEL;
        createInfo.depth                   = 1;
        createInfo.imageFormat             = grfx::FORMAT_R32G32_UINT;
        createInfo.sampleCount             = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount           = 1;
        createInfo.arrayLayerCount         = 1;
        createInfo.usageFlags.bits.storage = true;
        createInfo.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState            = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mBuffer.fragmentTexture));
    }

    // Clear pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mBuffer.countTexture->GetWidth();
        createInfo.height                     = mBuffer.countTexture->GetHeight();
        createInfo.renderTargetCount          = 1;
        createInfo.pRenderTargetImages[0]     = mBuffer.countTexture->GetImage();
        createInfo.pDepthStencilImage         = nullptr;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.clearPass));
    }

    // Gather pass
    {
        grfx::DrawPassCreateInfo2 createInfo = {};
        createInfo.width                     = mBuffer.countTexture->GetWidth();
        createInfo.height                    = mBuffer.countTexture->GetHeight();
        createInfo.renderTargetCount         = 0;
        createInfo.pDepthStencilImage        = nullptr;
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.gatherPass));
    }

    ////////////////////////////////////////
    // Gather
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.gatherDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.gatherDescriptorSetLayout, &mBuffer.gatherDescriptorSet));

        std::array<grfx::WriteDescriptor, 4> writes = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_TEXTURE_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

        writes[2].binding    = CUSTOM_UAV_0_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageView = mBuffer.countTexture->GetStorageImageView();

        writes[3].binding    = CUSTOM_UAV_1_REGISTER;
        writes[3].arrayIndex = 0;
        writes[3].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[3].pImageView = mBuffer.fragmentTexture->GetStorageImageView();

        PPX_CHECKED_CALL(mBuffer.gatherDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.gatherDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.gatherPipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsGather.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsGather.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
        gpCreateInfo.VS                                = {VS, "vsmain"};
        gpCreateInfo.PS                                = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount     = 1;
        gpCreateInfo.vertexInputState.bindings[0]      = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                          = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                   = false;
        gpCreateInfo.depthWriteEnable                  = false;
        gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount     = 0;
        gpCreateInfo.pPipelineInterface                = mBuffer.gatherPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.gatherPipeline));

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
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.combineDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.combineDescriptorSetLayout, &mBuffer.combineDescriptorSet));

        std::array<grfx::WriteDescriptor, 3> writes = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_UAV_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageView = mBuffer.countTexture->GetStorageImageView();

        writes[2].binding    = CUSTOM_UAV_1_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageView = mBuffer.fragmentTexture->GetStorageImageView();

        PPX_CHECKED_CALL(mBuffer.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.combineDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.combinePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsCombine.ps", &PS));

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
        gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mBuffer.combinePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.combinePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::RecordBuffer()
{
    if (mBuffer.countTextureNeedClear) {
        mCommandBuffer->TransitionImageLayout(
            mBuffer.clearPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->BeginRenderPass(mBuffer.clearPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

        mCommandBuffer->SetScissors(mBuffer.clearPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.clearPass->GetViewport());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mBuffer.clearPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);

        mBuffer.countTextureNeedClear = false;
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.countTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(mBuffer.fragmentTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->BeginRenderPass(mBuffer.gatherPass, 0);

        mCommandBuffer->SetScissors(mBuffer.gatherPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.gatherPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.gatherPipelineInterface, 1, &mBuffer.gatherDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.gatherPipeline);
        mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
        mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
        mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(mBuffer.countTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.fragmentTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.countTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(mBuffer.fragmentTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(mTransparencyPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

        mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
        mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.combinePipelineInterface, 1, &mBuffer.combineDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.combinePipeline);
        mCommandBuffer->Draw(3);

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.countTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.fragmentTexture, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }
}
