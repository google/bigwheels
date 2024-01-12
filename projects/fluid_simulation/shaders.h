// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FLUID_SIMULATION_SHADERS_H
#define FLUID_SIMULATION_SHADERS_H

#include "ppx/bitmap.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_texture.h"
#include "ppx/math_config.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

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

// Filter option bitmasks to use in ScalarInput::filterOptions.
//
// This MUST match the constants defined in assets/fluid_simulation/shaders/config.hlsli.
const uint32_t kAdvectionManualFiltering = 1 << 0;
const uint32_t kDisplayShading           = 1 << 1;
const uint32_t kDisplayBloom             = 1 << 2;
const uint32_t kDisplaySunrays           = 1 << 3;

// Scalar inputs for the filter programs.
//
// This needs to be 16-bit aligned to be copied into a uniform buffer.
//
// NOTE: Fields are organized so that they are packed into 4 word component vectors
// to match the HLSL packing rules (https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
//
// This MUST match the CSInputs structure in assets/fluid_simulation/shaders/config.hlsli.
struct alignas(16) ScalarInput
{
    ppx::float2 texelSize          = ppx::float2();
    ppx::float2 coordinate         = ppx::float2();
    ppx::float4 color              = ppx::float4();
    ppx::float3 curve              = ppx::float3();
    float       intensity          = .0f;
    ppx::float2 ditherScale        = ppx::float2();
    ppx::float2 dyeTexelSize       = ppx::float2();
    float       threshold          = .0f;
    float       aspectRatio        = .0f;
    float       clearValue         = .0f;
    float       dissipation        = .0f;
    float       dt                 = .0f;
    float       radius             = .0f;
    float       weight             = .0f;
    float       curl               = .0f;
    ppx::float2 normalizationScale = ppx::float2();
    uint32_t    filterOptions      = 0;
};

class ComputeShader
{
public:
    ComputeShader(const std::string& shaderFile, const std::vector<uint32_t>& gridBindingSlots);

    const std::string& GetName() const { return mShaderFile; }

    // Add a dispatch call to this shader in the given frame's command buffer.
    //
    // frame    The frame holding the command buffer to schedule into.
    // grids    A list of grids to be bound to the descriptor set. This list is assumed to be
    //              in the same order as the list of binding slots (mGridBindingSlots) set during
    //              construction.
    // pSI      A pointer to the scalar inputs to the compute shader.
    void Dispatch(PerFrame* pFrame, const std::vector<SimulationGrid*>& grids, ScalarInput* pSI);

private:
    ppx::grfx::ComputePipelinePtr mPipeline;
    std::string                   mShaderFile;
    std::vector<uint32_t>         mGridBindingSlots;
};

} // namespace FluidSim

std::ostream& operator<<(std::ostream& os, const FluidSim::ScalarInput& i);
std::ostream& operator<<(std::ostream& os, const FluidSim::SimulationGrid& i);

#endif // FLUID_SIMULATION_SHADERS_H
