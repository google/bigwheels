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

#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::ShaderModulePtr        mVS;
    grfx::ShaderModulePtr        mPS;
    grfx::PipelineInterfacePtr   mPipelineInterface;
    grfx::GraphicsPipelinePtr    mPipeline;
    grfx::BufferPtr              mVertexBuffer;
    grfx::BufferPtr              mIndexBuffer;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    grfx::DescriptorSetPtr       mDescriptorSet;
    grfx::BufferPtr              mUniformBuffer;
    grfx::Viewport               mViewport;
    grfx::Rect                   mScissorRect;
    grfx::VertexBinding          mVertexBinding;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "07_draw_indexed";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

void ProjApp::Setup()
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
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
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
            // position          // vertex colors
             1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  0  -Z side
             1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  1
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  2
            -1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  3

            -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  4  +Z side
            -1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  5
             1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  6
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  7

            -1.0f, 1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  //  8  -X side
            -1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  //  9
            -1.0f,-1.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 10
            -1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 11

             1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f,  // 12  +X side
             1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,  // 13
             1.0f,-1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // 14
             1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // 15

            -1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,  // 16  -Y side
            -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // 17
             1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // 18
             1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,  // 19

            -1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // 20  +Y side
            -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,  // 21
             1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,  // 22
             1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // 23
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

    // Index buffer
    {
        // clang-format off
        std::vector<uint16_t> indexData = {
            0,  1,  2, // -Z side
            0,  2,  3,

            4,  5,  6, // +Z side
            4,  6,  7,

            8,  9, 10, // -X side
            8, 10, 11,

           12, 13, 14, // +X side
           12, 14, 15,

           16, 17, 18, // -X side
           16, 18, 19,

           20, 21, 22, // +X side
           20, 22, 23,
        };
        // clang-format on
        uint32_t dataSize = ppx::SizeInBytesU32(indexData);

        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = dataSize;
        bufferCreateInfo.usageFlags.bits.indexBuffer = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mIndexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mIndexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, indexData.data(), dataSize);
        mIndexBuffer->UnmapMemory();
    }

    // Viewport and scissor rect
    mViewport    = {0, 0, float(GetWindowWidth()), float(GetWindowHeight()), 0, 1};
    mScissorRect = {0, 0, GetWindowWidth(), GetWindowHeight()};
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update uniform buffer
    {
        float    t   = GetElapsedSeconds();
        float4x4 P   = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V   = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 M   = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
        float4x4 mat = P * V * M;

        void* pData = nullptr;
        PPX_CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &mat, sizeof(mat));
        mUniformBuffer->UnmapMemory();
    }

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
            frame.cmd->SetScissors(1, &mScissorRect);
            frame.cmd->SetViewports(1, &mViewport);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindIndexBuffer(mIndexBuffer, grfx::INDEX_TYPE_UINT16);
            frame.cmd->DrawIndexed(36, 1, 0, 0, 0);

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

SETUP_APPLICATION(ProjApp)