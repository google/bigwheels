// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "shaders.h"
#include "sim.h"

#include "ppx/application.h"
#include "ppx/bitmap.h"
#include "ppx/config.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_constants.h"
#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_enums.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/grfx/grfx_pipeline.h"
#include "ppx/grfx/grfx_queue.h"
#include "ppx/grfx/grfx_shader.h"
#include "ppx/grfx/grfx_sync.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"

#include <cmath>
#include <cstdint>
#include <float.h>
#include <math.h>
#include <memory>
#include <string>
#include <vector>

namespace FluidSim {

// In a normal game, animations are linked to the frame delta-time to make them run
// as a fixed perceptible speed. For our use-case (benchmarking), determinism is important.
// Targeting 60 images per second.
constexpr float kFrameDeltaTime = 1.f / 60.f;

// Floating point formats used by the simulation grids.
const ppx::Bitmap::Format kR    = ppx::Bitmap::FORMAT_R_FLOAT;
const ppx::Bitmap::Format kRG   = ppx::Bitmap::FORMAT_RG_FLOAT;
const ppx::Bitmap::Format kRGBA = ppx::Bitmap::FORMAT_RGBA_FLOAT;

void FluidSimulationApp::InitKnobs()
{
    size_t indent = 2;

    // Fluid knobs.
    GetKnobManager().InitKnob(&mConfig.pCurl, "curl", 30.0f, 0.0f, 100.0f);
    mConfig.pCurl->SetDisplayName("Curl");
    mConfig.pCurl->SetFlagDescription("Curl represents the rotational component of the fluid. It determines the spin (vorticity) of the fluid at each point of the simulation. Higher values indicate stronger vortices or swirling motions in the fluid.");

    GetKnobManager().InitKnob(&mConfig.pDensityDissipation, "density-dissipation", 1.0f, 0.0f, 10.0f);
    mConfig.pDensityDissipation->SetDisplayName("Density Dissipation");
    mConfig.pDensityDissipation->SetFlagDescription("This controls the decay of the density field. It determines how quickly the density in the fluid diminshes over time. Higher values result in faster dissipation and smoother density fields.");

    GetKnobManager().InitKnob(&mConfig.pDyeResolution, "dye-resolution", 1024, 1, 2048);
    mConfig.pDyeResolution->SetDisplayName("Dye Resolution");
    mConfig.pDyeResolution->SetFlagDescription("This determines the level of detail in which the dye is represented. This changes the clarity of the dye patterns in the simulation. Higher values provide finer details and sharper patterns.");

    GetKnobManager().InitKnob(&mConfig.pPressure, "pressure", 0.8f, 0.0f, 1.0f);
    mConfig.pPressure->SetDisplayName("Pressure");
    mConfig.pPressure->SetFlagDescription("Indicates the force exerted by the fluid on its surrounding boundaries. Higher values cause a greater force exerted on the boundaries. This can lead to denser regions in the fluid.");

    GetKnobManager().InitKnob(&mConfig.pPressureIterations, "pressure-iterations", 20, 1, 100);
    mConfig.pPressureIterations->SetDisplayName("Pressure Iterations");
    mConfig.pPressureIterations->SetFlagDescription("This is the number of iterations performed when solving the pressure field. Higher values produce a more accurate and detailed pressure computation.");

    GetKnobManager().InitKnob(&mConfig.pVelocityDissipation, "velocity-dissipation", 0.2f, 0.0f, 1.0f);
    mConfig.pVelocityDissipation->SetDisplayName("Velocity Dissipations");
    mConfig.pVelocityDissipation->SetFlagDescription("This simulates the loss of energy within the fluid system. Higher values result in faster velocity reduction.");

    // Bloom knobs.
    GetKnobManager().InitKnob(&mConfig.pEnableBloom, "enable-bloom", true);
    mConfig.pEnableBloom->SetDisplayName("Enable Bloom");
    mConfig.pEnableBloom->SetFlagDescription("Enables bloom effects.");

    GetKnobManager().InitKnob(&mConfig.pBloomIntensity, "bloom-intensity", 0.8f, 0.0f, 1.0f);
    mConfig.pBloomIntensity->SetDisplayName("Intensity");
    mConfig.pBloomIntensity->SetFlagDescription("Strength of the bloom effect applied to the image. It determines how to enhance the bright areas and how pronounced the bloom effect is. Higher values result in a more pronounced effect that will make bright areas of the image appear brighter and more radiant. Lower values produce a more subdued glow.");
    mConfig.pBloomIntensity->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pBloomIterations, "bloom-iterations", 8, 1, 20);
    mConfig.pBloomIterations->SetDisplayName("Iterations");
    mConfig.pBloomIterations->SetFlagDescription("Number of iterations to use in the post-processing bloom pass. Each iteration blurs a downsampled version of the image with the original one. The number of iterations determines how intense the bloom effect is.");
    mConfig.pBloomIterations->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pBloomResolution, "bloom-resolution", 256, 1, 2048);
    mConfig.pBloomResolution->SetDisplayName("Resolution");
    mConfig.pBloomResolution->SetFlagDescription("Sets the size at which the bloom effect is applied. Higher values provide a more precise bloom result at the expense of computation complexity.");
    mConfig.pBloomResolution->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pBloomSoftKnee, "bloom-soft-knee", 0.7f, 0.0f, 1.0f);
    mConfig.pBloomSoftKnee->SetDisplayName("Soft Knee");
    mConfig.pBloomSoftKnee->SetFlagDescription("This controls the transition between bloomed and non-bloomed regions of the image. It determines the smoothness of the blending between regions. Higher values result in smoother transitions.");
    mConfig.pBloomSoftKnee->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pBloomThreshold, "bloom-threshold", 0.6f, 0.0f, 1.0f);
    mConfig.pBloomThreshold->SetDisplayName("Threshold");
    mConfig.pBloomThreshold->SetFlagDescription("Minimum brightness for a pixel to be considered as a candidate for bloom. Pixels with intensities below this threshold are not included in the bloom effect. Higher values limit bloom to the brighter areas of the image.");
    mConfig.pBloomThreshold->SetIndent(indent);

    // Marble knobs.
    GetKnobManager().InitKnob(&mConfig.pEnableMarble, "enable-marble", true);
    mConfig.pEnableMarble->SetDisplayName("Enable Marble");
    mConfig.pEnableMarble->SetFlagDescription("When set, this instantiates a marble that bounces around the simulation field. The marble bounces above the fluid, but it splashes down with certain frequency (controlled by --marble-drop-frequency). This option is not available in the original WebGL implementation.");

    GetKnobManager().InitKnob(&mConfig.pColorUpdateFrequency, "color-update-frequency", 0.9f, 0.0f, 1.0f);
    mConfig.pColorUpdateFrequency->SetDisplayName("Color Update Frequency");
    mConfig.pColorUpdateFrequency->SetFlagDescription("This takes effect only if the bouncing marble is enabled. This controls how often to change the bouncing marble color. This is the color used to produce the splash every time the marble drops.");
    mConfig.pColorUpdateFrequency->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pMarbleDropFrequency, "marble-drop-frequency", 0.1f, 0.0f, 1.0f);
    mConfig.pMarbleDropFrequency->SetDisplayName("Drop Frequency");
    mConfig.pMarbleDropFrequency->SetFlagDescription("The probability that the marble will splash on the fluid as it bounces around the field.");
    mConfig.pMarbleDropFrequency->SetIndent(indent);

    // Splats knobs.
    GetKnobManager().InitKnob(&mConfig.pNumSplats, "num-splats", 0, 0, 20);
    mConfig.pNumSplats->SetDisplayName("Number of Splats");
    mConfig.pNumSplats->SetFlagDescription("This is the number of splashes of color to use at the start of the simulation. This is also used when --splat-frequency is given. A value of 0 means a random number of splats.");

    GetKnobManager().InitKnob(&mConfig.pSplatForce, "splat-force", 6000.0f, 3000.0f, 10000.0f);
    mConfig.pSplatForce->SetDisplayName("Force");
    mConfig.pSplatForce->SetFlagDescription("This represents the magnitude of the impact applied when an external force (e.g. marble drops) on the fluid.");
    mConfig.pSplatForce->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pSplatFrequency, "splat-frequency", 0.03f, 0.0f, 1.0f);
    mConfig.pSplatFrequency->SetDisplayName("Splat frequency");
    mConfig.pSplatFrequency->SetFlagDescription("How frequent should new splats be generated at random.");
    mConfig.pSplatFrequency->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pSplatRadius, "splat-radius", 0.25f, 0.0f, 1.0f);
    mConfig.pSplatRadius->SetDisplayName("Radius");
    mConfig.pSplatRadius->SetFlagDescription("This represents the extent of the influence region around a specific point where the splat force is applied.");
    mConfig.pSplatRadius->SetIndent(indent);

    // Sunrays knobs.
    GetKnobManager().InitKnob(&mConfig.pEnableSunrays, "enable-sunrays", true);
    mConfig.pEnableSunrays->SetDisplayName("Enable Sunrays");
    mConfig.pEnableSunrays->SetFlagDescription("This enables the effect of rays of light shining through the fluid.");

    GetKnobManager().InitKnob(&mConfig.pSunraysResolution, "sunrays-resolution", 196, 1, 500);
    mConfig.pSunraysResolution->SetDisplayName("Resolution");
    mConfig.pSunraysResolution->SetFlagDescription("Indicates the level of detail for the light rays. Higher values produce a finer level of detail for the light.");
    mConfig.pSunraysResolution->SetIndent(indent);

    GetKnobManager().InitKnob(&mConfig.pSunraysWeight, "sunrays-weight", 1.0f, 0.0f, 5.0f);
    mConfig.pSunraysWeight->SetDisplayName("Weight");
    mConfig.pSunraysWeight->SetFlagDescription("Indicates the intensity of the light scattering effect. Higher values result in more prominent sun rays, making them appear brighter.");
    mConfig.pSunraysWeight->SetIndent(indent);

    // Misc knobs.
    GetKnobManager().InitKnob(&mConfig.pSimResolution, "sim-resolution", 128, 1, 1000);
    mConfig.pSimResolution->SetDisplayName("Simulation Resolution");
    mConfig.pSimResolution->SetFlagDescription("This determines the grid size of the grids used during simulation. Higher values produce finer grids which produce a more accurate representation.");

    GetKnobManager().InitKnob(&mConfig.pEnableShading, "shading", true);
    mConfig.pEnableShading->SetDisplayName("Shading");
    mConfig.pEnableShading->SetFlagDescription("Indicates whether to perform diffuse shading on the resulting output.");

    GetKnobManager().InitKnob(&mConfig.pEnableManualAdvection, "manual-advection", false);
    mConfig.pEnableManualAdvection->SetDisplayName("Manual advection");
    mConfig.pEnableManualAdvection->SetFlagDescription("Indicates whether to perform manual advection on the velocity field. If enabled, advection is computed as a bi-linear interpolation on the velocity field. Otherwise, it is computed directly from velocity.");

    // Debug knobs.
    GetKnobManager().InitKnob(&mConfig.pEnableGridDisplay, "display-grids", false);
    mConfig.pEnableGridDisplay->SetDisplayName("Display fluid simulation grids");
    mConfig.pEnableGridDisplay->SetFlagDescription("Indicates whether to overlay the grids used to compute the fluid dynamics properties of the simulation.");
}

