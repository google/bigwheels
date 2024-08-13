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
#include "ppx/grfx/grfx_format.h"
#include "ppx/math_config.h"
#include "ppx/timer.h"
#include <cstdint>

using namespace ppx;

static constexpr size_t SKYBOX_UNIFORM_BUFFER_REGISTER = 0;
static constexpr size_t SKYBOX_SAMPLED_IMAGE_REGISTER  = 1;
static constexpr size_t SKYBOX_SAMPLER_REGISTER        = 2;

static constexpr size_t SPHERE_COLOR_UNIFORM_BUFFER_REGISTER          = 0;
static constexpr size_t SPHERE_SCENEDATA_UNIFORM_BUFFER_REGISTER      = 1;
static constexpr size_t SPHERE_ALBEDO_SAMPLED_IMAGE_REGISTER          = 2;
static constexpr size_t SPHERE_ALBEDO_SAMPLER_REGISTER                = 3;
static constexpr size_t SPHERE_NORMAL_SAMPLED_IMAGE_REGISTER          = 4;
static constexpr size_t SPHERE_NORMAL_SAMPLER_REGISTER                = 5;
static constexpr size_t SPHERE_METAL_ROUGHNESS_SAMPLED_IMAGE_REGISTER = 6;
static constexpr size_t SPHERE_METAL_ROUGHNESS_SAMPLER_REGISTER       = 7;

static constexpr size_t QUADS_CONFIG_UNIFORM_BUFFER_REGISTER = 0;
static constexpr size_t QUADS_SAMPLED_IMAGE_REGISTER         = 1;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

template <typename T>
size_t GetPushConstCount(const T&)
{
    return sizeof(T) / sizeof(uint32_t);
}

void GraphicsBenchmarkApp::InitKnobs()
{
    const auto& cl_options = GetExtraOptions();
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("vs-shader-index"), "--vs-shader-index flag has been replaced, instead use --vs and specify the name of the vertex shader");
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("ps-shader-index"), "--ps-shader-index flag has been replaced, instead use --ps and specify the name of the pixel shader");

    GetKnobManager().InitKnob(&pEnableSkyBox, "enable-skybox", true);
    pEnableSkyBox->SetDisplayName("Enable SkyBox");
    pEnableSkyBox->SetFlagDescription("Enable the SkyBox in the scene.");

    GetKnobManager().InitKnob(&pEnableSpheres, "enable-spheres", true);
    pEnableSpheres->SetDisplayName("Enable Spheres");
    pEnableSpheres->SetFlagDescription("Enable the Spheres in the scene.");

    GetKnobManager().InitKnob(&pDebugViews, "debug-view", 0, kAvailableDebugViews);
    pDebugViews->SetDisplayName("Debug View");
    pDebugViews->SetFlagDescription("Select the debug view for spheres.");
    pDebugViews->SetIndent(1);

    GetKnobManager().InitKnob(&pKnobVs, "vs", 0, kAvailableVsShaders);
    pKnobVs->SetDisplayName("Vertex Shader");
    pKnobVs->SetFlagDescription("Select the vertex shader for the graphics pipeline.");
    pKnobVs->SetIndent(1);

    GetKnobManager().InitKnob(&pKnobPs, "ps", 0, kAvailablePsShaders);
    pKnobPs->SetDisplayName("Pixel Shader");
    pKnobPs->SetFlagDescription("Select the pixel shader for the graphics pipeline.");
    pKnobPs->SetIndent(1);

    GetKnobManager().InitKnob(&pAllTexturesTo1x1, "all-textures-to-1x1", false);
    pAllTexturesTo1x1->SetDisplayName("All Textures To 1x1");
    pAllTexturesTo1x1->SetFlagDescription("Replace all sphere textures with a 1x1 white texture.");
    pAllTexturesTo1x1->SetIndent(2);

    GetKnobManager().InitKnob(&pKnobLOD, "LOD", 0, kAvailableLODs);
    pKnobLOD->SetDisplayName("Level of Detail (LOD)");
    pKnobLOD->SetFlagDescription("Select the Level of Detail (LOD) for the sphere mesh.");
    pKnobLOD->SetIndent(1);

    GetKnobManager().InitKnob(&pKnobVbFormat, "vertex-buffer-format", 0, kAvailableVbFormats);
    pKnobVbFormat->SetDisplayName("Vertex Buffer Format");
    pKnobVbFormat->SetFlagDescription("Select the format for the vertex buffer.");
    pKnobVbFormat->SetIndent(1);

    GetKnobManager().InitKnob(&pKnobVertexAttrLayout, "vertex-attr-layout", 0, kAvailableVertexAttrLayouts);
    pKnobVertexAttrLayout->SetDisplayName("Vertex Attribute Layout");
    pKnobVertexAttrLayout->SetFlagDescription("Select the Vertex Attribute Layout for the graphics pipeline.");
    pKnobVertexAttrLayout->SetIndent(1);

    GetKnobManager().InitKnob(&pSphereInstanceCount, "sphere-count", /* defaultValue = */ kDefaultSphereInstanceCount, /* minValue = */ 1, kMaxSphereInstanceCount);
    pSphereInstanceCount->SetDisplayName("Sphere Count");
    pSphereInstanceCount->SetFlagDescription("Select the number of spheres to draw on the screen.");
    pSphereInstanceCount->SetIndent(1);

    GetKnobManager().InitKnob(&pDrawCallCount, "drawcall-count", /* defaultValue = */ 1, /* minValue = */ 1, kMaxSphereInstanceCount);
    pDrawCallCount->SetDisplayName("DrawCall Count");
    pDrawCallCount->SetFlagDescription("Select the number of draw calls to be used to draw the `--sphere-count` spheres.");
    pDrawCallCount->SetIndent(1);

    GetKnobManager().InitKnob(&pAlphaBlend, "alpha-blend", false);
    pAlphaBlend->SetDisplayName("Alpha Blend");
    pAlphaBlend->SetFlagDescription("Set blend mode of the spheres to alpha blending.");
    pAlphaBlend->SetIndent(1);

    GetKnobManager().InitKnob(&pDepthTestWrite, "depth-test-write", true);
    pDepthTestWrite->SetDisplayName("Depth Test & Write");
    pDepthTestWrite->SetFlagDescription("Enable depth test and depth write for spheres.");
    pDepthTestWrite->SetIndent(1);

    GetKnobManager().InitKnob(&pFullscreenQuadsCount, "fullscreen-quads-count", /* defaultValue = */ 0, /* minValue = */ 0, kMaxFullscreenQuadsCount);
    pFullscreenQuadsCount->SetDisplayName("Number of Fullscreen Quads");
    pFullscreenQuadsCount->SetFlagDescription("Select the number of fullscreen quads to render.");

    GetKnobManager().InitKnob(&pFullscreenQuadsType, "fullscreen-quads-type", 0, kFullscreenQuadsTypes);
    pFullscreenQuadsType->SetDisplayName("Type");
    pFullscreenQuadsType->SetFlagDescription("Select the type of the fullscreen quads. See also `--fullscreen-quads-count`.");
    pFullscreenQuadsType->SetIndent(1);

    GetKnobManager().InitKnob(&pFullscreenQuadsColor, "fullscreen-quads-color", 0, kFullscreenQuadsColors);
    pFullscreenQuadsColor->SetDisplayName("Color");
    pFullscreenQuadsColor->SetFlagDescription("Select the hue for the solid color fullscreen quads. See also `--fullscreen-quads-count`.");
    pFullscreenQuadsColor->SetIndent(2);

    GetKnobManager().InitKnob(&pQuadTextureFile, "fullscreen-quads-texture-path", kQuadTextureFile);
    pQuadTextureFile->SetFlagDescription("Texture used in fullscreen quads texture mode. See also `--fullscreen-quads-count` and `--fullscreen-quads-type`.");

    GetKnobManager().InitKnob(&pFullscreenQuadsSingleRenderpass, "fullscreen-quads-single-renderpass", true);
    pFullscreenQuadsSingleRenderpass->SetDisplayName("Single Renderpass");
    pFullscreenQuadsSingleRenderpass->SetFlagDescription("Render all fullscreen quads in a single renderpass. See also `--fullscreen-quads-count`");
    pFullscreenQuadsSingleRenderpass->SetIndent(1);

    // Offscreen rendering knobs:
    {
        GetKnobManager().InitKnob(&pRenderOffscreen, "enable-offscreen-rendering", false);
        pRenderOffscreen->SetDisplayName("Offscreen rendering");
        pRenderOffscreen->SetFlagDescription("Enable rendering to an offscreen/non-swapchain framebuffer.");

        GetKnobManager().InitKnob(&pBlitOffscreen, "offscreen-blit-to-swapchain", true);
        pBlitOffscreen->SetDisplayName("Blit to swapchain");
        pBlitOffscreen->SetFlagDescription("Blit offscreen rendering result to swapchain.");
        pBlitOffscreen->SetIndent(1);

        std::vector<DropdownEntry<grfx::Format, std::string>> formats;
        formats.push_back({"Swapchain", grfx::Format::FORMAT_UNDEFINED});
        for (const auto& format : kFramebufferFormatTypes) {
            formats.push_back({ToString(format), format});
        }
        GetKnobManager().InitKnob(&pFramebufferFormat, "offscreen-framebuffer-format", 0, formats);
        pFramebufferFormat->SetDisplayName("Framebuffer format");
        pFramebufferFormat->SetFlagDescription("Select the pixel format used in offscreen framebuffer.");
        pFramebufferFormat->SetIndent(1);

        std::vector<DropdownEntry<std::pair<int, int>, std::string>> resolutions;
        resolutions.push_back({"Swapchain", {0, 0}});
        for (const auto& res : kSimpleResolutions) {
            resolutions.push_back({std::to_string(res.first) + "x" + std::to_string(res.second), res});
        }
        for (const auto& res : kCommonResolutions) {
            resolutions.push_back({std::to_string(res.first) + "x" + std::to_string(res.second), res});
        }
        for (const auto& res : kVRPerEyeResolutions) {
            resolutions.push_back({
                std::to_string(res.first) + "x2x" + std::to_string(res.second),
                std::pair<int, int>(res.first * 2, res.second),
            });
        }
        GetKnobManager().InitKnob(&pResolution, "offscreen-framebuffer-resolution", 0, resolutions);
        pResolution->SetDisplayName("Framebuffer resolution");
        pResolution->SetFlagDescription("Select the size of offscreen framebuffer.");
        pResolution->SetIndent(1);
    }

    GetKnobManager().InitKnob(&pKnobAluCount, "alu-instruction-count", /* defaultValue = */ 100, /* minValue = */ 100, 400);
    pKnobAluCount->SetDisplayName("Number of ALU instructions in the shader");
    pKnobAluCount->SetFlagDescription("Select the number of ALU instructions in the shader.");

    GetKnobManager().InitKnob(&pKnobTextureCount, "texture-count", /* defaultValue = */ 1, /* minValue = */ 1, kMaxTextureCount);
    pKnobTextureCount->SetDisplayName("Number of texture to load in the shader");
    pKnobTextureCount->SetFlagDescription("Select the number of texture to load in the shader.");
}

void GraphicsBenchmarkApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "graphics_pipeline";
    settings.enableImGui                = true;
    settings.window.width               = 1920;
    settings.window.height              = 1080;
    settings.grfx.api                   = kApi;
    settings.grfx.numFramesInFlight     = 1;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
#if defined(PPX_BUILD_XR)
    // XR specific settings
    settings.grfx.pacedFrameRate = 0;
    settings.xr.enable           = true; // Change this to true to enable the XR mode
#endif
    settings.standardKnobsDefaultValue.enableMetrics        = true;
    settings.standardKnobsDefaultValue.overwriteMetricsFile = true;
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
        createInfo.sampler                        = 5 * GetNumFramesInFlight();  // 1 for skybox, 3 for spheres, 1 for blit
        createInfo.sampledImage                   = 15 * GetNumFramesInFlight(); // 1 for skybox, 3 for spheres, 10 for quads, 1 for blit
        createInfo.uniformBuffer                  = 2 * GetNumFramesInFlight();  // 1 for skybox, 1 for spheres

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    SetupSkyBoxResources();
    SetupSkyBoxMeshes();
    SetupSkyBoxPipelines();

    CreateColorsForDrawCalls();
    if (pEnableSpheres->GetValue()) {
        SetupSpheres();
    }

    // =====================================================================
    // FULLSCREEN QUADS
    // =====================================================================

    SetupFullscreenQuadsResources();
    SetupFullscreenQuadsMeshes();
    SetupFullscreenQuadsPipelines();

    // =====================================================================
    // BLIT (DX12 does not have vkCmdBlitImage equivalent)
    // =====================================================================

    PPX_CHECKED_CALL(CreateBlitContext(mBlit));

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

    {
        OffscreenFrame frame = {};
        PPX_CHECKED_CALL(CreateOffscreenFrame(frame, RenderFormat(), GetSwapchain()->GetDepthFormat(), GetSwapchain()->GetWidth(), GetSwapchain()->GetHeight()));
        mOffscreenFrame.push_back(frame);
    }
}

void GraphicsBenchmarkApp::SetupMetrics()
{
    Application::SetupMetrics();
    if (HasActiveMetricsRun()) {
        ppx::metrics::MetricMetadata metadata                     = {ppx::metrics::MetricType::GAUGE, "CPU Submission Time", "ms", ppx::metrics::MetricInterpretation::LOWER_IS_BETTER, {0.f, 10000.f}};
        mMetricsData.metrics[MetricsData::kTypeCPUSubmissionTime] = AddMetric(metadata);
        PPX_ASSERT_MSG(mMetricsData.metrics[MetricsData::kTypeCPUSubmissionTime] != ppx::metrics::kInvalidMetricID, "Failed to add CPU Submission Time metric");

        metadata                                          = {ppx::metrics::MetricType::GAUGE, "Bandwidth", "GB/s", ppx::metrics::MetricInterpretation::HIGHER_IS_BETTER, {0.f, 10000.f}};
        mMetricsData.metrics[MetricsData::kTypeBandwidth] = AddMetric(metadata);
        PPX_ASSERT_MSG(mMetricsData.metrics[MetricsData::kTypeBandwidth] != ppx::metrics::kInvalidMetricID, "Failed to add Bandwidth metric");
    }
}

void GraphicsBenchmarkApp::UpdateMetrics()
{
    if (!HasActiveMetricsRun()) {
        return;
    }

    uint64_t frequency = 0;
    PPX_CHECKED_CALL(GetGraphicsQueue()->GetTimestampFrequency(&frequency));

    ppx::metrics::MetricData data = {ppx::metrics::MetricType::GAUGE};
    data.gauge.seconds            = GetElapsedSeconds();
    data.gauge.value              = mCPUSubmissionTime.Value();
    RecordMetricData(mMetricsData.metrics[MetricsData::kTypeCPUSubmissionTime], data);

    const float        gpuWorkDurationInSec = static_cast<float>(mGpuWorkDuration.Value() / static_cast<double>(frequency));
    const grfx::Format swapchainColorFormat = GetSwapchain()->GetColorFormat();
    const uint32_t     swapchainWidth       = GetSwapchain()->GetWidth();
    const uint32_t     swapchainHeight      = GetSwapchain()->GetHeight();
    const bool         isOffscreen          = pRenderOffscreen->GetValue();
    const grfx::Format colorFormat          = isOffscreen ? mOffscreenFrame.back().colorFormat : swapchainColorFormat;
    const uint32_t     width                = isOffscreen ? mOffscreenFrame.back().width : swapchainWidth;
    const uint32_t     height               = isOffscreen ? mOffscreenFrame.back().height : swapchainHeight;
    const uint32_t     quadCount            = pFullscreenQuadsCount->GetValue();

    if (quadCount) {
        if (mSkipRecordBandwidthMetricFrameCounter == 0) {
            const auto  texelSize     = static_cast<float>(grfx::GetFormatDescription(colorFormat)->bytesPerTexel);
            const float dataWriteInGb = (static_cast<float>(width) * static_cast<float>(height) * texelSize * quadCount) / (1024.f * 1024.f * 1024.f);
            const float bandwidth     = dataWriteInGb / gpuWorkDurationInSec;

            ppx::metrics::MetricData data = {ppx::metrics::MetricType::GAUGE};
            data.gauge.seconds            = GetElapsedSeconds();
            data.gauge.value              = bandwidth;
            RecordMetricData(mMetricsData.metrics[MetricsData::kTypeBandwidth], data);
        }
        else {
            --mSkipRecordBandwidthMetricFrameCounter;
            PPX_ASSERT_MSG(mSkipRecordBandwidthMetricFrameCounter >= 0, "The counter should never be smaller than 0");
        }
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

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(SPHERE_SCENEDATA_UNIFORM_BUFFER_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
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
    while (mSphere.descriptorSets.size() < n) {
        grfx::DescriptorSetPtr pDescriptorSet;
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSphere.descriptorSetLayout, &pDescriptorSet));
        mSphere.descriptorSets.push_back(pDescriptorSet);
    }

    UpdateSphereDescriptors();

    // Vertex Shaders
    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        const std::string vsShaderBaseName = ToString(kAvailableVsShaders[i]);
        SetupShader(vsShaderBaseName + ".vs", &mVsShaders[i]);
    }
    // Pixel Shaders
    for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
        const std::string psShaderBaseName = ToString(kAvailablePsShaders[j]);
        SetupShader(psShaderBaseName + ".ps", &mPsShaders[j]);
    }
}

