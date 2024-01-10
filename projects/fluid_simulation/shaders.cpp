// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "shaders.h"
#include "sim.h"

#include "ppx/config.h"
#include "ppx/graphics_util.h"
#include "ppx/math_config.h"

std::ostream& operator<<(std::ostream& os, const FluidSim::ScalarInput& i)
{
    os << "texelSize           [" << offsetof(FluidSim::ScalarInput, texelSize) << "]: " << i.texelSize << "\n";
    os << "coordinate:         [" << offsetof(FluidSim::ScalarInput, coordinate) << "]: " << i.coordinate << "\n";
    os << "color:              [" << offsetof(FluidSim::ScalarInput, color) << "]: " << i.color << "\n";
    os << "curve:              [" << offsetof(FluidSim::ScalarInput, curve) << "]: " << i.curve << "\n";
    os << "intensity:          [" << offsetof(FluidSim::ScalarInput, intensity) << "]: " << i.intensity << "\n";
    os << "ditherScale:        [" << offsetof(FluidSim::ScalarInput, ditherScale) << "]: " << i.ditherScale << "\n";
    os << "dyeTexelSize:       [" << offsetof(FluidSim::ScalarInput, dyeTexelSize) << "]: " << i.dyeTexelSize << "\n";
    os << "threshold:          [" << offsetof(FluidSim::ScalarInput, threshold) << "]: " << i.threshold << "\n";
    os << "aspectRatio:        [" << offsetof(FluidSim::ScalarInput, aspectRatio) << "]: " << i.aspectRatio << "\n";
    os << "clearValue:         [" << offsetof(FluidSim::ScalarInput, clearValue) << "]: " << i.clearValue << "\n";
    os << "dissipation:        [" << offsetof(FluidSim::ScalarInput, dissipation) << "]: " << i.dissipation << "\n";
    os << "dt:                 [" << offsetof(FluidSim::ScalarInput, dt) << "]: " << i.dt << "\n";
    os << "radius:             [" << offsetof(FluidSim::ScalarInput, radius) << "]: " << i.radius << "\n";
    os << "weight:             [" << offsetof(FluidSim::ScalarInput, weight) << "]: " << i.weight << "\n";
    os << "curl:               [" << offsetof(FluidSim::ScalarInput, curl) << "]: " << i.curl << "\n";
    os << "normalizationScale: [" << offsetof(FluidSim::ScalarInput, normalizationScale) << "]: " << i.normalizationScale << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const FluidSim::SimulationGrid& i)
{
    os << "Grid '" << i.GetName() << "' [size:" << i.GetWidth() << " x " << i.GetHeight() << "] ";
    return os;
}