void FluidSimulationApp::Config(ppx::ApplicationSettings& settings)
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
    settings.window.resizable      = true;
}

void FluidSimulationApp::Setup()
{
    // Create descriptor pool shared by all pipelines.
    ppx::grfx::DescriptorPoolCreateInfo dpci = {};
    dpci.sampler                             = 1024;
    dpci.sampledImage                        = 1024;
    dpci.uniformBuffer                       = 1024;
    dpci.storageImage                        = 1024;
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&dpci, &mDescriptorPool));

    // Frame synchronization data.
    PerFrame                       frame = {};
    ppx::grfx::SemaphoreCreateInfo sci   = {};
    ppx::grfx::FenceCreateInfo     fci   = {};
    PPX_CHECKED_CALL(GetDevice()->GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&sci, &frame.imageAcquiredSemaphore));
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fci, &frame.imageAcquiredFence));
    fci = {true}; // Create signaled
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fci, &frame.renderCompleteFence));
    mPerFrame.push_back(frame);

    // Set up all the filters to use.
    SetupComputeShaders();

    // Set up the rendering pipeline.
    SetupRenderingPipeline();

    // Set up fluid simulation grids.
    SetupGrids();
}

void FluidSimulationApp::SetupComputeShaders()
{
    // Descriptor set layout.  This must match assets/fluid_simulation/shaders/config.hlsli and it
    // is shared across all ComputeShader instances.
    ppx::grfx::DescriptorSetLayoutCreateInfo lci = {};
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kConstantBufferBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kClampSamplerBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUTextureBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUVelocityBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUCurlBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUSourceBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUBloomBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUSunraysBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUDitheringBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUPressureBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kUDivergenceBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kOutputBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kComputeRepeatSamplerBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&lci, &mComputeDescriptorSetLayout));

    // Compute pipeline interface.
    ppx::grfx::PipelineInterfaceCreateInfo pici = {};
    pici.setCount                               = 1;
    pici.sets[0].set                            = 0;
    pici.sets[0].pLayout                        = mComputeDescriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&pici, &mComputePipelineInterface));

    // Compute sampler.
    ppx::grfx::SamplerCreateInfo sci = {};
    sci.magFilter                    = ppx::grfx::FILTER_LINEAR;
    sci.minFilter                    = ppx::grfx::FILTER_LINEAR;
    sci.mipmapMode                   = ppx::grfx::SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.minLod                       = 0.0f;
    sci.maxLod                       = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&sci, &mClampSampler));

    sci              = {};
    sci.magFilter    = ppx::grfx::FILTER_LINEAR;
    sci.minFilter    = ppx::grfx::FILTER_LINEAR;
    sci.mipmapMode   = ppx::grfx::SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.minLod       = 0.0f;
    sci.maxLod       = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&sci, &mRepeatSampler));

    // Create compute shaders for filtering.
    mAdvection         = std::make_unique<ComputeShader>("advection", std::vector<uint32_t>({kUVelocityBindingSlot, kUSourceBindingSlot, kOutputBindingSlot}));
    mBloomBlur         = std::make_unique<ComputeShader>("bloom_blur", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mBloomBlurAdditive = std::make_unique<ComputeShader>("bloom_blur_additive", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mBloomFinal        = std::make_unique<ComputeShader>("bloom_final", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mBloomPrefilter    = std::make_unique<ComputeShader>("bloom_prefilter", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mBlur              = std::make_unique<ComputeShader>("blur", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mClear             = std::make_unique<ComputeShader>("clear", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mCurl              = std::make_unique<ComputeShader>("curl", std::vector<uint32_t>({kUVelocityBindingSlot, kOutputBindingSlot}));
    mDisplay           = std::make_unique<ComputeShader>("display", std::vector<uint32_t>({kUTextureBindingSlot, kUBloomBindingSlot, kUSunraysBindingSlot, kUDitheringBindingSlot, kOutputBindingSlot}));
    mDivergence        = std::make_unique<ComputeShader>("divergence", std::vector<uint32_t>({kUVelocityBindingSlot, kOutputBindingSlot}));
    mGradientSubtract  = std::make_unique<ComputeShader>("gradient_subtract", std::vector<uint32_t>({kUPressureBindingSlot, kUVelocityBindingSlot, kOutputBindingSlot}));
    mPressure          = std::make_unique<ComputeShader>("pressure", std::vector<uint32_t>({kUPressureBindingSlot, kUDivergenceBindingSlot, kOutputBindingSlot}));
    mSplat             = std::make_unique<ComputeShader>("splat", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mSunrays           = std::make_unique<ComputeShader>("sunrays", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mSunraysMask       = std::make_unique<ComputeShader>("sunrays_mask", std::vector<uint32_t>({kUTextureBindingSlot, kOutputBindingSlot}));
    mVorticity         = std::make_unique<ComputeShader>("vorticity", std::vector<uint32_t>({kUVelocityBindingSlot, kUCurlBindingSlot, kOutputBindingSlot}));
}

void FluidSimulationApp::SetupRenderingPipeline()
{
    // Descriptor set layout and pipeline interface.
    ppx::grfx::DescriptorSetLayoutCreateInfo lci = {};
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kSampledImageBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(kGraphicsRepeatSamplerBindingSlot, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&lci, &mGraphicsDescriptorSetLayout));

    ppx::grfx::PipelineInterfaceCreateInfo pici = {};
    pici.setCount                               = 1;
    pici.sets[0].set                            = 0;
    pici.sets[0].pLayout                        = mGraphicsDescriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&pici, &mGraphicsPipelineInterface));

    // Create a graphics pipeline.
    ppx::grfx::ShaderModulePtr vs;
    std::vector<char>          bytecode = LoadShader("basic/shaders", "StaticTexture.vs");
    PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
    ppx::grfx::ShaderModuleCreateInfo sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&sci, &vs));

    ppx::grfx::ShaderModulePtr ps;
    bytecode = LoadShader("basic/shaders", "StaticTexture.ps");
    PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
    sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&sci, &ps));

    mGraphicsVertexBinding.AppendAttribute({"POSITION", 0, ppx::grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, ppx::grfx::VERTEX_INPUT_RATE_VERTEX});
    mGraphicsVertexBinding.AppendAttribute({"TEXCOORD", 1, ppx::grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, ppx::grfx::VERTEX_INPUT_RATE_VERTEX});

    ppx::grfx::GraphicsPipelineCreateInfo2 gpci = {};
    gpci.VS                                     = {vs.Get(), "vsmain"};
    gpci.PS                                     = {ps.Get(), "psmain"};
    gpci.vertexInputState.bindingCount          = 1;
    gpci.vertexInputState.bindings[0]           = mGraphicsVertexBinding;
    gpci.topology                               = ppx::grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpci.polygonMode                            = ppx::grfx::POLYGON_MODE_FILL;
    gpci.cullMode                               = ppx::grfx::CULL_MODE_NONE;
    gpci.frontFace                              = ppx::grfx::FRONT_FACE_CCW;
    gpci.depthReadEnable                        = false;
    gpci.depthWriteEnable                       = false;
    gpci.blendModes[0]                          = ppx::grfx::BLEND_MODE_NONE;
    gpci.outputState.renderTargetCount          = 1;
    gpci.outputState.renderTargetFormats[0]     = GetSwapchain()->GetColorFormat();
    gpci.pPipelineInterface                     = mGraphicsPipelineInterface;
    PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpci, &mGraphicsPipeline));
}

