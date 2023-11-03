// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GraphicsBenchmarkApp.h"
#include "SphereMesh.h"

#include "ppx/graphics_util.h"

using namespace ppx;

void GraphicsBenchmarkApp::InitKnobs()
{
    const auto& cl_options = GetExtraOptions();
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("vs-shader-index"), "--vs-shader-index flag has been replaced, instead use --vs and specify the name of the vertex shader");
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("ps-shader-index"), "--ps-shader-index flag has been replaced, instead use --ps and specify the name of the pixel shader");

    pKnobVs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vs", 0, kAvailableVsShaders);
    pKnobVs->SetDisplayName("Vertex Shader");
    pKnobVs->SetFlagDescription("Select the vertex shader for the graphics pipeline.");

    pKnobPs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("ps", 0, kAvailablePsShaders);
    pKnobPs->SetDisplayName("Pixel Shader");
    pKnobPs->SetFlagDescription("Select the pixel shader for the graphics pipeline.");

    pKnobLOD = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("LOD", 0, kAvailableLODs);
    pKnobLOD->SetDisplayName("Level of Detail (LOD)");
    pKnobLOD->SetFlagDescription("Select the Level of Detail (LOD) for the sphere mesh.");

    pKnobVbFormat = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vertex-buffer-format", 0, kAvailableVbFormats);
    pKnobVbFormat->SetDisplayName("Vertex Buffer Format");
    pKnobVbFormat->SetFlagDescription("Select the format for the vertex buffer.");

    pKnobVertexAttrLayout = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vertex-attr-layout", 0, kAvailableVertexAttrLayouts);
    pKnobVertexAttrLayout->SetDisplayName("Vertex Attribute Layout");
    pKnobVertexAttrLayout->SetFlagDescription("Select the Vertex Attribute Layout for the graphics pipeline.");

    pSphereInstanceCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sphere-count", /* defaultValue = */ 50, /* minValue = */ 1, kMaxSphereInstanceCount);
    pSphereInstanceCount->SetDisplayName("Sphere Count");
    pSphereInstanceCount->SetFlagDescription("Select the number of spheres to draw on the screen.");

    pDrawCallCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("drawcall-count", /* defaultValue = */ 1, /* minValue = */ 1, kMaxSphereInstanceCount);
    pDrawCallCount->SetDisplayName("DrawCall Count");
    pDrawCallCount->SetFlagDescription("Select the number of draw calls to be used to draw the `sphere-count` spheres.");

    pAlphaBlend = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("alpha-blend", false);
    pAlphaBlend->SetDisplayName("Alpha Blend");
    pAlphaBlend->SetFlagDescription("Set blend mode of the spheres to alpha blending.");

    pDepthTestWrite = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("depth-test-write", true);
    pDepthTestWrite->SetDisplayName("Depth Test & Write");
    pDepthTestWrite->SetFlagDescription("Enable depth test and depth write for spheres (Default: enabled).");

    pEnableSkyBox = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-skybox", true);
    pEnableSkyBox->SetDisplayName("Enable SkyBox");
    pEnableSkyBox->SetFlagDescription("Enable the SkyBox in the scene.");

    pFullscreenQuadsCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("fullscreen-quads-count", /* defaultValue = */ 0, /* minValue = */ 0, kMaxFullscreenQuadsCount);
    pFullscreenQuadsCount->SetDisplayName("Number of Fullscreen Quads");
    pFullscreenQuadsCount->SetFlagDescription("Select the number of fullscreen quads to render.");

    pFullscreenQuadsType = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("fullscreen-quads-type", 0, kFullscreenQuadsTypes);
    pFullscreenQuadsType->SetDisplayName("Type");
    pFullscreenQuadsType->SetFlagDescription("Select the type of the fullscreen quads (see --fullscreen-quads-count).");
    pFullscreenQuadsType->SetIndent(1);

    pFullscreenQuadsColor = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("fullscreen-quads-color", 0, kFullscreenQuadsColors);
    pFullscreenQuadsColor->SetDisplayName("Color");
    pFullscreenQuadsColor->SetFlagDescription("Select the hue for the solid color fullscreen quads (see --fullscreen-quads-count).");
    pFullscreenQuadsColor->SetIndent(2);

    pFullscreenQuadsSingleRenderpass = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("fullscreen-quads-single-renderpass", false);
    pFullscreenQuadsSingleRenderpass->SetDisplayName("Single Renderpass");
    pFullscreenQuadsSingleRenderpass->SetFlagDescription("Render all fullscreen quads (see --fullscreen-quads-count) in a single renderpass.");
    pFullscreenQuadsSingleRenderpass->SetIndent(1);
}

void GraphicsBenchmarkApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "graphics_pipeline";
    settings.enableImGui                = true;
    settings.window.width               = 1920;
    settings.window.height              = 1080;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
}

void GraphicsBenchmarkApp::Setup()
{
    // =====================================================================
    // SCENE (skybox and spheres)
    // =====================================================================

    // Camera
    {
        mCamera.LookAt(mCamera.GetEyePosition(), mCamera.GetTarget());
        mCamera.SetPerspective(60.f, GetWindowAspect());
    }
    // Meshes indexer
    {
        mMeshesIndexer.AddDimension(kAvailableLODs.size());
        mMeshesIndexer.AddDimension(kAvailableVbFormats.size());
        mMeshesIndexer.AddDimension(kAvailableVertexAttrLayouts.size());
    }
    // Graphics pipelines indexer
    {
        mGraphicsPipelinesIndexer.AddDimension(kAvailableVsShaders.size());
        mGraphicsPipelinesIndexer.AddDimension(kAvailablePsShaders.size());
        mGraphicsPipelinesIndexer.AddDimension(kAvailableVbFormats.size());
        mGraphicsPipelinesIndexer.AddDimension(kAvailableVertexAttrLayouts.size());
    }

    SetupSkyboxResources();
    SetupSkyboxMeshes();
    SetupSkyboxPipelines();

    SetupSphereResources();
    SetupSphereMeshes();
    SetupSpheresPipelines();

    // =====================================================================
    // FULLSCREEN QUADS
    // =====================================================================

    SetupFullscreenQuadsResources();
    SetupFullscreenQuadsMeshes();
    SetupFullscreenQuadsPipelines();

    // =====================================================================
    // PER FRAME DATA
    // =====================================================================
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        // Timestamp query
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        mPerFrame.push_back(frame);
    }
}

void GraphicsBenchmarkApp::SetupSkyboxResources()
{
    // Images
    {
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/skybox.jpg"), &mSkyBoxTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mSkyBoxTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSkyBoxTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSkyBoxTexture.sampler));
    }

    // Uniform buffers
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSkyBox.uniformBuffer));
    }

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.flags.bits.pushable                 = true;
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSkyBox.descriptorSetLayout));
    }

    // Shaders
    SetupShader("Benchmark_SkyBox.vs", &mVSSkybox);
    SetupShader("Benchmark_SkyBox.ps", &mPSSkybox);
}

void GraphicsBenchmarkApp::SetupSphereResources()
{
    // Images
    {
        // Albedo
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/albedo.png"), &mAlbedoTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mAlbedoTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mAlbedoTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mAlbedoTexture.sampler));
    }
    {
        // NormalMap
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/normal.png"), &mNormalMapTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mNormalMapTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mNormalMapTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mNormalMapTexture.sampler));
    }
    {
        // MetalRoughness
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/metalness-roughness.png"), &mMetalRoughnessTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mMetalRoughnessTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mMetalRoughnessTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mMetalRoughnessTexture.sampler));
    }

    // Uniform buffers
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSphere.uniformBuffer));
    }

    // Uniform buffers for draw calls
    {
        mDrawCallUniformBuffers.resize(kMaxSphereInstanceCount);
        for (uint32_t i = 0; i < kMaxSphereInstanceCount; i++) {
            grfx::BufferCreateInfo bufferCreateInfo        = {};
            bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
            bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
            bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
            PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mDrawCallUniformBuffers[i]));
        }
    }

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.flags.bits.pushable                 = true;
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(3, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(4, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(5, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(6, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSphere.descriptorSetLayout));
    }

    // Vertex Shaders
    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        const std::string vsShaderBaseName = kAvailableVsShaders[i];
        SetupShader(vsShaderBaseName + ".vs", &mVsShaders[i]);
    }
    // Pixel Shaders
    for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
        const std::string psShaderBaseName = kAvailablePsShaders[j];
        SetupShader(psShaderBaseName + ".ps", &mPsShaders[j]);
    }
}

