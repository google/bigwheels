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

#include "ppx/ppx.h"
#include "ppx/camera.h"
using namespace ppx;

#include <thread>

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
        grfx::CommandBufferPtr drawTextCmd;
        grfx::CommandBufferPtr drawImGuiCmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     presentReadySemaphore; // Binary semaphore

        grfx::SemaphorePtr timelineSemaphore;
        uint64_t           timelineValue = 0;
    };
    std::vector<PerFrame> mPerFrame;
    grfx::TextureFontPtr  mRoboto;
    grfx::TextDrawPtr     mDynamicText;
    PerspCamera           mCamera;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "timeline semaphore";
    settings.enableImGui      = true;
    settings.grfx.api         = kApi;
    settings.grfx.enableDebug = false;
}

void ProjApp::Setup()
{
    mCamera = PerspCamera(GetWindowWidth(), GetWindowHeight());

    // Per frame data
    {
        PerFrame frame = {};

        // Command buffers
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.drawTextCmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.drawImGuiCmd));

        // Defaults to binary semaphore
        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.presentReadySemaphore));

        // Change create info for timeline semaphore
        semaCreateInfo               = {};
        semaCreateInfo.semaphoreType = grfx::SEMAPHORE_TYPE_TIMELINE;
        semaCreateInfo.initialValue  = 0;
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.timelineSemaphore));

        mPerFrame.push_back(frame);
    }

    // Texture font
    {
        ppx::Font font;
        PPX_CHECKED_CALL(ppx::Font::CreateFromFile(GetAssetPath("basic/fonts/Roboto/Roboto-Regular.ttf"), &font));

        grfx::TextureFontCreateInfo createInfo = {};
        createInfo.font                        = font;
        createInfo.size                        = 48.0f;
        createInfo.characters                  = grfx::TextureFont::GetDefaultCharacters();

        PPX_CHECKED_CALL(GetDevice()->CreateTextureFont(&createInfo, &mRoboto));
    }

    // Text draw
    {
        grfx::ShaderModulePtr VS;
        grfx::ShaderModulePtr PS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "TextDraw.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        bytecode = LoadShader("basic/shaders", "TextDraw.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::TextDrawCreateInfo createInfo = {};
        createInfo.pFont                    = mRoboto;
        createInfo.maxTextLength            = 4096;
        createInfo.VS                       = {VS.Get(), "vsmain"};
        createInfo.PS                       = {PS.Get(), "psmain"};
        createInfo.renderTargetFormat       = GetSwapchain()->GetColorFormat();

        PPX_CHECKED_CALL(GetDevice()->CreateTextDraw(&createInfo, &mDynamicText));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for previous frame's render to complete (GPU signal to CPU wait)
    PPX_CHECKED_CALL(frame.timelineSemaphore->Wait(frame.timelineValue));

    // Spawn a thread that will spawn other threads to signal values on the CPU
    std::unique_ptr<std::thread> spawnerThread;
    {
        const uint32_t   kNumThreads = 4;
        grfx::Semaphore* pSemaphore  = frame.timelineSemaphore;

        // Normally, we increment after a wait and before the next signal so we need to add 1.
        const uint64_t startSignalValue = frame.timelineValue + 1;

        spawnerThread = std::unique_ptr<std::thread>(
            new std::thread([kNumThreads, pSemaphore, startSignalValue]() {
                std::vector<std::unique_ptr<std::thread>> signalThreads;

                // Create signaling threads
                for (uint32_t i = 0; i < kNumThreads; ++i) {
                    const uint64_t signalValue = startSignalValue + i;

                    auto signalThread = std::unique_ptr<std::thread>(
                        new std::thread([pSemaphore, signalValue] {
                            PPX_CHECKED_CALL(pSemaphore->Signal(signalValue, true));
                        }));

                    signalThreads.push_back(std::move(signalThread));
                }

                // Join threads
                for (auto& thread : signalThreads) {
                    thread->join();
                }
                signalThreads.clear();
            }));

        // Increment to account signaling thread values
        frame.timelineValue += kNumThreads;
    }

    // Wait on primary for secondary threads to signal on the CPU (CPU signals to CPU wait)
    PPX_CHECKED_CALL(frame.timelineSemaphore->Wait(frame.timelineValue));

    // Join spawner thread
    spawnerThread->join();
    spawnerThread.reset();

    // Signal values for text draw start and finish
    const uint64_t drawTextStartSignalValue  = ++frame.timelineValue;
    const uint64_t drawTextFinishSignalValue = ++frame.timelineValue;

    // Queue the text draw but don't start until the CPU signals (CPU signal to GPU wait)
    {
        PPX_CHECKED_CALL(frame.drawTextCmd->Begin());
        {
            // Prepare string outside of render pass
            {
                std::stringstream ss;
                ss << "Frame: " << GetFrameCount() << "\n";
                ss << "FPS: " << std::setw(6) << std::setprecision(6) << GetAverageFPS() << "\n";
                ss << "Timeline semaphores FTW!"
                   << "\n";
                ss << "Timeline value: " << frame.timelineValue;

                mDynamicText->Clear();
                mDynamicText->AddString(float2(15, 50), ss.str());
                mDynamicText->UploadToGpu(frame.drawTextCmd);
                mDynamicText->PrepareDraw(mCamera.GetViewProjectionMatrix(), frame.drawTextCmd);
            }

            // -------------------------------------------------------------------------------------

            grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();
            beginInfo.RTVClearCount             = 1;
            beginInfo.RTVClearValues[0]         = {{0.25f, 0.3f, 0.33f, 1}};

            frame.drawTextCmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
            frame.drawTextCmd->BeginRenderPass(&beginInfo);
            {
                grfx::Rect     scissorRect = renderPass->GetScissor();
                grfx::Viewport viewport    = renderPass->GetViewport();
                frame.drawTextCmd->SetScissors(1, &scissorRect);
                frame.drawTextCmd->SetViewports(1, &viewport);

                mDynamicText->Draw(frame.drawTextCmd);
            }
            frame.drawTextCmd->EndRenderPass();
            frame.drawTextCmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
        }
        PPX_CHECKED_CALL(frame.drawTextCmd->End());

        std::vector<grfx::Semaphore*> waitSemaphores = {frame.imageAcquiredSemaphore, frame.timelineSemaphore};
        std::vector<uint64_t>         waitValues     = {0, drawTextStartSignalValue};

        std::vector<grfx::Semaphore*> signalSemaphores = {frame.timelineSemaphore};
        std::vector<uint64_t>         signalValues     = {drawTextFinishSignalValue};

        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.drawTextCmd;
        submitInfo.waitSemaphoreCount   = CountU32(waitSemaphores);
        submitInfo.ppWaitSemaphores     = DataPtr(waitSemaphores);
        submitInfo.waitValues           = waitValues;
        submitInfo.signalSemaphoreCount = CountU32(signalSemaphores);
        submitInfo.ppSignalSemaphores   = DataPtr(signalSemaphores);
        submitInfo.signalValues         = signalValues;
        submitInfo.pFence               = nullptr;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }

    // Spawn a thread to signal on the CPU to kick off the text draw
    {
        grfx::Semaphore* pSemaphore = frame.timelineSemaphore;

        spawnerThread = std::unique_ptr<std::thread>(
            new std::thread([pSemaphore, drawTextStartSignalValue]() {
                PPX_CHECKED_CALL(pSemaphore->Signal(drawTextStartSignalValue));
            }));

        spawnerThread->join();
        spawnerThread.reset();
    }

    // Queue ImGui draw but wait on the text draw to finish (GPU signal to GPU wait)
    {
        PPX_CHECKED_CALL(frame.drawImGuiCmd->Begin());
        {
            grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_LOAD);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();

            frame.drawImGuiCmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
            frame.drawImGuiCmd->BeginRenderPass(&beginInfo);
            {
                grfx::Rect     scissorRect = renderPass->GetScissor();
                grfx::Viewport viewport    = renderPass->GetViewport();
                frame.drawImGuiCmd->SetScissors(1, &scissorRect);
                frame.drawImGuiCmd->SetViewports(1, &viewport);

                // Draw ImGui
                DrawDebugInfo();
                DrawImGui(frame.drawImGuiCmd);
            }
            frame.drawImGuiCmd->EndRenderPass();
            frame.drawImGuiCmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
        }
        PPX_CHECKED_CALL(frame.drawImGuiCmd->End());

        // Wait for text daw to finish
        std::vector<grfx::Semaphore*> waitSemaphores = {frame.timelineSemaphore};
        std::vector<uint64_t>         waitValues     = {drawTextFinishSignalValue};

        // Signal value for render work complete
        ++frame.timelineValue;

        std::vector<grfx::Semaphore*> signalSemaphores = {frame.presentReadySemaphore, frame.timelineSemaphore};
        std::vector<uint64_t>         signalValues     = {0, frame.timelineValue};

        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.drawImGuiCmd;
        submitInfo.waitSemaphoreCount   = CountU32(waitSemaphores);
        submitInfo.ppWaitSemaphores     = DataPtr(waitSemaphores);
        submitInfo.waitValues           = waitValues;
        submitInfo.signalSemaphoreCount = CountU32(signalSemaphores);
        submitInfo.ppSignalSemaphores   = DataPtr(signalSemaphores);
        submitInfo.signalValues         = signalValues;
        submitInfo.pFence               = nullptr;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.presentReadySemaphore));
}

SETUP_APPLICATION(ProjApp)