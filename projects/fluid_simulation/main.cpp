// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

// Fluid simulation.
//
// This code has been translated and adapted from the original WebGL implementation at
// https://github.com/PavelDoGreat/WebGL-Fluid-Simulation.
//
// The code is organized in 3 files:
//
// sim.cpp
//     Contains the main simulation logic. Everything is driven by `class FluidSimulation`. Simulation actions
//     generate dispatch records (see ComputeDispatchRecord, GraphicsDispatchRecord), which describe the
//     shader to execute and its inputs. Dispatch records are scheduled for execution using `FluidSimulation::ScheduleDR`.
//     Most of this code resembles the original JavaScript implementation (https://github.com/PavelDoGreat/WebGL-Fluid-Simulation/blob/master/script.js)
//
// shaders.cpp
//     Contains most the logic required to interact with the BigWheels API to setup and dispatch compute and graphics shaders.
//     Compute and graphics shaders all inherit from a common `class Shader`.  The main method in those classes is `GetDR()`,
//     which generates a dispatch record with all the necessary inputs to execute the shader (textures to use and scalar inputs
//     in `struct ScalarInput`.
//
// main.cpp
//     Contains the BigWheels API calls needed to launch the application and execute the main rendering loop.  On startup,
//     a single instance of `class FluidSimulation` is created and an initial splash of color computed by calling
//     `FluidSimulation::GenerateInitialSplat`.  The main rendering loop (ProjApp::Render) proceeds as follows:
//
//     1.  All the scheduled compute shaders are executed by calling `FluidSimulation::DispatchComputeShaders`.
//     2.  All the generated textures are drawn by calling `FluidSimulation::Render`.
//     3.  The resources used by compute shaders are released by calling `FluidSimulation::FreeComputeShaderResources`.
//         This prevents running out of pool resources and needlessly executing compute operations over and over.
//     4.  The next iteration of the simulation is executed.

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

