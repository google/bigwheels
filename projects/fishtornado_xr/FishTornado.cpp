// Copyright 2022 Google LLC
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

#include "FishTornado.h"
#include "ppx/graphics_util.h"
#include "ppx/csv_file_log.h"
#include "ppx/fs.h"

#include <filesystem>

#define ENABLE_GPU_QUERIES

namespace {

constexpr uint32_t kShadowRes          = 1024;
constexpr uint32_t kCausticsImageCount = 32;
constexpr float3   kFogColor           = float3(15.0f, 86.0f, 107.0f) / 255.0f;
constexpr float3   kFloorColor         = float3(145.0f, 189.0f, 155.0f) / 255.0f;
constexpr float    kMetricsWritePeriod = 5.f;
constexpr char     kMetricsFilename[]  = "ft_metrics.csv";

} // namespace

FishTornadoApp* FishTornadoApp::GetThisApp()
{
    FishTornadoApp* pApp = static_cast<FishTornadoApp*>(Application::Get());
    return pApp;
}

grfx::DescriptorSetPtr FishTornadoApp::GetSceneSet(uint32_t frameIndex) const
{
    return mPerFrame[frameIndex].sceneSet;
}

grfx::DescriptorSetPtr FishTornadoApp::GetSceneShadowSet(uint32_t frameIndex) const
{
    return mPerFrame[frameIndex].sceneShadowSet;
}

grfx::TexturePtr FishTornadoApp::GetShadowTexture(uint32_t frameIndex) const
{
    return mPerFrame[frameIndex].shadowDrawPass->GetDepthStencilTexture();
}

grfx::GraphicsPipelinePtr FishTornadoApp::CreateForwardPipeline(
    const std::filesystem::path& baseDir,
    const std::string&           vsBaseName,
    const std::string&           psBaseName,
    grfx::PipelineInterface*     pPipelineInterface)
{
    grfx::ShaderModulePtr VS, PS;
    PPX_CHECKED_CALL(CreateShader(baseDir, vsBaseName, &VS));
    PPX_CHECKED_CALL(CreateShader(baseDir, psBaseName, &PS));

    const grfx::VertexInputRate inputRate = grfx::VERTEX_INPUT_RATE_VERTEX;
    grfx::VertexDescription     vertexDescription;
    // clang-format off
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_POSITION , 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_COLOR    , 1, grfx::FORMAT_R32G32B32_FLOAT, 1, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_NORMAL   , 2, grfx::FORMAT_R32G32B32_FLOAT, 2, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_TEXCOORD , 3, grfx::FORMAT_R32G32_FLOAT,    3, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_TANGENT  , 4, grfx::FORMAT_R32G32B32_FLOAT, 4, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    vertexDescription.AppendBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_BITANGENT, 5, grfx::FORMAT_R32G32B32_FLOAT, 5, PPX_APPEND_OFFSET_ALIGNED, inputRate});
    // clang-format on

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
    gpCreateInfo.VS                                 = {VS, "vsmain"};
    gpCreateInfo.PS                                 = {PS, "psmain"};
    gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
    gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
    gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                    = true;
    gpCreateInfo.depthWriteEnable                   = true;
    gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
    gpCreateInfo.outputState.renderTargetCount      = 1;
    gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = IsNull(pPipelineInterface) ? mForwardPipelineInterface.Get() : pPipelineInterface;
    // Vertex description
    gpCreateInfo.vertexInputState.bindingCount = vertexDescription.GetBindingCount();
    for (uint32_t i = 0; i < vertexDescription.GetBindingCount(); ++i) {
        gpCreateInfo.vertexInputState.bindings[i] = *vertexDescription.GetBinding(i);
    }

    grfx::GraphicsPipelinePtr pipeline;
    PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &pipeline));

    GetDevice()->DestroyShaderModule(VS);
    GetDevice()->DestroyShaderModule(PS);

    return pipeline;
}

grfx::GraphicsPipelinePtr FishTornadoApp::CreateShadowPipeline(
    const std::filesystem::path& baseDir,
    const std::string&           vsBaseName,
    grfx::PipelineInterface*     pPipelineInterface)
{
    grfx::ShaderModulePtr VS;
    PPX_CHECKED_CALL(CreateShader(baseDir, vsBaseName, &VS));

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
    gpCreateInfo.VS                                = {VS.Get(), "vsmain"};
    gpCreateInfo.vertexInputState.bindingCount     = 1;
    gpCreateInfo.vertexInputState.bindings[0]      = grfx::VertexBinding(grfx::VertexAttribute{PPX_SEMANTIC_NAME_POSITION, 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
    gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
    gpCreateInfo.cullMode                          = grfx::CULL_MODE_FRONT;
    gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                   = true;
    gpCreateInfo.depthWriteEnable                  = true;
    gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
    gpCreateInfo.outputState.renderTargetCount     = 0;
    gpCreateInfo.outputState.depthStencilFormat    = grfx::FORMAT_D32_FLOAT;
    gpCreateInfo.pPipelineInterface                = IsNull(pPipelineInterface) ? mForwardPipelineInterface.Get() : pPipelineInterface;

    grfx::GraphicsPipelinePtr pipeline;
    PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &pipeline));

    GetDevice()->DestroyShaderModule(VS);

    return pipeline;
}

void FishTornadoApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "Fish Tornado";
    settings.grfx.api                   = kApi;
    settings.enableImGui                = true;
    settings.grfx.numFramesInFlight     = 2;
    settings.grfx.enableDebug           = false;
    settings.grfx.pacedFrameRate        = 0;
    settings.xr.enable                  = true;
    settings.xr.enableDebugCapture      = false;
    settings.grfx.swapchain.imageCount  = 3;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;

    settings.grfx.device.computeQueueCount = 1;
}

void FishTornadoApp::SetupDescriptorPool()
{
    grfx::DescriptorPoolCreateInfo createInfo = {};
    createInfo.sampler                        = 1000;
    createInfo.sampledImage                   = 1000;
    createInfo.uniformBuffer                  = 1000;
    createInfo.structuredBuffer               = 1000;
    createInfo.storageTexelBuffer             = 1000;

    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
}