void FluidSimulationApp::Resize(uint32_t width, uint32_t height)
{
    SetupGrids();
}

void FluidSimulationApp::SetupGrids()
{
    ppx::int2 simRes = GetResolution(GetConfig().pSimResolution->GetValue());
    ppx::int2 dyeRes = GetResolution(GetConfig().pDyeResolution->GetValue());

    // Generate all the grids.
    mCurlGrid        = std::make_unique<SimulationGrid>("curl", simRes.x, simRes.y, kR);
    mDivergenceGrid  = std::make_unique<SimulationGrid>("divergence", simRes.x, simRes.y, kR);
    mDisplayGrid     = std::make_unique<SimulationGrid>("display", GetWindowWidth(), GetWindowHeight(), kRGBA);
    mDitheringGrid   = std::make_unique<SimulationGrid>("fluid_simulation/textures/LDR_LLL1_0.png");
    mDyeGrid[0]      = std::make_unique<SimulationGrid>("dye[0]", dyeRes.x, dyeRes.y, kRGBA);
    mDyeGrid[1]      = std::make_unique<SimulationGrid>("dye[1]", dyeRes.x, dyeRes.y, kRGBA);
    mPressureGrid[0] = std::make_unique<SimulationGrid>("pressure[0]", simRes.x, simRes.y, kR);
    mPressureGrid[1] = std::make_unique<SimulationGrid>("pressure[1]", simRes.x, simRes.y, kR);
    mVelocityGrid[0] = std::make_unique<SimulationGrid>("velocity[0]", simRes.x, simRes.y, kRG);
    mVelocityGrid[1] = std::make_unique<SimulationGrid>("velocity[1]", simRes.x, simRes.y, kRG);

    SetupBloomGrids();
    SetupSunraysGrids();
}