void GraphicsBenchmarkApp::SetupFullscreenQuadsResources()
{
    // Image for texture shader
    {
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(1);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/resolution.jpg"), &mQuadsTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mQuadsTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mQuadsTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.minFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.minLod                  = 0.0f;
        samplerCreateInfo.maxLod                  = 1.0f;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mQuadsTexture.sampler));
    }

    // Descriptor set layout for texture shader
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.flags.bits.pushable                 = true;
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mFullscreenQuads.descriptorSetLayout));
    }

    SetupShader("Benchmark_VsSimpleQuads.vs", &mVSQuads);
    SetupShader("Benchmark_RandomNoise.ps", &mQuadsPs[0]);
    SetupShader("Benchmark_SolidColor.ps", &mQuadsPs[1]);
    SetupShader("Benchmark_Texture.ps", &mQuadsPs[2]);
}

void GraphicsBenchmarkApp::SetupSkyboxMeshes()
{
    TriMesh  mesh = TriMesh::CreateCube(float3(1, 1, 1), TriMeshOptions().TexCoords());
    Geometry geo;
    PPX_CHECKED_CALL(Geometry::Create(GeometryOptions::InterleavedU16().AddTexCoord(), mesh, &geo));
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSkyBox.mesh));
}

void GraphicsBenchmarkApp::SetupSphereMeshes()
{
    OrderedGrid grid(kMaxSphereInstanceCount, kSeed);

    // LODs for spheres
    mSphereLODs.push_back(LOD{/* longitudeSegments = */ 50, /* latitudeSegments = */ 50, kAvailableLODs[0]});
    mSphereLODs.push_back(LOD{/* longitudeSegments = */ 20, /* latitudeSegments = */ 20, kAvailableLODs[1]});
    mSphereLODs.push_back(LOD{/* longitudeSegments = */ 10, /* latitudeSegments = */ 10, kAvailableLODs[2]});
    PPX_ASSERT_MSG(mSphereLODs.size() == kAvailableLODs.size(), "LODs for spheres must be the same as the available LODs");

    // Create the meshes
    uint32_t meshIndex = 0;
    for (LOD lod : mSphereLODs) {
        PPX_LOG_INFO("LOD: " << lod.name);
        SphereMesh sphereMesh(/* radius = */ 1, lod.longitudeSegments, lod.latitudeSegments);
        sphereMesh.ApplyGrid(grid);

        // Create a giant vertex buffer for each vb type to accommodate all copies of the sphere mesh
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetLowPrecisionInterleaved(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetLowPrecisionPositionPlanar(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetHighPrecisionInterleaved(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetHighPrecisionPositionPlanar(), &mSphereMeshes[meshIndex++]));
    }
}

void GraphicsBenchmarkApp::SetupFullscreenQuadsMeshes()
{
    // Vertex buffer and vertex binding

    // clang-format off
    std::vector<float> vertexData = {
        // one large triangle covering entire screen area
        // position
        -1.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
    };
    // clang-format on
    uint32_t dataSize = SizeInBytesU32(vertexData);

    grfx::BufferCreateInfo bufferCreateInfo       = {};
    bufferCreateInfo.size                         = dataSize;
    bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
    bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;
    bufferCreateInfo.initialState                 = grfx::RESOURCE_STATE_VERTEX_BUFFER;

    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mFullscreenQuads.vertexBuffer));

    void* pAddr = nullptr;
    PPX_CHECKED_CALL(mFullscreenQuads.vertexBuffer->MapMemory(0, &pAddr));
    memcpy(pAddr, vertexData.data(), dataSize);
    mFullscreenQuads.vertexBuffer->UnmapMemory();

    mFullscreenQuads.vertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
}

void GraphicsBenchmarkApp::SetupSkyboxPipelines()
{
    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = mSkyBox.descriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSkyBox.pipelineInterface));

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
    gpCreateInfo.VS                                 = {mVSSkybox.Get(), "vsmain"};
    gpCreateInfo.PS                                 = {mPSSkybox.Get(), "psmain"};
    gpCreateInfo.vertexInputState.bindingCount      = 1;
    gpCreateInfo.vertexInputState.bindings[0]       = mSkyBox.mesh->GetDerivedVertexBindings()[0];
    gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
    gpCreateInfo.cullMode                           = grfx::CULL_MODE_FRONT;
    gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                    = true;
    gpCreateInfo.depthWriteEnable                   = false;
    gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
    gpCreateInfo.outputState.renderTargetCount      = 1;
    gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = mSkyBox.pipelineInterface;
    PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mSkyBox.pipeline));
}