void FishTornadoApp::SetupSetLayouts()
{
    // Scene
    grfx::DescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_SCENE_DATA_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_SHADOW_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_SHADOW_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER});
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mSceneDataSetLayout));

    // Model
    createInfo = {};
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_MODEL_DATA_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER});
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mModelDataSetLayout));

    // Material

    createInfo = {};
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_MATERIAL_DATA_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_ALBEDO_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_ROUGHNESS_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_NORMAL_MAP_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_CAUSTICS_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_CLAMPED_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER});
    createInfo.bindings.push_back(grfx::DescriptorBinding{RENDER_REPEAT_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER});
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mMaterialSetLayout));
}

void FishTornadoApp::SetupPipelineInterfaces()
{
    // Forward render pipeline interface
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 3;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mSceneDataSetLayout;
        piCreateInfo.sets[1].set                       = 1;
        piCreateInfo.sets[1].pLayout                   = mModelDataSetLayout;
        piCreateInfo.sets[2].set                       = 2;
        piCreateInfo.sets[2].pLayout                   = mMaterialSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mForwardPipelineInterface));
    }
}

void FishTornadoApp::SetupTextures()
{
    PPX_CHECKED_CALL(grfx_util::CreateTexture1x1<uint8_t>(GetGraphicsQueue(), {0, 0, 0, 0}, &m1x1BlackTexture));
}

void FishTornadoApp::SetupSamplers()
{
    grfx::SamplerCreateInfo createInfo = {};
    createInfo.magFilter               = grfx::FILTER_LINEAR;
    createInfo.minFilter               = grfx::FILTER_LINEAR;
    createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod                  = 0.0f;
    createInfo.maxLod                  = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mClampedSampler));

    createInfo              = {};
    createInfo.magFilter    = grfx::FILTER_LINEAR;
    createInfo.minFilter    = grfx::FILTER_LINEAR;
    createInfo.mipmapMode   = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.minLod       = 0.0f;
    createInfo.maxLod       = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mRepeatSampler));

    createInfo               = {};
    createInfo.addressModeU  = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV  = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW  = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.compareEnable = true;
    createInfo.compareOp     = grfx::COMPARE_OP_LESS_OR_EQUAL;
    createInfo.borderColor   = grfx::BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mShadowSampler));
}

void FishTornadoApp::SetupPerFrame()
{
    const uint32_t numFramesInFlight = GetNumFramesInFlight();

    mPerFrame.resize(numFramesInFlight);

    for (uint32_t i = 0; i < numFramesInFlight; ++i) {
        PerFrame& frame = mPerFrame[i];

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.gpuStartTimestampCmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.gpuEndTimestampCmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.copyConstantsCmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.grfxFlockingCmd));
        PPX_CHECKED_CALL(GetComputeQueue()->CreateCommandBuffer(&frame.asyncFlockingCmd));
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.shadowCmd));

        grfx::SemaphoreCreateInfo semaCreateInfo  = {};
        grfx::FenceCreateInfo     fenceCreateInfo = {};

        // Work sync objects
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.gpuStartTimestampSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.copyConstantsSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.flockingCompleteSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.shadowCompleteSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

        // Image acquired sync objects
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        // Frame complete sync objects
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.frameCompleteSemaphore));
        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.frameCompleteFence));

        // Scene constants buffer
        PPX_CHECKED_CALL(frame.sceneConstants.Create(GetDevice(), 3 * PPX_MINIMUM_CONSTANT_BUFFER_SIZE));

        // Shadow draw pass
        {
            grfx::DrawPassCreateInfo drawPassCreateInfo = {};
            drawPassCreateInfo.width                    = kShadowRes;
            drawPassCreateInfo.height                   = kShadowRes;
            drawPassCreateInfo.depthStencilFormat       = grfx::FORMAT_D32_FLOAT;
            drawPassCreateInfo.depthStencilUsageFlags   = grfx::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT | grfx::IMAGE_USAGE_SAMPLED;
            drawPassCreateInfo.depthStencilInitialState = grfx::RESOURCE_STATE_SHADER_RESOURCE;
            drawPassCreateInfo.depthStencilClearValue   = {1.0f, 0xFF};

            PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&drawPassCreateInfo, &frame.shadowDrawPass));
        }

        // Allocate scene descriptor set
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSceneDataSetLayout, &frame.sceneSet));

        // Update scene descriptor
        PPX_CHECKED_CALL(frame.sceneSet->UpdateUniformBuffer(RENDER_SCENE_DATA_REGISTER, 0, frame.sceneConstants.GetGpuBuffer()));
        PPX_CHECKED_CALL(frame.sceneSet->UpdateSampledImage(RENDER_SHADOW_TEXTURE_REGISTER, 0, frame.shadowDrawPass->GetDepthStencilTexture()));
        PPX_CHECKED_CALL(frame.sceneSet->UpdateSampler(RENDER_SHADOW_SAMPLER_REGISTER, 0, mShadowSampler));

        // Scene shadow
        //
        // NOTE: We store a separate just for the scene constants when rendering shadows because
        //       DX12 will throw a validation error if we don't set the descriptor to
        //       D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE in the descriptor table range.
        //       The default value is D3D12_DESCRIPTOR_RANGE_FLAG_NONE which sets the descriptor and
        //       data to static.
        //
        // Allocate scene shadow descriptor set
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSceneDataSetLayout, &frame.sceneShadowSet));
        // Update scene shadow descriptor
        PPX_CHECKED_CALL(frame.sceneShadowSet->UpdateUniformBuffer(RENDER_SCENE_DATA_REGISTER, 0, frame.sceneConstants.GetGpuBuffer()));
        PPX_CHECKED_CALL(frame.sceneShadowSet->UpdateSampledImage(RENDER_SHADOW_TEXTURE_REGISTER, 0, m1x1BlackTexture));
        PPX_CHECKED_CALL(frame.sceneShadowSet->UpdateSampler(RENDER_SHADOW_SAMPLER_REGISTER, 0, mClampedSampler));

        if (IsXrEnabled()) {
            PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.uiCmd));
            PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.uiRenderCompleteFence));
        }

