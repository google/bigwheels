// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FLUID_SIMULATION_SIM_H
#define FLUID_SIMULATION_SIM_H

#include "shaders.h"

#include "ppx/application.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/random.h"

namespace FluidSim {

// Binding slots for compute shaders.
const uint32_t kConstantBufferBindingSlot       = 0;
const uint32_t kClampSamplerBindingSlot         = 1;
const uint32_t kUTextureBindingSlot             = 2;
const uint32_t kUVelocityBindingSlot            = 3;
const uint32_t kUCurlBindingSlot                = 4;
const uint32_t kUSourceBindingSlot              = 5;
const uint32_t kUBloomBindingSlot               = 6;
const uint32_t kUSunraysBindingSlot             = 7;
const uint32_t kUDitheringBindingSlot           = 8;
const uint32_t kUPressureBindingSlot            = 9;
const uint32_t kUDivergenceBindingSlot          = 10;
const uint32_t kOutputBindingSlot               = 11;
const uint32_t kComputeRepeatSamplerBindingSlot = 12;

// Binding slots for graphics shaders.
const uint32_t kSampledImageBindingSlot          = 0;
const uint32_t kGraphicsRepeatSamplerBindingSlot = 1;

struct SimulationConfig
{
    // Fluid knobs.
    std::shared_ptr<ppx::KnobSlider<float>> pCurl;
    std::shared_ptr<ppx::KnobSlider<float>> pDensityDissipation;
    std::shared_ptr<ppx::KnobSlider<int>>   pDyeResolution;
    std::shared_ptr<ppx::KnobSlider<float>> pPressure;
    std::shared_ptr<ppx::KnobSlider<int>>   pPressureIterations;
    std::shared_ptr<ppx::KnobSlider<float>> pVelocityDissipation;

    // Bloom knobs.
    std::shared_ptr<ppx::KnobCheckbox>      pEnableBloom;
    std::shared_ptr<ppx::KnobSlider<float>> pBloomIntensity;
    std::shared_ptr<ppx::KnobSlider<int>>   pBloomIterations;
    std::shared_ptr<ppx::KnobSlider<int>>   pBloomResolution;
    std::shared_ptr<ppx::KnobSlider<float>> pBloomSoftKnee;
    std::shared_ptr<ppx::KnobSlider<float>> pBloomThreshold;

    // Marble knobs.
    std::shared_ptr<ppx::KnobCheckbox>      pEnableMarble;
    std::shared_ptr<ppx::KnobSlider<float>> pColorUpdateFrequency;
    std::shared_ptr<ppx::KnobSlider<float>> pMarbleDropFrequency;

    // Splat knobs.
    std::shared_ptr<ppx::KnobSlider<int>>   pNumSplats;
    std::shared_ptr<ppx::KnobSlider<float>> pSplatForce;
    std::shared_ptr<ppx::KnobSlider<float>> pSplatFrequency;
    std::shared_ptr<ppx::KnobSlider<float>> pSplatRadius;

    // Sunray knobs.
    std::shared_ptr<ppx::KnobCheckbox>      pEnableSunrays;
    std::shared_ptr<ppx::KnobSlider<int>>   pSunraysResolution;
    std::shared_ptr<ppx::KnobSlider<float>> pSunraysWeight;

    // Misc knobs.
    std::shared_ptr<ppx::KnobSlider<int>> pSimResolution;

    ppx::float4 backColor = {0.0f, 0.0f, 0.0f, 1.0f};
};

// Represents a virtual object bouncing around the field.
struct Bouncer
{
    ppx::float2 coord = {0.5, 0.5};
    ppx::float2 delta = {0.014, -0.073};
    ppx::float3 color = {0.5, 0.5, 0};
};

class FluidSimulationApp : public ppx::Application
{
public:
    virtual void InitKnobs() override;
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

    const SimulationConfig&           GetSimulationConfig() const { return mConfig; }
    ppx::grfx::SamplerPtr             GetClampSampler() const { return mClampSampler; }
    const SimulationConfig&           GetConfig() const { return mConfig; }
    ppx::grfx::DescriptorSetLayoutPtr GetComputeDescriptorSetLayout() const { return mComputeDescriptorSetLayout; }
    ppx::grfx::PipelineInterfacePtr   GetComputePipelineInterface() const { return mComputePipelineInterface; }
    ppx::grfx::DescriptorPoolPtr      GetDescriptorPool() const { return mDescriptorPool; }
    PerFrame&                         GetFrame(size_t ix) { return mPerFrame[ix]; }
    ppx::grfx::QueuePtr               GetGraphicsQueue(uint32_t index = 0) const { return GetDevice()->GetGraphicsQueue(index); }
    ppx::grfx::DescriptorSetLayoutPtr GetGraphicsDescriptorSetLayout() const { return mGraphicsDescriptorSetLayout; }
    ppx::grfx::GraphicsPipelinePtr    GetGraphicsPipeline() const { return mGraphicsPipeline; }
    ppx::grfx::PipelineInterfacePtr   GetGraphicsPipelineInterface() const { return mGraphicsPipelineInterface; }
    ppx::grfx::VertexBinding*         GetGraphicsVertexBinding() { return &mGraphicsVertexBinding; }
    ppx::grfx::SamplerPtr             GetRepeatSampler() const { return mRepeatSampler; }
    static FluidSimulationApp*        GetThisApp() { return static_cast<FluidSimulationApp*>(ppx::Application::Get()); }

    // Generate the initial splash of color.
    void GenerateInitialSplat(PerFrame* frame);

    // Update the state of the simulation for the given frame.
    void Update(PerFrame* frame);