void FluidSimulationApp::SetupBloomGrids()
{
    ppx::int2 res = GetResolution(GetConfig().pBloomResolution->GetValue());
    mBloomGrid    = std::make_unique<SimulationGrid>("bloom", res.x, res.y, kRGBA);
    mBloomGrids.clear();
    for (int i = 0; i < GetConfig().pBloomIterations->GetValue(); i++) {
        uint32_t width  = res.x >> (i + 1);
        uint32_t height = res.y >> (i + 1);
        if (width < 2 || height < 2)
            break;
        mBloomGrids.emplace_back(std::make_unique<SimulationGrid>(std::string("bloom frame buffer[") + std::to_string(i) + "]", width, height, kRGBA));
    }
}

void FluidSimulationApp::SetupSunraysGrids()
{
    ppx::int2 res    = GetResolution(GetConfig().pSunraysResolution->GetValue());
    mSunraysGrid     = std::make_unique<SimulationGrid>("sunrays", res.x, res.y, kR);
    mSunraysTempGrid = std::make_unique<SimulationGrid>("sunrays temp", res.x, res.y, kR);
}

void FluidSimulationApp::Render()
{
    PerFrame& frame = GetFrame(0);

    // Reset dispatch IDs to re-use descriptor sets and uniform buffers allocated before.
    frame.dispatchID = 0;

    ppx::grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset the image-acquired fence.
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset the render-complete fence.
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Draw Knobs window
    if (GetSettings()->enableImGui) {
        UpdateKnobVisibility();
        GetKnobManager().DrawAllKnobs();
    }

    // Build the command buffer.
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        if (GetFrameCount() == 0) {
            // Generate the initial screen with random splashes of color.
            GenerateInitialSplat(&frame);
        }
        else {
            // Update the simulation state and dispatch compute shaders.
            Update(&frame);
        }

        // Prepare the image to present.
        ScalarInput si;
        si.texelSize   = ppx::float2(1.0f / GetWindowWidth(), 1.0f / GetWindowHeight());
        si.ditherScale = mDitheringGrid->GetDitherScale(GetWindowWidth(), GetWindowHeight());
        if (GetConfig().pEnableBloom->GetValue()) {
            si.filterOptions |= kDisplayBloom;
        }
        if (GetConfig().pEnableSunrays->GetValue()) {
            si.filterOptions |= kDisplaySunrays;
        }
        if (GetConfig().pEnableShading->GetValue()) {
            si.filterOptions |= kDisplayShading;
        }
        mDisplay->Dispatch(&frame, {mDyeGrid[0].get(), mBloomGrid.get(), mSunraysGrid.get(), mDitheringGrid.get(), mDisplayGrid.get()}, &si);

        ppx::grfx::RenderPassPtr renderPass = GetSwapchain()->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");
        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_PRESENT, ppx::grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            RenderGrids(frame);

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
    submitInfo.ppSignalSemaphores    = &GetSwapchain()->GetPresentationReadySemaphore(imageIndex);
    submitInfo.pFence                = frame.renderCompleteFence;
    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    // Present and signal.
    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &GetSwapchain()->GetPresentationReadySemaphore(imageIndex)));
}

