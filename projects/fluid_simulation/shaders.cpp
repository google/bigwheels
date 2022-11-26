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

namespace FluidSim {

std::ostream& operator<<(std::ostream& os, const ppx::float2& i)
{
    os << "(" << i.x << ", " << i.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ppx::float3& i)
{
    os << "(" << i.x << ", " << i.y << ", " << i.z << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ppx::float4& i)
{
    os << "(" << i.r << ", " << i.g << ", " << i.b << ", " << i.a << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ScalarInput& i)
{
    os << "texelSize           [" << offsetof(ScalarInput, texelSize) << "]: " << i.texelSize << "\n";
    os << "coordinate:         [" << offsetof(ScalarInput, coordinate) << "]: " << i.coordinate << "\n";
    os << "color:              [" << offsetof(ScalarInput, color) << "]: " << i.color << "\n";
    os << "curve:              [" << offsetof(ScalarInput, curve) << "]: " << i.curve << "\n";
    os << "intensity:          [" << offsetof(ScalarInput, intensity) << "]: " << i.intensity << "\n";
    os << "ditherScale:        [" << offsetof(ScalarInput, ditherScale) << "]: " << i.ditherScale << "\n";
    os << "dyeTexelSize:       [" << offsetof(ScalarInput, dyeTexelSize) << "]: " << i.dyeTexelSize << "\n";
    os << "threshold:          [" << offsetof(ScalarInput, threshold) << "]: " << i.threshold << "\n";
    os << "aspectRatio:        [" << offsetof(ScalarInput, aspectRatio) << "]: " << i.aspectRatio << "\n";
    os << "clearValue:         [" << offsetof(ScalarInput, clearValue) << "]: " << i.clearValue << "\n";
    os << "dissipation:        [" << offsetof(ScalarInput, dissipation) << "]: " << i.dissipation << "\n";
    os << "dt:                 [" << offsetof(ScalarInput, dt) << "]: " << i.dt << "\n";
    os << "radius:             [" << offsetof(ScalarInput, radius) << "]: " << i.radius << "\n";
    os << "weight:             [" << offsetof(ScalarInput, weight) << "]: " << i.weight << "\n";
    os << "curl:               [" << offsetof(ScalarInput, curl) << "]: " << i.curl << "\n";
    os << "normalizationScale: [" << offsetof(ScalarInput, normalizationScale) << "]: " << i.normalizationScale << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Texture& i)
{
    os << ((!i.GetName().empty()) ? i.GetName() : "UNKNOWN");
    os << " [size: " << i.GetWidth() << "x" << i.GetHeight() << ", texel size: " << i.GetTexelSize() << "]";
    return os;
}

Texture::Texture(FluidSimulation* sim, const std::string& name, uint32_t width, uint32_t height, ppx::grfx::Format format)
    : mSim(sim), mName(name)
{
    ppx::grfx::ImageCreateInfo ici  = {};
    ici.type                        = ppx::grfx::IMAGE_TYPE_2D;
    ici.width                       = width;
    ici.height                      = height;
    ici.depth                       = 1;
    ici.format                      = format;
    ici.sampleCount                 = ppx::grfx::SAMPLE_COUNT_1;
    ici.mipLevelCount               = 1;
    ici.arrayLayerCount             = 1;
    ici.usageFlags.bits.transferDst = true;
    ici.usageFlags.bits.transferSrc = true;
    ici.usageFlags.bits.sampled     = true;
    ici.usageFlags.bits.storage     = true;
    ici.memoryUsage                 = ppx::grfx::MEMORY_USAGE_GPU_ONLY;
    ici.initialState                = ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateImage(&ici, &mTexture));

    ppx::grfx::SampledImageViewCreateInfo vci = ppx::grfx::SampledImageViewCreateInfo::GuessFromImage(mTexture);
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampledImageView(&vci, &mSampledView));

    ppx::grfx::StorageImageViewCreateInfo storageVCI = ppx::grfx::StorageImageViewCreateInfo::GuessFromImage(mTexture);
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateStorageImageView(&storageVCI, &mStorageView));

    mSim->AddTextureToInitialize(this);
}