void ProjApp::InitKnobs()
{
    size_t indent = 2;

    // Fluid
    mConfig.pCurl = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("curl", 30.0f, 0.0f, 100.0f);
    mConfig.pCurl->SetDisplayName("Curl");
    mConfig.pCurl->SetFlagDescription("Curl represents the rotational component of the fluid. It determines the spin (vorticity) of the fluid at each point of the simulation. Higher values indicate stronger vortices or swirling motions in the fluid.");

    mConfig.pDensityDissipation = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("density-dissipation", 1.0f, 0.0f, 10.0f);
    mConfig.pDensityDissipation->SetDisplayName("Density Dissipation");
    mConfig.pDensityDissipation->SetFlagDescription("This controls the decay of the density field. It determines how quickly the density in the fluid diminshes over time. Higher values result in faster dissipation and smoother density fields.");

    mConfig.pDyeResolution = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("dye-resolution", 1024, 1, 2048);
    mConfig.pDyeResolution->SetDisplayName("Dye Resolution");
    mConfig.pDyeResolution->SetFlagDescription("This determines the level of detail in which the dye is represented. This changes the clarity of the dye patterns in the simulation. Higher values provide finer details and sharper patterns.");

    mConfig.pPressure = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("pressure", 0.8f, 0.0f, 1.0f);
    mConfig.pPressure->SetDisplayName("Pressure");
    mConfig.pPressure->SetFlagDescription("Indicates the force exerted by the fluid on its surrounding boundaries. Higher values cause a greater force exerted on the boundaries. This can lead to denser regions in the fluid.");

    mConfig.pPressureIterations = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("pressure-iterations", 20, 1, 100);
    mConfig.pPressureIterations->SetDisplayName("Pressure Iterations");
    mConfig.pPressureIterations->SetFlagDescription("This is the number of iterations performed when solving the pressure field. Higher values produce a more accurate and detailed pressure computation.");

    mConfig.pVelocityDissipation = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("velocity-dissipation", 0.2f, 0.0f, 1.0f);
    mConfig.pVelocityDissipation->SetDisplayName("Velocity Dissipations");
    mConfig.pVelocityDissipation->SetFlagDescription("This simulates the loss of energy within the fluid system. Higher values result in faster velocity reduction.");

    // Bloom
    mConfig.pEnableBloom = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-bloom", true);
    mConfig.pEnableBloom->SetDisplayName("Enable Bloom");
    mConfig.pEnableBloom->SetFlagDescription("Enables bloom effects.");

    mConfig.pBloomIntensity = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("bloom-intensity", 0.8f, 0.0f, 1.0f);
    mConfig.pBloomIntensity->SetDisplayName("Intensity");
    mConfig.pBloomIntensity->SetFlagDescription("Strength of the bloom effect applied to the image. It determines how to enhance the bright areas and how pronounced the bloom effect is. Higher values result in a more pronounced effect that will make bright areas of the image appear brighter and more radiant. Lower values produce a more subdued glow.");
    mConfig.pBloomIntensity->SetIndent(indent);

    mConfig.pBloomIterations = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("bloom-iterations", 8, 1, 20);
    mConfig.pBloomIterations->SetDisplayName("Iterations");
    mConfig.pBloomIterations->SetFlagDescription("Number of iterations to use in the post-processing bloom pass. Each iteration blurs a downsampled version of the image with the original one. The number of iterations determines how intense the bloom effect is.");
    mConfig.pBloomIterations->SetIndent(indent);

    mConfig.pBloomResolution = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("bloom-resolution", 256, 1, 2048);
    mConfig.pBloomResolution->SetDisplayName("Resolution");
    mConfig.pBloomResolution->SetFlagDescription("Sets the size at which the bloom effect is applied. Higher values provide a more precise bloom result at the expense of computation complexity.");
    mConfig.pBloomResolution->SetIndent(indent);

    mConfig.pBloomSoftKnee = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("bloom-soft-knee", 0.7f, 0.0f, 1.0f);
    mConfig.pBloomSoftKnee->SetDisplayName("Soft Knee");
    mConfig.pBloomSoftKnee->SetFlagDescription("This controls the transition between bloomed and non-bloomed regions of the image. It determines the smoothness of the blending between regions. Higher values result in smoother transitions.");
    mConfig.pBloomSoftKnee->SetIndent(indent);

    mConfig.pBloomThreshold = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("bloom-threshold", 0.6f, 0.0f, 1.0f);
    mConfig.pBloomThreshold->SetDisplayName("Threshold");
    mConfig.pBloomThreshold->SetFlagDescription("Minimum brightness for a pixel to be considered as a candidate for bloom. Pixels with intensities below this threshold are not included in the bloom effect. Higher values limit bloom to the brighter areas of the image.");
    mConfig.pBloomThreshold->SetIndent(indent);

    // Marble
    mConfig.pEnableMarble = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-marble", true);
    mConfig.pEnableMarble->SetDisplayName("Enable Marble");
    mConfig.pEnableMarble->SetFlagDescription("When set, this instantiates a marble that bounces around the simulation field. The marble bounces above the fluid, but it splashes down with certain frequency (controlled by --marble-drop-frequency). This option is not available in the original WebGL implementation.");

    mConfig.pColorUpdateFrequency = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("color-update-frequency", 0.9f, 0.0f, 1.0f);
    mConfig.pColorUpdateFrequency->SetDisplayName("Color Update Frequency");
    mConfig.pColorUpdateFrequency->SetFlagDescription("This takes effect only if the bouncing marble is enabled. This controls how often to change the bouncing marble color. This is the color used to produce the splash every time the marble drops.");
    mConfig.pColorUpdateFrequency->SetIndent(indent);

    mConfig.pMarbleDropFrequency = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("marble-drop-frequency", 0.9f, 0.0f, 1.0f);
    mConfig.pMarbleDropFrequency->SetDisplayName("Drop Frequency");
    mConfig.pMarbleDropFrequency->SetFlagDescription("The probability that the marble will splash on the fluid as it bounces around the field.");
    mConfig.pMarbleDropFrequency->SetIndent(indent);

    // Splats
    mConfig.pNumSplats = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("num-splats", 0, 0, 20);
    mConfig.pNumSplats->SetDisplayName("Number of Splats");
    mConfig.pNumSplats->SetFlagDescription("This is the number of splashes of color to use at the start of the simulation. This is also used when --splat-frequency is given. A value of 0 means a random number of splats.");

    mConfig.pSplatForce = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("splat-force", 6000.0f, 3000.0f, 10000.0f);
    mConfig.pSplatForce->SetDisplayName("Force");
    mConfig.pSplatForce->SetFlagDescription("This represents the magnitude of the impact applied when an external force (e.g. marble drops) on the fluid.");
    mConfig.pSplatForce->SetIndent(indent);

    mConfig.pSplatFrequency = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("splat-frequency", 0.4f, 0.0f, 1.0f);
    mConfig.pSplatFrequency->SetDisplayName("Frequency");
    mConfig.pSplatFrequency->SetFlagDescription("How frequent should new splats be generated at random.");
    mConfig.pSplatFrequency->SetIndent(indent);

    mConfig.pSplatRadius = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("splat-radius", 0.25f, 0.0f, 1.0f);
    mConfig.pSplatRadius->SetDisplayName("Radius");
    mConfig.pSplatRadius->SetFlagDescription("This represents the extent of the influence region around a specific point where the splat force is applied.");
    mConfig.pSplatRadius->SetIndent(indent);

    // Sunrays
    mConfig.pEnableSunrays = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-sunrays", true);
    mConfig.pEnableSunrays->SetDisplayName("Enable Sunrays");
    mConfig.pEnableSunrays->SetFlagDescription("This enables the effect of rays of light shining through the fluid.");

    mConfig.pSunraysResolution = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sunrays-resolution", 196, 1, 500);
    mConfig.pSunraysResolution->SetDisplayName("Resolution");
    mConfig.pSunraysResolution->SetFlagDescription("Indicates the level of detail for the light rays. Higher values produce a finer level of detail for the light.");
    mConfig.pSunraysResolution->SetIndent(indent);

    mConfig.pSunraysWeight = GetKnobManager().CreateKnob<ppx::KnobSlider<float>>("sunrays-weight", 1.0f, 0.0f, 5.0f);
    mConfig.pSunraysWeight->SetDisplayName("Weight");
    mConfig.pSunraysWeight->SetFlagDescription("Indicates the intensity of the light scattering effect. Higher values result in more prominent sun rays, making them appear brighter.");
    mConfig.pSunraysWeight->SetIndent(indent);

    // Misc
    mConfig.pSimResolution = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sim-resolution", 128, 1, 1000);
    mConfig.pSimResolution->SetDisplayName("Simulation Resolution");
    mConfig.pSimResolution->SetFlagDescription("This determines the grid size of the compute textures used during simulation. Higher values produce finer grids which produce a more accurate representation.");
}

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
#if defined(USE_DX12)
    const ppx::grfx::Api kApi = ppx::grfx::API_DX_12_0;
