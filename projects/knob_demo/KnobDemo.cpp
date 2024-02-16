// Copyright 2024 Google LLC
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

#include "KnobDemo.h"
#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void KnobDemoApp::InitKnobs()
{
    GetKnobManagerNew().InitKnob(&(mKnobs.pGeneralStringA), "general-string-a", "hello world");
    mKnobs.pGeneralStringA->SetDisplayType(KnobDisplayType::PLAIN);

    GetKnobManagerNew().InitKnob(&(mKnobs.pGeneralBoolA), "general-bool-a", true);
    mKnobs.pGeneralBoolA->SetFlagDescription("Must be true");
    mKnobs.pGeneralBoolA->SetValidator([](bool value) {
        return value;
    });
    mKnobs.pGeneralBoolA->SetDisplayType(KnobDisplayType::CHECKBOX);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeIntA), "range-int-a", 0);
    mKnobs.pRangeIntA->SetFlagDescription("Minimum of range-int-c");
    mKnobs.pRangeIntA->SetMin(0);
    mKnobs.pRangeIntA->SetMax(5);
    mKnobs.pRangeIntA->SetDisplayType(KnobDisplayType::PLAIN);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeIntB), "range-int-b", 10);
    mKnobs.pRangeIntB->SetFlagDescription("Maximum of range-int-c");
    mKnobs.pRangeIntB->SetMin(5);
    mKnobs.pRangeIntB->SetMax(10);
    mKnobs.pRangeIntB->SetDisplayType(KnobDisplayType::SLOW_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeIntC), "range-int-c", 5);
    mKnobs.pRangeIntC->SetMin(0);
    mKnobs.pRangeIntC->SetMax(10);
    mKnobs.pRangeIntC->SetDisplayType(KnobDisplayType::FAST_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeInt3A), "range-int-3-a", std::vector<int>{0, 10, 20});
    mKnobs.pRangeInt3A->SetMin(0, 0);
    mKnobs.pRangeInt3A->SetMax(0, 9);
    mKnobs.pRangeInt3A->SetMin(1, 10);
    mKnobs.pRangeInt3A->SetMax(1, 19);
    mKnobs.pRangeInt3A->SetMin(2, 20);
    mKnobs.pRangeInt3A->SetMax(2, 29);
    mKnobs.pRangeInt3A->SetDisplaySuffixes(std::vector<std::string>{"X", "Y", "Z"});
    mKnobs.pRangeInt3A->SetFlagDescription("No effect");
    mKnobs.pRangeInt3A->SetDisplayType(KnobDisplayType::FAST_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeFloatA), "range-float-a", 0.5f);
    mKnobs.pRangeFloatA->SetFlagDescription("Slowly set range-float-3-a A");
    mKnobs.pRangeFloatA->SetMin(0.0f);
    mKnobs.pRangeFloatA->SetMax(1.0f);
    mKnobs.pRangeFloatA->SetDisplayType(KnobDisplayType::SLOW_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeFloatB), "range-float-b", 0.5f);
    mKnobs.pRangeFloatB->SetFlagDescription("Quickly set range-float-3-a B");
    mKnobs.pRangeFloatB->SetMin(0.0f);
    mKnobs.pRangeFloatB->SetMax(1.0f);
    mKnobs.pRangeFloatB->SetDisplayType(KnobDisplayType::FAST_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pRangeFloat3A), "range-float-3-a", std::vector<float>{0.5f, 0.5f, 0.5f});
    mKnobs.pRangeFloat3A->SetAllMins(0.0f);
    mKnobs.pRangeFloat3A->SetAllMaxes(1.0f);
    mKnobs.pRangeFloat3A->SetDisplaySuffixes(std::vector<std::string>{"A", "B", "None"});
    mKnobs.pRangeFloat3A->SetDisplayType(KnobDisplayType::FAST_SLIDER);

    GetKnobManagerNew().InitKnob(&(mKnobs.pOptionIntA), "option-bool-a", 0, kOptionBoolA);
    mKnobs.pOptionIntA->SetDisplayName("option-bool-a DISPLAY NAME");
    mKnobs.pOptionIntA->SetDisplayType(KnobDisplayType::PLAIN);

    GetKnobManagerNew().InitKnob(&(mKnobs.pGeneralBoolB), "general-bool-b", false);
    mKnobs.pGeneralBoolB->SetFlagDescription("Check to only show even numbers in option-int-a");
    mKnobs.pGeneralBoolB->SetDisplayType(KnobDisplayType::CHECKBOX);

    GetKnobManagerNew().InitKnob(&(mKnobs.pOptionIntA), "option-int-a", 0, kOptionIntA);
    mKnobs.pOptionIntA->SetFlagDescription("Filtered by general-knob-b");
    mKnobs.pOptionIntA->SetDisplayType(KnobDisplayType::DROPDOWN);
}

