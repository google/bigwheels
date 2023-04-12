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

void OITDemoApp::SetupUnsortedOver()
{
    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mUnsortedOver.descriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mUnsortedOver.descriptorSetLayout, &mUnsortedOver.descriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = SHADER_GLOBALS_REGISTER;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mShaderGlobalsBuffer;
        PPX_CHECKED_CALL(mUnsortedOver.descriptorSet->UpdateDescriptors(1, &write));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mUnsortedOver.descriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mUnsortedOver.pipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "UnsortedOver.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "UnsortedOver.ps", &PS));

        grfx::GraphicsPipelineCreateInfo gpCreateInfo                        = {};
        gpCreateInfo.VS                                                      = {VS, "vsmain"};
        gpCreateInfo.PS                                                      = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount                           = 1;
        gpCreateInfo.vertexInputState.bindings[0]                            = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.inputAssemblyState.topology                             = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.rasterState.polygonMode                                 = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.rasterState.frontFace                                   = grfx::FRONT_FACE_CCW;
        gpCreateInfo.rasterState.rasterizationSamples                        = grfx::SAMPLE_COUNT_1;
        gpCreateInfo.depthStencilState.depthTestEnable                       = true;
        gpCreateInfo.depthStencilState.depthWriteEnable                      = false;
        gpCreateInfo.colorBlendState.blendAttachmentCount                    = 1;
        gpCreateInfo.colorBlendState.blendAttachments[0].blendEnable         = true;
        gpCreateInfo.colorBlendState.blendAttachments[0].srcColorBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].dstColorBlendFactor = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        gpCreateInfo.colorBlendState.blendAttachments[0].colorBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[0].srcAlphaBlendFactor = grfx::BLEND_FACTOR_ONE;
        gpCreateInfo.colorBlendState.blendAttachments[0].dstAlphaBlendFactor = grfx::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        gpCreateInfo.colorBlendState.blendAttachments[0].alphaBlendOp        = grfx::BLEND_OP_ADD;
        gpCreateInfo.colorBlendState.blendAttachments[0].colorWriteMask      = grfx::ColorComponentFlags::RGBA();
        gpCreateInfo.outputState.renderTargetCount                           = 1;
        gpCreateInfo.outputState.renderTargetFormats[0]                      = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat                          = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                                      = mUnsortedOver.pipelineInterface;

        gpCreateInfo.rasterState.cullMode = grfx::CULL_MODE_NONE;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mUnsortedOver.meshAllFacesPipeline));

        gpCreateInfo.rasterState.cullMode = grfx::CULL_MODE_FRONT;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mUnsortedOver.meshBackFacesPipeline));

        gpCreateInfo.rasterState.cullMode = grfx::CULL_MODE_BACK;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mUnsortedOver.meshFrontFacesPipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::RecordUnsortedOver()
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

    mCommandBuffer->BindGraphicsDescriptorSets(mUnsortedOver.pipelineInterface, 1, &mUnsortedOver.descriptorSet);
    mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
    mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
    switch (mGuiParameters.unsortedOver.faceMode) {
        case FACE_MODE_ALL: {
            mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshAllFacesPipeline);
            mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
            break;
        }
        case FACE_MODE_ALL_BACK_THEN_FRONT: {
            mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshBackFacesPipeline);
            mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
            mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshFrontFacesPipeline);
            mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
            break;
        }
        case FACE_MODE_BACK_ONLY: {
            mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshBackFacesPipeline);
            mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
            break;
        }
        case FACE_MODE_FRONT_ONLY: {
            mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshFrontFacesPipeline);
            mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
            break;
        }
        default: {
            PPX_ASSERT_MSG(false, "unknown face mode");
            break;
        }
    }

    mCommandBuffer->EndRenderPass();
    mCommandBuffer->TransitionImageLayout(
        mTransparencyPass,
        grfx::RESOURCE_STATE_RENDER_TARGET,
        grfx::RESOURCE_STATE_SHADER_RESOURCE,
        grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
        grfx::RESOURCE_STATE_SHADER_RESOURCE);
}
