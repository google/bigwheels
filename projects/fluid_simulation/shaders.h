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

class FluidSimulation;

// Pipeline interface, descriptor layout and sampler used by compute shaders.
struct ComputeResources
{
    ppx::grfx::PipelineInterfacePtr   mPipelineInterface;
    ppx::grfx::SamplerPtr             mClampSampler;
    ppx::grfx::SamplerPtr             mRepeatSampler;
    ppx::grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
};

// Pipeline interface, descriptor layout, sampler and other resources used for graphics shaders.
struct GraphicsResources
{
    ppx::grfx::PipelineInterfacePtr   mPipelineInterface;
    ppx::grfx::VertexBinding          mVertexBinding;
    ppx::grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    ppx::grfx::SamplerPtr             mSampler;
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

// Representation of images used during simulation.
//
// This structure keeps sample and storage views for presenting and modifying
// each of the generated textures.
class Texture
{
public:
    // Initialize a new empty texture.
    // name     Name of the texture.
    // width    Texture width.
    // height   Texture height.
    // format   Texture format.
    // device   Device to use to create the storage and sampled views.
    Texture(const std::string& name, uint32_t width, uint32_t height, ppx::grfx::Format format, ppx::grfx::DevicePtr device);

    // Initialize a new texture from an image file.
    // fileName Image file to load.
    // device   Device to use to create the storage and sampled views.
    Texture(const std::string& fileName, ppx::grfx::DevicePtr device);

    uint32_t GetWidth() const { return mTexture->GetWidth(); }
    uint32_t GetHeight() const { return mTexture->GetHeight(); }

    // Compute and return the size of the texture normalized to the resolution.
    // given in pixels.  This maps the size of the texture to the normalized
    // coordinates ([-1, 1], [-1, 1]).
    ppx::float2 GetNormalizedSize(ppx::uint2 resolution) const
    {
        return ppx::float2(GetWidth() * 2.0f / static_cast<float>(resolution.x), GetHeight() * 2.0f / static_cast<float>(resolution.y));
    }

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
    ppx::grfx::ImagePtr            mTexture;
    ppx::grfx::SampledImageViewPtr mSampledView;
    ppx::grfx::StorageImageViewPtr mStorageView;
    std::string                    mName;
};

// Scalar inputs for the filter programs.
//
// This needs to be 16-bit aligned to be copied into a uniform buffer.
//
// NOTE: Fields are organized so that they are packed into 4 word component vectors
// to match the HLSL packing rules (https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
//
// This must match the CSInputs structure in assets/fluid_simulation/shaders/config.hlsli.
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
    Shader(const std::string& shaderFile, ppx::grfx::DevicePtr device, ppx::grfx::DescriptorPoolPtr descriptorPool)
        : mShaderFile(shaderFile), mDevice(device), mDescriptorPool(descriptorPool) {}

    const std::string&           GetName() const { return mShaderFile; }
    ppx::grfx::DevicePtr         GetDevice() const { return mDevice; }
    ppx::grfx::DescriptorPoolPtr GetDescriptorPool() const { return mDescriptorPool; }

protected:
    std::string                  mShaderFile;
    ppx::grfx::DevicePtr         mDevice;
    ppx::grfx::DescriptorPoolPtr mDescriptorPool;
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

    // Add a texture to sample from to the given descriptor set.
    // texture      Texture to bind.
    // inputBinding Binding slot to bind the texture in.
    void BindInputTexture(Texture* texture, uint32_t bindingSlot);

    // Add the output texture to the given descriptor set.
    // bindingSlot  Binding slot to bind the texture in.
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

    // Run this shader using the given dispatch record, output texture and inputs.
    // frame Frame to use.
    // dr    Dispatch record to use.
    void Dispatch(const PerFrame& frame, const std::unique_ptr<ComputeDispatchRecord>& dr);

    ComputeResources* GetResources() const { return mResources; }

private:
    ppx::grfx::ComputePipelinePtr mPipeline;
    ComputeResources*             mResources;
};