#if defined(ENABLE_GPU_QUERIES)
        // Timestamp query
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 1;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.startTimestampQuery));
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.endTimestampQuery));

        if (GetDevice()->PipelineStatsAvailable()) {
            // Pipeline statistics query pool
            queryCreateInfo       = {};
            queryCreateInfo.type  = grfx::QUERY_TYPE_PIPELINE_STATISTICS;
            queryCreateInfo.count = 1;
            PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.pipelineStatsQuery));
        }
#endif
    }
}

void FishTornadoApp::SetupCaustics()
{
    // Texture
    {
        // Load first file to get properties
        Bitmap bitmap;
        PPX_CHECKED_CALL(Bitmap::LoadFile(GetAssetPath("fishtornado/textures/ocean/caustics/save.00.png"), &bitmap));

        grfx::TextureCreateInfo createInfo = {};
        createInfo.imageType               = grfx::IMAGE_TYPE_2D;
        createInfo.width                   = bitmap.GetWidth();
        createInfo.height                  = bitmap.GetHeight();
        createInfo.depth                   = 1;
        createInfo.imageFormat             = grfx_util::ToGrfxFormat(bitmap.GetFormat());
        createInfo.sampleCount             = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount           = 1;
        createInfo.arrayLayerCount         = kCausticsImageCount;
        createInfo.usageFlags              = grfx::ImageUsageFlags::SampledImage() | grfx::IMAGE_USAGE_TRANSFER_DST;
        createInfo.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState            = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mCausticsTexture));
    }
}

void FishTornadoApp::UploadCaustics()
{
    for (uint32_t i = 0; i < kCausticsImageCount; ++i) {
        Timer timer;
        PPX_ASSERT_MSG(timer.Start() == ppx::TIMER_RESULT_SUCCESS, "timer start failed");
        double fnStartTime = timer.SecondsSinceStart();

        std::stringstream filename;
        filename << "fishtornado/textures/ocean/caustics/save." << std::setw(2) << std::setfill('0') << i << ".png";
        std::filesystem::path path = GetAssetPath(filename.str());

        Bitmap bitmap;
        PPX_CHECKED_CALL(Bitmap::LoadFile(path, &bitmap));

        PPX_CHECKED_CALL(grfx_util::CopyBitmapToTexture(GetGraphicsQueue(), &bitmap, mCausticsTexture, 0, i, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_SHADER_RESOURCE));

        double fnEndTime = timer.SecondsSinceStart();
        float  fnElapsed = static_cast<float>(fnEndTime - fnStartTime);
        PPX_LOG_INFO("Created image from image file: " << path << " (" << FloatString(fnElapsed) << " seconds)");
    }
}

void FishTornadoApp::SetupDebug()
{
#if !defined(PPX_D3D12)
    // Debug draw
    {
        mDebugDrawPipeline = CreateForwardPipeline(GetAssetPath("fishtornado/shaders"), "DebugDraw.vs", "DebugDraw.ps");
    }
#endif
    mViewCount = IsXrEnabled() ? GetXrComponent().GetViewCount() : 1;
    mViewGpuFrameTime.resize(mViewCount);
    mViewPipelineStatistics.resize(mViewCount);
}

void FishTornadoApp::SetupScene()
{
    mCamera.SetPerspective(45.0f, GetWindowAspect());
    mCamera.LookAt(float3(135.312f, 64.086f, -265.332f), float3(0.0f, 100.0f, 0.0f));
    mCamera.MoveAlongViewDirection(-300.0f);

    mShadowCamera.LookAt(float3(0.0f, 5000.0, 500.0f), float3(0.0f, 0.0f, 0.0f));
    mShadowCamera.SetPerspective(10.0f, 1.0f, 3500.0f, 5500.0f);
}