void GraphicsBenchmarkApp::SetupSpheresPipelines()
{
    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = mSphere.descriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSphere.pipelineInterface));

    uint32_t pipelineIndex = 0;
    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
            for (size_t k = 0; k < kAvailableVbFormats.size(); k++) {
                // Interleaved pipeline
                grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
                gpCreateInfo.VS                                 = {mVsShaders[i].Get(), "vsmain"};
                gpCreateInfo.PS                                 = {mPsShaders[j].Get(), "psmain"};
                gpCreateInfo.vertexInputState.bindingCount      = 1;
                gpCreateInfo.vertexInputState.bindings[0]       = mSphereMeshes[2 * k + 0]->GetDerivedVertexBindings()[0];
                gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
                gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
                gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
                gpCreateInfo.depthReadEnable                    = pDepthTestWrite->GetValue();
                gpCreateInfo.depthWriteEnable                   = pDepthTestWrite->GetValue();
                gpCreateInfo.blendModes[0]                      = pAlphaBlend->GetValue() ? grfx::BLEND_MODE_ALPHA : grfx::BLEND_MODE_NONE;
                gpCreateInfo.outputState.renderTargetCount      = 1;
                gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
                gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
                gpCreateInfo.pPipelineInterface                 = mSphere.pipelineInterface;
                PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipelines[pipelineIndex++]));

                // Position Planar Pipeline
                gpCreateInfo.vertexInputState.bindingCount = 2;
                gpCreateInfo.vertexInputState.bindings[0]  = mSphereMeshes[2 * k + 1]->GetDerivedVertexBindings()[0];
                gpCreateInfo.vertexInputState.bindings[1]  = mSphereMeshes[2 * k + 1]->GetDerivedVertexBindings()[1];
                PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipelines[pipelineIndex++]));
            }
        }
    }
}

void GraphicsBenchmarkApp::SetupFullscreenQuadsPipelines()
{
    // Noise
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = sizeof(uint32_t) / 4;
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[0]));
    }
    // Solid color
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = sizeof(float3) / 4;
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[1]));
    }
    // Texture
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mFullscreenQuads.descriptorSetLayout;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[2]));
    }

    for (size_t i = 0; i < kFullscreenQuadsTypes.size(); i++) {
        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVSQuads.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mQuadsPs[i].Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mFullscreenQuads.vertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mQuadsPipelineInterfaces[i];
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mQuadsPipelines[i]));
    }
}

void GraphicsBenchmarkApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    if (!mEnableMouseMovement) {
        return;
    }

    float2 prevPos  = GetNormalizedDeviceCoordinates(x - dx, y - dy);
    float2 currPos  = GetNormalizedDeviceCoordinates(x, y);
    float2 deltaPos = currPos - prevPos;

    // In the NDC: -1 <= x, y <= 1, so the maximum value for dx and dy is 2
    // which turns the camera by pi/2 radians, so for a specific dx and dy
    // we turn (dx * pi / 4, dy * pi / 4) respectively.
    float deltaTheta = deltaPos[0] * pi<float>() / 4.0f;
    float deltaPhi   = deltaPos[1] * pi<float>() / 4.0f;
    mCamera.Turn(deltaTheta, -deltaPhi);
}

void GraphicsBenchmarkApp::KeyDown(ppx::KeyCode key)
{
    mPressedKeys[key] = true;
}

void GraphicsBenchmarkApp::KeyUp(ppx::KeyCode key)
{
    mPressedKeys[key] = false;
    if (key == KEY_SPACE) {
        mEnableMouseMovement = !mEnableMouseMovement;
    }
}

void GraphicsBenchmarkApp::ProcessInput()
{
    float deltaTime = GetPrevFrameTime();

    if (mPressedKeys[KEY_W]) {
        mCamera.Move(FreeCamera::MovementDirection::FORWARD, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_A]) {
        mCamera.Move(FreeCamera::MovementDirection::LEFT, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_S]) {
        mCamera.Move(FreeCamera::MovementDirection::BACKWARD, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_D]) {
        mCamera.Move(FreeCamera::MovementDirection::RIGHT, kCameraSpeed * deltaTime);
    }
}