#elif defined(USE_VK)
    const ppx::grfx::Api kApi = ppx::grfx::API_VK_1_1;
#endif

    const auto& clOptions = GetExtraOptions();

    settings.appName               = "fluid_simulation";
    settings.enableImGui           = true;
    settings.grfx.api              = kApi;
    settings.grfx.enableDebug      = clOptions.HasExtraOption("enable-debug");
    settings.allowThirdPartyAssets = true;
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

    // Draw Knobs window
    if (GetSettings()->enableImGui) {
        UpdateKnobVisibility();
        GetKnobManager().DrawAllKnobs();
    }

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

void ProjApp::UpdateKnobVisibility()
{
    if (mConfig.pEnableBloom->DigestUpdate()) {
        bool bloomEnabled = mConfig.pEnableBloom->GetValue();
        mConfig.pBloomIntensity->SetVisible(bloomEnabled);
        mConfig.pBloomIterations->SetVisible(bloomEnabled);
        mConfig.pBloomResolution->SetVisible(bloomEnabled);
        mConfig.pBloomSoftKnee->SetVisible(bloomEnabled);
        mConfig.pBloomThreshold->SetVisible(bloomEnabled);
    }
    if (mConfig.pEnableMarble->DigestUpdate()) {
        bool marbleEnabled = mConfig.pEnableMarble->GetValue();
        mConfig.pColorUpdateFrequency->SetVisible(marbleEnabled);
        mConfig.pMarbleDropFrequency->SetVisible(marbleEnabled);
    }
    if (mConfig.pEnableSunrays->DigestUpdate()) {
        bool sunraysEnabled = mConfig.pEnableSunrays->GetValue();
        mConfig.pSunraysResolution->SetVisible(sunraysEnabled);
        mConfig.pSunraysWeight->SetVisible(sunraysEnabled);
    }
}

} // namespace FluidSim

SETUP_APPLICATION(FluidSim::ProjApp)