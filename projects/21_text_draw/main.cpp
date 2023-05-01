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
#include "ppx/camera.h"
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
    std::vector<PerFrame> mPerFrame;
    grfx::TextureFontPtr  mRoboto;
    grfx::TextDrawPtr     mStaticText;
    grfx::TextDrawPtr     mDynamicText;
    PerspCamera           mCamera;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "21_text_draw";
    settings.enableImGui      = false;
    settings.grfx.api         = kApi;
    settings.grfx.enableDebug = false;
}

void ProjApp::Setup()
{
    mCamera = PerspCamera(GetWindowWidth(), GetWindowHeight());

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

        PPX_CHECKED_CALL(GetDevice()->CreateTextDraw(&createInfo, &mStaticText));
        PPX_CHECKED_CALL(GetDevice()->CreateTextDraw(&createInfo, &mDynamicText));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    mStaticText->AddString(float2(50, 100), "Diego brazenly plots pixels for\nmaking, very quirky, images with just code!", float3(0.7f, 0.7f, 0.8f));
    mStaticText->AddString(float2(50, 200), "RED: 0xFF0000", float3(1, 0, 0));
    mStaticText->AddString(float2(50, 240), "GREEN: 0x00FF00", float3(0, 1, 0));
    mStaticText->AddString(float2(50, 280), "BLUE: 0x0000FF", float3(0, 0, 1));
    mStaticText->AddString(float2(50, 330), "This string has\tsome\ttabs that are 400% the size of a space!", 4.0f, 1.0f, float3(1), 1);
    mStaticText->AddString(float2(50, 370), "This string has 70%\nline\nspacing!", 3.0f, 0.7f, float3(1), 1);

    PPX_CHECKED_CALL(mStaticText->UploadToGpu(GetGraphicsQueue()));
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

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        // Add some dynamic text
        {
            std::stringstream ss;
            ss << "Frame: " << GetFrameCount() << "\n";
            ss << "FPS: " << std::setw(6) << std::setprecision(6) << GetAverageFPS();

            mDynamicText->Clear();
            mDynamicText->AddString(float2(50, 500), ss.str());

            mDynamicText->UploadToGpu(frame.cmd);
        }

        // Update constnat buffer
        mStaticText->PrepareDraw(mCamera.GetViewProjectionMatrix(), frame.cmd);
        mDynamicText->PrepareDraw(mCamera.GetViewProjectionMatrix(), frame.cmd);

        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0.25f, 0.3f, 0.33f, 1}};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            grfx::Rect     scissorRect = renderPass->GetScissor();
            grfx::Viewport viewport    = renderPass->GetViewport();
            frame.cmd->SetScissors(1, &scissorRect);
            frame.cmd->SetViewports(1, &viewport);

            mStaticText->Draw(frame.cmd);
            mDynamicText->Draw(frame.cmd);

            // Draw ImGui
            // DrawDebugInfo();
#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
            DrawProfilerGrfxApiFunctions();
#endif // defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS) \
    //DrawImGui(frame.cmd);
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