void FluidSimulationApp::UpdateKnobVisibility()
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

ppx::uint2 FluidSimulationApp::GetResolution(uint32_t resolution) const
{
    float aspectRatio = GetWindowAspect();
    if (aspectRatio < 1.0f) {
        aspectRatio = 1.0f / aspectRatio;
    }

    uint32_t min = resolution;
    uint32_t max = static_cast<uint32_t>(std::round(static_cast<float>(resolution) * aspectRatio));

    return (GetWindowWidth() > GetWindowHeight()) ? ppx::uint2(max, min) : ppx::uint2(min, max);
}

void FluidSimulationApp::GenerateInitialSplat(PerFrame* pFrame)
{
    MultipleSplats(pFrame, GetConfig().pNumSplats->GetValue());
}

ppx::float3 FluidSimulationApp::HSVtoRGB(ppx::float3 hsv)
{
    float       h = hsv[0];
    float       s = hsv[1];
    float       v = hsv[2];
    float       i = floorf(h * 6);
    float       f = h * 6 - i;
    float       p = v * (1 - s);
    float       q = v * (1 - f * s);
    float       t = v * (1 - (1 - f) * s);
    ppx::float3 rgb(0, 0, 0);

    switch (static_cast<int>(i) % 6) {
        case 0: rgb = ppx::float3(v, t, p); break;
        case 1: rgb = ppx::float3(q, v, p); break;
        case 2: rgb = ppx::float3(p, v, t); break;
        case 3: rgb = ppx::float3(p, q, v); break;
        case 4: rgb = ppx::float3(t, p, v); break;
        case 5: rgb = ppx::float3(v, p, q); break;
        default: PPX_ASSERT_MSG(false, "Invalid HSV to RGB conversion");
    }

    return rgb;
}

