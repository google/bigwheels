// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FLUID_SIMULATION_SHADERS_H
#define FLUID_SIMULATION_SHADERS_H

#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_buffer.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/math_config.h"

#include <iostream>
#include <map>

namespace FluidSim {

class ProjApp;
class FluidSimulation;

// Pipeline interface, descriptor layout and sampler used by compute shaders.
struct ComputeResources
{
    ppx::grfx::PipelineInterfacePtr   mPipelineInterface;
    ppx::grfx::SamplerPtr             mSampler;
    ppx::grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
};

// Pipeline interface, descriptor layout, sampler and other resources used for graphics shaders.
struct GraphicsResources
{
    ppx::grfx::PipelineInterfacePtr   mPipelineInterface;
    ppx::grfx::VertexBinding          mVertexBinding;
    ppx::grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    ppx::grfx::SamplerPtr             mSampler;
    ppx::grfx::BufferPtr              mVertexBuffer;
};

// Frame synchronization data.
struct PerFrame
{
    ppx::grfx::CommandBufferPtr cmd;
    ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
    ppx::grfx::FencePtr         imageAcquiredFence;
    ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
    ppx::grfx::FencePtr         renderCompleteFence;
};

/// @brief Representation of images used during simulation.
///
/// This structure keeps sample and storage views for presenting and modifying
/// each of the generated textures.
class Texture
{
public:
    /// @brief Initialize a new empty texture.
    /// @param sim      Pointer to the main simulator instance (for accessing global state).
    /// @param name     Name of the texture.
    /// @param width    Texture width.
    /// @param height   Texture height.
    /// @param format   Texture format.
    Texture(FluidSimulation* sim, const std::string& name, uint32_t width, uint32_t height, ppx::grfx::Format format);

    /// @brief Initialize a new texture from an image file.
    /// @param sim      Pointer to the main simulator instance (for accessing global state).
    /// @param fileName Image file to load.
    Texture(FluidSimulation* sim, const std::string& fileName);

    uint32_t                       GetWidth() const { return mTexture->GetWidth(); }
    uint32_t                       GetHeight() const { return mTexture->GetHeight(); }
    const std::string&             GetName() const { return mName; }
    ppx::grfx::ImagePtr            GetImagePtr() { return mTexture; }
    ppx::grfx::SampledImageViewPtr GetSampledView() { return mSampledView; }
    ppx::grfx::StorageImageViewPtr GetStorageView() { return mStorageView; }

    ppx::float2 GetTexelSize() const
    {
        return ppx::float2(1.0f / static_cast<float>(GetWidth()), 1.0f / static_cast<float>(GetHeight()));
    }

    ppx::float2 GetDitherScale(uint32_t width, uint32_t height)
    {
        return ppx::float2(static_cast<float>(width) / static_cast<float>(GetWidth()), static_cast<float>(height) / static_cast<float>(GetHeight()));
    }

private:
    ProjApp* GetApp() const;

    FluidSimulation*               mSim;
    ppx::grfx::ImagePtr            mTexture;
    ppx::grfx::SampledImageViewPtr mSampledView;
    ppx::grfx::StorageImageViewPtr mStorageView;
    std::string                    mName;
};

/// @brief Scalar inputs for the filter programs.
///
/// This needs to be 16-bit aligned to be copied into a uniform buffer.
///
/// NOTE: Fields are organized so that they are packed into 4 word component vectors
/// to match the HLSL packing rules (https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
///
/// This must match the CSInputs structure in assets/fluid_simulation/shaders/config.hlsli.
struct alignas(16) ScalarInput
{
    ScalarInput(Texture* output)
        : texelSize(),
          coordinate(),
          color(),
          curve(),
          intensity(),
          ditherScale(),
          dyeTexelSize(),
          threshold(),
          aspectRatio(),
          clearValue(),
          dissipation(),
          dt(),
          radius(),
          weight(),
          curl(),
          normalizationScale(1.0f / output->GetWidth(), 1.0f / output->GetHeight()) {}

    ppx::float2 texelSize;
    ppx::float2 coordinate;

    ppx::float4 color;

    ppx::float3 curve;
    float       intensity;

    ppx::float2 ditherScale;
    ppx::float2 dyeTexelSize;

    float threshold;
    float aspectRatio;
    float clearValue;
    float dissipation;

    float dt;
    float radius;
    float weight;
    float curl;

    ppx::float2 normalizationScale;
};

class Shader
{
public:
    Shader(FluidSimulation* sim, const std::string& shaderFile)
        : mSim(sim), mShaderFile(shaderFile) {}

