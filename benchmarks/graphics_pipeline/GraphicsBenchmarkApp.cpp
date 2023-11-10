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

static constexpr size_t SKYBOX_UNIFORM_BUFFER_REGISTER = 0;
static constexpr size_t SKYBOX_SAMPLED_IMAGE_REGISTER  = 1;
static constexpr size_t SKYBOX_SAMPLER_REGISTER        = 2;

static constexpr size_t SPHERE_UNIFORM_BUFFER_REGISTER                = 0;
static constexpr size_t SPHERE_ALBEDO_SAMPLED_IMAGE_REGISTER          = 1;
static constexpr size_t SPHERE_ALBEDO_SAMPLER_REGISTER                = 2;
static constexpr size_t SPHERE_NORMAL_SAMPLED_IMAGE_REGISTER          = 3;
static constexpr size_t SPHERE_NORMAL_SAMPLER_REGISTER                = 4;
static constexpr size_t SPHERE_METAL_ROUGHNESS_SAMPLED_IMAGE_REGISTER = 5;
static constexpr size_t SPHERE_METAL_ROUGHNESS_SAMPLER_REGISTER       = 6;

static constexpr size_t QUADS_SAMPLED_IMAGE_REGISTER = 0;

void GraphicsBenchmarkApp::InitKnobs()
{
    const auto& cl_options = GetExtraOptions();
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("vs-shader-index"), "--vs-shader-index flag has been replaced, instead use --vs and specify the name of the vertex shader");
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("ps-shader-index"), "--ps-shader-index flag has been replaced, instead use --ps and specify the name of the pixel shader");

    pEnableSkyBox = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-skybox", true);
    pEnableSkyBox->SetDisplayName("Enable SkyBox");
    pEnableSkyBox->SetFlagDescription("Enable the SkyBox in the scene.");

    pEnableSpheres = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("enable-spheres", true);
    pEnableSpheres->SetDisplayName("Enable Spheres");
    pEnableSpheres->SetFlagDescription("Enable the Spheres in the scene.");

    pKnobVs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vs", 0, kAvailableVsShaders);
    pKnobVs->SetDisplayName("Vertex Shader");
    pKnobVs->SetFlagDescription("Select the vertex shader for the graphics pipeline.");
    pKnobVs->SetIndent(1);

    pKnobPs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("ps", 0, kAvailablePsShaders);
    pKnobPs->SetDisplayName("Pixel Shader");
    pKnobPs->SetFlagDescription("Select the pixel shader for the graphics pipeline.");
    pKnobPs->SetIndent(1);

    pAllTexturesTo1x1 = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("all-textures-to-1x1", false);
    pAllTexturesTo1x1->SetDisplayName("All Textures To 1x1");
    pAllTexturesTo1x1->SetFlagDescription("Replace all sphere textures with a 1x1 white texture.");
    pAllTexturesTo1x1->SetIndent(2);

    pKnobLOD = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("LOD", 0, kAvailableLODs);
    pKnobLOD->SetDisplayName("Level of Detail (LOD)");
    pKnobLOD->SetFlagDescription("Select the Level of Detail (LOD) for the sphere mesh.");
    pKnobLOD->SetIndent(1);

    pKnobVbFormat = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vertex-buffer-format", 0, kAvailableVbFormats);
    pKnobVbFormat->SetDisplayName("Vertex Buffer Format");
    pKnobVbFormat->SetFlagDescription("Select the format for the vertex buffer.");
    pKnobVbFormat->SetIndent(1);

    pKnobVertexAttrLayout = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vertex-attr-layout", 0, kAvailableVertexAttrLayouts);
    pKnobVertexAttrLayout->SetDisplayName("Vertex Attribute Layout");
    pKnobVertexAttrLayout->SetFlagDescription("Select the Vertex Attribute Layout for the graphics pipeline.");
    pKnobVertexAttrLayout->SetIndent(1);

    pSphereInstanceCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sphere-count", /* defaultValue = */ 50, /* minValue = */ 1, kMaxSphereInstanceCount);
    pSphereInstanceCount->SetDisplayName("Sphere Count");
    pSphereInstanceCount->SetFlagDescription("Select the number of spheres to draw on the screen.");
    pSphereInstanceCount->SetIndent(1);

    pDrawCallCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("drawcall-count", /* defaultValue = */ 1, /* minValue = */ 1, kMaxSphereInstanceCount);
    pDrawCallCount->SetDisplayName("DrawCall Count");
    pDrawCallCount->SetFlagDescription("Select the number of draw calls to be used to draw the `sphere-count` spheres.");
    pDrawCallCount->SetIndent(1);

    pAlphaBlend = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("alpha-blend", false);
    pAlphaBlend->SetDisplayName("Alpha Blend");
    pAlphaBlend->SetFlagDescription("Set blend mode of the spheres to alpha blending.");
    pAlphaBlend->SetIndent(1);

    pDepthTestWrite = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("depth-test-write", true);
    pDepthTestWrite->SetDisplayName("Depth Test & Write");
    pDepthTestWrite->SetFlagDescription("Enable depth test and depth write for spheres (Default: enabled).");
    pDepthTestWrite->SetIndent(1);

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
    settings.grfx.numFramesInFlight     = 1;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
