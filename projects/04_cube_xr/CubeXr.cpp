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

#include "CubeXr.h"
#include "ppx/math_config.h"
#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void CubeXrApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "04_cube";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
    settings.grfx.pacedFrameRate        = 0;
#if defined(PPX_BUILD_XR)
    settings.xr.enable                  = true;
    settings.xr.enableDebugCapture      = false;
#else
    settings.xr.enable = false;
#endif
}

void CubeXrApp::Setup()
{
    PerView pv;
    mPerView.push_back(pv);
    mPerView.push_back(pv);

    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mPerView[0].mUniformBuffer));
        if (IsXrEnabled()) {
            PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mPerView[1].mUniformBuffer));
        }
    }

    // Descriptor
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = IsXrEnabled() ? 2 : 1;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));

        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mPerView[0].mDescriptorSet));
        if (IsXrEnabled()) {
            PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mPerView[1].mDescriptorSet));
        }

        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mPerView[0].mUniformBuffer;
        PPX_CHECKED_CALL(mPerView[0].mDescriptorSet->UpdateDescriptors(1, &write));
        if (IsXrEnabled()) {
            write.pBuffer = mPerView[1].mUniformBuffer;
            PPX_CHECKED_CALL(mPerView[1].mDescriptorSet->UpdateDescriptors(1, &write));
        }
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

        if (IsXrEnabled()) {
            PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.uiCmd));
            PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.uiRenderCompleteFence));
        }

        mPerFrame.push_back(frame);
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

    // Viewport and scissor rect
    mViewport    = {0, 0, float(GetWindowWidth()), float(GetWindowHeight()), 0, 1};
    mScissorRect = {0, 0, GetWindowWidth(), GetWindowHeight()};
}

void CubeXrApp::DispatchRender()
{
    bool useOld = false;
    if (useOld) {
        // This is default behaviour
        if (IsXrEnabled()) {
            mViewIndex = 0;
            Render();
            mViewIndex = 1;
            Render();
        }
        else {
            Render();
        }
    }
    else {
        RenderSingleCommandBuffer();
    }
}

void CubeXrApp::Render()
{
    PerFrame& frame            = mPerFrame[0];
    uint32_t  imageIndex       = UINT32_MAX;

    // Render UI into a different composition layer.
    if (IsXrEnabled() && (mViewIndex == 0) && GetSettings()->enableImGui) {
        grfx::SwapchainPtr uiSwapchain = GetUISwapchain();
        PPX_CHECKED_CALL(uiSwapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
        PPX_CHECKED_CALL(frame.uiRenderCompleteFence->WaitAndReset());

        PPX_CHECKED_CALL(frame.uiCmd->Begin());
        {
            grfx::RenderPassPtr renderPass = uiSwapchain->GetRenderPass(imageIndex);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();
            beginInfo.RTVClearCount             = 1;
            beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
            beginInfo.DSVClearValue             = {1.0f, 0xFF};

            frame.uiCmd->BeginRenderPass(&beginInfo);
            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.uiCmd);
            frame.uiCmd->EndRenderPass();
        }
        PPX_CHECKED_CALL(frame.uiCmd->End());

        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.uiCmd;
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;

        submitInfo.pFence = frame.uiRenderCompleteFence;
        uiSwapchain->Wait(imageIndex);

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
        uiSwapchain->Present(imageIndex, 0, nullptr);
    }

    grfx::SwapchainPtr swapchain = GetSwapchain(mViewIndex);

    if (swapchain->ShouldSkipExternalSynchronization()) {
        // No need to
        // - Signal imageAcquiredSemaphore & imageAcquiredFence.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
    }
    else {
        // Wait semaphore is ignored for XR.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

        // Wait for and reset image acquired fence.
        PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    }

    // Wait for and reset render complete fence.
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update uniform buffer.
    {
        float    t = GetElapsedSeconds();
        float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V = glm::lookAt(float3(0, 0, 0), float3(0, 0, 1), float3(0, 1, 0));

#if defined(PPX_BUILD_XR)
        if (IsXrEnabled()) {
            P = GetXrComponent().GetProjectionMatrixForViewAndSetFrustumPlanes(mViewIndex, 0.001f, 10000.0f);
            V = GetXrComponent().GetViewMatrixForView(mViewIndex);
        }
#endif
        float4x4 M   = glm::translate(float3(0, 0, -3)) * glm::rotate(t, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
        float4x4 mat = P * V * M;

        void* pData = nullptr;
        PPX_CHECKED_CALL(mPerView[0].mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &mat, sizeof(mat));
        mPerView[0].mUniformBuffer->UnmapMemory();
    }

    // Build command buffer.
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

        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        }

        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(1, &mScissorRect);
            frame.cmd->SetViewports(1, &mViewport);
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPerView[0].mDescriptorSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->Draw(36, 1, 0, 0);

            if (!IsXrEnabled()) {
                // Draw ImGui
                DrawDebugInfo();
                DrawImGui(frame.cmd);
            }
        }
        frame.cmd->EndRenderPass();
        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
        }
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &frame.cmd;
    // No need to use semaphore when XR is enabled.
    if (IsXrEnabled()) {
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;
    }
    else {
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    }
    submitInfo.pFence = frame.renderCompleteFence;
    swapchain->Wait(imageIndex);
    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    // No need to present when XR is enabled.
    uint32_t         semaphoreCount = 0;
    grfx::Semaphore* pPresentSemaphores;
    if (IsXrEnabled()) {
        semaphoreCount = 1;
    }
    PPX_CHECKED_CALL(swapchain->Present(imageIndex, semaphoreCount, IsXrEnabled() ? nullptr : &frame.renderCompleteSemaphore));