ppx::float3 FluidSimulationApp::GenerateColor()
{
    ppx::float3 c = HSVtoRGB(ppx::float3(Random().Float(), 1, 1));
    c.r *= 0.15f;
    c.g *= 0.15f;
    c.b *= 0.15f;
    return c;
}

float FluidSimulationApp::CorrectRadius(float radius) const
{
    float aspectRatio = GetWindowAspect();
    return (aspectRatio > 1) ? radius * aspectRatio : radius;
}

void FluidSimulationApp::Splat(PerFrame* pFrame, ppx::float2 coordinate, ppx::float2 delta, ppx::float3 color)
{
    ScalarInput si;
    si.coordinate  = coordinate;
    si.aspectRatio = GetWindowAspect();
    si.radius      = CorrectRadius(GetConfig().pSplatRadius->GetValue() / 100.0f);
    si.color       = ppx::float4(delta.x, delta.y, 0.0f, 1.0f);
    mSplat->Dispatch(pFrame, {mVelocityGrid[0].get(), mVelocityGrid[1].get()}, &si);
    std::swap(mVelocityGrid[0], mVelocityGrid[1]);

    si.color = ppx::float4(color, 1.0f);
    mSplat->Dispatch(pFrame, {mDyeGrid[0].get(), mDyeGrid[1].get()}, &si);
    std::swap(mDyeGrid[0], mDyeGrid[1]);
}

void FluidSimulationApp::MultipleSplats(PerFrame* pFrame, uint32_t amount)
{
    // Emit a random number of splats if the stated amount is 0.
    if (amount == 0) {
        amount = Random().UInt32() % 20 + 5;
    }

    for (uint32_t i = 0; i < amount; i++) {
        ppx::float3 color = GenerateColor();
        color.r *= 10.0f;
        color.g *= 10.0f;
        color.b *= 10.0f;
        ppx::float2 coordinate(Random().Float(), Random().Float());
        ppx::float2 delta(1000.0f * (Random().Float() - 0.5f), 1000.0f * (Random().Float() - 0.5f));
        Splat(pFrame, coordinate, delta, color);
    }
}

void FluidSimulationApp::RenderGrids(const PerFrame& frame)
{
    mDisplayGrid->Draw(frame, ppx::float2(-1.0f, 1.0f));

    if (GetConfig().pEnableGridDisplay->GetValue()) {
        DebugGrids(frame);
    }
}