#if defined(PPX_BUILD_XR)
    // XR specific settings
    settings.grfx.pacedFrameRate   = 0;
    settings.xr.enable             = true; // Change this to true to enable the XR mode
    settings.xr.enableDebugCapture = false;
#endif
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
    // Sampler
    {
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mLinearSampler));
    }
    // Descriptor Pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 4 * GetNumFramesInFlight(); // 1 for skybox, 3 for spheres
        createInfo.sampledImage                   = 5 * GetNumFramesInFlight(); // 1 for skybox, 3 for spheres, 1 for quads
        createInfo.uniformBuffer                  = 2 * GetNumFramesInFlight(); // 1 for skybox, 1 for spheres

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    SetupSkyBoxResources();
    SetupSkyBoxMeshes();
    SetupSkyBoxPipelines();

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

#if defined(PPX_BUILD_XR)
        // For XR, we need to render the UI into a separate composition layer with a different swapchain
        if (IsXrEnabled()) {
            PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.uiCmd));
            PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.uiRenderCompleteFence));
        }
#endif

        mPerFrame.push_back(frame);
    }
}

void GraphicsBenchmarkApp::SetupSkyBoxResources()
{
    // Textures
    {
        // Albedo
        grfx_util::TextureOptions options = grfx_util::TextureOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/skybox.jpg"), &mSkyBoxTexture, options));
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
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SKYBOX_UNIFORM_BUFFER_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SKYBOX_SAMPLED_IMAGE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SKYBOX_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSkyBox.descriptorSetLayout));
    }

    // Allocate descriptor sets
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet;
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSkyBox.descriptorSetLayout, &pDescriptorSet));
        mSkyBox.descriptorSets.push_back(pDescriptorSet);
    }

    UpdateSkyBoxDescriptors();

    // Shaders
    SetupShader("Benchmark_SkyBox.vs", &mVSSkyBox);
    SetupShader("Benchmark_SkyBox.ps", &mPSSkyBox);
}

void GraphicsBenchmarkApp::SetupSphereResources()
{
    // Textures
    {
        // Altimeter textures
        grfx_util::TextureOptions options = grfx_util::TextureOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/albedo.png"), &mAlbedoTexture, options));
        PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/normal.png"), &mNormalMapTexture, options));
        PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/metalness-roughness.png"), &mMetalRoughnessTexture, options));
    }
    {
        // 1x1 White Texture
        PPX_CHECKED_CALL(grfx_util::CreateTexture1x1<uint8_t>(GetDevice()->GetGraphicsQueue(), {255, 255, 255, 255}, &mWhitePixelTexture));
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
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSphere.uniformBuffer));
    }

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_UNIFORM_BUFFER_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_ALBEDO_SAMPLED_IMAGE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_ALBEDO_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_NORMAL_SAMPLED_IMAGE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_NORMAL_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_METAL_ROUGHNESS_SAMPLED_IMAGE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_METAL_ROUGHNESS_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSphere.descriptorSetLayout));
    }

    // Allocate descriptor sets
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet;
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSphere.descriptorSetLayout, &pDescriptorSet));
        mSphere.descriptorSets.push_back(pDescriptorSet);
    }

    UpdateSphereDescriptors();

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
    // Textures
    {
        // Large resolution image
        grfx_util::TextureOptions options = grfx_util::TextureOptions().MipLevelCount(1);
        PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/resolution.jpg"), &mQuadsTexture, options));
    }

    // Descriptor set layout for texture shader
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(QUADS_SAMPLED_IMAGE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mFullscreenQuads.descriptorSetLayout));
    }

    // Allocate descriptor sets
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet;
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mFullscreenQuads.descriptorSetLayout, &pDescriptorSet));
        mFullscreenQuads.descriptorSets.push_back(pDescriptorSet);
    }

    UpdateFullscreenQuadsDescriptors();

    SetupShader("Benchmark_VsSimpleQuads.vs", &mVSQuads);
    SetupShader("Benchmark_RandomNoise.ps", &mQuadsPs[0]);
    SetupShader("Benchmark_SolidColor.ps", &mQuadsPs[1]);
    SetupShader("Benchmark_Texture.ps", &mQuadsPs[2]);
}