    const std::string& GetName() const { return mShaderFile; }
    FluidSimulation*   GetSim() const { return mSim; }
    ProjApp*           GetApp() const;
    GraphicsResources* GetGraphicsResources() const;
    ComputeResources*  GetComputeResources() const;

protected:
    FluidSimulation* mSim;
    std::string      mShaderFile;
};

// A dispatch record holds data needed to execute a compute shader.  The simulator will
// sequence dispatch records so that they can all be executed inside a single frame.
// Each record holds a pointer to the shader to execute, a uniform buffer with shader
// inputs and a descriptor set.
class ComputeShader;
struct ComputeDispatchRecord
{
    ComputeDispatchRecord(ComputeShader* cs, Texture* output, const ScalarInput& si);
    void FreeResources();

    /// @brief Add a texture to sample from to the given descriptor set.
    /// @param texture      Texture to bind.
    /// @param inputBinding Binding slot to bind the texture in.
    void BindInputTexture(Texture* texture, uint32_t bindingSlot);

    /// @brief Add the output texture to the given descriptor set.
    /// @param bindingSlot  Binding slot to bind the texture in.
    void BindOutputTexture(uint32_t bindingSlot);

    ComputeShader*              mShader;
    ppx::grfx::BufferPtr        mUniformBuffer;
    ppx::grfx::DescriptorSetPtr mDescriptorSet;
    Texture*                    mOutput;
};

class ComputeShader : public Shader
{
public:
    ComputeShader(FluidSimulation* sim, const std::string& shaderFile);

    /// @brief Run this shader using the given dispatch record, output texture and inputs.
    /// @param frame Frame to use.
    /// @param dr    Dispatch record to use.
    void Dispatch(const PerFrame& frame, ComputeDispatchRecord& dr);

private:
    ppx::grfx::ComputePipelinePtr mPipeline;
};

class AdvectionShader : public ComputeShader
{
public:
    AdvectionShader(FluidSimulation* sim)
        : ComputeShader(sim, "advection") {}