void GraphicsBenchmarkApp::SetupFullscreenQuadsResources()
{
    // Textures
    {
        // Large resolution image
        grfx_util::TextureOptions options = grfx_util::TextureOptions().MipLevelCount(1);
        for (uint32_t i = 0; i < kMaxTextureCount; i++) {
            // Load the same image.
            PPX_CHECKED_CALL(CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(pQuadTextureFile->GetValue()), &mQuadsTextures[i], options));
        }
    }

    // Descriptor set layout for texture shader
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        for (int i = 0; i < kMaxTextureCount; i++) {
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(QUADS_SAMPLED_IMAGE_REGISTER + i, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        }
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
    GetDevice()->WaitIdle();
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
    GetDevice()->WaitIdle();
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet = mSphere.descriptorSets[i];

        PPX_CHECKED_CALL(pDescriptorSet->UpdateUniformBuffer(SPHERE_SCENEDATA_UNIFORM_BUFFER_REGISTER, 0, mSphere.uniformBuffer));

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
    GetDevice()->WaitIdle();
    uint32_t n = GetNumFramesInFlight();
    for (size_t i = 0; i < n; i++) {
        grfx::DescriptorSetPtr pDescriptorSet = mFullscreenQuads.descriptorSets[i];
        for (uint32_t j = 0; j < kMaxTextureCount; j++) {
            PPX_CHECKED_CALL(pDescriptorSet->UpdateSampledImage(QUADS_SAMPLED_IMAGE_REGISTER + j, 0, mQuadsTextures[j]));
        }
    }
}

void GraphicsBenchmarkApp::SetupSkyBoxMeshes()
{
    TriMesh  mesh = TriMesh::CreateCube(float3(1, 1, 1), TriMeshOptions().TexCoords());
    Geometry geo;
    PPX_CHECKED_CALL(Geometry::Create(GeometryCreateInfo::InterleavedU16().AddTexCoord(), mesh, &geo));
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSkyBox.mesh));
}

void GraphicsBenchmarkApp::SetupSphereMeshes()
{
    GetDevice()->WaitIdle();
    // Destroy the meshes if they were created.
    for (auto& mesh : mSphereMeshes) {
        if (!mesh.IsNull()) {
            GetDevice()->DestroyMesh(mesh);
            mesh.Reset();
        }
    }

    const uint32_t requiredSphereCount = std::max<uint32_t>(pSphereInstanceCount->GetValue(), kDefaultSphereInstanceCount);
    const uint32_t initSphereCount     = std::min<uint32_t>(kMaxSphereInstanceCount, 2 * std::max(requiredSphereCount, mInitializedSpheres));

    // Create the meshes
    OrderedGrid grid(initSphereCount, kSeed);
    uint32_t    meshIndex = 0;
    for (const auto& lod : kAvailableLODs) {
        PPX_LOG_INFO("LOD: " << lod.name);
        SphereMesh sphereMesh(/* radius = */ 1, lod.value.longitudeSegments, lod.value.latitudeSegments);
        sphereMesh.ApplyGrid(grid);
        // Create a giant vertex buffer for each vb type to accommodate all copies of the sphere mesh
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetLowPrecisionInterleaved(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetLowPrecisionPositionPlanar(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetHighPrecisionInterleaved(), &mSphereMeshes[meshIndex++]));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), sphereMesh.GetHighPrecisionPositionPlanar(), &mSphereMeshes[meshIndex++]));
    }
    mInitializedSpheres = initSphereCount;
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

    // Pre-load the current pipeline variant.
    GetSkyBoxPipeline();
}

void GraphicsBenchmarkApp::SetupSpheresPipelines()
{
    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = mSphere.descriptorSetLayout;
    piCreateInfo.pushConstants.count               = kDebugColorPushConstantCount;
    piCreateInfo.pushConstants.shaderVisiblity     = grfx::SHADER_STAGE_PS;
    piCreateInfo.pushConstants.binding             = SPHERE_COLOR_UNIFORM_BUFFER_REGISTER;
    piCreateInfo.pushConstants.set                 = 0;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSphere.pipelineInterface));

    // Pre-load the current pipeline variant.
    GetSpherePipeline();
}

Result GraphicsBenchmarkApp::CompilePipeline(const SkyBoxPipelineKey& key)
{
    if (mSkyBoxPipelines.find(key) != mSkyBoxPipelines.end()) {
        return SUCCESS;
    }

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
    gpCreateInfo.outputState.renderTargetFormats[0] = key.renderFormat;
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = mSkyBox.pipelineInterface;

    grfx::GraphicsPipelinePtr pipeline = nullptr;
    Result                    ppxres   = GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &pipeline);
    if (ppxres == SUCCESS) {
        // Insert a new pipeline to the cache.
        // We don't delete old pipeline while we run benchmark
        // which require this function to be synchronized to end of frame.
        mSkyBoxPipelines[key] = pipeline;
    }
    return ppxres;
}

Result GraphicsBenchmarkApp::CompilePipeline(const SpherePipelineKey& key)
{
    if (mPipelines.find(key) != mPipelines.end()) {
        return SUCCESS;
    }

    bool            interleaved = (key.vertexAttributeLayout == 0);
    size_t          meshIndex   = kAvailableVertexAttrLayouts.size() * key.vertexFormat + key.vertexAttributeLayout;
    grfx::BlendMode blendMode   = (key.enableAlphaBlend ? grfx::BLEND_MODE_ALPHA : grfx::BLEND_MODE_NONE);

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
    gpCreateInfo.VS                                = {mVsShaders[key.vs].Get(), "vsmain"};
    gpCreateInfo.PS                                = {mPsShaders[key.ps].Get(), "psmain"};
    if (interleaved) {
        // Interleaved pipeline
        gpCreateInfo.vertexInputState.bindingCount = 1;
        gpCreateInfo.vertexInputState.bindings[0]  = mSphereMeshes[meshIndex]->GetDerivedVertexBindings()[0];
    }
    else {
        // Position Planar Pipeline
        gpCreateInfo.vertexInputState.bindingCount = 2;
        gpCreateInfo.vertexInputState.bindings[0]  = mSphereMeshes[meshIndex]->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]  = mSphereMeshes[meshIndex]->GetDerivedVertexBindings()[1];
    }
    gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpCreateInfo.polygonMode                        = (key.enablePolygonModeLine ? grfx::POLYGON_MODE_LINE : grfx::POLYGON_MODE_FILL);
    gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
    gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                    = key.enableDepth;
    gpCreateInfo.depthWriteEnable                   = key.enableDepth;
    gpCreateInfo.blendModes[0]                      = blendMode;
    gpCreateInfo.outputState.renderTargetCount      = 1;
    gpCreateInfo.outputState.renderTargetFormats[0] = key.renderFormat;
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = mSphere.pipelineInterface;

    grfx::GraphicsPipelinePtr pipeline = nullptr;
    Result                    ppxres   = GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &pipeline);
    if (ppxres == SUCCESS) {
        // Insert a new pipeline to the cache.
        // We don't delete old pipeline while we run benchmark
        // which require this function to be synchronized to end of frame.
        mPipelines[key] = pipeline;
    }
    return ppxres;
}