void GraphicsBenchmarkApp::UpdateSkyBoxDescriptors()
{
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet = mSkyBox.descriptorSets[i];
        PPX_CHECKED_CALL(pDescriptorSet->UpdateUniformBuffer(SKYBOX_UNIFORM_BUFFER_REGISTER, 0, mSkyBox.uniformBuffer));
        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SKYBOX_SAMPLED_IMAGE_REGISTER, 0, mSkyBoxTexture));
        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampler(SKYBOX_SAMPLER_REGISTER, 0, mLinearSampler));
    }
}

void GraphicsBenchmarkApp::UpdateSphereDescriptors()
{
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet = mSphere.descriptorSets[i];

        PPX_CHECKED_CALL(pDescriptorSet->UpdateUniformBuffer(SPHERE_UNIFORM_BUFFER_REGISTER, 0, mSphere.uniformBuffer));

        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampler(SPHERE_ALBEDO_SAMPLER_REGISTER, 0, mLinearSampler));
        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampler(SPHERE_NORMAL_SAMPLER_REGISTER, 0, mLinearSampler));
        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampler(SPHERE_METAL_ROUGHNESS_SAMPLER_REGISTER, 0, mLinearSampler));

        if (pAllTexturesTo1x1->GetValue()) {
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_ALBEDO_SAMPLED_IMAGE_REGISTER, 0, mWhitePixelTexture));
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_NORMAL_SAMPLED_IMAGE_REGISTER, 0, mWhitePixelTexture));
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_METAL_ROUGHNESS_SAMPLED_IMAGE_REGISTER, 0, mWhitePixelTexture));
        }
        else {
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_ALBEDO_SAMPLED_IMAGE_REGISTER, 0, mAlbedoTexture));
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_NORMAL_SAMPLED_IMAGE_REGISTER, 0, mNormalMapTexture));
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(SPHERE_METAL_ROUGHNESS_SAMPLED_IMAGE_REGISTER, 0, mMetalRoughnessTexture));
        }
    }
}

void GraphicsBenchmarkApp::UpdateFullscreenQuadsDescriptors()
{
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet = mFullscreenQuads.descriptorSets[i];
        PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(QUADS_SAMPLED_IMAGE_REGISTER, 0, mQuadsTexture));
    }
}

void GraphicsBenchmarkApp::SetupSkyBoxMeshes()
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

void GraphicsBenchmarkApp::SetupSkyBoxPipelines()
{
    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = mSkyBox.descriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSkyBox.pipelineInterface));

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
    gpCreateInfo.VS                                 = {mVSSkyBox.Get(), "vsmain"};
    gpCreateInfo.PS                                 = {mPSSkyBox.Get(), "psmain"};
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
    bool rebuildSpherePipeline   = false;
    bool updateSphereDescriptors = false;
    bool updateQuadsDescriptors  = false;

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

    if (pAllTexturesTo1x1->DigestUpdate()) {
        updateSphereDescriptors = true;
        updateQuadsDescriptors  = true;
    }

    // Set visibilities
    bool enableSpheres = pEnableSpheres->GetValue();
    if (pEnableSpheres->DigestUpdate()) {
        pKnobVs->SetVisible(enableSpheres);
        pKnobPs->SetVisible(enableSpheres);
        pKnobLOD->SetVisible(enableSpheres);
        pKnobVbFormat->SetVisible(enableSpheres);
        pKnobVertexAttrLayout->SetVisible(enableSpheres);
        pSphereInstanceCount->SetVisible(enableSpheres);
        pDrawCallCount->SetVisible(enableSpheres);
        pAlphaBlend->SetVisible(enableSpheres);
        pDepthTestWrite->SetVisible(enableSpheres);
    }
    pAllTexturesTo1x1->SetVisible(enableSpheres && (pKnobPs->GetIndex() == static_cast<size_t>(SpherePS::SPHERE_PS_MEM_BOUND)));

    // Update descriptors
    if (updateSphereDescriptors) {
        UpdateSphereDescriptors();
    }
    if (updateQuadsDescriptors) {
        UpdateFullscreenQuadsDescriptors();
    }

    // Rebuild pipelines
    if (rebuildSpherePipeline) {
        SetupSpheresPipelines();
    }

    ProcessQuadsKnobs();
}

