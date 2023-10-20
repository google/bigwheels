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

ComputeShader::ComputeShader(const std::string& shaderFile)
    : mShaderFile(shaderFile)
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

void ComputeShader::Dispatch(PerFrame* frame, SimulationGrid* output, ComputeDispatchData* dd, const ScalarInput& si)
{
    FluidSimulationApp* pApp         = FluidSimulationApp::GetThisApp();
    ppx::uint3          dispatchSize = ppx::uint3(output->GetWidth(), output->GetHeight(), 1);

    PPX_LOG_DEBUG("Scheduling compute shader '" << mShaderFile << ".cs' (" << dispatchSize << ")\n");

    // Copy the input data into the uniform buffer.
    void* pData = nullptr;
    PPX_CHECKED_CALL(dd->mUniformBuffer->MapMemory(0, &pData));
    memcpy(pData, &si, sizeof(si));
    dd->mUniformBuffer->UnmapMemory();

    // Bind the output grid.
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateStorageImage(kOutputBindingSlot, 0, output->GetTexture()));

    frame->cmd->TransitionImageLayout(output->GetImage(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS);
    frame->cmd->BindComputeDescriptorSets(pApp->GetComputePipelineInterface(), 1, &dd->mDescriptorSet);
    frame->cmd->BindComputePipeline(mPipeline);
    frame->cmd->Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
    frame->cmd->TransitionImageLayout(output->GetImage(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE);

    // Update the dispatch ID for the next dispatch operation.
    frame->dispatchID++;
}

void AdvectionShader::Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* uSource, SimulationGrid* output, float delta, float dissipation, ppx::float2 texelSize, ppx::float2 dyeTexelSize)
{
    ScalarInput si(output);
    si.texelSize    = texelSize;
    si.dyeTexelSize = dyeTexelSize;
    si.dissipation  = dissipation;
    si.dt           = delta;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUVelocityBindingSlot, 0, uVelocity->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUSourceBindingSlot, 0, uSource->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void BloomBlurShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void BloomBlurAdditiveShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void BloomFinalShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize, float intensity)
{
    ScalarInput si(output);
    si.texelSize = texelSize;
    si.intensity = intensity;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void BloomPrefilterShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float3 curve, float threshold)
{
    ScalarInput si(output);
    si.curve     = curve;
    si.threshold = threshold;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void BlurShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void ClearShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, float clearValue)
{
    ScalarInput si(output);
    si.clearValue = clearValue;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void ColorShader::Dispatch(PerFrame* frame, SimulationGrid* output, ppx::float4 color)
{
    ScalarInput si(output);
    si.color = color;

    ComputeDispatchData* dd = frame->GetDispatchData();

    ComputeShader::Dispatch(frame, output, dd, si);
}

void CurlShader::Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUVelocityBindingSlot, 0, uVelocity->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void DisplayShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* uBloom, SimulationGrid* uSunrays, SimulationGrid* uDithering, SimulationGrid* output, ppx::float2 texelSize, ppx::float2 ditherScale)
{
    ScalarInput si(output);
    si.texelSize   = texelSize;
    si.ditherScale = ditherScale;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUBloomBindingSlot, 0, uBloom->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUSunraysBindingSlot, 0, uSunrays->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUDitheringBindingSlot, 0, uDithering->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void DivergenceShader::Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUVelocityBindingSlot, 0, uVelocity->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void GradientSubtractShader::Dispatch(PerFrame* frame, SimulationGrid* uPressure, SimulationGrid* uVelocity, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUPressureBindingSlot, 0, uPressure->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUVelocityBindingSlot, 0, uVelocity->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void PressureShader::Dispatch(PerFrame* frame, SimulationGrid* uPressure, SimulationGrid* uDivergence, SimulationGrid* output, ppx::float2 texelSize)
{
    ScalarInput si(output);
    si.texelSize = texelSize;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUPressureBindingSlot, 0, uPressure->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUDivergenceBindingSlot, 0, uDivergence->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void SplatShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, ppx::float2 coordinate, float aspectRatio, float radius, ppx::float4 color)
{
    ScalarInput si(output);
    si.coordinate  = coordinate;
    si.aspectRatio = aspectRatio;
    si.radius      = radius;
    si.color       = color;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void SunraysMaskShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output)
{
    ScalarInput si(output);

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void SunraysShader::Dispatch(PerFrame* frame, SimulationGrid* uTexture, SimulationGrid* output, float weight)
{
    ScalarInput si(output);
    si.weight = weight;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUTextureBindingSlot, 0, uTexture->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

void VorticityShader::Dispatch(PerFrame* frame, SimulationGrid* uVelocity, SimulationGrid* uCurl, SimulationGrid* output, ppx::float2 texelSize, float curl, float delta)
{
    ScalarInput si(output);
    si.curl = curl;
    si.dt   = delta;

    ComputeDispatchData* dd = frame->GetDispatchData();
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUVelocityBindingSlot, 0, uVelocity->GetTexture()));
    PPX_CHECKED_CALL(dd->mDescriptorSet->UpdateSampledImage(kUCurlBindingSlot, 0, uCurl->GetTexture()));

    ComputeShader::Dispatch(frame, output, dd, si);
}

} // namespace FluidSim