void GraphicsBenchmarkApp::ProcessKnobs()
{
    bool rebuildSpherePipeline = false;

    // TODO: Ideally, the `maxValue` of the drawcall-count slider knob should be changed at runtime.
    // Currently, the value of the drawcall-count is adjusted to the sphere-count in case the
    // former exceeds the value of the sphere-count.
    if (pDrawCallCount->GetValue() > pSphereInstanceCount->GetValue()) {
        pDrawCallCount->SetValue(pSphereInstanceCount->GetValue());
    }

    if (pAlphaBlend->DigestUpdate()) {
        rebuildSpherePipeline = true;
    }

    if (pDepthTestWrite->DigestUpdate()) {
        rebuildSpherePipeline = true;
    }

    // Rebuild pipelines
    if (rebuildSpherePipeline) {
        SetupSpheresPipelines();
    }

    ProcessQuadsKnobs();
}

void GraphicsBenchmarkApp::ProcessQuadsKnobs()
{
    bool rebuildPipeline = false;

    // Set Visibilities
    if (pFullscreenQuadsCount->GetValue() > 0) {
        pFullscreenQuadsType->SetVisible(true);
        pFullscreenQuadsSingleRenderpass->SetVisible(true);
        if (pFullscreenQuadsType->GetIndex() == 1) {
            pFullscreenQuadsColor->SetVisible(true);
        }
        else {
            pFullscreenQuadsColor->SetVisible(false);
        }
    }
    else {
        pFullscreenQuadsType->SetVisible(false);
        pFullscreenQuadsSingleRenderpass->SetVisible(false);
        pFullscreenQuadsColor->SetVisible(false);
    }
}

void GraphicsBenchmarkApp::Render()
{
    PerFrame&          frame      = mPerFrame[0];
    grfx::SwapchainPtr swapchain  = GetSwapchain();
    uint32_t           imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0, 0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, sizeof(data)));
        mGpuWorkDuration = data[1] - data[0];
    }
    // Reset query
    frame.timestampQuery->Reset(/* firstQuery= */ 0, frame.timestampQuery->GetCount());

    ProcessInput();
    ProcessKnobs();
    UpdateGUI();

    RecordCommandBuffer(frame, swapchain, imageIndex);

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void GraphicsBenchmarkApp::UpdateGUI()
{
    if (!GetSettings()->enableImGui) {
        return;
    }

    // GUI
    ImGui::Begin("Debug Window");
    GetKnobManager().DrawAllKnobs(true);
    ImGui::Separator();
    DrawExtraInfo();
    ImGui::End();
}

void GraphicsBenchmarkApp::DrawExtraInfo()
{
    uint64_t frequency = 0;
    GetGraphicsQueue()->GetTimestampFrequency(&frequency);

    ImGui::Columns(2);
    const float gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
    ImGui::Text("GPU Work Duration");
    ImGui::NextColumn();
    ImGui::Text("%f ms ", gpuWorkDuration);
    ImGui::NextColumn();

    ImGui::Columns(2);
    const float gpuFPS = static_cast<float>(frequency / static_cast<double>(mGpuWorkDuration));
    ImGui::Text("GPU FPS");
    ImGui::NextColumn();
    ImGui::Text("%f fps ", gpuFPS);
    ImGui::NextColumn();
}