class AdvectionShader : public ComputeShader
{
public:
    AdvectionShader(FluidSimulation* sim)
        : ComputeShader(sim, "advection") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uVelocity, Texture* uSource, Texture* output, float delta, float dissipation, ppx::float2 texelSize, ppx::float2 dyeTexelSize)
    {
        ScalarInput si(output);
        si.texelSize    = texelSize;
        si.dyeTexelSize = dyeTexelSize;
        si.dissipation  = dissipation;
        si.dt           = delta;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uVelocity, 3);
        dr->BindInputTexture(uSource, 5);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class BloomBlurShader : public ComputeShader
{
public:
    BloomBlurShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_blur") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    // texelSize    Texel size.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class BloomBlurAdditiveShader : public ComputeShader
{
public:
    BloomBlurAdditiveShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_blur_additive") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class BloomFinalShader : public ComputeShader
{
public:
    BloomFinalShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_final") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    // intensity    Intensity parameter.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize, float intensity)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;
        si.intensity = intensity;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class BloomPrefilterShader : public ComputeShader
{
public:
    BloomPrefilterShader(FluidSimulation* sim)
        : ComputeShader(sim, "bloom_prefilter") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    // curve        Curve parameter.
    // threshold    Threshold parameter.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float3 curve, float threshold)
    {
        ScalarInput si(output);
        si.curve     = curve;
        si.threshold = threshold;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class BlurShader : public ComputeShader
{
public:
    BlurShader(FluidSimulation* sim)
        : ComputeShader(sim, "blur") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class CheckerboardShader : public ComputeShader
{
public:
    CheckerboardShader(FluidSimulation* sim)
        : ComputeShader(sim, "checkerboard") {}

    // Create a dispatch record to execute this shader instance.
    // output       Texture to write to.
    // aspectRatio  Aspect ratio parameter.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* output, float aspectRatio)
    {
        ScalarInput si(output);
        si.aspectRatio = aspectRatio;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class ClearShader : public ComputeShader
{
public:
    ClearShader(FluidSimulation* sim)
        : ComputeShader(sim, "clear") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, float clearValue)
    {
        ScalarInput si(output);
        si.clearValue = clearValue;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class ColorShader : public ComputeShader
{
public:
    ColorShader(FluidSimulation* sim)
        : ComputeShader(sim, "color") {}

    // Create a dispatch record to execute this shader instance.
    // output   Texture to write to.
    // color    Color to write to the whole texture.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* output, ppx::float4 color)
    {
        ScalarInput si(output);
        si.color = color;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class CurlShader : public ComputeShader
{
public:
    CurlShader(FluidSimulation* sim)
        : ComputeShader(sim, "curl") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* velocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(velocity, 3);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class DisplayShader : public ComputeShader
{
public:
    DisplayShader(FluidSimulation* sim)
        : ComputeShader(sim, "display") {}

    // Create a dispatch record to execute this shader instance.
    // output   Texture to write to.
    // color    Color to write to the whole texture.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* uBloom, Texture* uSunrays, Texture* uDithering, Texture* output, ppx::float2 texelSize, ppx::float2 ditherScale)
    {
        ScalarInput si(output);
        si.texelSize   = texelSize;
        si.ditherScale = ditherScale;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindInputTexture(uBloom, 6);
        dr->BindInputTexture(uSunrays, 7);
        dr->BindInputTexture(uDithering, 8);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class DivergenceShader : public ComputeShader
{
public:
    DivergenceShader(FluidSimulation* sim)
        : ComputeShader(sim, "divergence") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uVelocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uVelocity, 3);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class GradientSubtractShader : public ComputeShader
{
public:
    GradientSubtractShader(FluidSimulation* sim)
        : ComputeShader(sim, "gradient_subtract") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uPressure, Texture* uVelocity, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uPressure, 9);
        dr->BindInputTexture(uVelocity, 3);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class PressureShader : public ComputeShader
{
public:
    PressureShader(FluidSimulation* sim)
        : ComputeShader(sim, "pressure") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uPressure, Texture* uDivergence, Texture* output, ppx::float2 texelSize)
    {
        ScalarInput si(output);
        si.texelSize = texelSize;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uPressure, 9);
        dr->BindInputTexture(uDivergence, 10);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class SplatShader : public ComputeShader
{
public:
    SplatShader(FluidSimulation* sim)
        : ComputeShader(sim, "splat") {}

    // Create a dispatch record to execute this shader instance.
    // dr           Dispatch record to update.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    // coordinate   Coordinate shader parameter.
    // aspectRatio  Aspect ratio shader parameter.
    // radius       Radius shader parameter.
    // color        Color shader parameter.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, ppx::float2 coordinate, float aspectRatio, float radius, ppx::float4 color)
    {
        ScalarInput si(output);
        si.coordinate  = coordinate;
        si.aspectRatio = aspectRatio;
        si.radius      = radius;
        si.color       = color;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class SunraysMaskShader : public ComputeShader
{
public:
    SunraysMaskShader(FluidSimulation* sim)
        : ComputeShader(sim, "sunrays_mask") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output)
    {
        ScalarInput si(output);

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class SunraysShader : public ComputeShader
{
public:
    SunraysShader(FluidSimulation* sim)
        : ComputeShader(sim, "sunrays") {}

    // Create a dispatch record to execute this shader instance.
    // uTexture     Texture to sample from.
    // output       Texture to write to.
    // weight       Weight parameter.
    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uTexture, Texture* output, float weight)
    {
        ScalarInput si(output);
        si.weight = weight;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uTexture, 2);
        dr->BindOutputTexture(11);
        return dr;
    }
};

class VorticityShader : public ComputeShader
{
public:
    VorticityShader(FluidSimulation* sim)
        : ComputeShader(sim, "vorticity") {}

    std::unique_ptr<ComputeDispatchRecord> GetDR(Texture* uVelocity, Texture* uCurl, Texture* output, ppx::float2 texelSize, float curl, float delta)
    {
        ScalarInput si(output);
        si.curl = curl;
        si.dt   = delta;

        auto dr = std::make_unique<ComputeDispatchRecord>(this, output, si);
        dr->BindInputTexture(uVelocity, 3);
        dr->BindInputTexture(uCurl, 4);
        dr->BindOutputTexture(11);
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
    GraphicsDispatchRecord(GraphicsShader* gs, Texture* image, ppx::float2 coord, ppx::uint2 resolution);
    void FreeResources();

    GraphicsShader*             mShader;
    ppx::grfx::DescriptorSetPtr mDescriptorSet;
    Texture*                    mImage;
    ppx::grfx::BufferPtr        mVertexBuffer;
};

class GraphicsShader : public Shader
{
public:
    GraphicsShader(FluidSimulation* sim);

    // Draw the given texture.
    // frame        Frame to use.
    // dr           GraphicsDispatchRecord instance to use for setting up the pipeline.
    void Dispatch(const PerFrame& frame, const std::unique_ptr<GraphicsDispatchRecord>& dr);

    // Create a dispatch record to execute this shader instance.
    // image    Texture to draw.
    // coord    Normalized coordinate where to draw the texture.
    std::unique_ptr<GraphicsDispatchRecord> GetDR(Texture* image, ppx::float2 coord)
    {
        return std::make_unique<GraphicsDispatchRecord>(this, image, coord, mResolution);
    }

    GraphicsResources* GetResources() const { return mResources; }

private:
    ppx::grfx::GraphicsPipelinePtr mPipeline;
    GraphicsResources*             mResources;
    ppx::uint2                     mResolution;
};

} // namespace FluidSim

std::ostream& operator<<(std::ostream& os, const FluidSim::ScalarInput& i);
std::ostream& operator<<(std::ostream& os, const FluidSim::Texture& i);

#endif // FLUID_SIMULATION_SHADERS_H
