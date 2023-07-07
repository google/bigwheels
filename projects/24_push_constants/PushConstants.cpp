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

#include "PushConstants.h"

#include "ppx/graphics_util.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void PushConstantsApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "24_push_constants";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
}

void PushConstantsApp::Setup()
{
    // Texture image, view, and sampler
    {
        // Image 0
        {
            const uint32_t textureIndex = 0;

            grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/textures/box_panel.jpg"), &mImages[textureIndex], options, true));

            grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSampledImageViews[textureIndex]));
        }
        // Image 1
        {
            const uint32_t textureIndex = 1;

            grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/textures/chinatown.jpg"), &mImages[textureIndex], options, true));

            grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSampledImageViews[textureIndex]));
        }
        // Image 2
        {
            const uint32_t textureIndex = 2;

            grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/textures/hanging_lights.jpg"), &mImages[textureIndex], options, true));

            grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSampledImageViews[textureIndex]));
        }

        // Sampler
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
    }

    // Descriptor
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 8;
        poolCreateInfo.sampledImage                   = 8;
        poolCreateInfo.sampler                        = 8;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));

        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(4, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));

        // Allocate descriptor set and write descriptors
        {
            PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

            grfx::WriteDescriptor write = {};
            // Texture[0]
            write            = {};
            write.binding    = 1;
            write.arrayIndex = 0;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mSampledImageViews[0];
            PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
            // Texture[1]
            write            = {};
            write.binding    = 1;
            write.arrayIndex = 1;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mSampledImageViews[1];
            PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
            // Texture[2]
            write            = {};
            write.binding    = 1;
            write.arrayIndex = 2;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mSampledImageViews[2];
            PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));

            // Sampler
            write          = {};
            write.binding  = 4;
            write.type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
            write.pSampler = mSampler;
            PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
        }
    }

    // Pipeline
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "PushConstantsTexture.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "PushConstantsTexture.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
        piCreateInfo.pushConstants.count               = (sizeof(float4x4) + sizeof(uint32_t)) / sizeof(uint32_t);
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        mVertexBinding.AppendAttribute({"TEXCOORD", 1, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mVertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
    }

    // Per frame data
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        mPerFrame.push_back(frame);
    }

    // Vertex buffer and geometry data
    {
        // clang-format off
        std::vector<float> vertexData = {
            -1.0f,-1.0f,-1.0f,   1.0f, 1.0f,  // -Z side
             1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
             1.0f,-1.0f,-1.0f,   0.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,   1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
             1.0f, 1.0f,-1.0f,   0.0f, 0.0f,

            -1.0f, 1.0f, 1.0f,   0.0f, 0.0f,  // +Z side
            -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
             1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
            -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,   1.0f, 0.0f,

            -1.0f,-1.0f,-1.0f,   0.0f, 1.0f,  // -X side
            -1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
            -1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
            -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
            -1.0f,-1.0f,-1.0f,   0.0f, 1.0f,

             1.0f, 1.0f,-1.0f,   0.0f, 1.0f,  // +X side
             1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 0.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 0.0f,
             1.0f,-1.0f,-1.0f,   0.0f, 0.0f,
             1.0f, 1.0f,-1.0f,   0.0f, 1.0f,

            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f,  // -Y side
             1.0f,-1.0f,-1.0f,   1.0f, 1.0f,
             1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
             1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,   0.0f, 0.0f,

            -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,  // +Y side
            -1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
             1.0f, 1.0f,-1.0f,   1.0f, 1.0f,
        };
        // clang-format on
        uint32_t dataSize = ppx::SizeInBytesU32(vertexData);

        grfx::BufferCreateInfo bufferCreateInfo       = {};
        bufferCreateInfo.size                         = dataSize;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mVertexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, vertexData.data(), dataSize);
        mVertexBuffer->UnmapMemory();
    }
}

void PushConstantsApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

            // Bind descriptor set
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);

            // Get elapsed seconds
            float t = GetElapsedSeconds();

            // Calculate perspective and view matrices
            float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
            float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));

            // Draw center cube
            {
                // Calculate MVP
                float4x4 T   = glm::translate(float3(0, 0, -10 * (1 + sin(t / 2))));
                float4x4 R   = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 4, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
                float4x4 M   = T * R;
                float4x4 mat = P * V * M;
                // Set MVP push constants
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 16, &mat);
                // Set texture index push constant at dstOffset=16
                uint32_t textureIndex = 0;
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 1, &textureIndex, 16);
            }
            frame.cmd->Draw(36, 1, 0, 0);

            // Draw left cube
            {
                // Calculate MVP
                float4x4 T   = glm::translate(float3(-4, 0, -10 * (1 + sin(t / 2))));
                float4x4 R   = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 2, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
                float4x4 M   = T * R;
                float4x4 mat = P * V * M;
                // Set MVP push constants
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 16, &mat);
                // Set texture index push constant at offset=16
                uint32_t textureIndex = 1;
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 1, &textureIndex, 16);
            }
            frame.cmd->Draw(36, 1, 0, 0);

            // Draw right cube
            {
                // Calculate MVP
                float4x4 T   = glm::translate(float3(4, 0, -10 * (1 + sin(t / 2))));
                float4x4 R   = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
                float4x4 M   = T * R;
                float4x4 mat = P * V * M;
                // Set MVP push constants
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 16, &mat);
                // Set texture index push constant at offset=16
                uint32_t textureIndex = 2;
                frame.cmd->SetGraphicsPushConstants(mPipelineInterface, 1, &textureIndex, 16);
            }
            frame.cmd->Draw(36, 1, 0, 0);

            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}