Result GraphicsBenchmarkApp::CompilePipeline(const QuadPipelineKey& key)
{
    if (mQuadsPipelines.find(key) != mQuadsPipelines.end()) {
        return SUCCESS;
    }

    const size_t                      quadTypeIndex = static_cast<size_t>(key.quadType);
    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
    gpCreateInfo.VS                                 = {mVSQuads.Get(), "vsmain"};
    gpCreateInfo.PS                                 = {mQuadsPs[quadTypeIndex].Get(), "psmain"};
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
    gpCreateInfo.outputState.renderTargetFormats[0] = key.renderFormat;
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = mQuadsPipelineInterfaces[quadTypeIndex];

    grfx::GraphicsPipelinePtr pipeline = nullptr;
    Result                    ppxres   = GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &pipeline);
    if (ppxres == SUCCESS) {
        // Insert a new pipeline to the cache.
        // We don't delete old pipeline while we run benchmark
        // which require this function to be synchronized to end of frame.
        mQuadsPipelines[key] = pipeline;
    }
    return ppxres;
}

// Compile or load from cache currently required pipeline.
grfx::GraphicsPipelinePtr GraphicsBenchmarkApp::GetSpherePipeline()
{
    SpherePipelineKey key     = {};
    key.ps                    = static_cast<uint8_t>(pKnobPs->GetIndex());
    key.vs                    = static_cast<uint8_t>(pKnobVs->GetIndex());
    key.vertexFormat          = static_cast<uint8_t>(pKnobVbFormat->GetIndex());
    key.vertexAttributeLayout = static_cast<uint8_t>(pKnobVertexAttrLayout->GetIndex());
    key.enableDepth           = pDepthTestWrite->GetValue();
    key.enableAlphaBlend      = pAlphaBlend->GetValue();
    key.renderFormat          = RenderFormat();
    key.enablePolygonModeLine = (pDebugViews->GetValue() == DebugView::WIREFRAME_MODE);
    PPX_CHECKED_CALL(CompilePipeline(key));
    return mPipelines[key];
}

grfx::GraphicsPipelinePtr GraphicsBenchmarkApp::GetFullscreenQuadPipeline()
{
    QuadPipelineKey key = {};
    key.renderFormat    = RenderFormat();
    key.quadType        = pFullscreenQuadsType->GetValue();
    PPX_CHECKED_CALL(CompilePipeline(key));
    return mQuadsPipelines[key];
}

grfx::GraphicsPipelinePtr GraphicsBenchmarkApp::GetSkyBoxPipeline()
{
    SkyBoxPipelineKey key = {};
    key.renderFormat      = RenderFormat();
    PPX_CHECKED_CALL(CompilePipeline(key));
    return mSkyBoxPipelines[key];
}

void GraphicsBenchmarkApp::SetupFullscreenQuadsPipelines()
{
    // Noise
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = GetPushConstCount(mQuadPushConstant);
        piCreateInfo.pushConstants.binding             = QUADS_CONFIG_UNIFORM_BUFFER_REGISTER;
        piCreateInfo.pushConstants.set                 = 0;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[0]));
    }
    // Solid color
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = GetPushConstCount(mQuadPushConstant);
        piCreateInfo.pushConstants.binding             = QUADS_CONFIG_UNIFORM_BUFFER_REGISTER;
        piCreateInfo.pushConstants.set                 = 0;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[1]));
    }
    // Texture
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mFullscreenQuads.descriptorSetLayout;
        piCreateInfo.pushConstants.count               = GetPushConstCount(mQuadPushConstant);
        piCreateInfo.pushConstants.binding             = QUADS_CONFIG_UNIFORM_BUFFER_REGISTER;
        piCreateInfo.pushConstants.set                 = 0;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mQuadsPipelineInterfaces[2]));
    }

    // Pre-load the current pipeline variant.
    GetFullscreenQuadPipeline();
}

void GraphicsBenchmarkApp::SetupSpheres()
{
    SetupSphereResources();
    SetupSphereMeshes();
    // Pipelines must be setup after meshes to use in vertex bindings.
    SetupSpheresPipelines();
    mSpheresAreSetUp = true;
}

