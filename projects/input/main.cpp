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
    virtual void WindowIconify(bool iconified) override;
    virtual void WindowMaximize(bool maximized) override;
    virtual void KeyDown(ppx::KeyCode key) override;
    virtual void KeyUp(ppx::KeyCode key) override;
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;
    virtual void MouseDown(int32_t x, int32_t y, uint32_t buttons) override;
    virtual void MouseUp(int32_t x, int32_t y, uint32_t buttons) override;
    virtual void Render() override;

protected:
    virtual void DrawGui() override;

private:
    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame> mPerFrame;
    int32_t               mMouseX = 0;
    int32_t               mMouseY = 0;
    uint32_t              mMouseButtons;
    KeyState              mKeyStates[TOTAL_KEY_COUNT] = {0};
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "input";
    settings.grfx.api         = kApi;
    settings.enableImGui      = true;
    settings.window.resizable = true;
}

void ProjApp::Setup()
{
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
}

void ProjApp::WindowIconify(bool iconified)
{
    PPX_LOG_INFO("Window " << (iconified ? "iconified" : "restored"));
}

void ProjApp::WindowMaximize(bool maximized)
{
    PPX_LOG_INFO("Window " << (maximized ? "maximized" : "restored"));
}

void ProjApp::KeyDown(ppx::KeyCode key)
{
    mKeyStates[key].down     = true;
    mKeyStates[key].timeDown = GetElapsedSeconds();
}

void ProjApp::KeyUp(ppx::KeyCode key)
{
    mKeyStates[key].down     = false;
    mKeyStates[key].timeDown = FLT_MAX;
}

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    mMouseX = x;
    mMouseY = y;
}

void ProjApp::MouseDown(int32_t x, int32_t y, uint32_t buttons)
{
    mMouseButtons |= buttons;
}

void ProjApp::MouseUp(int32_t x, int32_t y, uint32_t buttons)
{
    mMouseButtons &= ~buttons;
}

void ProjApp::Render()
{
    if (IsWindowIconified()) {
        return;
    }

    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

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

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
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
    submitInfo.ppSignalSemaphores   = &GetSwapchain()->GetPresentationReadySemaphore(imageIndex);
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(Present(swapchain, imageIndex, 1, &GetSwapchain()->GetPresentationReadySemaphore(imageIndex)));
}

void ProjApp::DrawGui()
{
    if (ImGui::Begin("Mouse Info")) {
        ImGui::Columns(2);

        ImGui::Text("Position");
        ImGui::NextColumn();
        ImGui::Text("%d, %d", mMouseX, mMouseY);
        ImGui::NextColumn();

        ImGui::Text("Left Button");
        ImGui::NextColumn();
        ImGui::Text((mMouseButtons & MOUSE_BUTTON_LEFT) ? "DOWN" : "UP");
        ImGui::NextColumn();

        ImGui::Text("Middle Button");
        ImGui::NextColumn();
        ImGui::Text((mMouseButtons & MOUSE_BUTTON_MIDDLE) ? "DOWN" : "UP");
        ImGui::NextColumn();

        ImGui::Text("Right Button");
        ImGui::NextColumn();
        ImGui::Text((mMouseButtons & MOUSE_BUTTON_RIGHT) ? "DOWN" : "UP");
        ImGui::NextColumn();
    }
    ImGui::End();

    if (ImGui::Begin("Key State")) {
        ImGui::Columns(3);

        ImGui::Text("KEY CODE");
        ImGui::NextColumn();
        ImGui::Text("STATE");
        ImGui::NextColumn();
        ImGui::Text("TIME DOWN");
        ImGui::NextColumn();

        float currentTime = GetElapsedSeconds();
        for (uint32_t i = KEY_RANGE_FIRST; i <= KEY_RANGE_LAST; ++i) {
            const KeyState& state         = mKeyStates[i];
            float           timeSinceDown = std::max(0.0f, currentTime - state.timeDown);

            ImGui::Text("%s", GetKeyCodeString(static_cast<ppx::KeyCode>(i)));
            ImGui::NextColumn();
            ImGui::Text(state.down ? "DOWN" : "UP");
            ImGui::NextColumn();
            ImGui::Text("%.06f", timeSinceDown);
            ImGui::NextColumn();
        }
    }
    ImGui::End();
}

SETUP_APPLICATION(ProjApp)
