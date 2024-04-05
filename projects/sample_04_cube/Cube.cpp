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

#include "Cube.h"
#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void CubeApp::Config(ApplicationSettings& settings)
{
    settings.appName                    = "sample_04_cube";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
}

void CubeApp::Setup()
{
    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mUniformBuffer));
    }

    // Descriptor
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 1;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));

        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mUniformBuffer;
        PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
    }

    // Pipeline
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "VertexColors.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "VertexColors.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        mVertexBinding.AppendAttribute({"COLOR", 1, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

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

    // Render pass
    {
        for (uint32_t i = 0; i < GetSwapchain()->GetImageCount(); ++i) {
            auto pRenderTargetImage = GetSwapchain()->GetColorImage(i);
            auto pDepthStencilimage = GetSwapchain()->GetDepthImage(i);

            // Explicitly use OP_LOAD for all attachments
            grfx::RenderPassCreateInfo3 createInfo = {};
            createInfo.width                       = pRenderTargetImage->GetWidth();
            createInfo.height                      = pRenderTargetImage->GetHeight();
            createInfo.renderTargetCount           = 1;
            createInfo.pRenderTargetImages[0]      = pRenderTargetImage;
            createInfo.renderTargetLoadOps[0]      = grfx::ATTACHMENT_LOAD_OP_LOAD;
            createInfo.pDepthStencilImage          = pDepthStencilimage;
            createInfo.depthLoadOp                 = grfx::ATTACHMENT_LOAD_OP_LOAD;
            createInfo.stencilLoadOp               = grfx::ATTACHMENT_LOAD_OP_LOAD;

            grfx::RenderPassPtr renderPass = nullptr;
            PPX_CHECKED_CALL(GetDevice()->CreateRenderPass(&createInfo, &renderPass));

            mRenderPasses.push_back(renderPass);
        }
    }

    // Vertex buffer and geometry data
    {
        // clang-format off
        std::vector<float> vertexData = {  
            // position          // vertex colors
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  // -Z side
             1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
             1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
            -1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
             1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,

            -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  // +Z side
            -1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
            -1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
             1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

            -1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  // -X side
            -1.0f,-1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,   0.0f, 0.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,

             1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // +X side
             1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
             1.0f,-1.0f,-1.0f,   1.0f, 1.0f, 0.0f,
             1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,

            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // -Y side
             1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,
             1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,

            -1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // +Y side
            -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
             1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,
        };
        // clang-format on
        uint32_t dataSize = SizeInBytesU32(vertexData);

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

void CubeApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Update uniform buffer
    {
        float    t   = GetElapsedSeconds();
        float4x4 P   = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V   = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 M   = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
        float4x4 mat = P * V * M;

        void* pData = nullptr;
        PPX_CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &mat, sizeof(mat));
        mUniformBuffer->UnmapMemory();
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = mRenderPasses[imageIndex];
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();

        // Clear RTV to greyish blue
        grfx::RenderTargetClearValue rtvClearValue = {0.23f, 0.23f, 0.33f, 0};
        grfx::DepthStencilClearValue dsvClearValue = {1.0f, 0xFF};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->ClearRenderTarget(renderPass->GetRenderTargetImage(0), rtvClearValue);
            frame.cmd->ClearDepthStencil(renderPass->GetDepthStencilImage(), dsvClearValue, grfx::CLEAR_FLAG_DEPTH);
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
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