    ComputeDispatchRecord GetDR(Texture* uVelocity, Texture* uSource, Texture* output, float delta, float dissipation, ppx::float2 texelSize, ppx::float2 dyeTexelSize)
    {
        ScalarInput si(output);
        si.texelSize    = texelSize;
        si.dyeTexelSize = dyeTexelSize;
        si.dissipation  = dissipation;
        si.dt           = delta;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uVelocity, 3);
        dr.BindInputTexture(uSource, 5);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class BloomBlurShader : public ComputeShader
{
public:
    BloomBlurShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_blur") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @param texelSize    Texel size.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class BloomBlurAdditiveShader : public ComputeShader
{
public:
    BloomBlurAdditiveShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_blur_additive") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class BloomFinalShader : public ComputeShader
{
public:
    BloomFinalShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_final") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @param intensity    Intensity parameter.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize, float intensity)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;
        si.intensity = intensity;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class BloomPrefilterShader : public ComputeShader
{
public:
    BloomPrefilterShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_prefilter") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @param curve        Curve parameter.
    /// @param threshold    Threshold parameter.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float3 curve, float threshold)
    {
        ScalarInput si(output);
        si.curve     = curve;
        si.threshold = threshold;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class BlurShader : public ComputeShader
{
public:
    BlurShader(FluidSimulation* sim)
        : ComputeShader(sim, "blur") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class CheckerboardShader : public ComputeShader
{
public:
    CheckerboardShader(FluidSimulation* sim)
        : ComputeShader(sim, "checkerboard") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param output       Texture to write to.
    /// @param aspectRatio  Aspect ratio parameter.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* output, float aspectRatio)
    {
        ScalarInput si(output);
        si.aspectRatio = aspectRatio;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class ClearShader : public ComputeShader
{
public:
    ClearShader(FluidSimulation* sim)
        : ComputeShader(sim, "clear") {}

    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, float clearValue)
    {
        ScalarInput si(output);
        si.clearValue = clearValue;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class ColorShader : public ComputeShader
{
public:
    ColorShader(FluidSimulation* sim)
        : ComputeShader(sim, "color") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param output   Texture to write to.
    /// @param color    Color to write to the whole texture.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* output, ppx::float4 color)
    {
        ScalarInput si(output);
        si.color = color;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class CurlShader : public ComputeShader
{
public:
    CurlShader(FluidSimulation* sim)
        : ComputeShader(sim, "curl") {}

    ComputeDispatchRecord GetDR(Texture* velocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(velocity, 3);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class DisplayShader : public ComputeShader
{
public:
    DisplayShader(FluidSimulation* sim)
        : ComputeShader(sim, "display") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param output   Texture to write to.
    /// @param color    Color to write to the whole texture.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* uBloom, Texture* uSunrays, Texture* uDithering, Texture* output, ppx::float2 texelSize, ppx::float2 ditherScale)
    {
        ScalarInput si(output);
        si.texelSize   = texelSize;
        si.ditherScale = ditherScale;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindInputTexture(uBloom, 6);
        dr.BindInputTexture(uSunrays, 7);
        dr.BindInputTexture(uDithering, 8);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class DivergenceShader : public ComputeShader
{
public:
    DivergenceShader(FluidSimulation* sim)
        : ComputeShader(sim, "divergence") {}

    ComputeDispatchRecord GetDR(Texture* uVelocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uVelocity, 3);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class GradientSubtractShader : public ComputeShader
{
public:
    GradientSubtractShader(FluidSimulation* sim)
        : ComputeShader(sim, "gradient_subtract") {}

    ComputeDispatchRecord GetDR(Texture* uPressure, Texture* uVelocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uPressure, 9);
        dr.BindInputTexture(uVelocity, 3);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class PressureShader : public ComputeShader
{
public:
    PressureShader(FluidSimulation* sim)
        : ComputeShader(sim, "pressure") {}

    ComputeDispatchRecord GetDR(Texture* uPressure, Texture* uDivergence, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uPressure, 9);
        dr.BindInputTexture(uDivergence, 10);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class SplatShader : public ComputeShader
{
public:
    SplatShader(FluidSimulation* sim)
        : ComputeShader(sim, "splat") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param dr           Dispatch record to update.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @param coordinate   Coordinate shader parameter.
    /// @param aspectRatio  Aspect ratio shader parameter.
    /// @param radius       Radius shader parameter.
    /// @param color        Color shader parameter.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, ppx::float2 coordinate, float aspectRatio, float radius, ppx::float4 color)
    {
        ScalarInput si(output);
        si.coordinate  = coordinate;
        si.aspectRatio = aspectRatio;
        si.radius      = radius;
        si.color       = color;

        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class SunraysMaskShader : public ComputeShader
{
public:
    SunraysMaskShader(FluidSimulation* sim)
        : ComputeShader(sim, "sunrays_mask") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output)
    {
        ScalarInput           si(output);
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class SunraysShader : public ComputeShader
{
public:
    SunraysShader(FluidSimulation* sim)
        : ComputeShader(sim, "sunrays") {}

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param uTexture     Texture to sample from.
    /// @param output       Texture to write to.
    /// @param weight       Weight parameter.
    /// @return The dispatch record to schedule.
    ComputeDispatchRecord GetDR(Texture* uTexture, Texture* output, float weight)
    {
        ScalarInput si(output);
        si.weight = weight;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uTexture, 2);
        dr.BindOutputTexture(11);
        return dr;
    }
};

class VorticityShader : public ComputeShader
{
public:
    VorticityShader(FluidSimulation* sim)
        : ComputeShader(sim, "vorticity") {}

    ComputeDispatchRecord GetDR(Texture* uVelocity, Texture* uCurl, Texture* output, ppx::float2 texelSize, int curl, float delta)
    {
        ScalarInput si(output);
        si.curl = curl;
        si.dt   = delta;
        ComputeDispatchRecord dr(this, output, si);
        dr.BindInputTexture(uVelocity, 3);
        dr.BindInputTexture(uCurl, 4);
        dr.BindOutputTexture(11);
        return dr;
    }
};

// A dispatch record holds data needed to execute a graphic shader.  The simulator will
// sequence dispatch records so that they can all be executed inside a single frame.
// Each record holds a pointer to the shader to execute, a descriptor set and the texture
// to present.
class GraphicsShader;
struct GraphicsDispatchRecord
{
    GraphicsDispatchRecord(GraphicsShader* gs, Texture* image);
    void FreeResources();

    GraphicsShader*             mShader;
    ppx::grfx::DescriptorSetPtr mDescriptorSet;
    Texture*                    mImage;
};

class GraphicsShader : public Shader
{
public:
    GraphicsShader(FluidSimulation* sim);

    /// @brief Draw the given texture.
    /// @param frame        Frame to use.
    /// @param dr           GraphicsDispatchRecord instance to use for setting up the pipeline.
    void Dispatch(const PerFrame& frame, GraphicsDispatchRecord& dr);

    /// @brief Create a dispatch record to execute this shader instance.
    /// @param image    Texture to draw.
    /// @return The dispatch record to schedule.
    GraphicsDispatchRecord GetDR(Texture* image)
    {
        GraphicsDispatchRecord dr(this, image);
        return dr;
    }

private:
    ppx::grfx::GraphicsPipelinePtr mPipeline;
};

} // namespace FluidSim

std::ostream& operator<<(std::ostream& os, const FluidSim::ScalarInput& i);
std::ostream& operator<<(std::ostream& os, const FluidSim::Texture& i);

#endif // FLUID_SIMULATION_SHADERS_H