void GraphicsBenchmarkApp::RecordCommandBuffer(PerFrame& frame, grfx::SwapchainPtr swapchain, uint32_t imageIndex)
{
    PPX_CHECKED_CALL(frame.cmd->Begin());

    // Write start timestamp
    frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 0);

    frame.cmd->SetScissors(GetScissor());
    frame.cmd->SetViewports(GetViewport());

    grfx::RenderPassPtr currentRenderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_CLEAR);
    PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");

    // Transition image layout PRESENT->RENDER before the first renderpass
    frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);

    // Record commands for the scene using one renderpass
    frame.cmd->BeginRenderPass(currentRenderPass);
    if (pEnableSkyBox->GetValue()) {
        RecordCommandBufferSkybox(frame);
    }
    RecordCommandBufferSpheres(frame);
    frame.cmd->EndRenderPass();

    // Record commands for the fullscreen quads using one/multiple renderpasses
    uint32_t quadsCount       = pFullscreenQuadsCount->GetValue();
    bool     singleRenderpass = pFullscreenQuadsSingleRenderpass->GetValue();
    if (quadsCount > 0) {
        currentRenderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_DONT_CARE);
        frame.cmd->BindGraphicsPipeline(mQuadsPipelines[pFullscreenQuadsType->GetIndex()]);
        frame.cmd->BindVertexBuffers(1, &mFullscreenQuads.vertexBuffer, &mFullscreenQuads.vertexBinding.GetStride());

        // Begin the first renderpass used for quads
        frame.cmd->BeginRenderPass(currentRenderPass);
        for (size_t i = 0; i < quadsCount; i++) {
            RecordCommandBufferFullscreenQuad(frame, i);
            if (!singleRenderpass) {
                // If quads are using multiple renderpasses, transition image layout in between to force resolve
                frame.cmd->EndRenderPass();
                frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_SHADER_RESOURCE);
                frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);

                if (i == (quadsCount - 1)) { // For the last quad, do not begin another renderpass
                    break;
                }
                frame.cmd->BeginRenderPass(currentRenderPass);
            }
        }
        if (singleRenderpass) {
            frame.cmd->EndRenderPass();
        }
    }

    // Write end timestamp
    frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 1);

    // Record commands for the GUI using one last renderpass
    if (GetSettings()->enableImGui) {
        currentRenderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_LOAD);
        PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");
        frame.cmd->BeginRenderPass(currentRenderPass);
        RecordCommandBufferGUI(frame);
        frame.cmd->EndRenderPass();
    }

    // Transition image layout RENDER->PRESENT after the last renderpass
    frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);

    // Resolve queries
    frame.cmd->ResolveQueryData(frame.timestampQuery, /* startIndex= */ 0, frame.timestampQuery->GetCount());

    PPX_CHECKED_CALL(frame.cmd->End());
}

void GraphicsBenchmarkApp::RecordCommandBufferSkybox(PerFrame& frame)
{
    frame.cmd->BindGraphicsPipeline(mSkyBox.pipeline);
    frame.cmd->BindIndexBuffer(mSkyBox.mesh);
    frame.cmd->BindVertexBuffers(mSkyBox.mesh);
    {
        SkyboxData data = {};
        data.MVP        = mCamera.GetViewProjectionMatrix() * glm::scale(float3(500.0f, 500.0f, 500.0f));
        mSkyBox.uniformBuffer->CopyFromSource(sizeof(data), &data);

        frame.cmd->PushGraphicsUniformBuffer(mSkyBox.pipelineInterface, /* binding = */ 0, /* set = */ 0, /* bufferOffset = */ 0, mSkyBox.uniformBuffer);
        frame.cmd->PushGraphicsSampledImage(mSkyBox.pipelineInterface, /* binding = */ 1, /* set = */ 0, mSkyBoxTexture.sampledImageView);
        frame.cmd->PushGraphicsSampler(mSkyBox.pipelineInterface, /* binding = */ 2, /* set = */ 0, mSkyBoxTexture.sampler);
    }
    frame.cmd->DrawIndexed(mSkyBox.mesh->GetIndexCount());
}