#if defined(PPX_BUILD_XR)
    if (GetSettings()->xr.enableDebugCapture && (mViewIndex == 1)) {
        // We could use semaphore to sync to have better performance,
        // but this requires modifying the submission code.
        // For debug capture we don't care about the performance,
        // so use existing fence to sync for simplicity.
        grfx::SwapchainPtr debugSwapchain = GetDebugCaptureSwapchain();
        PPX_CHECKED_CALL(debugSwapchain->AcquireNextImage(UINT64_MAX, nullptr, frame.imageAcquiredFence, &imageIndex));
        frame.imageAcquiredFence->WaitAndReset();
        PPX_CHECKED_CALL(debugSwapchain->Present(imageIndex, 0, nullptr));
    }
#endif
}

void CubeXrApp::RenderSingleCommandBuffer()
{
    PerFrame& frame = mPerFrame[0];

    //=========================================================================================
    // PrepareFrame
    // Does image acquisition
    //=========================================================================================
    std::vector<uint32_t> viewImageIndices(mPerView.size());
    uint32_t              uiImageIndex = UINT32_MAX;

    if (!IsXrEnabled()) {
        PPX_CHECKED_CALL(GetSwapchain()->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &viewImageIndices[0]));

        // Wait for and reset image acquired fence.
        // Why?
        PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    }
    else {
        for (uint32_t viewIndex = 0; viewIndex < viewImageIndices.size(); viewIndex++) {
            grfx::SwapchainPtr swapchain = GetSwapchain(viewIndex);
            PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &viewImageIndices[viewIndex]));
        }

        if (GetSettings()->enableImGui) {
            grfx::SwapchainPtr uiSwapchain = GetUISwapchain();
            PPX_CHECKED_CALL(uiSwapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &uiImageIndex));
        }
    }

    //=========================================================================================
    // RecordFrame
    // Records command buffers
    //=========================================================================================

    // Wait for and reset render complete fence before recording the command buffer.
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update uniform buffers.
    {
        float    t = GetElapsedSeconds();
        float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V = glm::lookAt(float3(0, 0, 0), float3(0, 0, 1), float3(0, 1, 0));

        uint32_t viewIndex = 0;
#if defined(PPX_BUILD_XR)
        if (IsXrEnabled()) {
            P = GetXrComponent().GetProjectionMatrixForViewAndSetFrustumPlanes(viewIndex, 0.001f, 10000.0f);
            V = GetXrComponent().GetViewMatrixForView(viewIndex);
        }
#endif
        float4x4 M   = glm::translate(float3(0, 0, -3)) * glm::rotate(t, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
        float4x4 mat = P * V * M;

        void* pData = nullptr;
        PPX_CHECKED_CALL(mPerView[viewIndex].mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &mat, sizeof(mat));
        mPerView[viewIndex].mUniformBuffer->UnmapMemory();

        if (IsXrEnabled()) {
            viewIndex    = 1;
            P            = GetXrComponent().GetProjectionMatrixForViewAndSetFrustumPlanes(viewIndex, 0.001f, 10000.0f);
            V            = GetXrComponent().GetViewMatrixForView(viewIndex);
            float4x4 M   = glm::translate(float3(0, 0, -3)) * glm::rotate(t, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
            float4x4 mat = P * V * M;

            void* pData = nullptr;
            PPX_CHECKED_CALL(mPerView[viewIndex].mUniformBuffer->MapMemory(0, &pData));
            memcpy(pData, &mat, sizeof(mat));
            mPerView[viewIndex].mUniformBuffer->UnmapMemory();
        }
    }

    // Build command buffer.
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        if (IsXrEnabled() && GetSettings()->enableImGui) {
            grfx::RenderPassPtr renderPass = GetUISwapchain()->GetRenderPass(uiImageIndex);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");
            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();
            beginInfo.RTVClearCount             = 1;
            beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
            beginInfo.DSVClearValue             = {1.0f, 0xFF};

            frame.cmd->BeginRenderPass(&beginInfo);
            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.cmd);
            frame.cmd->EndRenderPass();
        }

        for (size_t view = 0; view < mPerView.size(); view++) {
            grfx::SwapchainPtr  swapchain  = GetSwapchain(view);
            grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(viewImageIndices[view]);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();
            beginInfo.RTVClearCount             = 1;
            beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
            beginInfo.DSVClearValue             = {1.0f, 0xFF};

            if (!IsXrEnabled()) {
                frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
            }

            frame.cmd->BeginRenderPass(&beginInfo);
            {
                frame.cmd->SetScissors(1, &mScissorRect);
                frame.cmd->SetViewports(1, &mViewport);
                frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPerView[view].mDescriptorSet);
                frame.cmd->BindGraphicsPipeline(mPipeline);
                frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
                frame.cmd->Draw(36, 1, 0, 0);

                if (!IsXrEnabled()) {
                    // Draw ImGui
                    DrawDebugInfo();
                    DrawImGui(frame.cmd);
                }
            }
            frame.cmd->EndRenderPass();
            if (!IsXrEnabled()) {
                frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
            }
        }
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    //=========================================================================================
    // SubmitFrame
    // Submit GPU work
    //=========================================================================================
    for (size_t view = 0; view < mPerView.size(); view++) {
        grfx::SwapchainPtr swapchain = GetSwapchain(view);
        swapchain->Wait(viewImageIndices[view]);
    }

    if (IsXrEnabled() && GetSettings()->enableImGui) {
        GetUISwapchain()->Wait(uiImageIndex);
    }

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &frame.cmd;
    // No need to use semaphore when XR is enabled.
    if (IsXrEnabled()) {
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;
    }
    else {
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    }
    submitInfo.pFence = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    //=========================================================================================
    // PresentFrame
    // Present frame
    //=========================================================================================
    if (!IsXrEnabled()) {
        PPX_CHECKED_CALL(GetSwapchain()->Present(viewImageIndices[0], 1, &frame.renderCompleteSemaphore));
    }
    else {
        PPX_CHECKED_CALL(GetSwapchain(0)->Present(viewImageIndices[0], 0, nullptr));
        PPX_CHECKED_CALL(GetSwapchain(1)->Present(viewImageIndices[1], 0, nullptr));
        if (GetSettings()->enableImGui) {
            PPX_CHECKED_CALL(GetUISwapchain()->Present(uiImageIndex, 0, nullptr));
        }
    }

#if defined(PPX_BUILD_XR)
    if (GetSettings()->xr.enableDebugCapture && (mViewIndex == 1)) {
        // We could use semaphore to sync to have better performance,
        // but this requires modifying the submission code.
        // For debug capture we don't care about the performance,
        // so use existing fence to sync for simplicity.
        uint32_t           imageIndex     = -1;
        grfx::SwapchainPtr debugSwapchain = GetDebugCaptureSwapchain();
        PPX_CHECKED_CALL(debugSwapchain->AcquireNextImage(UINT64_MAX, nullptr, frame.imageAcquiredFence, &imageIndex));
        frame.imageAcquiredFence->WaitAndReset();
        PPX_CHECKED_CALL(debugSwapchain->Present(imageIndex, 0, nullptr));
    }
#endif
}