void GraphicsBenchmarkApp::UpdateOffscreenBuffer(grfx::Format format, int w, int h)
{
    {
        // Checking if we still have the same buffer format.
        const auto& frame = mOffscreenFrame.back();
        if ((frame.colorFormat == format) && (frame.width == w) && (frame.height == h)) {
            return;
        }
    }
    GetDevice()->WaitIdle();
    for (auto& frame : mOffscreenFrame) {
        DestroyOffscreenFrame(frame);
        PPX_CHECKED_CALL(CreateOffscreenFrame(frame, format, GetSwapchain()->GetDepthFormat(), w, h));
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
    // Skip the first kSkipFrameCount frames after the knob of quad count being changed to avoid noise
    constexpr uint32_t kSkipFrameCount = 2;

    const bool sphereChanged      = ProcessSphereKnobs();
    const bool quadChanged        = ProcessQuadsKnobs();
    const bool framebufferChanged = ProcessOffscreenRenderKnobs();

    if (sphereChanged || quadChanged || framebufferChanged) {
        mGpuWorkDuration.ClearHistory();
        mCPUSubmissionTime.ClearHistory();

        mSkipRecordBandwidthMetricFrameCounter = kSkipFrameCount;
    }
}

bool GraphicsBenchmarkApp::ProcessSphereKnobs()
{
    // Detect if any knob value has been changed
    // Note: DigestUpdate should be called only once per frame (DigestUpdate unflags the internal knob variable)!
    const bool allTexturesTo1x1KnobChanged    = pAllTexturesTo1x1->DigestUpdate();
    const bool alphaBlendKnobChanged          = pAlphaBlend->DigestUpdate();
    const bool depthTestWriteKnobChanged      = pDepthTestWrite->DigestUpdate();
    const bool enableSpheresKnobChanged       = pEnableSpheres->DigestUpdate();
    const bool sphereInstanceCountKnobChanged = pSphereInstanceCount->DigestUpdate();
    const bool debugViewChanged               = pDebugViews->DigestUpdate();
    const bool anyUpdate =
        (allTexturesTo1x1KnobChanged ||
         alphaBlendKnobChanged ||
         depthTestWriteKnobChanged ||
         enableSpheresKnobChanged ||
         sphereInstanceCountKnobChanged ||
         debugViewChanged);

    // TODO: Ideally, the `maxValue` of the drawcall-count slider knob should be changed at runtime.
    // Currently, the value of the drawcall-count is adjusted to the sphere-count in case the
    // former exceeds the value of the sphere-count.
    if (pDrawCallCount->GetValue() > pSphereInstanceCount->GetValue()) {
        pDrawCallCount->SetValue(pSphereInstanceCount->GetValue());
    }

    // If the debug view is enabled, then change the pixel shader to PsSimple.
    if (pDebugViews->GetIndex() == static_cast<size_t>(DebugView::SHOW_DRAWCALLS)) {
        pKnobPs->SetIndex(static_cast<size_t>(SpherePS::SPHERE_PS_SIMPLE));
    }

    const bool enableSpheres = pEnableSpheres->GetValue();
    // Set visibilities
    if (enableSpheresKnobChanged) {
        pDebugViews->SetVisible(enableSpheres);
        pKnobVs->SetVisible(enableSpheres);
        pKnobLOD->SetVisible(enableSpheres);
        pKnobVbFormat->SetVisible(enableSpheres);
        pKnobVertexAttrLayout->SetVisible(enableSpheres);
        pSphereInstanceCount->SetVisible(enableSpheres);
        pDrawCallCount->SetVisible(enableSpheres);
        pAlphaBlend->SetVisible(enableSpheres);
        pDepthTestWrite->SetVisible(enableSpheres);
    }
    pKnobPs->SetVisible(enableSpheres && (pDebugViews->GetIndex() != static_cast<size_t>(DebugView::SHOW_DRAWCALLS)));
    pAllTexturesTo1x1->SetVisible(enableSpheres && (pKnobPs->GetValue() == SpherePS::SPHERE_PS_MEM_BOUND));

    if (enableSpheres) {
        // Update sphere resources and mesh
        if (!mSpheresAreSetUp) {
            // This creates all resources.
            SetupSpheres();
        }
        else {
            const uint32_t initializedCount   = static_cast<uint32_t>(pSphereInstanceCount->GetValue());
            const bool     requireMoreSpheres = sphereInstanceCountKnobChanged && (initializedCount > mInitializedSpheres);
            if (requireMoreSpheres) {
                SetupSphereMeshes();
            }
            // Update descriptors
            if (allTexturesTo1x1KnobChanged) {
                UpdateSphereDescriptors();
            }
        }
    }
    return anyUpdate;
}

bool GraphicsBenchmarkApp::ProcessOffscreenRenderKnobs()
{
    const bool offscreenChanged   = pRenderOffscreen->DigestUpdate();
    const bool framebufferChanged = pResolution->DigestUpdate() || pFramebufferFormat->DigestUpdate();
    const bool anyUpdate          = offscreenChanged || framebufferChanged;

    if (offscreenChanged) {
        pBlitOffscreen->SetVisible(pRenderOffscreen->GetValue());
        pFramebufferFormat->SetVisible(pRenderOffscreen->GetValue());
        pResolution->SetVisible(pRenderOffscreen->GetValue());
    }

    if ((offscreenChanged && pRenderOffscreen->GetValue()) || framebufferChanged) {
        std::pair<int, int> resolution = pResolution->GetValue();

        int fbWidth  = (resolution.first > 0 ? resolution.first : GetSwapchain()->GetWidth());
        int fbHeight = (resolution.second > 0 ? resolution.second : GetSwapchain()->GetHeight());
        UpdateOffscreenBuffer(RenderFormat(), fbWidth, fbHeight);
    }
    return anyUpdate;
}

bool GraphicsBenchmarkApp::ProcessQuadsKnobs()
{
    const bool countUpdated      = pFullscreenQuadsCount->DigestUpdate();
    const bool typeUpdated       = pFullscreenQuadsType->DigestUpdate();
    const bool renderpassUpdated = pFullscreenQuadsSingleRenderpass->DigestUpdate();
    const bool colorUpdated      = pFullscreenQuadsColor->DigestUpdate();
    const bool anyUpdate         = countUpdated || typeUpdated || renderpassUpdated || colorUpdated;

    // Set Visibilities
    if (pFullscreenQuadsCount->GetValue() > 0) {
        pFullscreenQuadsType->SetVisible(true);
        pFullscreenQuadsSingleRenderpass->SetVisible(true);
        if (pFullscreenQuadsType->GetValue() == FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_SOLID_COLOR) {
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
    return anyUpdate;
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

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.uiCmd;
    submitInfo.waitSemaphoreCount   = 0;
    submitInfo.ppWaitSemaphores     = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.ppSignalSemaphores   = nullptr;

    submitInfo.pFence = frame.uiRenderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
}
#endif

void GraphicsBenchmarkApp::Render()
{
    ProcessInput();
    ProcessKnobs();

    PerFrame& frame            = mPerFrame[0];
    uint32_t  currentViewIndex = 0;

#if defined(PPX_BUILD_XR)
    // Render UI into a different composition layer.
    if (IsXrEnabled()) {
        currentViewIndex = GetXrComponent().GetCurrentViewIndex();
        if ((currentViewIndex == 0) && GetSettings()->enableImGui) {
            RecordAndSubmitCommandBufferGUIXR(frame);
        }
    }
#endif

    uint32_t           imageIndex = UINT32_MAX;
    grfx::SwapchainPtr swapchain  = GetSwapchain(currentViewIndex);

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

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

    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0, 0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, sizeof(data)));
        mGpuWorkDuration.Update(data[1] - data[0]);
    }
    // Reset query
    frame.timestampQuery->Reset(/* firstQuery= */ 0, frame.timestampQuery->GetCount());

    // Update scene data

    const Camera& camera                 = GetCamera();
    frame.sceneData.viewProjectionMatrix = camera.GetViewProjectionMatrix();

    RenderPasses swapchainRenderPasses = SwapchainRenderPasses(swapchain, imageIndex);
    RenderPasses renderPasses          = swapchainRenderPasses;
    if (pRenderOffscreen->GetValue()) {
        renderPasses = OffscreenRenderPasses(mOffscreenFrame[0]);
        if (pBlitOffscreen->GetValue()) {
            renderPasses.uiRenderPass      = swapchainRenderPasses.uiRenderPass;
            renderPasses.uiClearRenderPass = swapchainRenderPasses.uiClearRenderPass;
            renderPasses.blitRenderPass    = swapchainRenderPasses.noloadRenderPass;
        }
        else {
            renderPasses.uiRenderPass      = swapchainRenderPasses.uiClearRenderPass;
            renderPasses.uiClearRenderPass = swapchainRenderPasses.uiClearRenderPass;
        }
    }

    RecordCommandBuffer(frame, renderPasses, imageIndex);

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

    Timer timerSubmit;
    PPX_ASSERT_MSG(timerSubmit.Start() == TIMER_RESULT_SUCCESS, "Error starting the Timer");
    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    mCPUSubmissionTime.Update(timerSubmit.MillisSinceStart());

#if defined(PPX_BUILD_XR)
    // No need to present when XR is enabled.
    if (!IsXrEnabled())
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
        ImGui::SetNextWindowPos({(GetUIWidth() - lastImGuiWindowSize.x) / 2, (GetUIHeight() - lastImGuiWindowSize.y) / 2}, ImGuiCond_FirstUseEver, {0.0f, 0.0f});
        ImGui::SetNextWindowSize({512, 512}, ImGuiCond_FirstUseEver);
    }
#endif

    // GUI
    ImGui::Begin("Debug Window");
    DrawExtraInfo();
    ImGui::Separator();
    GetKnobManager().DrawAllKnobs(true);
    ImGui::End();
}

