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

namespace FluidSim {

struct SimulationConfig
{
    float       simResolution;
    float       dyeResolution;
    float       captureResolution;
    float       densityDissipation;
    float       velocityDissipation;
    float       pressure;
    int         pressureIterations;
    int         curl;
    float       splatRadius;
    int         splatForce;
    bool        shading;
    bool        colorful;
    int         colorUpdateSpeed;
    bool        paused;
    ppx::float4 backColor;
    bool        transparent;
    bool        bloom;
    int         bloomIterations;
    int         bloomResolution;
    float       bloomIntensity;
    float       bloomThreshold;
    float       bloomSoftKnee;
    bool        sunrays;
    int         sunraysResolution;
    float       sunraysWeight;

    SimulationConfig()
        : simResolution(128),
          dyeResolution(1024),
          captureResolution(512),
          densityDissipation(1),
          velocityDissipation(0.2f),
          pressure(0.8f),
          pressureIterations(20),
          curl(30),
          splatRadius(0.25f),
          splatForce(6000),
          shading(true),
          colorful(true),
          colorUpdateSpeed(10),
          paused(false),
          backColor(ppx::float4(0, 0, 0, 1)),
          transparent(false),
          bloom(true),
          bloomIterations(8),
          bloomResolution(256),
          bloomIntensity(0.8f),
          bloomThreshold(0.6f),
          bloomSoftKnee(0.7f),
          sunrays(true),
          sunraysResolution(196),
          sunraysWeight(1.0f) {}
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
    void FreeGraphicsShaders();

    /// @brief Register the given texture to be filled with an initial color.
    /// @param texture Texture to initialize.
    void AddTextureToInitialize(Texture* texture);

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

    // Textures used for filtering.
    std::unique_ptr<Texture>              mBloomTexture;
    std::unique_ptr<Texture>              mCheckerboardTexture;
    std::unique_ptr<Texture>              mDisplayTexture;
    std::unique_ptr<Texture>              mDitheringTexture;
    std::unique_ptr<Texture>              mDrawColorTexture;
    std::unique_ptr<Texture>              mDyeTexture[2];
    std::unique_ptr<Texture>              mSunraysTempTexture;
    std::unique_ptr<Texture>              mSunraysTexture;
    std::unique_ptr<Texture>              mVelocityTexture[2];
    std::vector<std::unique_ptr<Texture>> mBloomTextures;

    // Compute shader filters.
    std::unique_ptr<BloomBlurShader>         mBloomBlur;
    std::unique_ptr<BloomBlurAdditiveShader> mBloomBlurAdditive;
    std::unique_ptr<BloomFinalShader>        mBloomFinal;
    std::unique_ptr<BloomPrefilterShader>    mBloomPrefilter;
    std::unique_ptr<BlurShader>              mBlur;
    std::unique_ptr<CheckerboardShader>      mCheckerboard;
    std::unique_ptr<ColorShader>             mColor;
    std::unique_ptr<DisplayShader>           mDisplayMaterial;
    std::unique_ptr<SplatShader>             mSplat;
    std::unique_ptr<SunraysMaskShader>       mSunraysMask;
    std::unique_ptr<SunraysShader>           mSunrays;

    // Graphics shader for emitting textures to the swapchain.
    std::unique_ptr<GraphicsShader> mDraw;

    // Queue of compute shaders to execute.
    std::vector<ComputeDispatchRecord> mComputeDispatchQueue;

    // Textures that should be rendered after a round of simulation.
    std::vector<GraphicsDispatchRecord> mGraphicsDispatchQueue;

    // Textures that should be initialized before simulation starts.
    std::vector<Texture*> mTexturesToInitialize;

    ppx::Random mRandom;

    void        ApplyBloom(Texture* source, Texture* destination);
    void        ApplySunrays(Texture* source, Texture* mask, Texture* destination);
    void        Blur(Texture* target, Texture* temp, uint32_t iterations);
    float       CorrectRadius(float radius);
    void        DrawCheckerboard();
    void        DrawColor(ppx::float4 color);
    void        DrawDisplay();
    ppx::float3 GenerateColor();

    /// @brief             Return a vector describing a rectangle with dimensions that can fit "resolution" pixels.
    ///
    /// @param resolution  The minimum size of the rectangle to fit this many pixels.
    ///
    /// @return            A vector of 2 dimensions. The dimensions have the same aspect ratio as
    ///                    the application window and can fit at least "resolution" pixels in it.
    ppx::int2    GetResolution(float resolution);
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