namespace FluidSim {

SimulationGrid::SimulationGrid(const std::string& name, uint32_t width, uint32_t height, ppx::Bitmap::Format format)
    : mName(name)
{
    FluidSimulationApp*            pApp           = FluidSimulationApp::GetThisApp();
    ppx::Bitmap                    bitmap         = ppx::Bitmap::Create(width, height, format);
    ppx::grfx_util::TextureOptions textureOptions = ppx::grfx_util::TextureOptions().InitialState(ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE).AdditionalUsage(ppx::grfx::IMAGE_USAGE_STORAGE).MipLevelCount(1);
    PPX_CHECKED_CALL(ppx::grfx_util::CreateTextureFromBitmap(pApp->GetGraphicsQueue(), &bitmap, &mTexture, textureOptions));
    SetupGrid();
}

SimulationGrid::SimulationGrid(const std::string& textureFile)
    : mName(textureFile)
{
    FluidSimulationApp*            pApp    = FluidSimulationApp::GetThisApp();
    ppx::grfx_util::TextureOptions options = ppx::grfx_util::TextureOptions().AdditionalUsage(ppx::grfx::IMAGE_USAGE_STORAGE).MipLevelCount(1);
    PPX_CHECKED_CALL(ppx::grfx_util::CreateTextureFromFile(pApp->GetGraphicsQueue(), pApp->GetAssetPath(textureFile), &mTexture, options));
    SetupGrid();
}

void SimulationGrid::SetupGrid()
{
    FluidSimulationApp* pApp = FluidSimulationApp::GetThisApp();

    // Allocate and setup a descriptor set for rendering this grid.
    PPX_CHECKED_CALL(pApp->GetDevice()->AllocateDescriptorSet(pApp->GetDescriptorPool(), pApp->GetGraphicsDescriptorSetLayout(), &mDescriptorSet));
    PPX_CHECKED_CALL(mDescriptorSet->UpdateSampledImage(0, 0, mTexture));
    PPX_CHECKED_CALL(mDescriptorSet->UpdateSampler(1, 0, pApp->GetRepeatSampler()));

    // Create a vertex and geometry data buffer.
    // clang-format off
    std::vector<float> vertexData = {
        // Texture position     // Texture sampling coordinates
        0.0f, 0.0f, 0.0f,       0.0f, 0.0f, // A --> Upper left.
        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, // B --> Bottom left.
        0.0f, 0.0f, 0.0f,       1.0f, 1.0f, // C --> Bottom right.

        0.0f, 0.0f, 0.0f,       0.0f, 0.0f, // A --> Upper left.
        0.0f, 0.0f, 0.0f,       1.0f, 1.0f, // C --> Bottom right.
        0.0f, 0.0f, 0.0f,       1.0f, 0.0f, // D --> Top right.
    };
    // clang-format on

    ppx::grfx::BufferCreateInfo bci  = {};
    bci.size                         = ppx::SizeInBytesU32(vertexData);
    bci.usageFlags.bits.vertexBuffer = true;
    bci.memoryUsage                  = ppx::grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(pApp->GetDevice()->CreateBuffer(&bci, &mVertexBuffer));
}

void SimulationGrid::Draw(const PerFrame& frame, ppx::float2 coord)
{
    FluidSimulationApp* pApp = FluidSimulationApp::GetThisApp();

    // Normalize image dimensions.
    ppx::float2 normDim = GetNormalizedSize(ppx::uint2(pApp->GetWindowWidth(), pApp->GetWindowHeight()));

    // Compute the vertices for the grid position.
    ppx::float2 va = coord;
    ppx::float2 vb = ppx::float2(coord.x, coord.y - normDim.y);
    ppx::float2 vc = ppx::float2(coord.x + normDim.x, coord.y - normDim.y);
    ppx::float2 vd = ppx::float2(coord.x + normDim.x, coord.y);

    // Initialize vertex and geometry data.
    // clang-format off
    std::vector<float> vertexData = {
        // Texture position     // Texture sampling coordinates
        va.x, va.y, 0.0f,       0.0f, 0.0f, // A --> Upper left.
        vb.x, vb.y, 0.0f,       0.0f, 1.0f, // B --> Bottom left.
        vc.x, vc.y, 0.0f,       1.0f, 1.0f, // C --> Bottom right.

        va.x, va.y, 0.0f,       0.0f, 0.0f, // A --> Upper left.
        vc.x, vc.y, 0.0f,       1.0f, 1.0f, // C --> Bottom right.
        vd.x, vd.y, 0.0f,       1.0f, 0.0f, // D --> Top right.
    };
    // clang-format on

    void* pAddr = nullptr;
    PPX_CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
    memcpy(pAddr, vertexData.data(), ppx::SizeInBytesU32(vertexData));
    mVertexBuffer->UnmapMemory();

    frame.cmd->BindGraphicsDescriptorSets(pApp->GetGraphicsPipelineInterface(), 1, &mDescriptorSet);
    frame.cmd->BindGraphicsPipeline(pApp->GetGraphicsPipeline());
    frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &pApp->GetGraphicsVertexBinding()->GetStride());
    frame.cmd->Draw(6, 1, 0, 0);
}

ComputeShader::ComputeShader(const std::string& shaderFile, const std::vector<uint32_t>& gridBindingSlots)
    : mShaderFile(shaderFile), mGridBindingSlots(gridBindingSlots)
{
    FluidSimulationApp* pApp = FluidSimulationApp::GetThisApp();

    std::vector<char> bytecode = pApp->LoadShader("fluid_simulation/shaders", mShaderFile + ".cs");
    PPX_ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
    ppx::grfx::ShaderModuleCreateInfo sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    ppx::grfx::ShaderModulePtr        cs;
    PPX_CHECKED_CALL(pApp->GetDevice()->CreateShaderModule(&sci, &cs));

    ppx::grfx::ComputePipelineCreateInfo pci = {};
    pci.CS                                   = {cs.Get(), "csmain"};
    pci.pPipelineInterface                   = pApp->GetComputePipelineInterface();
    PPX_CHECKED_CALL(pApp->GetDevice()->CreateComputePipeline(&pci, &mPipeline));
}

