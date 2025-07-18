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

#include "DynamicRendering.h"
#include "ppx/graphics_util.h"
#include "ppx/ppx.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void DynamicRenderingApp::Config(ApplicationSettings& settings)
{
    settings.appName                          = "dynamic_rendering";
    settings.enableImGui                      = true;
    settings.grfx.api                         = kApi;
    settings.grfx.swapchain.depthFormat       = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableImGuiDynamicRendering = true;
}

void DynamicRenderingApp::Setup()
{
    // Vertex buffer and geometry data
    {
        GeometryCreateInfo geometryCreateInfo = GeometryCreateInfo::Planar().AddColor();
        TriMeshOptions     triMeshOptions     = TriMeshOptions().Indices().VertexColors();
        TriMesh            sphereTriMesh      = TriMesh::CreateSphere(/* radius */ 1.0f, 16, 8, triMeshOptions);
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromTriMesh(GetGraphicsQueue(), &sphereTriMesh, &mSphereMesh));
    }

    // Pipeline
    {
        grfx::ShaderModulePtr vs;
        std::vector<char>     bytecode = LoadShader("basic/shaders", "VertexColorsPushConstants.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &vs));

        grfx::ShaderModulePtr ps;
        bytecode = LoadShader("basic/shaders", "VertexColorsPushConstants.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &ps));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = (sizeof(float4x4) + sizeof(uint32_t)) / sizeof(uint32_t);
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.dynamicRenderPass                  = true;
        gpCreateInfo.VS                                 = {vs.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {ps.Get(), "psmain"};
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

        const std::vector<grfx::VertexBinding> bindings = mSphereMesh->GetDerivedVertexBindings();
        gpCreateInfo.vertexInputState.bindingCount      = static_cast<uint32_t>(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++) {
            gpCreateInfo.vertexInputState.bindings[i] = bindings[i];
        }

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));

        GetDevice()->DestroyShaderModule(vs);
        GetDevice()->DestroyShaderModule(ps);
    }

    // Per frame data
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        mPerFrame.push_back(frame);
    }

    grfx::SwapchainPtr swapchain = GetSwapchain();

    for (uint32_t imageIndex = 0; imageIndex < swapchain->GetImageCount(); imageIndex++) {
        grfx::CommandBufferPtr preRecordedCmd;
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&preRecordedCmd));
        mPreRecordedCmds.push_back(preRecordedCmd);

        PPX_CHECKED_CALL(preRecordedCmd->Begin());
        {
            preRecordedCmd->TransitionImageLayout(swapchain->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
            grfx::RenderingInfo renderingInfo   = {};
            renderingInfo.flags.bits.suspending = true;
            renderingInfo.renderArea            = {0, 0, swapchain->GetWidth(), swapchain->GetHeight()};
            renderingInfo.renderTargetCount     = 1;
            // There will be an explicit clear inside a renderpass.
            renderingInfo.pRenderTargetViews[0] = swapchain->GetRenderTargetView(imageIndex, grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_LOAD);
            renderingInfo.pDepthStencilView     = swapchain->GetDepthStencilView(imageIndex);

            float4x4 P   = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
            float4x4 V   = glm::lookAt(float3(0, 0, 5), float3(0, 0, 0), float3(0, 1, 0));
            float4x4 M   = glm::translate(float3(0.0, 0.0, -2.0)) * glm::scale(float3(2.0, 2.0, 2.0));
            float4x4 mat = P * V * M;

            preRecordedCmd->BeginRendering(&renderingInfo);
            {
                grfx::RenderTargetClearValue rtvClearValue = {0.7f, 0.7f, 0.7f, 1.0f};
                grfx::DepthStencilClearValue dsvClearValue = {1.0f, 0xFF};
                preRecordedCmd->ClearRenderTarget(swapchain->GetColorImage(imageIndex), rtvClearValue);
                preRecordedCmd->ClearDepthStencil(swapchain->GetDepthImage(imageIndex), dsvClearValue, grfx::CLEAR_FLAG_DEPTH);
                preRecordedCmd->SetScissors(GetScissor());
                preRecordedCmd->SetViewports(GetViewport());
                preRecordedCmd->PushGraphicsConstants(mPipelineInterface, 16, &mat);
                preRecordedCmd->BindGraphicsPipeline(mPipeline);
                preRecordedCmd->BindIndexBuffer(mSphereMesh);
                preRecordedCmd->BindVertexBuffers(mSphereMesh);
                preRecordedCmd->DrawIndexed(mSphereMesh->GetIndexCount());
            }
            preRecordedCmd->EndRendering();
        }
        PPX_CHECKED_CALL(preRecordedCmd->End());
    }
}

void DynamicRenderingApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderingInfo renderingInfo   = {};
        renderingInfo.flags.bits.resuming   = true;
        renderingInfo.renderArea            = {0, 0, swapchain->GetWidth(), swapchain->GetHeight()};
        renderingInfo.renderTargetCount     = 1;
        renderingInfo.pRenderTargetViews[0] = swapchain->GetRenderTargetView(imageIndex, grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_LOAD);
        renderingInfo.pDepthStencilView     = swapchain->GetDepthStencilView(imageIndex);

        float    t   = GetElapsedSeconds();
        float4x4 P   = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V   = glm::lookAt(float3(0, 0, 5), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 M   = glm::rotate(t, float3(0, 1, 0)) * glm::translate(float3(0.0, 0.0, -3.0)) * glm::scale(float3(0.5, 0.5, 0.5));
        float4x4 mat = P * V * M;

        frame.cmd->BeginRendering(&renderingInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            frame.cmd->PushGraphicsConstants(mPipelineInterface, 16, &mat);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindIndexBuffer(mSphereMesh);
            frame.cmd->BindVertexBuffers(mSphereMesh);
            frame.cmd->DrawIndexed(mSphereMesh->GetIndexCount());
        }
        frame.cmd->EndRendering();

        if (GetSettings()->enableImGui) {
            // ImGui expects the rendering info without depth attachment.
            renderingInfo.pDepthStencilView = nullptr;

            frame.cmd->BeginRendering(&renderingInfo);
            {
                // Draw ImGui
                DrawDebugInfo();
                DrawImGui(frame.cmd);
            }
            frame.cmd->EndRendering();
        }

        frame.cmd->TransitionImageLayout(swapchain->GetColorImage(imageIndex), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    const grfx::CommandBuffer* commands[2] = {mPreRecordedCmds[imageIndex], frame.cmd};

    grfx::Semaphore* presentationReadySemaphore = swapchain->GetPresentationReadySemaphore(imageIndex);

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 2;
    submitInfo.ppCommandBuffers     = commands;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &presentationReadySemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &presentationReadySemaphore));
}