void GraphicsBenchmarkApp::DrawExtraInfo()
{
    constexpr const char* kUTF8PlusMinus = "\xC2\xB1";

    ImGui::Columns(2);

    ImGui::Text("CPU Submission Time");
    ImGui::NextColumn();
    ImGui::Text("%.4f ms\n%.4f%s%.4f ms", mCPUSubmissionTime.Value(), mCPUSubmissionTime.Mean(), kUTF8PlusMinus, mCPUSubmissionTime.Std());
    ImGui::NextColumn();

    if (HasActiveMetricsRun()) {
        const auto cpuSubmissionTime = GetGaugeBasicStatistics(mMetricsData.metrics[MetricsData::kTypeCPUSubmissionTime]);

        ImGui::Text("CPU Average Submission Time");
        ImGui::NextColumn();
        ImGui::Text("%.2f ms", cpuSubmissionTime.average);
        ImGui::NextColumn();

        ImGui::Text("CPU Min Submission Time");
        ImGui::NextColumn();
        ImGui::Text("%.2f ms", cpuSubmissionTime.min);
        ImGui::NextColumn();

        ImGui::Text("CPU Max Submission Time");
        ImGui::NextColumn();
        ImGui::Text("%.2f ms", cpuSubmissionTime.max);
        ImGui::NextColumn();
    }

    if (mInitializedSpheres) {
        ImGui::Text("Initialized Spheres");
        ImGui::NextColumn();
        ImGui::Text("%d ", mInitializedSpheres);
        ImGui::NextColumn();
    }

    uint64_t frequency = 0;
    PPX_CHECKED_CALL(GetGraphicsQueue()->GetTimestampFrequency(&frequency));
    const float sPerTick  = 1.0f / static_cast<float>(frequency);
    const float msPerTick = 1000.0f / static_cast<float>(frequency);

    const float gpuWorkDurationInMs    = msPerTick * mGpuWorkDuration.Value();
    const float gpuWorkDurationAvgInMs = msPerTick * mGpuWorkDuration.Mean();
    const float gpuWorkDurationStdInMs = msPerTick * mGpuWorkDuration.Std();
    ImGui::Text("GPU Work Duration");
    ImGui::NextColumn();
    ImGui::Text("%.4f ms\n%.4f%s%.4f ms", gpuWorkDurationInMs, gpuWorkDurationAvgInMs, kUTF8PlusMinus, gpuWorkDurationStdInMs);
    ImGui::NextColumn();

    const float gpuFPS    = 1.0f / (sPerTick * mGpuWorkDuration.Value());
    const float gpuAvgFPS = 1.0f / (sPerTick * mGpuWorkDuration.Mean());
    const float gpuStdFPS = 1.0f / (sPerTick * (mGpuWorkDuration.Mean() - mGpuWorkDuration.Std())) - gpuAvgFPS;
    ImGui::Text("GPU FPS");
    ImGui::NextColumn();
    ImGui::Text("%.2f fps\n%.2f%s%.2f fps", gpuFPS, gpuAvgFPS, kUTF8PlusMinus, gpuStdFPS);
    ImGui::NextColumn();

    const grfx::Format swapchainColorFormat = GetSwapchain()->GetColorFormat();
    const uint32_t     swapchainWidth       = GetSwapchain()->GetWidth();
    const uint32_t     swapchainHeight      = GetSwapchain()->GetHeight();
    ImGui::Text("Swapchain resolution");
    ImGui::NextColumn();
    ImGui::Text("%d x %d", swapchainWidth, swapchainHeight);
    ImGui::NextColumn();

    const bool         isOffscreen = pRenderOffscreen->GetValue();
    const grfx::Format colorFormat = isOffscreen ? mOffscreenFrame.back().colorFormat : swapchainColorFormat;
    const uint32_t     width       = isOffscreen ? mOffscreenFrame.back().width : swapchainWidth;
    const uint32_t     height      = isOffscreen ? mOffscreenFrame.back().height : swapchainHeight;
    if (isOffscreen) {
        ImGui::Text("Framebuffer");
        ImGui::NextColumn();
        ImGui::Text("%d x %d (%s)", width, height, ToString(colorFormat));
        ImGui::NextColumn();
    }

    const uint32_t quad_count = pFullscreenQuadsCount->GetValue();
    if (quad_count) {
        const auto  texelSize     = static_cast<float>(grfx::GetFormatDescription(colorFormat)->bytesPerTexel);
        const float dataWriteInGb = (static_cast<float>(width) * static_cast<float>(height) * texelSize * quad_count) / (1024.f * 1024.f * 1024.f);
        ImGui::Text("Write Data");
        ImGui::NextColumn();
        ImGui::Text("%.2f GB", dataWriteInGb);
        ImGui::NextColumn();

        const float gpuWorkDurationInS    = mGpuWorkDuration.Value() * sPerTick;
        const float gpuWorkDurationAvgInS = mGpuWorkDuration.Mean() * sPerTick;
        const float gpuWorkDurationStdInS = mGpuWorkDuration.Std() * sPerTick;
        const float gpuBandwidth          = dataWriteInGb / gpuWorkDurationInS;
        const float gpuBandwidthAvg       = dataWriteInGb / gpuWorkDurationAvgInS;
        const float gpuBandwidthStd       = dataWriteInGb / (gpuWorkDurationAvgInS - gpuWorkDurationStdInS) - gpuBandwidthAvg;
        ImGui::Text("Write Bandwidth");
        ImGui::NextColumn();
        ImGui::Text("%.2f GB/s\n%.2f%s%.2f GB/s", gpuBandwidth, gpuBandwidthAvg, kUTF8PlusMinus, gpuBandwidthStd);
        ImGui::NextColumn();

        if (HasActiveMetricsRun()) {
            const auto bandwidth = GetGaugeBasicStatistics(mMetricsData.metrics[MetricsData::kTypeBandwidth]);
            ImGui::Text("Average Write Bandwidth");
            ImGui::NextColumn();
            ImGui::Text("%.2f GB/s", bandwidth.average);
            ImGui::NextColumn();

            ImGui::Text("Min Write Bandwidth");
            ImGui::NextColumn();
            ImGui::Text("%.2f GB/s", bandwidth.min);
            ImGui::NextColumn();

            ImGui::Text("Max Write Bandwidth");
            ImGui::NextColumn();
            ImGui::Text("%.2f GB/s", bandwidth.max);
            ImGui::NextColumn();
        }
    }
    ImGui::Columns(1);
}

GraphicsBenchmarkApp::RenderPasses GraphicsBenchmarkApp::SwapchainRenderPasses(grfx::SwapchainPtr swapchain, uint32_t imageIndex)
{
    return RenderPasses{
        swapchain->GetRenderPass(imageIndex, ppx::grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_LOAD),
        swapchain->GetRenderPass(imageIndex, ppx::grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_CLEAR),
        swapchain->GetRenderPass(imageIndex, ppx::grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_DONT_CARE),
        swapchain->GetRenderPass(imageIndex, ppx::grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_LOAD),
        swapchain->GetRenderPass(imageIndex, ppx::grfx::AttachmentLoadOp::ATTACHMENT_LOAD_OP_CLEAR),
    };
}

ppx::Result GraphicsBenchmarkApp::CreateBlitContext(BlitContext& blit)
{
    // Descriptor set layout
    {
        // Descriptor set layout
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLER));

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &blit.descriptorSetLayout));
    }

    // Pipeline
    {
        SetupShader("basic/shaders", "FullScreenTriangle.vs", &blit.vs);
        SetupShader("basic/shaders", "FullScreenTriangle.ps", &blit.ps);

        grfx::FullscreenQuadCreateInfo createInfo = {};
        createInfo.VS                             = blit.vs;
        createInfo.PS                             = blit.ps;
        createInfo.setCount                       = 1;
        createInfo.sets[0].set                    = 0;
        createInfo.sets[0].pLayout                = blit.descriptorSetLayout;
        createInfo.renderTargetCount              = 1;
        createInfo.renderTargetFormats[0]         = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat             = GetSwapchain()->GetDepthFormat();

        PPX_CHECKED_CALL(GetDevice()->CreateFullscreenQuad(&createInfo, &blit.quad));
    }

    return ppx::SUCCESS;
}

GraphicsBenchmarkApp::RenderPasses GraphicsBenchmarkApp::OffscreenRenderPasses(const OffscreenFrame& frame)
{
    return RenderPasses{
        frame.loadRenderPass,
        frame.clearRenderPass,
        frame.noloadRenderPass,
        nullptr,
        nullptr,
        nullptr,
        &frame,
    };
}

void GraphicsBenchmarkApp::DestroyOffscreenFrame(OffscreenFrame& frame)
{
    GetDevice()->DestroyRenderPass(frame.loadRenderPass);
    GetDevice()->DestroyRenderPass(frame.clearRenderPass);
    GetDevice()->DestroyRenderPass(frame.noloadRenderPass);

    for (auto& rtv : frame.renderTargetViews) {
        GetDevice()->DestroyRenderTargetView(rtv);
    }
    GetDevice()->DestroyDepthStencilView(frame.depthStencilView);

    GetDevice()->FreeDescriptorSet(frame.blitDescriptorSet);
    GetDevice()->DestroyTexture(frame.blitSource);

    GetDevice()->DestroyImage(frame.colorImage);
    GetDevice()->DestroyImage(frame.depthImage);

    // Reset all the pointers to nullptr.
    frame = {};
}