ComputeDispatchData::ComputeDispatchData()
{
    FluidSimulationApp* pApp = FluidSimulationApp::GetThisApp();

    // Allocate a new uniform buffer.
    ppx::grfx::BufferCreateInfo bci   = {};
    bci.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
    bci.usageFlags.bits.uniformBuffer = true;
    bci.memoryUsage                   = ppx::grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(pApp->GetDevice()->CreateBuffer(&bci, &mUniformBuffer));

    // Allocate a new descriptor set.
    PPX_CHECKED_CALL(pApp->GetDevice()->AllocateDescriptorSet(pApp->GetDescriptorPool(), pApp->GetComputeDescriptorSetLayout(), &mDescriptorSet));
    PPX_CHECKED_CALL(mDescriptorSet->UpdateUniformBuffer(0, 0, mUniformBuffer));
    PPX_CHECKED_CALL(mDescriptorSet->UpdateSampler(1, 0, pApp->GetClampSampler()));
    PPX_CHECKED_CALL(mDescriptorSet->UpdateSampler(12, 0, pApp->GetRepeatSampler()));
}

void ComputeShader::Dispatch(PerFrame* pFrame, const std::vector<SimulationGrid*>& grids, ScalarInput* si)
{
    FluidSimulationApp* pApp = FluidSimulationApp::GetThisApp();

    // Decode the input grids. This assumes, but cannot verify, the following:
    //
    // - Binding slots and input grids are given in the appropriate order: mGridBindingSlots[i] is the binding
    //   slot for grids[i].
    //
    // It verifies the following:
    //
    // - The very last element of 'grids' is always the output grid, which must be bound to
    //   kOutputBindingSlot.
    // - Both lists 'grids' and 'mGridBindingSlots' must be the same length.
    PPX_ASSERT_MSG(grids.size() == mGridBindingSlots.size(), "Dispatch signature mismatch for shader " << mShaderFile);

    SimulationGrid* output     = grids.back();
    uint32_t        outputSlot = mGridBindingSlots.back();
    PPX_ASSERT_MSG(outputSlot == kOutputBindingSlot, "The last element of the grid binding slots should be binding slot " << kOutputBindingSlot);

    // Retrieve the dispatch data for this invocation.
    ComputeDispatchData* dd = pFrame->GetDispatchData();

    // Bind all the input grids.
    for (auto i = 0; i < mGridBindingSlots.size() - 1; i++) {
        PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(mGridBindingSlots[i], 0, grids[i]->GetTexture()));
    }

    // Bind the output grid.
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateStorageImage(kOutputBindingSlot, 0, output->GetTexture()));

    // Compute the normalization scale based on output's resolution.
    si->normalizationScale = ppx::float2(1.0f / output->GetWidth(), 1.0f / output->GetHeight());

    // Copy the input data into the uniform buffer.
    void* pData = nullptr;
    PPX_CHECKED_CALL(dd->mUniformBuffer->MapMemory(0, &pData));
    memcpy(pData, si, sizeof(*si));
    dd->mUniformBuffer->UnmapMemory();

    // Queue the dispatch operation.
    ppx::uint3 dispatchSize = ppx::uint3(output->GetWidth(), output->GetHeight(), 1);
    pFrame->cmd->TransitionImageLayout(output->GetImage(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS);
    pFrame->cmd->BindComputeDescriptorSets(pApp->GetComputePipelineInterface(), 1, &dd->mDescriptorSet);
    pFrame->cmd->BindComputePipeline(mPipeline);
    pFrame->cmd->Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
    pFrame->cmd->TransitionImageLayout(output->GetImage(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE);

    // Update the dispatch ID for the next dispatch operation.
    pFrame->dispatchID++;
}

} // namespace FluidSim