void KnobDemoApp::Config(ApplicationSettings& settings)
{
    settings.appName           = "knob_demo";
    settings.enableImGui       = true;
    settings.grfx.api          = kApi;
    settings.grfx.enableDebug  = false;
    settings.window.resizable  = true;
    settings.useKnobManagerNew = true;
}

void KnobDemoApp::Setup()
{
    // Pipeline
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "StaticVertexColors.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "StaticVertexColors.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
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
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
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

    // Buffer and geometry data
    {
        // clang-format off
        std::vector<float> vertexData = {
            // position           // vertex colors
             0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
             0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
        };
        // clang-format on
        uint32_t dataSize = SizeInBytesU32(vertexData);

        grfx::BufferCreateInfo bufferCreateInfo       = {};
        bufferCreateInfo.size                         = dataSize;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;
        bufferCreateInfo.initialState                 = grfx::RESOURCE_STATE_VERTEX_BUFFER;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mVertexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, vertexData.data(), dataSize);
        mVertexBuffer->UnmapMemory();
    }
}

void KnobDemoApp::ProcessKnobs()
{
    if (mKnobs.pGeneralBoolB->DigestUpdate()) {
        PPX_LOG_INFO("pGeneralBoolB updated");
        std::vector<bool> newMask;
        if (mKnobs.pGeneralBoolB->GetValue()) {
            // Show only even numbers in pOptionIntA when pGeneralBoolB is true
            for (auto& element : kOptionIntA) {
                newMask.push_back(element.value % 2 == 0);
            }
            mKnobs.pOptionIntA->SetMask(newMask);
        }
        else {
            newMask = std::vector<bool>(kOptionIntA.size(), true);
        }
        mKnobs.pOptionIntA->SetMask(newMask);
    }

    if (mKnobs.pRangeIntA->DigestUpdate()) {
        PPX_LOG_INFO("pRangeIntA updated");
        mKnobs.pRangeIntC->SetMin(mKnobs.pRangeIntA->GetValue());
    }
    if (mKnobs.pRangeIntB->DigestUpdate()) {
        PPX_LOG_INFO("pRangeIntB updated");
        mKnobs.pRangeIntC->SetMax(mKnobs.pRangeIntB->GetValue());
    }

    if (mKnobs.pRangeFloatA->DigestUpdate()) {
        PPX_LOG_INFO("pRangeFloatA updated");
        mKnobs.pRangeFloat3A->SetValue(0, mKnobs.pRangeFloatA->GetValue());
    }
    if (mKnobs.pRangeFloatB->DigestUpdate()) {
        PPX_LOG_INFO("pRangeFloatB updated");
        mKnobs.pRangeFloat3A->SetValue(1, mKnobs.pRangeFloatB->GetValue());
    }
}

void KnobDemoApp::Render()
{
    ProcessKnobs();

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
        beginInfo.RTVClearValues[0]         = {{1, 0, 0, 1}};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->Draw(3, 1, 0, 0);

            // Draw ImGui
            DrawDebugInfo();
#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
            DrawProfilerGrfxApiFunctions();
#endif // defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)

            // GUI
            ImGui::Begin("Debug Info");
            ImGui::Separator();
            ImGui::Text("Knobs");
            GetKnobManagerNew().DrawAllKnobs(true);
            ImGui::End();
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