ppx::Result GraphicsBenchmarkApp::CreateOffscreenFrame(OffscreenFrame& frame, grfx::Format colorFormat, grfx::Format depthFormat, uint32_t width, uint32_t height)
{
    frame = OffscreenFrame{width, height, colorFormat, depthFormat};
    {
        grfx::ImageCreateInfo colorCreateInfo   = grfx::ImageCreateInfo::RenderTarget2D(width, height, colorFormat);
        colorCreateInfo.initialState            = grfx::RESOURCE_STATE_RENDER_TARGET;
        colorCreateInfo.usageFlags.bits.sampled = true;
        ppx::Result ppxres                      = GetDevice()->CreateImage(&colorCreateInfo, &frame.colorImage);
        if (ppxres != ppx::SUCCESS) {
            return ppxres;
        }
    }

    if (depthFormat != grfx::Format::FORMAT_UNDEFINED) {
        grfx::ImageCreateInfo depthCreateInfo = grfx::ImageCreateInfo::DepthStencilTarget(width, height, depthFormat);
        ppx::Result           ppxres          = GetDevice()->CreateImage(&depthCreateInfo, &frame.depthImage);
        if (ppxres != ppx::SUCCESS) {
            return ppxres;
        }
    }

    if (frame.depthImage) {
        grfx::DepthStencilViewCreateInfo dsvCreateInfo = {};
        dsvCreateInfo =
            grfx::DepthStencilViewCreateInfo::GuessFromImage(frame.depthImage);
        dsvCreateInfo.depthLoadOp   = ppx::grfx::ATTACHMENT_LOAD_OP_CLEAR;
        dsvCreateInfo.stencilLoadOp = ppx::grfx::ATTACHMENT_LOAD_OP_CLEAR;
        dsvCreateInfo.ownership     = ppx::grfx::OWNERSHIP_RESTRICTED;

        Result ppxres = GetDevice()->CreateDepthStencilView(&dsvCreateInfo, &frame.depthStencilView);
        if (ppxres != ppx::SUCCESS) {
            return ppxres;
        }
    }

    struct
    {
        grfx::RenderPassPtr&       ptr;
        grfx::AttachmentLoadOp     op;
        grfx::RenderTargetViewPtr& rtv;
    } renderpasses[] = {
        {frame.loadRenderPass, grfx::ATTACHMENT_LOAD_OP_LOAD, frame.renderTargetViews[0]},
        {frame.clearRenderPass, grfx::ATTACHMENT_LOAD_OP_CLEAR, frame.renderTargetViews[1]},
        {frame.noloadRenderPass, grfx::ATTACHMENT_LOAD_OP_DONT_CARE, frame.renderTargetViews[2]},
    };

    for (auto& renderpass : renderpasses) {
        grfx::RenderTargetViewCreateInfo rtvCreateInfo =
            grfx::RenderTargetViewCreateInfo::GuessFromImage(frame.colorImage);
        rtvCreateInfo.loadOp    = renderpass.op;
        rtvCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
        Result ppxres           = GetDevice()->CreateRenderTargetView(&rtvCreateInfo, &renderpass.rtv);

        grfx::RenderPassCreateInfo rpCreateInfo = {};
        rpCreateInfo.width                      = width;
        rpCreateInfo.height                     = height;
        rpCreateInfo.renderTargetCount          = 1;
        rpCreateInfo.pRenderTargetViews[0]      = renderpass.rtv;
        rpCreateInfo.pDepthStencilView          = frame.depthStencilView;
        rpCreateInfo.renderTargetClearValues[0] = {{0.0f, 0.0f, 0.0f, 0.0f}};
        rpCreateInfo.depthStencilClearValue     = {1.0f, 0xFF};
        rpCreateInfo.ownership                  = grfx::OWNERSHIP_RESTRICTED;

        GetDevice()->CreateRenderPass(&rpCreateInfo, &renderpass.ptr);
    }

    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.pImage                          = frame.colorImage;
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = width;
        createInfo.height                          = height;
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = colorFormat;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.inputAttachment = true;
        createInfo.usageFlags.bits.sampled         = true;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.storage         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &frame.blitSource));
    }

    // Allocate descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBlit.descriptorSetLayout, &frame.blitDescriptorSet));

    // Write descriptors
    {
        grfx::WriteDescriptor writes[2] = {};
        writes[0].binding               = 0;
        writes[0].arrayIndex            = 0;
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageView            = frame.blitSource->GetSampledImageView();

        writes[1].binding  = 1;
        writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pSampler = mLinearSampler;

        PPX_CHECKED_CALL(frame.blitDescriptorSet->UpdateDescriptors(2, writes));
    }

    return ppx::SUCCESS;
}

ppx::grfx::Format GraphicsBenchmarkApp::RenderFormat()
{
    grfx::Format renderFormat = pFramebufferFormat->GetValue();
    if (!pRenderOffscreen->GetValue() || renderFormat == grfx::Format::FORMAT_UNDEFINED) {
        return GetSwapchain()->GetColorFormat();
    }
    return renderFormat;
}

void GraphicsBenchmarkApp::CreateColorsForDrawCalls()
{
    // Create colors randomly.
    mColorsForDrawCalls.resize(kMaxSphereInstanceCount);
    for (size_t i = 0; i < kMaxSphereInstanceCount; i++) {
        mColorsForDrawCalls[i] = float4(mRandom.Float(), mRandom.Float(), mRandom.Float(), 0.5f);
    }
}