void GraphicsBenchmarkApp::RecordCommandBufferSpheres(PerFrame& frame)
{
    const size_t pipelineIndex = mGraphicsPipelinesIndexer.GetIndex({pKnobVs->GetIndex(), pKnobPs->GetIndex(), pKnobVbFormat->GetIndex(), pKnobVertexAttrLayout->GetIndex()});
    frame.cmd->BindGraphicsPipeline(mPipelines[pipelineIndex]);
    const size_t meshIndex = mMeshesIndexer.GetIndex({pKnobLOD->GetIndex(), pKnobVbFormat->GetIndex(), pKnobVertexAttrLayout->GetIndex()});
    frame.cmd->BindIndexBuffer(mSphereMeshes[meshIndex]);
    frame.cmd->BindVertexBuffers(mSphereMeshes[meshIndex]);

    // Snapshot some scene-related values for the current frame
    uint32_t currentSphereCount   = pSphereInstanceCount->GetValue();
    uint32_t currentDrawCallCount = pDrawCallCount->GetValue();
    uint32_t mSphereIndexCount    = mSphereMeshes[meshIndex]->GetIndexCount() / kMaxSphereInstanceCount;
    uint32_t indicesPerDrawCall   = (currentSphereCount * mSphereIndexCount) / currentDrawCallCount;

    // Make `indicesPerDrawCall` multiple of 3 given that each consecutive three vertices (3*i + 0, 3*i + 1, 3*i + 2)
    // defines a single triangle primitive (PRIMITIVE_TOPOLOGY_TRIANGLE_LIST).
    indicesPerDrawCall -= indicesPerDrawCall % 3;
    SphereData data                 = {};
    data.modelMatrix                = float4x4(1.0f);
    data.ITModelMatrix              = glm::inverse(glm::transpose(data.modelMatrix));
    data.ambient                    = float4(0.3f);
    data.cameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
    data.lightPosition              = float4(mLightPosition, 0.0f);
    data.eyePosition                = float4(mCamera.GetEyePosition(), 0.0f);

    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 1, /* set = */ 0, mAlbedoTexture.sampledImageView);
    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 2, /* set = */ 0, mAlbedoTexture.sampler);
    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 3, /* set = */ 0, mNormalMapTexture.sampledImageView);
    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 4, /* set = */ 0, mNormalMapTexture.sampler);
    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 5, /* set = */ 0, mMetalRoughnessTexture.sampledImageView);
    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 6, /* set = */ 0, mMetalRoughnessTexture.sampler);

    for (uint32_t i = 0; i < currentDrawCallCount; i++) {
        mDrawCallUniformBuffers[i]->CopyFromSource(sizeof(data), &data);

        frame.cmd->PushGraphicsUniformBuffer(mSphere.pipelineInterface, /* binding = */ 0, /* set = */ 0, /* bufferOffset = */ 0, mDrawCallUniformBuffers[i]);

        uint32_t indexCount = indicesPerDrawCall;
        // Add the remaining indices to the last drawcall
        if (i == (currentDrawCallCount - 1)) {
            indexCount += (currentSphereCount * mSphereIndexCount - currentDrawCallCount * indicesPerDrawCall);
        }
        uint32_t firstIndex = i * indicesPerDrawCall;
        frame.cmd->DrawIndexed(indexCount, /* instanceCount = */ 1, firstIndex);
    }
}

void GraphicsBenchmarkApp::RecordCommandBufferFullscreenQuad(PerFrame& frame, size_t seed)
{
    switch (pFullscreenQuadsType->GetIndex()) {
        case static_cast<size_t>(FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_NOISE): {
            uint32_t noiseQuadRandomSeed = (uint32_t)seed;
            frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[0], 1, &noiseQuadRandomSeed);
            break;
        }
        case static_cast<size_t>(FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_SOLID_COLOR): {
            // zigzag the intensity between (0.5 ~ 1.0) in steps of 0.1
            //     index:   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0...
            // intensity: 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0...
            float index = seed % 10;
            float intensity;
            if (index > 4.5) {
                intensity = (index / 10.0f);
            }
            else {
                intensity = (1.0f - (index / 10.f));
            }
            float3 colorValues = kFullscreenQuadsColorsValues[pFullscreenQuadsColor->GetIndex()];
            colorValues *= intensity;
            frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[1], 3, &colorValues);
            break;
        }
        case static_cast<size_t>(FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_TEXTURE): {
            frame.cmd->PushGraphicsSampledImage(mQuadsPipelineInterfaces[2], /* binding = */ 0, /* set = */ 0, mQuadsTexture.sampledImageView);
            break;
        }
    }
    frame.cmd->Draw(3, 1, 0, 0);
}

void GraphicsBenchmarkApp::RecordCommandBufferGUI(PerFrame& frame)
{
    DrawImGui(frame.cmd);
}

void GraphicsBenchmarkApp::SetupShader(const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule)
{
    std::vector<char> bytecode = LoadShader(kShaderBaseDir, fileName);
    PPX_ASSERT_MSG(!bytecode.empty(), "shader bytecode load failed for " << kShaderBaseDir << " " << fileName);
    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, ppShaderModule));
}