void FluidSimulationApp::ApplyBloom(PerFrame* pFrame, SimulationGrid* pSource, SimulationGrid* pDestination)
{
    if (mBloomGrids.size() < 2)
        return;

    SimulationGrid* pLast = pDestination;

    float       knee   = GetConfig().pBloomThreshold->GetValue() * GetConfig().pBloomSoftKnee->GetValue() + 0.0001f;
    float       curve0 = GetConfig().pBloomThreshold->GetValue() - knee;
    float       curve1 = knee * 2.0f;
    float       curve2 = 0.25f / knee;
    ScalarInput si;
    si.curve     = ppx::float3(curve0, curve1, curve2);
    si.threshold = GetConfig().pBloomThreshold->GetValue();
    mBloomPrefilter->Dispatch(pFrame, {pSource, pLast}, &si);

    si = ScalarInput();
    for (auto& dest : mBloomGrids) {
        si.texelSize = pLast->GetTexelSize();
        mBloomBlur->Dispatch(pFrame, {pLast, dest.get()}, &si);
        pLast = dest.get();
    }

    si = ScalarInput();
    for (int i = static_cast<int>(mBloomGrids.size() - 2); i >= 0; i--) {
        SimulationGrid* pBaseTex = mBloomGrids[i].get();
        si.texelSize             = pLast->GetTexelSize();
        mBloomBlurAdditive->Dispatch(pFrame, {pLast, pBaseTex}, &si);
        pLast = pBaseTex;
    }

    si           = ScalarInput();
    si.texelSize = pLast->GetTexelSize();
    si.intensity = GetConfig().pBloomIntensity->GetValue();
    mBloomFinal->Dispatch(pFrame, {pLast, pDestination}, &si);
}

void FluidSimulationApp::ApplySunrays(PerFrame* pFrame, SimulationGrid* pSource, SimulationGrid* pMask, SimulationGrid* pDestination)
{
    ScalarInput si;
    mSunraysMask->Dispatch(pFrame, {pSource, pMask}, &si);

    si.weight = GetConfig().pSunraysWeight->GetValue();
    mSunrays->Dispatch(pFrame, {pMask, pDestination}, &si);
}

void FluidSimulationApp::Blur(PerFrame* pFrame, SimulationGrid* pTarget, SimulationGrid* pTemp, uint32_t iterations)
{
    ScalarInput si;
    for (uint32_t i = 0; i < iterations; i++) {
        si.texelSize = ppx::float2(pTarget->GetTexelSize().x, 0.0f);
        mBlur->Dispatch(pFrame, {pTarget, pTemp}, &si);

        si.texelSize = ppx::float2(0.0f, pTarget->GetTexelSize().y);
        mBlur->Dispatch(pFrame, {pTemp, pTarget}, &si);
    }
}

ppx::float4 FluidSimulationApp::NormalizeColor(ppx::float4 input)
{
    return ppx::float4(input.r / 255, input.g / 255, input.b / 255, input.a);
}

void FluidSimulationApp::DebugGrids(const PerFrame& frame)
{
    std::vector<SimulationGrid*> v = {
        mBloomGrid.get(),
        mCurlGrid.get(),
        mDivergenceGrid.get(),
        mPressureGrid[0].get(),
        mPressureGrid[1].get(),
        mVelocityGrid[0].get(),
        mVelocityGrid[1].get(),
    };
    auto  coord   = ppx::float2(-1.0f, 1.0f);
    float maxDimY = 0.0f;
    for (const auto& t : v) {
        auto dim = t->GetNormalizedSize(ppx::uint2(GetWindowWidth(), GetWindowHeight()));
        if (coord.x + dim.x >= 1.0) {
            coord.x = -1.0;
            coord.y -= maxDimY;
            maxDimY = 0.0f;
        }
        t->Draw(frame, coord);
        coord.x += dim.x + 0.005f;
        if (dim.y > maxDimY) {
            maxDimY = dim.y + 0.005f;
        }
    }
}

