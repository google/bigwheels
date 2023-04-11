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
#include "ppx/math_config.h"
#include "ppx/random.h"
#include "ppx/timer.h"

#include <queue>

namespace FluidSim {

struct SimulationConfig
{
    float       simResolution       = 128;
    float       dyeResolution       = 1024;
    float       captureResolution   = 512;
    float       densityDissipation  = 1.0f;
    float       velocityDissipation = 0.2f;
    float       pressure            = 0.8f;
    int         pressureIterations  = 20;
    int         curl                = 30;
    float       splatRadius         = 0.25f;
    float       splatForce          = 6000.0f;
    bool        shading             = true;
    bool        colorful            = true;
    float       colorUpdateSpeed    = 10.0f;
    bool        paused              = false;
    ppx::float4 backColor           = {0, 0, 0, 1};
    bool        transparent         = false;
    bool        bloom               = true;
    int         bloomIterations     = 8;
    int         bloomResolution     = 256;
    float       bloomIntensity      = 0.8f;
    float       bloomThreshold      = 0.6f;
    float       bloomSoftKnee       = 0.7f;
    bool        sunrays             = true;
    int         sunraysResolution   = 196;
    float       sunraysWeight       = 1.0f;

    SimulationConfig() {}
};

/// @brief Represents a virtual object bouncing around the field.
struct Bouncer
{
    ppx::float2 coord = {0.5, 0.5};
    ppx::float2 delta = {0.014, -0.073};
    ppx::float3 color = {30, 0, 300};
};

class ProjApp;

class FluidSimulation
{
public:
    FluidSimulation(ProjApp* app);
    ProjApp*                     GetApp() const { return mApp; }
    ppx::grfx::DescriptorPoolPtr GetDescriptorPool() const { return mDescriptorPool; }
    ComputeResources*            GetComputeResources() { return &mCompute; }
    GraphicsResources*           GetGraphicsResources() { return &mGraphics; }
    PerFrame&                    GetFrame(size_t ix) { return mPerFrame[ix]; }

    /// @brief Generate the initial splash of color.
    void GenerateInitialSplat();

    /// @brief Execute all the scheduled compute shaders in sequence.
    void DispatchComputeShaders(const PerFrame& frame);

    /// @brief Free descriptor sets and uniform buffers used by compute shaders. This
    /// also clears the execution schedule.
    ///
    /// TODO(https://github.com/google/bigwheels/issues/26): Implement resource pool.
    void FreeComputeShaderResources();

    /// @brief Execute all the scheduled graphics shaders in sequence.
    void Render(const PerFrame& frame);

    /// @brief Free descriptor sets used by graphics shaders. This also clears
    /// the execution schedule.
    void FreeGraphicsShaderResources();

    /// @brief Register the given texture to be filled with an initial color.
    /// @param texture Texture to initialize.
    void AddTextureToInitialize(Texture* texture);

    /// @brief  Update the state of the simulation.  This moves the virtual
    ///         bodies producing the wake.
    void Update();

private:
    // Parent application data.
    ProjApp* mApp;

    // Frame synchronization data.
    std::vector<PerFrame> mPerFrame;

    // Descriptor pool for compute and graphics shaders.
    ppx::grfx::DescriptorPoolPtr mDescriptorPool;

    // Compute resources (pipeline interface, descriptor layout, sampler, etc).
    ComputeResources mCompute;

    // Graphics resources (pipeline interface, descriptor layout, sampler, etc).
    GraphicsResources mGraphics;

    // Configuration settings for the simulation.
    SimulationConfig mConfig;

    // Time tracker to determine the delta since we called FluidSimulation::Update.  Used to
    // pass to compute values like decay and velocity to cycle colors.
    float mLastUpdateTime;

    // Textures used for filtering.
    std::unique_ptr<Texture>              mBloomTexture;
    std::vector<std::unique_ptr<Texture>> mBloomTextures;
    std::unique_ptr<Texture>              mCheckerboardTexture;
    std::unique_ptr<Texture>              mCurlTexture;
    std::unique_ptr<Texture>              mDivergenceTexture;
    std::unique_ptr<Texture>              mDisplayTexture;
    std::unique_ptr<Texture>              mDitheringTexture;
    std::unique_ptr<Texture>              mDrawColorTexture;
    std::unique_ptr<Texture>              mDyeTexture[2];
    std::unique_ptr<Texture>              mPressureTexture[2];
    std::unique_ptr<Texture>              mSunraysTempTexture;
    std::unique_ptr<Texture>              mSunraysTexture;
    std::unique_ptr<Texture>              mVelocityTexture[2];