void FishTornadoApp::Setup()
{
    // For boolean options: if the option is present with no value, default to true. Otherwise obey the value.
    const auto& clOptions         = GetExtraOptions();
    mSettings.usePCF              = !(clOptions.HasExtraOption("ft-disable-pcf-shadows") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-disable-pcf-shadows", true));
    bool forceSingleCommandBuffer = (clOptions.HasExtraOption("ft-use-single-command-buffer") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-use-single-command-buffer", true));
    bool useAsyncCompute          = (clOptions.HasExtraOption("ft-use-async-compute") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-use-async-compute", true));
    if (forceSingleCommandBuffer && useAsyncCompute) {
        PPX_LOG_WARN("Single command buffer selected WITH async compute! Disabling async compute!");
        useAsyncCompute = false;
    }
    mSettings.forceSingleCommandBuffer = forceSingleCommandBuffer;
    mSettings.useAsyncCompute          = useAsyncCompute;
    mSettings.renderFish               = !(clOptions.HasExtraOption("ft-disable-fish") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-disable-fish", true));
    mSettings.renderOcean              = !(clOptions.HasExtraOption("ft-disable-ocean") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-disable-ocean", true));
    mSettings.renderShark              = !(clOptions.HasExtraOption("ft-disable-shark") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-disable-shark", true));
    mSettings.useTracking              = !(clOptions.HasExtraOption("ft-disable-tracking") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-disable-tracking", true));
    mSettings.outputMetrics            = (clOptions.HasExtraOption("ft-enable-metrics") && clOptions.GetExtraOptionValueOrDefault<bool>("ft-enable-metrics", true));

    mSettings.fishResX = clOptions.GetExtraOptionValueOrDefault<uint32_t>("ft-fish-res-x", kDefaultFishResX);
    PPX_ASSERT_MSG(mSettings.fishResX < 65536, "Fish X resolution out-of-range.");
    mSettings.fishResY = clOptions.GetExtraOptionValueOrDefault<uint32_t>("ft-fish-res-y", kDefaultFishResY);
    PPX_ASSERT_MSG(mSettings.fishResY < 65536, "Fish Y resolution out-of-range.");
    mSettings.fishThreadsX = clOptions.GetExtraOptionValueOrDefault<uint32_t>("ft-fish-threads-x", kDefaultFishThreadsX);
    PPX_ASSERT_MSG(mSettings.fishThreadsX < 65536, "Fish X threads out-of-range.");
    mSettings.fishThreadsY = clOptions.GetExtraOptionValueOrDefault<uint32_t>("ft-fish-threads-y", kDefaultFishThreadsY);
    PPX_ASSERT_MSG(mSettings.fishThreadsY < 65536, "Fish Y threads out of range.");

    SetupDescriptorPool();
    SetupSetLayouts();
    SetupPipelineInterfaces();
    SetupTextures();
    SetupSamplers();
    SetupPerFrame();
    SetupCaustics();
    SetupDebug();
    SetupMetrics();

    const uint32_t numFramesInFlight = GetNumFramesInFlight();
    // Always setup all elements of the scene, even if they're not in use.
    mFlocking.Setup(numFramesInFlight, mSettings);
    mOcean.Setup(numFramesInFlight);
    mShark.Setup(numFramesInFlight);

    // Caustic iamge copy to GPU texture is giving Vulkan some grief
    // so we split up for now.
    //
    UploadCaustics();

    SetupScene();
}

void FishTornadoApp::Shutdown()
{
    // Always shutdown all elements of the scene, even if they're not in use.
    mFlocking.Shutdown();
    mOcean.Shutdown();
    mShark.Shutdown();

    for (size_t i = 0; i < mPerFrame.size(); ++i) {
        PerFrame& frame = mPerFrame[i];
        frame.sceneConstants.Destroy();
    }

    // TODO(slumpwuffle): Replace these one-off metrics with the new metrics system when it arrives.
    if (mSettings.outputMetrics) {
        WriteMetrics();
    }
}

void FishTornadoApp::Scroll(float dx, float dy)
{
    mCamera.MoveAlongViewDirection(dy * -5.0f);
}

void FishTornadoApp::UpdateTime()
{
    static float sPrevTime = GetElapsedSeconds();
    float        curTime   = GetElapsedSeconds();
    float        dt        = curTime - sPrevTime;

    mDt       = std::min<float>(dt, 1.0f / 60.0f) * 6.0f;
    mTime     = mTime + mDt;
    sPrevTime = curTime;
}

void FishTornadoApp::UpdateScene(uint32_t frameIndex)
{
    PerFrame& frame = mPerFrame[frameIndex];

    hlsl::SceneData* pSceneData            = static_cast<hlsl::SceneData*>(frame.sceneConstants.GetMappedAddress());
    pSceneData->time                       = GetTime();
    pSceneData->eyePosition                = mCamera.GetEyePosition();
    pSceneData->viewMatrix                 = mCamera.GetViewMatrix();
    pSceneData->projectionMatrix           = mCamera.GetProjectionMatrix();
    pSceneData->viewProjectionMatrix       = mCamera.GetViewProjectionMatrix();
    pSceneData->fogNearDistance            = 20.0f;
    pSceneData->fogFarDistance             = 900.0f;
    pSceneData->fogPower                   = 1.0f;
    pSceneData->fogColor                   = kFogColor;
    pSceneData->lightPosition              = float3(0.0f, 5000.0, 500.0f);
    pSceneData->ambient                    = float3(0.45f, 0.45f, 0.5f) * 0.25f;
    pSceneData->shadowViewProjectionMatrix = mShadowCamera.GetViewProjectionMatrix();
    pSceneData->shadowTextureDim           = float2(kShadowRes);
    pSceneData->usePCF                     = static_cast<uint32_t>(mSettings.usePCF);

    if (IsXrEnabled() && mSettings.useTracking) {
        const XrVector3f& pos            = GetXrComponent().GetPoseForCurrentView().position;
        pSceneData->eyePosition          = {pos.x, pos.y, pos.z};
        const glm::mat4 v                = GetXrComponent().GetViewMatrixForCurrentView();
        const glm::mat4 p                = GetXrComponent().GetProjectionMatrixForCurrentViewAndSetFrustumPlanes(PPX_CAMERA_DEFAULT_NEAR_CLIP, PPX_CAMERA_DEFAULT_FAR_CLIP);
        pSceneData->viewMatrix           = v;
        pSceneData->projectionMatrix     = p;
        pSceneData->viewProjectionMatrix = p * v;
    }
}

void FishTornadoApp::RenderSceneUsingSingleCommandBuffer(
    uint32_t            frameIndex,
    PerFrame&           frame,
    uint32_t            prevFrameIndex,
    PerFrame&           prevFrame,
    grfx::SwapchainPtr& swapchain,
    uint32_t            imageIndex)
{
    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
#if defined(ENABLE_GPU_QUERIES)
        frame.startTimestampQuery->Reset(0, 1);
        frame.endTimestampQuery->Reset(0, 1);
        if (GetDevice()->PipelineStatsAvailable()) {
            frame.pipelineStatsQuery->Reset(0, 1);
        }

        // Write start timestamp
        frame.cmd->WriteTimestamp(frame.startTimestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif

        if (mSettings.renderShark) {
            mShark.CopyConstantsToGpu(frameIndex, frame.cmd);
        }
        if (mSettings.renderFish) {
            mFlocking.CopyConstantsToGpu(frameIndex, frame.cmd);
        }
        if (mSettings.renderOcean) {
            mOcean.CopyConstantsToGpu(frameIndex, frame.cmd);
        }

        // Scene constants
        {
            frame.cmd->BufferResourceBarrier(frame.sceneConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_COPY_DST);

            grfx::BufferToBufferCopyInfo copyInfo = {};
            copyInfo.size                         = frame.sceneConstants.GetSize();

            frame.cmd->CopyBufferToBuffer(
                &copyInfo,
                frame.sceneConstants.GetCpuBuffer(),
                frame.sceneConstants.GetGpuBuffer());

            frame.cmd->BufferResourceBarrier(frame.sceneConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
        }

        // -----------------------------------------------------------------------------------------
        bool updateFlocking = true;
        // only need to update flocking once per frame
        if (IsXrEnabled()) {
            if (GetXrComponent().GetCurrentViewIndex() != 0) {
                updateFlocking = false;
            }
        }
        if (mSettings.renderFish && updateFlocking) {
            // Compute flocking
            mFlocking.BeginCompute(frameIndex, frame.cmd, false);
            mFlocking.Compute(frameIndex, frame.cmd);
            mFlocking.EndCompute(frameIndex, frame.cmd, false);
        }

        // -----------------------------------------------------------------------------------------
        if (mSettings.renderFish) {
            mFlocking.BeginGraphics(frameIndex, frame.cmd, false);
        }

        // Shadow mapping
        frame.cmd->TransitionImageLayout(frame.shadowDrawPass, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        frame.cmd->BeginRenderPass(frame.shadowDrawPass);
        {
            frame.cmd->SetScissors(frame.shadowDrawPass->GetScissor());
            frame.cmd->SetViewports(frame.shadowDrawPass->GetViewport());

            if (mSettings.renderShark) {
                mShark.DrawShadow(frameIndex, frame.cmd);
            }
            if (mSettings.renderFish) {
                mFlocking.DrawShadow(frameIndex, frame.cmd);
            }
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(frame.shadowDrawPass, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE, grfx::RESOURCE_STATE_SHADER_RESOURCE);

        // -----------------------------------------------------------------------------------------

        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{kFogColor.r, kFogColor.g, kFogColor.b, 1.0f}};

        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        }
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(renderPass->GetScissor());
            frame.cmd->SetViewports(renderPass->GetViewport());

            if (mSettings.renderShark) {
                mShark.DrawForward(frameIndex, frame.cmd);
            }
#if defined(ENABLE_GPU_QUERIES)
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
            }
#endif
            if (mSettings.renderFish) {
                mFlocking.DrawForward(frameIndex, frame.cmd);
            }
#if defined(ENABLE_GPU_QUERIES)
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
            }
#endif

            if (mSettings.renderOcean) {
                mOcean.DrawForward(frameIndex, frame.cmd);
            }

            if (!IsXrEnabled()) {
                // Draw ImGui
                DrawDebugInfo();
#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
                DrawProfilerGrfxApiFunctions();
#endif // defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
                DrawImGui(frame.cmd);
            }
        }
        frame.cmd->EndRenderPass();
        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
        }

#if defined(ENABLE_GPU_QUERIES)
        // Write end timestamp
        frame.cmd->WriteTimestamp(frame.endTimestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif
    }

#if defined(ENABLE_GPU_QUERIES)
    // Resolve queries
    frame.cmd->ResolveQueryData(frame.startTimestampQuery, 0, 1);
    frame.cmd->ResolveQueryData(frame.endTimestampQuery, 0, 1);
    if (GetDevice()->PipelineStatsAvailable()) {
        frame.cmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
    }
#endif

    if (mSettings.renderFish) {
        mFlocking.EndGraphics(frameIndex, frame.cmd, false);
    }

    PPX_CHECKED_CALL(frame.cmd->End());

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &frame.cmd;
    // no need to use semaphore when XR is enabled
    if (IsXrEnabled()) {
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.ppWaitSemaphores     = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.ppSignalSemaphores   = nullptr;
    }
    else {
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.frameCompleteSemaphore;
    }
    submitInfo.pFence = frame.frameCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
}

void FishTornadoApp::RenderSceneUsingMultipleCommandBuffers(
    uint32_t            frameIndex,
    PerFrame&           frame,
    uint32_t            prevFrameIndex,
    PerFrame&           prevFrame,
    grfx::SwapchainPtr& swapchain,
    uint32_t            imageIndex)
{
#if defined(ENABLE_GPU_QUERIES)
    frame.startTimestampQuery->Reset(0, 1);
    frame.endTimestampQuery->Reset(0, 1);
    if (GetDevice()->PipelineStatsAvailable()) {
        frame.pipelineStatsQuery->Reset(0, 1);
    }

    PPX_CHECKED_CALL(frame.gpuStartTimestampCmd->Begin());
    // Write start timestamp
    frame.gpuStartTimestampCmd->WriteTimestamp(frame.startTimestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
    PPX_CHECKED_CALL(frame.gpuStartTimestampCmd->End());

    // Submit GPU write start timestamp
    {
        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.gpuStartTimestampCmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.gpuStartTimestampSemaphore;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }
#endif

    // ---------------------------------------------------------------------------------------------

    // Copy constants
    PPX_CHECKED_CALL(frame.copyConstantsCmd->Begin());
    {
        if (mSettings.renderShark) {
            mShark.CopyConstantsToGpu(frameIndex, frame.copyConstantsCmd);
        }
        if (mSettings.renderFish) {
            mFlocking.CopyConstantsToGpu(frameIndex, frame.copyConstantsCmd);
        }
        if (mSettings.renderOcean) {
            mOcean.CopyConstantsToGpu(frameIndex, frame.copyConstantsCmd);
        }

        // Scene constants
        {
            frame.copyConstantsCmd->BufferResourceBarrier(frame.sceneConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_COPY_DST);

            grfx::BufferToBufferCopyInfo copyInfo = {};
            copyInfo.size                         = frame.sceneConstants.GetSize();

            frame.copyConstantsCmd->CopyBufferToBuffer(
                &copyInfo,
                frame.sceneConstants.GetCpuBuffer(),
                frame.sceneConstants.GetGpuBuffer());

            frame.copyConstantsCmd->BufferResourceBarrier(frame.sceneConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
        }
    }
    PPX_CHECKED_CALL(frame.copyConstantsCmd->End());

    // Submit GPU write start timestamp
    {
        grfx::SubmitInfo submitInfo   = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.ppCommandBuffers   = &frame.copyConstantsCmd;
#if defined(ENABLE_GPU_QUERIES)
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.ppWaitSemaphores   = &frame.gpuStartTimestampSemaphore;
#endif
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.copyConstantsSemaphore;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }

    // ---------------------------------------------------------------------------------------------
    bool updateFlocking = true;
    // only need to update flocking once per frame
    if (IsXrEnabled()) {
        if (GetXrComponent().GetCurrentViewIndex() != 0) {
            updateFlocking = false;
        }
    }
    if (mSettings.renderFish && updateFlocking) {
        grfx::CommandBuffer* pFlockingCmd = frame.grfxFlockingCmd;
        if (mSettings.useAsyncCompute) {
            pFlockingCmd = frame.asyncFlockingCmd;
        }

        // Compute flocking
        PPX_CHECKED_CALL(pFlockingCmd->Begin());
        {
            mFlocking.BeginCompute(frameIndex, pFlockingCmd, mSettings.useAsyncCompute);
            mFlocking.Compute(frameIndex, pFlockingCmd);
            mFlocking.EndCompute(frameIndex, pFlockingCmd, mSettings.useAsyncCompute);
        }
        PPX_CHECKED_CALL(pFlockingCmd->End());

        // Submit flocking
        {
            grfx::SubmitInfo submitInfo     = {};
            submitInfo.commandBufferCount   = 1;
            submitInfo.ppCommandBuffers     = &pFlockingCmd;
            submitInfo.waitSemaphoreCount   = 1;
            submitInfo.ppWaitSemaphores     = &frame.copyConstantsSemaphore;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.ppSignalSemaphores   = &frame.flockingCompleteSemaphore;

            if (mSettings.useAsyncCompute) {
                PPX_CHECKED_CALL(GetComputeQueue()->Submit(&submitInfo));
            }
            else {
                PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
            }
        }
    }

    // ---------------------------------------------------------------------------------------------

    // Shadow mapping
    PPX_CHECKED_CALL(frame.shadowCmd->Begin());
    {
        if (mSettings.renderFish) {
            mFlocking.BeginGraphics(frameIndex, frame.shadowCmd, mSettings.useAsyncCompute);
        }
        frame.shadowCmd->TransitionImageLayout(frame.shadowDrawPass, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        frame.shadowCmd->BeginRenderPass(frame.shadowDrawPass);
        {
            frame.shadowCmd->SetScissors(frame.shadowDrawPass->GetScissor());
            frame.shadowCmd->SetViewports(frame.shadowDrawPass->GetViewport());

            if (mSettings.renderShark) {
                mShark.DrawShadow(frameIndex, frame.shadowCmd);
            }
            if (mSettings.renderFish) {
                mFlocking.DrawShadow(frameIndex, frame.shadowCmd);
            }
        }
        frame.shadowCmd->EndRenderPass();
        frame.shadowCmd->TransitionImageLayout(frame.shadowDrawPass, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_UNDEFINED, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }
    PPX_CHECKED_CALL(frame.shadowCmd->End());

    // Submit shadow
    {
        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.shadowCmd;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.flockingCompleteSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.shadowCompleteSemaphore;

        if (!mSettings.renderFish || !updateFlocking) {
            submitInfo.ppWaitSemaphores = &frame.copyConstantsSemaphore;
        }
        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }

    // ---------------------------------------------------------------------------------------------

    // Render
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{kFogColor.r, kFogColor.g, kFogColor.b, 1.0f}};

        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        }
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(renderPass->GetScissor());
            frame.cmd->SetViewports(renderPass->GetViewport());

            if (mSettings.renderShark) {
                mShark.DrawForward(frameIndex, frame.cmd);
            }
#if defined(ENABLE_GPU_QUERIES)
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
            }
#endif
            if (mSettings.renderFish) {
                mFlocking.DrawForward(frameIndex, frame.cmd);
            }
#if defined(ENABLE_GPU_QUERIES)
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
            }
#endif

            if (mSettings.renderOcean) {
                mOcean.DrawForward(frameIndex, frame.cmd);
            }

            if (!IsXrEnabled()) {
                // Draw ImGui
                DrawDebugInfo();
#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
                DrawProfilerGrfxApiFunctions();
#endif // defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
                DrawImGui(frame.cmd);
            }
        }
        frame.cmd->EndRenderPass();
        if (!IsXrEnabled()) {
            frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
        }

        if (mSettings.renderFish) {
            mFlocking.EndGraphics(frameIndex, frame.cmd, mSettings.useAsyncCompute);
        }
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    // Submit render work
    // no need to use semaphore when XR is enabled
    if (IsXrEnabled()) {
        const grfx::Semaphore* pWaitSemaphores    = frame.shadowCompleteSemaphore;
        uint32_t               waitSemaphoreCount = 1;

        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.cmd;
        submitInfo.waitSemaphoreCount   = waitSemaphoreCount;
        submitInfo.ppWaitSemaphores     = &pWaitSemaphores;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }
    else {
        const grfx::Semaphore* ppWaitSemaphores[2] = {frame.imageAcquiredSemaphore, frame.shadowCompleteSemaphore};
        uint32_t               waitSemaphoreCount  = 2;

        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.cmd;
        submitInfo.waitSemaphoreCount   = waitSemaphoreCount;
        submitInfo.ppWaitSemaphores     = ppWaitSemaphores;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }

    // ---------------------------------------------------------------------------------------------

#if defined(ENABLE_GPU_QUERIES)
    PPX_CHECKED_CALL(frame.gpuEndTimestampCmd->Begin());
    {
        // Write end timestamp
        frame.gpuEndTimestampCmd->WriteTimestamp(frame.endTimestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
        // Resolve queries
        frame.gpuEndTimestampCmd->ResolveQueryData(frame.startTimestampQuery, 0, 1);
        frame.gpuEndTimestampCmd->ResolveQueryData(frame.endTimestampQuery, 0, 1);
        if (GetDevice()->PipelineStatsAvailable()) {
            frame.gpuEndTimestampCmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
        }
    }
    PPX_CHECKED_CALL(frame.gpuEndTimestampCmd->End());

    // Submit GPU write end timestamp
    {
        grfx::SubmitInfo submitInfo     = {};
        submitInfo.commandBufferCount   = 1;
        submitInfo.ppCommandBuffers     = &frame.gpuEndTimestampCmd;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.renderCompleteSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.frameCompleteSemaphore;
        submitInfo.pFence               = frame.frameCompleteFence;

        // no need to use semaphore when XR is enabled
        if (IsXrEnabled()) {
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.ppSignalSemaphores   = nullptr;
        }

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }
#else
    // Submit a wait for render complete and a signal for frame complete
    {
        grfx::SubmitInfo submitInfo     = {};
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.ppWaitSemaphores     = &frame.renderCompleteSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.ppSignalSemaphores   = &frame.frameCompleteSemaphore;
        submitInfo.pFence               = frame.frameCompleteFence;
        // no need to use semaphore when XR is enabled
        if (IsXrEnabled()) {
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.ppSignalSemaphores   = nullptr;
        }

        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }
#endif
}

void FishTornadoApp::Render()
{
    uint32_t  frameIndex     = GetInFlightFrameIndex();
    PerFrame& frame          = mPerFrame[frameIndex];
    uint32_t  prevFrameIndex = GetPreviousInFlightFrameIndex();
    PerFrame& prevFrame      = mPerFrame[prevFrameIndex];

    uint32_t imageIndex       = UINT32_MAX;
    uint32_t currentViewIndex = 0;
    if (IsXrEnabled()) {
        currentViewIndex = GetXrComponent().GetCurrentViewIndex();
    }

    // Render UI into a different composition layer.
    if (IsXrEnabled() && (currentViewIndex == 0) && GetSettings()->enableImGui) {
        grfx::SwapchainPtr uiSwapchain = GetUISwapchain();
        PPX_CHECKED_CALL(uiSwapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
        PPX_CHECKED_CALL(frame.uiRenderCompleteFence->WaitAndReset());

        PPX_CHECKED_CALL(frame.uiCmd->Begin());
        {
            grfx::RenderPassPtr renderPass = uiSwapchain->GetRenderPass(imageIndex);
            PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

            grfx::RenderPassBeginInfo beginInfo = {};
            beginInfo.pRenderPass               = renderPass;
            beginInfo.renderArea                = renderPass->GetRenderArea();
            beginInfo.RTVClearCount             = 1;
            beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
            beginInfo.DSVClearValue             = {1.0f, 0xFF};

            frame.uiCmd->BeginRenderPass(&beginInfo);
            // Draw ImGui
            DrawDebugInfo();
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

    grfx::SwapchainPtr swapchain = GetSwapchain(currentViewIndex);

    UpdateTime();

    if (swapchain->ShouldSkipExternalSynchronization()) {
        // No need to
        // - Signal imageAcquiredSemaphore & imageAcquiredFence.
        // - Wait for imageAcquiredFence since xrWaitSwapchainImage is called in AcquireNextImage.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, nullptr, nullptr, &imageIndex));
    }
    else {
        // Wait semaphore is ignored for XR.
        PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

        // Wait for and reset image acquired fence.
        PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    }

    // Wait for and reset render complete fence.
    PPX_CHECKED_CALL(frame.frameCompleteFence->WaitAndReset());

    // Move this after waiting for frameCompleteFence to make sure the previous view is done.
    UpdateScene(frameIndex);
    if (mSettings.renderShark) {
        mShark.Update(frameIndex);
    }
    if (mSettings.renderFish) {
        mFlocking.Update(frameIndex);
    }
    if (mSettings.renderOcean) {
        mOcean.Update(frameIndex);
    }

    // Read query results.
    if (GetFrameCount() > 0) {
#if defined(ENABLE_GPU_QUERIES)
        uint64_t data[2] = {0, 0};
        PPX_CHECKED_CALL(prevFrame.startTimestampQuery->GetData(&data[0], 1 * sizeof(uint64_t)));
        PPX_CHECKED_CALL(prevFrame.endTimestampQuery->GetData(&data[1], 1 * sizeof(uint64_t)));
        mViewGpuFrameTime[currentViewIndex] = (data[1] - data[0]);
        if (GetDevice()->PipelineStatsAvailable()) {
            PPX_CHECKED_CALL(prevFrame.pipelineStatsQuery->GetData(&(mViewPipelineStatistics[currentViewIndex]), sizeof(grfx::PipelineStatistics)));
        }
#endif
    }

    if (mSettings.forceSingleCommandBuffer) {
        RenderSceneUsingSingleCommandBuffer(frameIndex, frame, prevFrameIndex, prevFrame, swapchain, imageIndex);
    }
    else {
        RenderSceneUsingMultipleCommandBuffers(frameIndex, frame, prevFrameIndex, prevFrame, swapchain, imageIndex);
    }

    mLastFrameWasAsyncCompute = mSettings.useAsyncCompute;

    // No need to present when XR is enabled.
    if (!IsXrEnabled()) {
        PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.frameCompleteSemaphore));
    }
    else {
        if (GetSettings()->xr.enableDebugCapture && (currentViewIndex == 1)) {
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
}

void FishTornadoApp::DrawGui()
{
    ImGui::Separator();

    {
        uint64_t totalGpuFrameTime  = 0;
        uint64_t totalIAVertices    = 0;
        uint64_t totalIAPrimitives  = 0;
        uint64_t totalVSInvocations = 0;
        uint64_t totalCInvocations  = 0;
        uint64_t totalCPrimitives   = 0;
        uint64_t totalPSInvocations = 0;

        for (int i = 0; i < mViewCount; i++) {
            totalGpuFrameTime += mViewGpuFrameTime[i];
            totalIAVertices += mViewPipelineStatistics[i].IAVertices;
            totalIAPrimitives += mViewPipelineStatistics[i].IAPrimitives;
            totalVSInvocations += mViewPipelineStatistics[i].VSInvocations;
            totalCInvocations += mViewPipelineStatistics[i].CInvocations;
            totalCPrimitives += mViewPipelineStatistics[i].CPrimitives;
            totalPSInvocations += mViewPipelineStatistics[i].PSInvocations;
        }

        uint64_t frequency = 0;
        GetGraphicsQueue()->GetTimestampFrequency(&frequency);

        float frameCount = static_cast<float>(GetFrameCount());

        ImGui::Columns(2);

        float prevGpuFrameTime = static_cast<float>(totalGpuFrameTime / static_cast<double>(frequency)) * 1000.0f;
        if (mSettings.outputMetrics) {
            auto now              = GetElapsedSeconds();
            auto prevCpuFrameTime = GetPrevFrameTime();
            mMetricsData.metrics[MetricsData::kTypeGpuFrameTime]->RecordEntry(now, prevGpuFrameTime);
            mMetricsData.metrics[MetricsData::kTypeCpuFrameTime]->RecordEntry(now, prevCpuFrameTime);
            mMetricsData.metrics[MetricsData::kTypeIAVertices]->RecordEntry(now, totalIAVertices);
            mMetricsData.metrics[MetricsData::kTypeIAPrimitives]->RecordEntry(now, totalIAPrimitives);
            mMetricsData.metrics[MetricsData::kTypeVSInvocations]->RecordEntry(now, totalVSInvocations);
            mMetricsData.metrics[MetricsData::kTypeCInvocations]->RecordEntry(now, totalCInvocations);
            mMetricsData.metrics[MetricsData::kTypeCPrimitives]->RecordEntry(now, totalCPrimitives);
            mMetricsData.metrics[MetricsData::kTypePSInvocations]->RecordEntry(now, totalPSInvocations);
        }

        ImGui::Text("Previous GPU Frame Time");
        ImGui::NextColumn();
        ImGui::Text("%f ms ", prevGpuFrameTime);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Fish IAVertices");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalIAVertices);
        ImGui::NextColumn();

        ImGui::Text("Fish IAPrimitives");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalIAPrimitives);
        ImGui::NextColumn();

        ImGui::Text("Fish VSInvocations");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalVSInvocations);
        ImGui::NextColumn();

        ImGui::Text("Fish CInvocations");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalCInvocations);
        ImGui::NextColumn();

        ImGui::Text("Fish CPrimitives");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalCPrimitives);
        ImGui::NextColumn();

        ImGui::Text("Fish PSInvocations");
        ImGui::NextColumn();
        ImGui::Text("%" PRIu64, totalPSInvocations);
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Separator();

    ImGui::Checkbox("Render Shark", &mSettings.renderShark);
    ImGui::Checkbox("Render Fish", &mSettings.renderFish);
    ImGui::Checkbox("Render Ocean", &mSettings.renderOcean);

    ImGui::Checkbox("Use Head Tracking", &mSettings.useTracking);

    ImGui::Checkbox("Use PCF Shadows", &mSettings.usePCF);

    if (mSettings.useAsyncCompute) {
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Use Single CommandBuffer", &mSettings.forceSingleCommandBuffer);
    if (mSettings.useAsyncCompute) {
        ImGui::EndDisabled();
    }

    if (mSettings.forceSingleCommandBuffer) {
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Use Async Compute", &mSettings.useAsyncCompute);
    if (mSettings.forceSingleCommandBuffer) {
        ImGui::EndDisabled();
    }
}

// TODO(slumpwuffle): Replace these one-off metrics with the new metrics system when it arrives.
void FishTornadoApp::SetupMetrics()
{
    if (!mSettings.outputMetrics) {
        return;
    }

    auto* run = mMetricsData.manager.AddRun("FishTornado Metrics");

    mMetricsData.metrics[MetricsData::kTypeGpuFrameTime]  = run->AddMetric<ppx::metrics::MetricGauge>({"GPU Frame Time", "ms", ppx::metrics::MetricInterpretation::LOWER_IS_BETTER, {0.f, 60000.f}});
    mMetricsData.metrics[MetricsData::kTypeCpuFrameTime]  = run->AddMetric<ppx::metrics::MetricGauge>({"Total (CPU) Frame Time", "ms", ppx::metrics::MetricInterpretation::LOWER_IS_BETTER, {0.f, 60000.f}});
    mMetricsData.metrics[MetricsData::kTypeIAVertices]    = run->AddMetric<ppx::metrics::MetricGauge>({"IA Vertices", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
    mMetricsData.metrics[MetricsData::kTypeIAPrimitives]  = run->AddMetric<ppx::metrics::MetricGauge>({"IA Primitives", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
    mMetricsData.metrics[MetricsData::kTypeVSInvocations] = run->AddMetric<ppx::metrics::MetricGauge>({"VS Invocations", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
    mMetricsData.metrics[MetricsData::kTypeCInvocations]  = run->AddMetric<ppx::metrics::MetricGauge>({"C Invocations", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
    mMetricsData.metrics[MetricsData::kTypeCPrimitives]   = run->AddMetric<ppx::metrics::MetricGauge>({"C Primitives", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
    mMetricsData.metrics[MetricsData::kTypePSInvocations] = run->AddMetric<ppx::metrics::MetricGauge>({"PS Invocations", "", ppx::metrics::MetricInterpretation::NONE, {0.f, 1000000000.f}});
}

// TODO(slumpwuffle): Replace these one-off metrics with the new metrics system when it arrives.
void FishTornadoApp::WriteMetrics()
{
    if (!mSettings.outputMetrics) {
        return;
    }

#if defined(PPX_ANDROID)
    auto filePath = ppx::fs::GetInternalDataPath() / std::filesystem::path(kMetricsFilename);
#else
    auto filePath = std::filesystem::path(kMetricsFilename);
#endif
    std::filesystem::create_directories(filePath.parent_path());
    ppx::CSVFileLog metricsFileLog(filePath);

    metricsFileLog.LogField("Frame Count");
    metricsFileLog.LastField(GetFrameCount());
    metricsFileLog.LogField("Average FPS");
    metricsFileLog.LastField(GetAverageFPS());

    for (ppx::metrics::MetricGauge* pMetric : mMetricsData.metrics) {
        auto basic   = pMetric->GetBasicStatistics();
        auto complex = pMetric->ComputeComplexStatistics();
        auto name    = pMetric->GetName();
        metricsFileLog.LogField(name + " Min");
        metricsFileLog.LastField(basic.min);
        metricsFileLog.LogField(name + " Max");
        metricsFileLog.LastField(basic.max);
        metricsFileLog.LogField(name + " Mean");
        metricsFileLog.LastField(basic.average);
        metricsFileLog.LogField(name + " Median");
        metricsFileLog.LastField(complex.median);
        metricsFileLog.LogField(name + " P90");
        metricsFileLog.LastField(complex.percentile90);
        metricsFileLog.LogField(name + " P95");
        metricsFileLog.LastField(complex.percentile95);
        metricsFileLog.LogField(name + " P99");
        metricsFileLog.LastField(complex.percentile99);
        metricsFileLog.LogField(name + " StdDev");
        metricsFileLog.LastField(complex.standardDeviation);
    }
}