void GraphicsBenchmarkApp::ProcessQuadsKnobs()
{
    // Set Visibilities
    if (pFullscreenQuadsCount->GetValue() > 0) {
        pFullscreenQuadsType->SetVisible(true);
        pFullscreenQuadsSingleRenderpass->SetVisible(true);
        if (pFullscreenQuadsType->GetIndex() == static_cast<size_t>(FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_SOLID_COLOR)) {
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

#if defined(PPX_BUILD_XR)
void GraphicsBenchmarkApp::RecordAndSubmitCommandBufferGUIXR(PerFrame& frame)
{
    uint32_t           imageIndex  = UINT32_MAX;
    grfx::SwapchainPtr uiSwapchain = GetUISwapchain();
    PPX_CHECKED_CALL(uiSwapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
    PPX_CHECKED_CALL(frame.uiRenderCompleteFence->WaitAndReset());

    PPX_CHECKED_CALL(frame.uiCmd->Begin());
    {
        grfx::RenderPassPtr renderPass = uiSwapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_CLEAR);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.uiCmd->BeginRenderPass(renderPass);
        // Draw ImGui
        UpdateGUI();
        DrawImGui(frame.uiCmd);
        frame.uiCmd->EndRenderPass();
    }
    PPX_CHECKED_CALL(frame.uiCmd->End());
    uiSwapchain->Wait(imageIndex);

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.uiCmd;
    submitInfo.waitSemaphoreCount   = 0;
    submitInfo.ppWaitSemaphores     = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.ppSignalSemaphores   = nullptr;

    submitInfo.pFence = frame.uiRenderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    uiSwapchain->Present(imageIndex, 0, nullptr);
}
#endif

void GraphicsBenchmarkApp::DispatchRender()
{
    if (!IsXrEnabled()) {
        Render();
        return;
    }
    mViewIndex = 0;
    Render();
    mViewIndex = 1;
    Render();
}

void GraphicsBenchmarkApp::Render()
{
    ProcessInput();
    ProcessKnobs();

    PerFrame& frame = mPerFrame[0];

#if defined(PPX_BUILD_XR)
    // Render UI into a different composition layer.
    if (IsXrEnabled()) {
        if ((mViewIndex == 0) && GetSettings()->enableImGui) {
            RecordAndSubmitCommandBufferGUIXR(frame);
        }
    }
#endif

    uint32_t           imageIndex = UINT32_MAX;
    grfx::SwapchainPtr swapchain  = GetSwapchain(mViewIndex);

#if defined(PPX_BUILD_XR)
    if (IsXrEnabled()) {
        PPX_ASSERT_MSG(swapchain->ShouldSkipExternalSynchronization(), "XRComponent should not be nullptr when XR is enabled!");
        // No need to
        // - Signal imageAcquiredSemaphore & imageAcquiredFence.
        // - Wait for imageAcquiredFence since xrWaitSwapchainImage is called in AcquireNextImage.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
    }
    else
#endif
    {
        // Wait semaphore is ignored for XR.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

        // Wait for and reset image acquired fence.
        PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    }

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

    // Update scene data
    frame.sceneData.viewProjectionMatrix = mCamera.GetViewProjectionMatrix();
#if defined(PPX_BUILD_XR)
    if (IsXrEnabled()) {
        const glm::mat4 v                    = GetXrComponent().GetViewMatrixForView(mViewIndex);
        const glm::mat4 p                    = GetXrComponent().GetProjectionMatrixForViewAndSetFrustumPlanes(mViewIndex, PPX_CAMERA_DEFAULT_NEAR_CLIP, PPX_CAMERA_DEFAULT_FAR_CLIP);
        frame.sceneData.viewProjectionMatrix = p * v;
    }
#endif

    RecordCommandBuffer(frame, swapchain, imageIndex);

    swapchain->Wait(imageIndex);

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &frame.cmd;
#if defined(PPX_BUILD_XR)
    // No need to use semaphore when XR is enabled.
    if (IsXrEnabled()) {
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;
    }
    else
#endif
    {
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    }
    submitInfo.pFence = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

#if defined(PPX_BUILD_XR)
    // No need to present when XR is enabled.
    if (IsXrEnabled()) {
        PPX_CHECKED_CALL(swapchain->Present(imageIndex, 0, nullptr));
        if (GetSettings()->xr.enableDebugCapture && (mViewIndex == 1)) {
            // We could use semaphore to sync to have better performance,
            // but this requires modifying the submission code.
            // For debug capture we don't care about the performance,
            // so use existing fence to sync for simplicity.
            grfx::SwapchainPtr debugSwapchain = GetDebugCaptureSwapchain();
            PPX_CHECKED_CALL(debugSwapchain->AcquireNextImage(UINT64_MAX, nullptr, frame.imageAcquiredFence, &imageIndex));
            frame.imageAcquiredFence->WaitAndReset();
            PPX_CHECKED_CALL(debugSwapchain->Present(imageIndex, 0, nullptr));
        }
    }
    else
#endif
    {
        PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.renderCompleteSemaphore));
    }
}

void GraphicsBenchmarkApp::UpdateGUI()
{
    if (!GetSettings()->enableImGui) {
        return;
    }

#if defined(PPX_BUILD_XR)
    // Apply the same trick as in Application::DrawDebugInfo() to reposition UI to the center for XR
    if (IsXrEnabled()) {
        ImVec2 lastImGuiWindowSize = ImGui::GetWindowSize();
        // For XR, force the diagnostic window to the center with automatic sizing for legibility and since control is limited.
        ImGui::SetNextWindowPos({(GetUIWidth() - lastImGuiWindowSize.x) / 2, (GetUIHeight() - lastImGuiWindowSize.y) / 2}, 0, {0.0f, 0.0f});
        ImGui::SetNextWindowSize({0, 0});
    }
#endif

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
    const float gpuWorkDurationInSec = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency));
    const float gpuWorkDurationInMs  = gpuWorkDurationInSec * 1000.0f;
    ImGui::Text("GPU Work Duration");
    ImGui::NextColumn();
    ImGui::Text("%.2f ms ", gpuWorkDurationInMs);
    ImGui::NextColumn();

    ImGui::Columns(2);
    const float gpuFPS = static_cast<float>(frequency / static_cast<double>(mGpuWorkDuration));
    ImGui::Text("GPU FPS");
    ImGui::NextColumn();
    ImGui::Text("%.2f fps ", gpuFPS);
    ImGui::NextColumn();

    const uint32_t width  = GetSwapchain()->GetWidth();
    const uint32_t height = GetSwapchain()->GetHeight();
    ImGui::Columns(2);
    ImGui::Text("Swapchain resolution");
    ImGui::NextColumn();
    ImGui::Text("%d x %d", width, height);
    ImGui::NextColumn();

    const uint32_t quad_count    = pFullscreenQuadsCount->GetValue();
    const float    dataWriteInGb = (static_cast<float>(width) * static_cast<float>(height) * 4.f * quad_count) / (1024.f * 1024.f * 1024.f);
    ImGui::Columns(2);
    ImGui::Text("Write Data");
    ImGui::NextColumn();
    ImGui::Text("%.2f GB", dataWriteInGb);
    ImGui::NextColumn();

    const float bandwidth = dataWriteInGb / gpuWorkDurationInSec;
    ImGui::Columns(2);
    ImGui::Text("Write Bandwidth");
    ImGui::NextColumn();
    ImGui::Text("%.2f GB/s", bandwidth);
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