    // Render the grids computed in the Update() invocation.
    void RenderGrids(const PerFrame& frame);

private:
    // Simulation parameters.
    SimulationConfig mConfig;

    // Frame synchronization data.
    std::vector<PerFrame> mPerFrame;

    // Descriptor pool for compute and graphics shaders.
    ppx::grfx::DescriptorPoolPtr mDescriptorPool = nullptr;

    // Pipeline interface, descriptor layout and sampler used by compute shaders.
    ppx::grfx::PipelineInterfacePtr   mComputePipelineInterface   = nullptr;
    ppx::grfx::DescriptorSetLayoutPtr mComputeDescriptorSetLayout = nullptr;
    ppx::grfx::SamplerPtr             mClampSampler               = nullptr;
    ppx::grfx::SamplerPtr             mRepeatSampler              = nullptr;

    // Pipeline interface, descriptor layout and other resources used for rendering textures.
    ppx::grfx::PipelineInterfacePtr   mGraphicsPipelineInterface   = nullptr;
    ppx::grfx::DescriptorSetLayoutPtr mGraphicsDescriptorSetLayout = nullptr;
    ppx::grfx::GraphicsPipelinePtr    mGraphicsPipeline            = nullptr;
    ppx::grfx::VertexBinding          mGraphicsVertexBinding;

    // Grids used for filtering.
    std::vector<std::unique_ptr<SimulationGrid>> mBloomGrids;
    std::unique_ptr<SimulationGrid>              mBloomGrid        = nullptr;
    std::unique_ptr<SimulationGrid>              mCurlGrid         = nullptr;
    std::unique_ptr<SimulationGrid>              mDisplayGrid      = nullptr;
    std::unique_ptr<SimulationGrid>              mDitheringGrid    = nullptr;
    std::unique_ptr<SimulationGrid>              mDivergenceGrid   = nullptr;
    std::unique_ptr<SimulationGrid>              mDrawColorGrid    = nullptr;
    std::unique_ptr<SimulationGrid>              mDyeGrid[2];
    std::unique_ptr<SimulationGrid>              mPressureGrid[2];
    std::unique_ptr<SimulationGrid>              mSunraysTempGrid = nullptr;
    std::unique_ptr<SimulationGrid>              mSunraysGrid     = nullptr;
    std::unique_ptr<SimulationGrid>              mVelocityGrid[2];

    // Compute shader filters.
    std::unique_ptr<AdvectionShader>         mAdvection         = nullptr;
    std::unique_ptr<BloomBlurShader>         mBloomBlur         = nullptr;
    std::unique_ptr<BloomBlurAdditiveShader> mBloomBlurAdditive = nullptr;
    std::unique_ptr<BloomFinalShader>        mBloomFinal        = nullptr;
    std::unique_ptr<BloomPrefilterShader>    mBloomPrefilter    = nullptr;
    std::unique_ptr<BlurShader>              mBlur              = nullptr;
    std::unique_ptr<ClearShader>             mClear             = nullptr;
    std::unique_ptr<ColorShader>             mColor             = nullptr;
    std::unique_ptr<CurlShader>              mCurl              = nullptr;
    std::unique_ptr<DisplayShader>           mDisplay           = nullptr;
    std::unique_ptr<DivergenceShader>        mDivergence        = nullptr;
    std::unique_ptr<GradientSubtractShader>  mGradientSubtract  = nullptr;
    std::unique_ptr<PressureShader>          mPressure          = nullptr;
    std::unique_ptr<SplatShader>             mSplat             = nullptr;
    std::unique_ptr<SunraysMaskShader>       mSunraysMask       = nullptr;
    std::unique_ptr<SunraysShader>           mSunrays           = nullptr;
    std::unique_ptr<VorticityShader>         mVorticity         = nullptr;

    // Random numbers used to initialize the simulation.
    ppx::Random mRandom;

    // Virtual object moving through the simulation field causing wakes in the fluid.
    Bouncer mMarble;

    // Return a vector describing a rectangle with dimensions that can fit "resolution" pixels.
    //
    // resolution  The minimum size of the rectangle to fit this many pixels.
    //
    // Returns A vector of 2 dimensions. The dimensions have the same aspect ratio as
    // the application window and can fit at least "resolution" pixels in it.
    ppx::uint2 GetResolution(uint32_t resolution) const;

    void         ApplyBloom(PerFrame* frame, SimulationGrid* source, SimulationGrid* destination);
    void         ApplySunrays(PerFrame* frame, SimulationGrid* source, SimulationGrid* mask, SimulationGrid* destination);
    void         Blur(PerFrame* frame, SimulationGrid* target, SimulationGrid* temp, uint32_t iterations);
    float        CorrectRadius(float radius) const;
    void         DebugGrids(const PerFrame& frame);
    void         DrawGrid(const PerFrame& frame, SimulationGrid* grid, ppx::float2 coord);
    ppx::float3  GenerateColor();
    ppx::float3  HSVtoRGB(ppx::float3 hsv);
    void         MoveMarble();
    void         MultipleSplats(PerFrame* frame, uint32_t amount);
    ppx::float4  NormalizeColor(ppx::float4 input);
    ppx::Random& Random() { return mRandom; }
    void         SetupBloomGrids();
    void         SetupComputeShaders();
    void         SetupGraphicsShaders();
    void         SetupGrids();
    void         SetupRenderingPipeline();
    void         SetupSunraysGrids();
    void         Splat(PerFrame* frame, ppx::float2 point, ppx::float2 delta, ppx::float3 color);
    void         Step(PerFrame* frame, float deltaTime);
    void         UpdateKnobVisibility();
};

} // namespace FluidSim

#endif // FLUID_SIMULATION_SIM_H