Texture::Texture(FluidSimulation* sim, const std::string& textureFile)
    : mSim(sim), mName(textureFile)
{
    ppx::grfx_util::ImageOptions options = ppx::grfx_util::ImageOptions().AdditionalUsage(ppx::grfx::IMAGE_USAGE_STORAGE).MipLevelCount(1);
    PPX_CHECKED_CALL(ppx::grfx_util::CreateImageFromFile(GetApp()->GetDevice()->GetGraphicsQueue(), GetApp()->GetAssetPath(textureFile), &mTexture, options, false));

    ppx::grfx::SampledImageViewCreateInfo vci = ppx::grfx::SampledImageViewCreateInfo::GuessFromImage(mTexture);
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampledImageView(&vci, &mSampledView));

    ppx::grfx::StorageImageViewCreateInfo storageVCI = ppx::grfx::StorageImageViewCreateInfo::GuessFromImage(mTexture);
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateStorageImageView(&storageVCI, &mStorageView));
}

ProjApp* Texture::GetApp() const
{
    return mSim->GetApp();
}

ProjApp* Shader::GetApp() const
{
    return mSim->GetApp();
}

GraphicsResources* Shader::GetGraphicsResources() const
{
    return mSim->GetGraphicsResources();
}
ComputeResources* Shader::GetComputeResources() const
{
    return mSim->GetComputeResources();
}

ComputeShader::ComputeShader(FluidSimulation* sim, const std::string& shaderFile)
    : Shader(sim, shaderFile)
{
    ppx::grfx::ShaderModulePtr cs;
    std::vector<char>          bytecode = GetApp()->LoadShader(GetApp()->GetAssetPath("fluid_simulation/shaders"), mShaderFile + ".cs");
    PPX_ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
    ppx::grfx::ShaderModuleCreateInfo sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateShaderModule(&sci, &cs));

    ppx::grfx::ComputePipelineCreateInfo pci = {};
    pci.CS                                   = {cs.Get(), "csmain"};
    pci.pPipelineInterface                   = GetComputeResources()->mPipelineInterface;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateComputePipeline(&pci, &mPipeline));
}

ComputeDispatchRecord::ComputeDispatchRecord(ComputeShader* cs, Texture* output, const ScalarInput& si)
    : mShader(cs), mOutput(output)
{
    // Allocate a new descriptor set.
    PPX_CHECKED_CALL(mShader->GetApp()->GetDevice()->AllocateDescriptorSet(mShader->GetSim()->GetDescriptorPool(), mShader->GetComputeResources()->mDescriptorSetLayout, &mDescriptorSet));

    // Allocate a new uniform buffer and initialize it with input data.
    ppx::grfx::BufferCreateInfo bci   = {};
    bci.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
    bci.usageFlags.bits.uniformBuffer = true;
    bci.memoryUsage                   = ppx::grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(mShader->GetApp()->GetDevice()->CreateBuffer(&bci, &mUniformBuffer));

    void* pData = nullptr;
    PPX_CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
    memcpy(pData, &si, sizeof(si));
    mUniformBuffer->UnmapMemory();

    const size_t               numBindings = 2;
    ppx::grfx::WriteDescriptor write[numBindings];

    write[0].binding      = 0;
    write[0].type         = ppx::grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write[0].bufferOffset = 0;
    write[0].bufferRange  = PPX_WHOLE_SIZE;
    write[0].pBuffer      = mUniformBuffer;

    write[1].binding  = 1;
    write[1].type     = ppx::grfx::DESCRIPTOR_TYPE_SAMPLER;
    write[1].pSampler = mShader->GetComputeResources()->mSampler;

    PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(numBindings, write));
}

void ComputeDispatchRecord::FreeResources()
{
    PPX_LOG_DEBUG("Freeing up uniform buffer and descriptor set for " << mShader->GetName());
    mShader->GetApp()->GetDevice()->DestroyBuffer(mUniformBuffer);
    mShader->GetApp()->GetDevice()->FreeDescriptorSet(mDescriptorSet);
}

void ComputeDispatchRecord::BindSamplingTexture(Texture* texture, uint32_t bindingSlot)
{
    ppx::grfx::WriteDescriptor write;
    write.binding    = bindingSlot;
    write.type       = ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.pImageView = texture->GetSampledView();
    PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
}

void ComputeDispatchRecord::BindOutputTexture(Texture* texture, uint32_t bindingSlot)
{
    ppx::grfx::WriteDescriptor write;
    write.binding    = bindingSlot;
    write.type       = ppx::grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageView = texture->GetStorageView();
    PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
}