#if defined(PPX_BUILD_XR)
    if (!IsXrEnabled())
#endif
    {
        // Transition image layout PRESENT->RENDER before the first renderpass
        frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
    }

    bool renderScene = pEnableSkyBox->GetValue() || pEnableSpheres->GetValue();
    if (renderScene) {
        // Record commands for the scene using one renderpass
        frame.cmd->BeginRenderPass(currentRenderPass);
        if (pEnableSkyBox->GetValue()) {
            RecordCommandBufferSkyBox(frame);
        }
        if (pEnableSpheres->GetValue()) {
            RecordCommandBufferSpheres(frame);
        }
        frame.cmd->EndRenderPass();
    }

    // Record commands for the fullscreen quads using one/multiple renderpasses
    uint32_t quadsCount       = pFullscreenQuadsCount->GetValue();
    bool     singleRenderpass = pFullscreenQuadsSingleRenderpass->GetValue();
    if (quadsCount > 0) {
        currentRenderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_DONT_CARE);
        frame.cmd->BindGraphicsPipeline(mQuadsPipelines[pFullscreenQuadsType->GetIndex()]);
        frame.cmd->BindVertexBuffers(1, &mFullscreenQuads.vertexBuffer, &mFullscreenQuads.vertexBinding.GetStride());

        if (pFullscreenQuadsType->GetIndex() == static_cast<size_t>(FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_TEXTURE)) {
            frame.cmd->BindGraphicsDescriptorSets(mQuadsPipelineInterfaces.at(pFullscreenQuadsType->GetIndex()), 1, &mFullscreenQuads.descriptorSets.at(GetInFlightFrameIndex()));
        }

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
    if (
#if defined(PPX_BUILD_XR)
        !IsXrEnabled() &&
#endif
        GetSettings()->enableImGui) {
        currentRenderPass = swapchain->GetRenderPass(imageIndex, (renderScene || quadsCount > 0) ? grfx::ATTACHMENT_LOAD_OP_LOAD : grfx::ATTACHMENT_LOAD_OP_CLEAR);
        PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");
        frame.cmd->BeginRenderPass(currentRenderPass);
        UpdateGUI();
        DrawImGui(frame.cmd);
        frame.cmd->EndRenderPass();
    }

