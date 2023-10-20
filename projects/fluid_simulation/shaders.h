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
#include "ppx/grfx/grfx_texture.h"
#include "ppx/math_config.h"

#include <iostream>
#include <unordered_map>

namespace FluidSim {

// Command buffer, dispatch data for compute shaders and frame synchronization.
struct ComputeDispatchData
{
    ComputeDispatchData();

    ppx::grfx::BufferPtr        mUniformBuffer = nullptr;
    ppx::grfx::DescriptorSetPtr mDescriptorSet = nullptr;
};

struct PerFrame
{
    ppx::grfx::CommandBufferPtr      cmd                     = nullptr;
    ppx::grfx::SemaphorePtr          imageAcquiredSemaphore  = nullptr;
    ppx::grfx::FencePtr              imageAcquiredFence      = nullptr;
    ppx::grfx::SemaphorePtr          renderCompleteSemaphore = nullptr;
    ppx::grfx::FencePtr              renderCompleteFence     = nullptr;
    uint32_t                         dispatchID              = 0;
    std::vector<ComputeDispatchData> mDispatchData;

    ComputeDispatchData* GetDispatchData()
    {
        if (dispatchID >= mDispatchData.size()) {
            mDispatchData.push_back(ComputeDispatchData());
        }
        return &mDispatchData[dispatchID];
    }
};

// A fluid simulation grid is used as a discrete representation of
// the continuous physical properties (e.g., velocity, density, pressure)
// within the simulation domain.
class SimulationGrid
{
public:
    SimulationGrid(const std::string& name, uint32_t width, uint32_t height, ppx::Bitmap::Format format);
    SimulationGrid(const std::string& textureFile);

    uint32_t              GetHeight() const { return mTexture->GetHeight(); }
    const std::string&    GetName() const { return mName; }
    ppx::grfx::BufferPtr  GetVertexBuffer() const { return mVertexBuffer; }
    uint32_t              GetWidth() const { return mTexture->GetWidth(); }
    ppx::grfx::TexturePtr GetTexture() const { return mTexture; }
    ppx::grfx::ImagePtr   GetImage() const { return mTexture->GetImage(); }
    void                  SetName(const std::string& name) { mName = name; }

    // Render this grid.
    void Draw(const PerFrame& frame, ppx::float2 coord);

    // Compute and return the size of the texture normalized to the resolution.
    // given in pixels.  This maps the size of the texture to the normalized
    // coordinates ([-1, 1], [-1, 1]).
    ppx::float2 GetNormalizedSize(ppx::uint2 resolution) const
    {
        return ppx::float2(GetWidth() * 2.0f / static_cast<float>(resolution.x), GetHeight() * 2.0f / static_cast<float>(resolution.y));
    }

    ppx::float2 GetTexelSize() const
    {
        return ppx::float2(1.0f / static_cast<float>(GetWidth()), 1.0f / static_cast<float>(GetHeight()));
    }

    ppx::float2 GetDitherScale(uint32_t width, uint32_t height) const
    {
        return ppx::float2(static_cast<float>(width) / static_cast<float>(GetWidth()), static_cast<float>(height) / static_cast<float>(GetHeight()));
    }

private:
    void SetupGrid();

    ppx::grfx::TexturePtr       mTexture       = nullptr;
    std::string                 mName          = "unknown";
    ppx::grfx::BufferPtr        mVertexBuffer  = nullptr;
    ppx::grfx::DescriptorSetPtr mDescriptorSet = nullptr;
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
    ScalarInput(SimulationGrid* output)
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

class ComputeShader
{
public:
    ComputeShader(const std::string& shaderFile);
    const std::string& GetName() const { return mShaderFile; }

protected:
    // Add a dispatch call to this shader in the given frame's command buffer.
    //
    // frame    The frame holding the command buffer to schedule into.
    // output   The grid where the compute shader will emit its output to.
    // ds       The descriptor set to use with all the input and output grinds bound.
    // si       The scalar inputs to the compute shader.
    void Dispatch(PerFrame* frame, SimulationGrid* output, ComputeDispatchData* ds, const ScalarInput& si);

private:
    ppx::grfx::ComputePipelinePtr mPipeline;
    std::string                   mShaderFile;
};

class AdvectionShader : public ComputeShader
{
public:
    AdvectionShader()
        : ComputeShader("advection") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* uSource, SimulationGrid* output, float delta, float dissipation, ppx::float2 texelSize, ppx::float2 dyeTexelSize);
};

class BloomBlurShader : public ComputeShader
{
public:
    BloomBlurShader()
        : ComputeShader("bloom_blur") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize);
};

class BloomBlurAdditiveShader : public ComputeShader
{
public:
    BloomBlurAdditiveShader()
        : ComputeShader("bloom_blur_additive") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize);
};

class BloomFinalShader : public ComputeShader
{
public:
    BloomFinalShader()
        : ComputeShader("bloom_final") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize, float intensity);
};

class BloomPrefilterShader : public ComputeShader
{
public:
    BloomPrefilterShader()
        : ComputeShader("bloom_prefilter") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float3 curve, float threshold);
};

class BlurShader : public ComputeShader
{
public:
    BlurShader()
        : ComputeShader("blur") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize);
};

class ClearShader : public ComputeShader
{
public:
    ClearShader()
        : ComputeShader("clear") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, float clearValue);
};

class ColorShader : public ComputeShader
{
public:
    ColorShader()
        : ComputeShader("color") {}

    void Dispatch(PerFrame* frame, SimulationGrid* output, ppx::float4 color);
};

class CurlShader : public ComputeShader
{
public:
    CurlShader()
        : ComputeShader("curl") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize);
};

class DisplayShader : public ComputeShader
{
public:
    DisplayShader()
        : ComputeShader("display") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* uBloom, SimulationGrid* uSunrays, SimulationGrid* uDithering, SimulationGrid* output, ppx::float2 texelSize, ppx::float2 ditherScale);
};

class DivergenceShader : public ComputeShader
{
public:
    DivergenceShader()
        : ComputeShader("divergence") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize);
};

class GradientSubtractShader : public ComputeShader
{
public:
    GradientSubtractShader()
        : ComputeShader("gradient_subtract") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uPressure, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize);
};

class PressureShader : public ComputeShader
{
public:
    PressureShader()
        : ComputeShader("pressure") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uPressure, SimulationGrid* uDivergence, SimulationGrid* output, ppx::float2 texelSize);
};

class SplatShader : public ComputeShader
{
public:
    SplatShader()
        : ComputeShader("splat") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 coordinate, float aspectRatio, float radius, ppx::float4 color);
};

class SunraysMaskShader : public ComputeShader
{
public:
    SunraysMaskShader()
        : ComputeShader("sunrays_mask") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output);
};

class SunraysShader : public ComputeShader
{
public:
    SunraysShader()
        : ComputeShader("sunrays") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, float weight);
};

class VorticityShader : public ComputeShader
{
public:
    VorticityShader()
        : ComputeShader("vorticity") {}

    void Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* uCurl, SimulationGrid* output, ppx::float2 texelSize, float curl, float delta);
};

} // namespace FluidSim

std::ostream& operator<<(std::ostream& os, const FluidSim::ScalarInput& i);
std::ostream& operator<<(std::ostream& os, const FluidSim::SimulationGrid& i);

#endif // FLUID_SIMULATION_SHADERS_H