    // Compute shader filters.
    std::unique_ptr<AdvectionShader>         mAdvection;
    std::unique_ptr<BloomBlurShader>         mBloomBlur;
    std::unique_ptr<BloomBlurAdditiveShader> mBloomBlurAdditive;
    std::unique_ptr<BloomFinalShader>        mBloomFinal;
    std::unique_ptr<BloomPrefilterShader>    mBloomPrefilter;
    std::unique_ptr<BlurShader>              mBlur;
    std::unique_ptr<CheckerboardShader>      mCheckerboard;
    std::unique_ptr<ClearShader>             mClear;
    std::unique_ptr<ColorShader>             mColor;
    std::unique_ptr<CurlShader>              mCurl;
    std::unique_ptr<DisplayShader>           mDisplay;
    std::unique_ptr<DivergenceShader>        mDivergence;
    std::unique_ptr<GradientSubtractShader>  mGradientSubtract;
    std::unique_ptr<PressureShader>          mPressure;
    std::unique_ptr<SplatShader>             mSplat;
    std::unique_ptr<SunraysMaskShader>       mSunraysMask;
    std::unique_ptr<SunraysShader>           mSunrays;
    std::unique_ptr<VorticityShader>         mVorticity;

    // Graphics shader for emitting textures to the swapchain.
    std::unique_ptr<GraphicsShader> mDraw;

    // Queue of compute shaders to execute.
    std::vector<ComputeDispatchRecord> mComputeDispatchQueue;

    // Textures that should be rendered after a round of simulation.
    std::vector<GraphicsDispatchRecord> mGraphicsDispatchQueue;

    // Textures that should be initialized before simulation starts.
    std::vector<Texture*> mTexturesToInitialize;

    // Random numbers used to initialize the simulation.
    ppx::Random mRandom;

    // Time tracker used to cycle colours as the pointer moves.
    ppx::Timer mTimer;

    // Color cycler.  This value cycles between 0.0 and 1.0 based on
    // SimulationConfig::colorUpdateSpeed and the elapsed time.
    float mColorCycler = 0.0;

    // Virtual object moving through the simulation field causing wakes in the fluid.
    Bouncer mMarble;

    // Queue of splats to generate at random when moving objects.  These emulate
    // the spacebar keystroke action in the original WebGL application.
    std::queue<uint32_t> mSplatQ;

    void        ApplyBloom(Texture* source, Texture* destination);
    void        ApplySunrays(Texture* source, Texture* mask, Texture* destination);
    void        Blur(Texture* target, Texture* temp, uint32_t iterations);
    float       CorrectRadius(float radius);
    void        DrawCheckerboard();
    void        DrawColor(ppx::float4 color);
    void        DrawDisplay();
    ppx::float3 GenerateColor();
    float       CalcDeltaTime();
    void        UpdateColors(float deltaTime);
    void        MoveObjects();
    void        Step(float deltaTime);

    /// @brief             Return a vector describing a rectangle with dimensions that can fit "resolution" pixels.
    ///
    /// @param resolution  The minimum size of the rectangle to fit this many pixels.
    ///
    /// @return            A vector of 2 dimensions. The dimensions have the same aspect ratio as
    ///                    the application window and can fit at least "resolution" pixels in it.
    ppx::int2 GetResolution(float resolution);

    ppx::float3  HSVtoRGB(ppx::float3 hsv);
    void         InitBloomTextures();
    void         InitComputeShaders();
    void         InitGraphicsShaders();
    void         InitSunraysTextures();
    void         InitTextures();
    void         MultipleSplats(uint32_t amount);
    ppx::float4  NormalizeColor(ppx::float4 input);
    void         Render();
    ppx::Random& Random() { return mRandom; }
    void         Splat(ppx::float2 point, ppx::float2 delta, ppx::float3 color);

    /// @brief Schedule a compute shader for execution.
    ///
    /// @param dr   The dispatch record describing the shader to be executed and
    ///             the data used to execute it (descriptor set and uniform buffer).
    ///             @see ComputeDispatchRecord.
    void ScheduleDR(const ComputeDispatchRecord& dr) { mComputeDispatchQueue.push_back(dr); }

    /// @brief Schedule a graphics shader for execution.
    ///
    /// @param dr   The dispatch record describing the shader to be executed and
    ///             the descriptor set and texture used to execute it.
    ///             @see GraphicsDispatchRecord.
    void ScheduleDR(const GraphicsDispatchRecord& dr) { mGraphicsDispatchQueue.push_back(dr); }
};

class ProjApp : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    // Fluid simulation driver.
    std::unique_ptr<FluidSimulation> mSim;
};

} // namespace FluidSim

#endif // FLUID_SIMULATION_SIM_H