#if defined(PPX_BUILD_XR)
    if (!IsXrEnabled())
#endif
    {
        // Transition image layout RENDER->PRESENT after the last renderpass
        frame.cmd->TransitionImageLayout(currentRenderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
    }

    // Resolve queries
    frame.cmd->ResolveQueryData(frame.timestampQuery, /* startIndex= */ 0, frame.timestampQuery->GetCount());

    PPX_CHECKED_CALL(frame.cmd->End());
}

void GraphicsBenchmarkApp::RecordCommandBufferSkyBox(PerFrame& frame)
{
    // Bind resources
    frame.cmd->BindGraphicsPipeline(mSkyBox.pipeline);
    frame.cmd->BindIndexBuffer(mSkyBox.mesh);
    frame.cmd->BindVertexBuffers(mSkyBox.mesh);

    frame.cmd->BindGraphicsDescriptorSets(mSkyBox.pipelineInterface, 1, &mSkyBox.descriptorSets.at(GetInFlightFrameIndex()));

    // Update uniform buffer with current view data
    SkyBoxData data = {};
    data.MVP        = frame.sceneData.viewProjectionMatrix * glm::scale(float3(500.0f, 500.0f, 500.0f));
    mSkyBox.uniformBuffer->CopyFromSource(sizeof(data), &data);

    frame.cmd->DrawIndexed(mSkyBox.mesh->GetIndexCount());
}

void GraphicsBenchmarkApp::RecordCommandBufferSpheres(PerFrame& frame)
{
    // Bind resources
    const size_t pipelineIndex = mGraphicsPipelinesIndexer.GetIndex({pKnobVs->GetIndex(), pKnobPs->GetIndex(), pKnobVbFormat->GetIndex(), pKnobVertexAttrLayout->GetIndex()});
    frame.cmd->BindGraphicsPipeline(mPipelines[pipelineIndex]);
    const size_t meshIndex = mMeshesIndexer.GetIndex({pKnobLOD->GetIndex(), pKnobVbFormat->GetIndex(), pKnobVertexAttrLayout->GetIndex()});
    frame.cmd->BindIndexBuffer(mSphereMeshes[meshIndex]);
    frame.cmd->BindVertexBuffers(mSphereMeshes[meshIndex]);

    frame.cmd->BindGraphicsDescriptorSets(mSphere.pipelineInterface, 1, &mSphere.descriptorSets.at(GetInFlightFrameIndex()));

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
    data.cameraViewProjectionMatrix = frame.sceneData.viewProjectionMatrix;
    data.lightPosition              = float4(mLightPosition, 0.0f);
    data.eyePosition                = float4(mCamera.GetEyePosition(), 0.0f);
    mSphere.uniformBuffer->CopyFromSource(sizeof(data), &data);

    for (uint32_t i = 0; i < currentDrawCallCount; i++) {
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
    }
    frame.cmd->Draw(3, 1, 0, 0);
}

void GraphicsBenchmarkApp::SetupShader(const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule)
{
    std::vector<char> bytecode = LoadShader(kShaderBaseDir, fileName);
    PPX_ASSERT_MSG(!bytecode.empty(), "shader bytecode load failed for " << kShaderBaseDir << " " << fileName);
    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, ppShaderModule));
}
