// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

///
/// Fluid simulation.
///
/// This code has been translated and adapted from the original WebGL implementation at
/// https://github.com/PavelDoGreat/WebGL-Fluid-Simulation.
///
/// The code is organized in 3 files:
///
/// sim.cpp
///     Contains the main simulation logic. Everything is driven by `class FluidSimulation`. Simulation actions
///     generate dispatch records (@see ComputeDispatchRecord, GraphicsDispatchRecord), which describe the
///     shader to execute and its inputs. Dispatch records are scheduled for execution using `FluidSimulation::ScheduleDR`.
///     Most of this code resembles the original JavaScript implementation (@see https://github.com/PavelDoGreat/WebGL-Fluid-Simulation/blob/master/script.js)
///
/// shaders.cpp
///     Contains most the logic required to interact with the BigWheels API to setup and dispatch compute and graphics shaders.
///     Compute and graphics shaders all inherit from a common `class Shader`.  The main method in those classes is `GetDR()`,
///     which generates a dispatch record with all the necessary inputs to execute the shader (textures to use and scalar inputs
///     in `struct ScalarInput`.
///
/// main.cpp
///     Contains the BigWheels API calls needed to launch the application and execute the main rendering loop.  On startup,
///     a single instance of `class FluidSimulation` is created and an initial splash of color computed by calling
///     `FluidSimulation::GenerateInitialSplat`.  The main rendering loop (@see ProjApp::Render) proceeds as follows:
///
///     1.  All the scheduled compute shaders are executed by calling `FluidSimulation::DispatchComputeShaders`.
///     2.  All the generated textures are drawn by calling `FluidSimulation::Render`.
///     3.  The resources used by compute shaders are released by calling `FluidSimulation::FreeComputeShaderResources`.
///         This prevents running out of pool resources and needlessly executing compute operations over and over.
///     4.  The next iteration of the simulation is executed (@note This is still not implemented).
///

#include "sim.h"
#include "shaders.h"

#include "ppx/config.h"
#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_enums.h"
#include "ppx/log.h"
#include "ppx/math_config.h"
#include "ppx/ppx.h"

namespace FluidSim {

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
#if defined(USE_DX12)
    const ppx::grfx::Api kApi = ppx::grfx::API_DX_12_0;
#elif defined(USE_VK)
    const ppx::grfx::Api kApi = ppx::grfx::API_VK_1_1;
#endif

    const auto& clOptions = GetExtraOptions();

    settings.appName               = "fluid_simulation";
    settings.enableImGui           = false;
    settings.grfx.api              = kApi;
    settings.grfx.enableDebug      = clOptions.HasExtraOption("enable-debug");
    settings.allowThirdPartyAssets = true;

    // Parse command line options to setup all the simulation values.
    mConfig.bloom                = clOptions.GetExtraOptionValueOrDefault<bool>("enable-bloom", true);
    mConfig.bloomIntensity       = clOptions.GetExtraOptionValueOrDefault<float>("bloom-intensity", 0.8f);
    mConfig.bloomIterations      = clOptions.GetExtraOptionValueOrDefault<uint32_t>("bloom-iterations", 8);
    mConfig.bloomResolution      = clOptions.GetExtraOptionValueOrDefault<uint32_t>("bloom-resolution", 256);
    mConfig.bloomSoftKnee        = clOptions.GetExtraOptionValueOrDefault<float>("bloom-soft-knee", 0.7f);
    mConfig.bloomThreshold       = clOptions.GetExtraOptionValueOrDefault<float>("bloom-threshold", 0.6f);
    mConfig.colorUpdateFrequency = clOptions.GetExtraOptionValueOrDefault<float>("color-update-frequency", 0.9f);
    mConfig.curl                 = clOptions.GetExtraOptionValueOrDefault<float>("curl", 30.0f);
    mConfig.densityDissipation   = clOptions.GetExtraOptionValueOrDefault<float>("density-dissipation", 1.0f);
    mConfig.dyeResolution        = clOptions.GetExtraOptionValueOrDefault<uint32_t>("dye-resolution", 1024);
    mConfig.marble               = clOptions.GetExtraOptionValueOrDefault<bool>("enable-marble", true);
    mConfig.marbleDropFrequency  = clOptions.GetExtraOptionValueOrDefault<float>("marble-drop-frequency", 0.8f);
    mConfig.numSplats            = clOptions.GetExtraOptionValueOrDefault<int>("num-splats", 0);
    mConfig.pressure             = clOptions.GetExtraOptionValueOrDefault<float>("pressure", 0.8f);
    mConfig.pressureIterations   = clOptions.GetExtraOptionValueOrDefault<uint32_t>("pressure-iterations", 20);
    mConfig.simResolution        = clOptions.GetExtraOptionValueOrDefault<uint32_t>("sim-resolution", 128);
    mConfig.splatForce           = clOptions.GetExtraOptionValueOrDefault<float>("splat-force", 6000.0f);
    mConfig.splatFrequency       = clOptions.GetExtraOptionValueOrDefault<float>("splat-frequency", 0.1f);
    mConfig.splatRadius          = clOptions.GetExtraOptionValueOrDefault<float>("splat-radius", 0.25f);
    mConfig.sunrays              = clOptions.GetExtraOptionValueOrDefault<bool>("enable-sunrays", true);
    mConfig.sunraysResolution    = clOptions.GetExtraOptionValueOrDefault<uint32_t>("sunrays-resolution", 196);
    mConfig.sunraysWeight        = clOptions.GetExtraOptionValueOrDefault<float>("sunrays-weight", 1.0f);
    mConfig.velocityDissipation  = clOptions.GetExtraOptionValueOrDefault<float>("velocity-dissipation", 0.2f);
}

void ProjApp::Setup()
{
    // Create the main simulation driver.
    mSim = std::make_unique<FluidSimulation>(this);

    // Generate the initial screen with random splashes of color.
    mSim->GenerateInitialSplat();
}

void ProjApp::Render()
{
    PerFrame& frame = mSim->GetFrame(0);

    ppx::grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset the image-acquired fence.
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset the render-complete fence.
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update the simulation state.  This schedules new compute shaders to draw the next frame.
    mSim->Update();

    // Build the command buffer.
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        // Dispatch all the scheduled compute shaders.
        mSim->DispatchComputeShaders(frame);

        ppx::grfx::RenderPassPtr renderPass = GetSwapchain()->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");
        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_PRESENT, ppx::grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            mSim->DispatchGraphicsShaders(frame);

            // Draw ImGui.
            DrawDebugInfo();
            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_RENDER_TARGET, ppx::grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    // Submit the command buffer.
    ppx::grfx::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount    = 1;
    submitInfo.ppCommandBuffers      = &frame.cmd;
    submitInfo.waitSemaphoreCount    = 1;
    submitInfo.ppWaitSemaphores      = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount  = 1;
    submitInfo.ppSignalSemaphores    = &frame.renderCompleteSemaphore;
    submitInfo.pFence                = frame.renderCompleteFence;
    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    // Present and signal.
    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.renderCompleteSemaphore));

    mSim->FreeComputeShaderResources();
    mSim->FreeGraphicsShaderResources();
}

} // namespace FluidSim

SETUP_APPLICATION(FluidSim::ProjApp)