void FluidSimulationApp::Update(PerFrame* pFrame)
{
    // If the marble has been selected, move it around and drop it at random.
    if (GetConfig().pEnableMarble->GetValue()) {
        MoveMarble();

        // Update the color of the marble.
        if (mRandom.Float() <= GetConfig().pColorUpdateFrequency->GetValue()) {
            mMarble.color = GenerateColor();
        }

        // Drop the marble at random.
        if (mRandom.Float() <= GetConfig().pMarbleDropFrequency->GetValue()) {
            ppx::float2 delta = mMarble.delta * GetConfig().pSplatForce->GetValue();
            Splat(pFrame, mMarble.coord, delta, mMarble.color);
        }
    }

    // Queue up some splats at random. But limit the amount of outstanding splats so it doesn't get too busy.
    if (Random().Float() <= GetConfig().pSplatFrequency->GetValue()) {
        MultipleSplats(pFrame, 1);
    }

    Step(pFrame, kFrameDeltaTime);

    if (GetConfig().pEnableBloom->GetValue()) {
        ApplyBloom(pFrame, mDyeGrid[0].get(), mBloomGrid.get());
    }

    if (GetConfig().pEnableSunrays->GetValue()) {
        ApplySunrays(pFrame, mDyeGrid[0].get(), mDyeGrid[1].get(), mSunraysGrid.get());
        Blur(pFrame, mSunraysGrid.get(), mSunraysTempGrid.get(), 1);
    }
}

void FluidSimulationApp::MoveMarble()
{
    // Move the marble so that it bounces off of the window borders.
    mMarble.coord += mMarble.delta;

    if (mMarble.coord.x < 0.0f) {
        mMarble.coord.x = 0.0f;
        mMarble.delta.x *= -1.0f;
    }
    if (mMarble.coord.x > 1.0f) {
        mMarble.coord.x = 1.0f;
        mMarble.delta.x *= -1.0f;
    }
    if (mMarble.coord.y < 0.0f) {
        mMarble.coord.y = 0.0f;
        mMarble.delta.y *= -1.0f;
    }
    if (mMarble.coord.y > 1.0f) {
        mMarble.coord.y = 1.0f;
        mMarble.delta.y *= -1.0f;
    }
}

void FluidSimulationApp::Step(PerFrame* pFrame, float delta)
{
    ppx::float2 texelSize = mVelocityGrid[0]->GetTexelSize();

    ScalarInput si;
    si.texelSize = texelSize;
    mCurl->Dispatch(pFrame, {mVelocityGrid[0].get(), mCurlGrid.get()}, &si);

    si           = ScalarInput();
    si.texelSize = texelSize;
    si.curl      = GetConfig().pCurl->GetValue();
    si.dt        = delta;
    mVorticity->Dispatch(pFrame, {mVelocityGrid[0].get(), mCurlGrid.get(), mVelocityGrid[1].get()}, &si);
    std::swap(mVelocityGrid[0], mVelocityGrid[1]);

    si           = ScalarInput();
    si.texelSize = texelSize;
    mDivergence->Dispatch(pFrame, {mVelocityGrid[0].get(), mDivergenceGrid.get()}, &si);

    si            = ScalarInput();
    si.clearValue = GetConfig().pPressure->GetValue();
    mClear->Dispatch(pFrame, {mPressureGrid[0].get(), mPressureGrid[1].get()}, &si);
    std::swap(mPressureGrid[0], mPressureGrid[1]);

    si           = ScalarInput();
    si.texelSize = texelSize;
    for (int i = 0; i < GetConfig().pPressureIterations->GetValue(); ++i) {
        mPressure->Dispatch(pFrame, {mPressureGrid[0].get(), mDivergenceGrid.get(), mPressureGrid[1].get()}, &si);
        std::swap(mPressureGrid[0], mPressureGrid[1]);
    }

    si           = ScalarInput();
    si.texelSize = texelSize;
    mGradientSubtract->Dispatch(pFrame, {mPressureGrid[0].get(), mVelocityGrid[0].get(), mVelocityGrid[1].get()}, &si);
    std::swap(mVelocityGrid[0], mVelocityGrid[1]);

    si               = ScalarInput();
    si.dt            = delta;
    si.dissipation   = GetConfig().pVelocityDissipation->GetValue();
    si.texelSize     = texelSize;
    si.dyeTexelSize  = texelSize;
    si.filterOptions = GetConfig().pEnableManualAdvection->GetValue() ? kAdvectionManualFiltering : 0;
    mAdvection->Dispatch(pFrame, {mVelocityGrid[0].get(), mVelocityGrid[0].get(), mVelocityGrid[1].get()}, &si);
    std::swap(mVelocityGrid[0], mVelocityGrid[1]);

    si              = ScalarInput();
    si.dt           = delta;
    si.dissipation  = GetConfig().pDensityDissipation->GetValue();
    si.texelSize    = texelSize;
    si.dyeTexelSize = mDyeGrid[0]->GetTexelSize();
    mAdvection->Dispatch(pFrame, {mVelocityGrid[0].get(), mDyeGrid[0].get(), mDyeGrid[1].get()}, &si);
    std::swap(mDyeGrid[0], mDyeGrid[1]);
}

} // namespace FluidSim