void GraphicsBenchmarkApp::RecordCommandBuffer(PerFrame& frame, const RenderPasses& renderpasses, uint32_t imageIndex)
{
    PPX_CHECKED_CALL(frame.cmd->Begin());

    // Write start timestamp
    frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 0);
    if (!pRenderOffscreen->GetValue()) {
        frame.cmd->SetScissors(GetScissor());
        frame.cmd->SetViewports(GetViewport());
    }
    else {
        uint32_t width  = mOffscreenFrame.back().width;
        uint32_t height = mOffscreenFrame.back().height;
        frame.cmd->SetScissors({0, 0, width, height});
        frame.cmd->SetViewports({0, 0, static_cast<float>(width), static_cast<float>(height), 0.0, 1.0});
    }

    grfx::RenderPassPtr currentRenderPass = renderpasses.clearRenderPass;
    PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");

    const grfx::ImagePtr framebufferImage = currentRenderPass->GetRenderTargetImage(0);
    const grfx::ImagePtr swapchainImage   = (renderpasses.offscreen ? renderpasses.uiRenderPass->GetRenderTargetImage(0) : framebufferImage);

    const grfx::ResourceState presentState      = IsXrEnabled() ? grfx::RESOURCE_STATE_RENDER_TARGET : grfx::RESOURCE_STATE_PRESENT;
    grfx::ResourceState       resourceStates[2] = {presentState, presentState};
    grfx::ResourceState*      swapchainState    = &resourceStates[0];
    grfx::ResourceState*      framebufferState  = (renderpasses.offscreen ? &resourceStates[1] : &resourceStates[0]);

    if (*framebufferState != grfx::RESOURCE_STATE_RENDER_TARGET) {
        // Transition image layout PRESENT->RENDER before the first renderpass
        frame.cmd->TransitionImageLayout(framebufferImage, PPX_ALL_SUBRESOURCES, *framebufferState, grfx::RESOURCE_STATE_RENDER_TARGET);
        *framebufferState = grfx::RESOURCE_STATE_RENDER_TARGET;
    }

    bool renderScene = pEnableSkyBox->GetValue() || pEnableSpheres->GetValue();
    if (renderScene) {
        // Record commands for the scene using one renderpass
        frame.cmd->BeginRenderPass(currentRenderPass);
        if (pEnableSkyBox->GetValue()) {
            RecordCommandBufferSkyBox(frame);
        }
        if (pEnableSpheres->GetValue() && mInitializedSpheres > 0) {
            RecordCommandBufferSpheres(frame);
        }
        frame.cmd->EndRenderPass();
    }

    // Record commands for the fullscreen quads using one/multiple renderpasses
    uint32_t quadsCount       = pFullscreenQuadsCount->GetValue();
    bool     singleRenderpass = pFullscreenQuadsSingleRenderpass->GetValue();
    if (quadsCount > 0) {
        currentRenderPass = renderpasses.noloadRenderPass;
        frame.cmd->BindGraphicsPipeline(GetFullscreenQuadPipeline());
        frame.cmd->BindVertexBuffers(1, &mFullscreenQuads.vertexBuffer, &mFullscreenQuads.vertexBinding.GetStride());

        if (pFullscreenQuadsType->GetValue() == FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_TEXTURE) {
            frame.cmd->BindGraphicsDescriptorSets(mQuadsPipelineInterfaces.at(pFullscreenQuadsType->GetIndex()), 1, &mFullscreenQuads.descriptorSets.at(GetInFlightFrameIndex()));
        }

        // Begin the first renderpass used for quads
        frame.cmd->BeginRenderPass(currentRenderPass);
        for (size_t i = 0; i < quadsCount; i++) {
            RecordCommandBufferFullscreenQuad(frame, i);
            if (!singleRenderpass) {
                // If quads are using multiple renderpasses, transition image layout in between to force resolve
                frame.cmd->EndRenderPass();
                frame.cmd->TransitionImageLayout(framebufferImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_SHADER_RESOURCE);
                frame.cmd->TransitionImageLayout(framebufferImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);

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
    // Note the framebuffer is still in RENDER_TARGET state, although it should not really be a problem.
    frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, /* queryIndex = */ 1);

    if (renderpasses.blitRenderPass) {
        currentRenderPass = renderpasses.blitRenderPass;
        PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");

        if (*framebufferState != grfx::RESOURCE_STATE_SHADER_RESOURCE) {
            frame.cmd->TransitionImageLayout(framebufferImage, PPX_ALL_SUBRESOURCES, *framebufferState, grfx::RESOURCE_STATE_SHADER_RESOURCE);
            *framebufferState = grfx::RESOURCE_STATE_SHADER_RESOURCE;
        }
        if (*swapchainState != grfx::RESOURCE_STATE_RENDER_TARGET) {
            frame.cmd->TransitionImageLayout(swapchainImage, PPX_ALL_SUBRESOURCES, *swapchainState, grfx::RESOURCE_STATE_RENDER_TARGET);
            *swapchainState = grfx::RESOURCE_STATE_RENDER_TARGET;
        }

        frame.cmd->SetScissors(GetScissor());
        frame.cmd->SetViewports(GetViewport());

        frame.cmd->BeginRenderPass(currentRenderPass);
        const grfx::DescriptorSet* descriptorSet = renderpasses.offscreen->blitDescriptorSet.Get();
        frame.cmd->Draw(mBlit.quad, 1, &descriptorSet);
        frame.cmd->EndRenderPass();
    }

    // Record commands for the GUI using one last renderpass
    if (
#if defined(PPX_BUILD_XR)
        !IsXrEnabled() &&
#endif
        GetSettings()->enableImGui) {

        if (*swapchainState != grfx::RESOURCE_STATE_RENDER_TARGET) {
            frame.cmd->TransitionImageLayout(swapchainImage, PPX_ALL_SUBRESOURCES, *swapchainState, grfx::RESOURCE_STATE_RENDER_TARGET);
            *swapchainState = grfx::RESOURCE_STATE_RENDER_TARGET;
        }
        currentRenderPass = (renderScene || quadsCount > 0) ? renderpasses.uiRenderPass : renderpasses.uiClearRenderPass;
        PPX_ASSERT_MSG(!currentRenderPass.IsNull(), "render pass object is null");
        frame.cmd->BeginRenderPass(currentRenderPass);
        UpdateGUI();
        DrawImGui(frame.cmd);
        frame.cmd->EndRenderPass();
    }

    if (*framebufferState != presentState) {
        frame.cmd->TransitionImageLayout(framebufferImage, PPX_ALL_SUBRESOURCES, *framebufferState, presentState);
        *framebufferState = presentState;
    }

    if (*swapchainState != presentState) {
        frame.cmd->TransitionImageLayout(swapchainImage, PPX_ALL_SUBRESOURCES, *swapchainState, presentState);
        *swapchainState = presentState;
    }

    // Resolve queries
    frame.cmd->ResolveQueryData(frame.timestampQuery, /* startIndex= */ 0, frame.timestampQuery->GetCount());

    PPX_CHECKED_CALL(frame.cmd->End());
}

void GraphicsBenchmarkApp::RecordCommandBufferSkyBox(PerFrame& frame)
{
    // Bind resources
    frame.cmd->BindGraphicsPipeline(GetSkyBoxPipeline());
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
    frame.cmd->BindGraphicsPipeline(GetSpherePipeline());
    const size_t meshIndex = mMeshesIndexer.GetIndex({pKnobLOD->GetIndex(), pKnobVbFormat->GetIndex(), pKnobVertexAttrLayout->GetIndex()});
    frame.cmd->BindIndexBuffer(mSphereMeshes[meshIndex]);
    frame.cmd->BindVertexBuffers(mSphereMeshes[meshIndex]);

    frame.cmd->BindGraphicsDescriptorSets(mSphere.pipelineInterface, 1, &mSphere.descriptorSets.at(GetInFlightFrameIndex()));

    // Snapshot some scene-related values for the current frame
    uint32_t currentSphereCount   = std::min<uint32_t>(pSphereInstanceCount->GetValue(), mInitializedSpheres);
    uint32_t currentDrawCallCount = pDrawCallCount->GetValue();
    uint32_t mSphereIndexCount    = mSphereMeshes[meshIndex]->GetIndexCount() / mInitializedSpheres;
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
    frame.cmd->PushGraphicsConstants(mSphere.pipelineInterface, kDebugColorPushConstantCount, &kDefaultDrawCallColor);

    for (uint32_t i = 0; i < currentDrawCallCount; i++) {
        if (pDebugViews->GetIndex() == static_cast<size_t>(DebugView::SHOW_DRAWCALLS)) {
            frame.cmd->PushGraphicsConstants(mSphere.pipelineInterface, kDebugColorPushConstantCount, &mColorsForDrawCalls[i]);
        }

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
    // Vertex shader push constant
    {
        mQuadPushConstant.InstCount = pKnobAluCount->GetValue();
        frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[0], GetPushConstCount(mQuadPushConstant.InstCount), &mQuadPushConstant.InstCount, offsetof(QuadPushConstant, InstCount) / sizeof(uint32_t));
    }
    switch (pFullscreenQuadsType->GetValue()) {
        case FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_NOISE: {
            mQuadPushConstant.RandomSeed = seed;
            frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[0], GetPushConstCount(mQuadPushConstant.RandomSeed), &mQuadPushConstant.RandomSeed, offsetof(QuadPushConstant, RandomSeed) / sizeof(uint32_t));
            break;
        }
        case FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_SOLID_COLOR: {
            // zigzag the intensity between (0.5 ~ 1.0) in steps of 0.1
            //     index:   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0...
            // intensity: 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0...
            float index = static_cast<float>(seed % 10);
            float intensity;
            if (index > 4.5) {
                intensity = (index / 10.0f);
            }
            else {
                intensity = (1.0f - (index / 10.f));
            }
            float3 colorValues = pFullscreenQuadsColor->GetValue();
            colorValues *= intensity;
            mQuadPushConstant.ColorValue = colorValues;
            frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[0], GetPushConstCount(mQuadPushConstant.ColorValue), &mQuadPushConstant.ColorValue, offsetof(QuadPushConstant, ColorValue) / sizeof(uint32_t));
            break;
        }
        case FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_TEXTURE:
            mQuadPushConstant.TextureCount = pKnobTextureCount->GetValue();
            frame.cmd->PushGraphicsConstants(mQuadsPipelineInterfaces[0], GetPushConstCount(mQuadPushConstant.TextureCount), &mQuadPushConstant.TextureCount, offsetof(QuadPushConstant, TextureCount) / sizeof(uint32_t));

            break;
        default:
            PPX_ASSERT_MSG(true, "unsupported FullscreenQuadsType: " << static_cast<int>(pFullscreenQuadsType->GetValue()));
            break;
    }

    frame.cmd->Draw(3, 1, 0, 0);
}

void GraphicsBenchmarkApp::SetupShader(const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule)
{
    SetupShader(kShaderBaseDir, fileName, ppShaderModule);
}

void GraphicsBenchmarkApp::SetupShader(const char* baseDir, const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule)
{
    std::vector<char> bytecode = LoadShader(baseDir, fileName);
    PPX_ASSERT_MSG(!bytecode.empty(), "shader bytecode load failed for " << kShaderBaseDir << " " << fileName);
    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, ppShaderModule));
}

const ppx::Camera& GraphicsBenchmarkApp::GetCamera() const
{
#if defined(PPX_BUILD_XR)
    if (IsXrEnabled()) {
        return GetXrComponent().GetCamera();
    }
#endif
    return mCamera;
}