void ComputeShader::Dispatch(const PerFrame& frame, ComputeDispatchRecord& dr)
{
    ppx::float3 dispatchSize = ppx::float3(dr.mOutput->GetWidth(), dr.mOutput->GetHeight(), 1);

    PPX_LOG_DEBUG("Running compute shader '" << mShaderFile << ".cs' (" << dispatchSize.x << ", " << dispatchSize.y << ", " << dispatchSize.z << ")\n");

    frame.cmd->TransitionImageLayout(dr.mOutput->GetImagePtr(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS);
    frame.cmd->BindComputeDescriptorSets(dr.mShader->GetComputeResources()->mPipelineInterface, 1, &dr.mDescriptorSet);
    frame.cmd->BindComputePipeline(mPipeline);
    frame.cmd->Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
    frame.cmd->TransitionImageLayout(dr.mOutput->GetImagePtr(), PPX_ALL_SUBRESOURCES, ppx::grfx::RESOURCE_STATE_UNORDERED_ACCESS, ppx::grfx::RESOURCE_STATE_SHADER_RESOURCE);
}

GraphicsDispatchRecord::GraphicsDispatchRecord(GraphicShader* gs, Texture* image)
    : mShader(gs), mImage(image)
{
    // Allocate a new descriptor set.
    PPX_CHECKED_CALL(mShader->GetApp()->GetDevice()->AllocateDescriptorSet(mShader->GetSim()->GetDescriptorPool(), mShader->GetGraphicsResources()->mDescriptorSetLayout, &mDescriptorSet));

    // Update descriptors.
    const size_t               numBindings = 3;
    ppx::grfx::WriteDescriptor write[numBindings];

    write[0].binding      = 0;
    write[0].type         = ppx::grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write[0].bufferOffset = 0;
    write[0].bufferRange  = PPX_WHOLE_SIZE;
    write[0].pBuffer      = mShader->GetGraphicsResources()->mTransformBuffer;

    write[1].binding    = 1;
    write[1].type       = ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write[1].pImageView = image->GetSampledView();

    write[2].binding  = 2;
    write[2].type     = ppx::grfx::DESCRIPTOR_TYPE_SAMPLER;
    write[2].pSampler = mShader->GetGraphicsResources()->mSampler;

    PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(numBindings, write));

    PPX_LOG_DEBUG("Created graphic descriptor set for " << image);
}

void GraphicsDispatchRecord::FreeResources()
{
    PPX_LOG_DEBUG("Freeing up descriptor set for " << mShader->GetName());
    mShader->GetApp()->GetDevice()->FreeDescriptorSet(mDescriptorSet);
}

GraphicShader::GraphicShader(FluidSimulation* sim)
    : Shader(sim, "Texture")
{
    // Graphic pipeline.
    ppx::grfx::ShaderModulePtr vs;
    std::vector<char>          bytecode = GetApp()->LoadShader(GetApp()->GetAssetPath("basic/shaders"), mShaderFile + ".vs");
    PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
    ppx::grfx::ShaderModuleCreateInfo sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateShaderModule(&sci, &vs));

    ppx::grfx::ShaderModulePtr ps;
    bytecode = GetApp()->LoadShader(GetApp()->GetAssetPath("basic/shaders"), mShaderFile + ".ps");
    PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
    sci = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateShaderModule(&sci, &ps));

    ppx::grfx::GraphicsPipelineCreateInfo2 gpci = {};
    gpci.VS                                     = {vs.Get(), "vsmain"};
    gpci.PS                                     = {ps.Get(), "psmain"};
    gpci.vertexInputState.bindingCount          = 1;
    gpci.vertexInputState.bindings[0]           = GetGraphicsResources()->mVertexBinding;
    gpci.topology                               = ppx::grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpci.polygonMode                            = ppx::grfx::POLYGON_MODE_FILL;
    gpci.cullMode                               = ppx::grfx::CULL_MODE_NONE;
    gpci.frontFace                              = ppx::grfx::FRONT_FACE_CCW;
    gpci.depthReadEnable                        = false;
    gpci.depthWriteEnable                       = false;
    gpci.blendModes[0]                          = ppx::grfx::BLEND_MODE_NONE;
    gpci.outputState.renderTargetCount          = 1;
    gpci.outputState.renderTargetFormats[0]     = GetApp()->GetSwapchain()->GetColorFormat();
    gpci.pPipelineInterface                     = GetGraphicsResources()->mPipelineInterface;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateGraphicsPipeline(&gpci, &mPipeline));
}

void GraphicShader::Dispatch(const PerFrame& frame, uint32_t imageIndex, GraphicsDispatchRecord& dr)
{
    frame.cmd->BindGraphicsDescriptorSets(GetGraphicsResources()->mPipelineInterface, 1, &dr.mDescriptorSet);
    frame.cmd->BindGraphicsPipeline(mPipeline);
    frame.cmd->BindVertexBuffers(1, &GetGraphicsResources()->mVertexBuffer, &GetGraphicsResources()->mVertexBinding.GetStride());
    frame.cmd->Draw(6, 1, 0, 0);
}

} // namespace FluidSim
