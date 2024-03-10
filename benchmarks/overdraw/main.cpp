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

#include <filesystem>

#include "ppx/config.h"
#include "ppx/math_config.h"
#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_constants.h"
#include "ppx/grfx/grfx_enums.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/grfx/grfx_image.h"
#include "ppx/log.h"
#include "ppx/ppx.h"
#include "ppx/csv_file_log.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

const std::unordered_map<std::string, grfx::BlendMode> blendModeStringToBlendMode = {
    {"none", grfx::BLEND_MODE_NONE},
    {"additive", grfx::BLEND_MODE_ADDITIVE},
    {"alpha", grfx::BLEND_MODE_ALPHA},
    {"over", grfx::BLEND_MODE_OVER},
    {"under", grfx::BLEND_MODE_UNDER},
    {"premult_alpha", grfx::BLEND_MODE_PREMULT_ALPHA},
};

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

    void SaveResultsToFile();

private:
    std::vector<float> GetRectSizeScaleFactors();

    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
        ppx::grfx::FencePtr         renderCompleteFence;
        ppx::grfx::QueryPtr         timestampQuery;
    };

    std::vector<PerFrame>           mPerFrame;
    grfx::DescriptorPoolPtr         mDescriptorPool;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;
    ppx::grfx::BufferPtr            mVertexBuffer;
    grfx::DrawPassPtr               mDrawPass;
    grfx::Viewport                  mViewport;
    grfx::Rect                      mScissorRect;
    grfx::VertexBinding             mVertexBinding;
    uint2                           mRenderTargetSize;
    uint32_t                        mNumTriangles;
    grfx::DescriptorSetLayoutPtr    mDescriptorSetLayout;
    grfx::DescriptorSetPtr          mDescriptorSet;

    // Texture and sampler.
    ppx::grfx::ImagePtr            mImage;
    ppx::grfx::SampledImageViewPtr mSampledImageView;
    ppx::grfx::SamplerPtr          mSampler;
    std::string                    mSamplerFilterType;

    // Overdraw parameters.
    uint32_t    mNumLayers               = 4;
    bool        mEnableDepth             = true;
    bool        mDrawFrontToBack         = true;
    bool        mUseExplicitEarlyZShader = false;
    std::string mBlendMode               = "none";

    // Stats
    uint64_t    mGpuWorkDuration = 0;
    std::string mCSVFileName;
    struct PerFrameRegister
    {
        uint64_t frameNumber;
        float    gpuWorkDurationMs;
        float    cpuFrameTimeMs;
    };
    std::deque<PerFrameRegister> mFrameRegisters;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "overdraw";
    settings.enableImGui                = false;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger{std::filesystem::path(mCSVFileName)};
    for (const auto& row : mFrameRegisters) {
        fileLogger.LogField(row.frameNumber);
        fileLogger.LogField(row.gpuWorkDurationMs);
        fileLogger.LastField(row.cpuFrameTimeMs);
    }
}

void ProjApp::Setup()
{
    auto cl_options = GetExtraOptions();

    // Number of layers to draw. The layers cover the entire screen but have different depth.
    mNumLayers = cl_options.GetExtraOptionValueOrDefault<uint32_t>("num-layers", 4);
    if (mNumLayers == 0) {
        mNumLayers = 4;
        PPX_LOG_WARN("Number of layers must be greater or equal to 1, defaulting to: " + std::to_string(mNumLayers));
    }

    // Name of the CSV output file.
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
    }

    // Sampler filter operation.
    mSamplerFilterType = cl_options.GetExtraOptionValueOrDefault<std::string>("filter-type", "linear");
    if (mSamplerFilterType != "linear" && mSamplerFilterType != "nearest") {
        mSamplerFilterType = "linear";
        PPX_LOG_WARN("Invalid sampler filter type (must be `linear` or `nearest`), defaulting to: " + mSamplerFilterType);
    }

    // Whether to draw layers in front-to-back order or back-to-front.
    mDrawFrontToBack = cl_options.GetExtraOptionValueOrDefault<bool>("draw-front-to-back", true);

    // Whether to use the shader that enables explicit early-z in the pixel shader.
    mUseExplicitEarlyZShader = cl_options.HasExtraOption("use-explicit-early-z");

    // Whether to use depth read-write in the pipeline.
    mEnableDepth = cl_options.GetExtraOptionValueOrDefault<bool>("enable-depth", true);

    // Which blending mode to use when drawing layers.
    mBlendMode = cl_options.GetExtraOptionValueOrDefault<std::string>("blend-mode", "none");
    if (blendModeStringToBlendMode.find(mBlendMode) == blendModeStringToBlendMode.end()) {
        mBlendMode = "none";
        PPX_LOG_WARN("Invalid blend mode (must be `none`, `additive`, `alpha`, `over`, `under` or `premult_alpha`), defaulting to: " + mBlendMode);
    }

    // Per frame data
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

        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        mPerFrame.push_back(frame);
    }

    mRenderTargetSize = ppx::uint2(GetWindowWidth(), GetWindowHeight());

    mViewport    = {0, 0, float(mRenderTargetSize.x), float(mRenderTargetSize.y), 0, 1};
    mScissorRect = {0, 0, mRenderTargetSize.x, mRenderTargetSize.y};

    // Vertex buffer for layers rectangles (one full-screen quad for each layer).
    // Each layer's depth is uniformly distributed across [0.0f, 1.0f].
    {
        std::vector<float> vertexData;

        // Map rectangle sides length to [-1,+1] space.
        float rectX = (-1.0f + (2.0f / mRenderTargetSize.x) * mRenderTargetSize.x) + 1.0f;
        float rectY = (-1.0f + (2.0f / mRenderTargetSize.y) * mRenderTargetSize.y) + 1.0f;

        // Center rectangle on screen.
        rectX /= 2;
        rectY /= 2;

        float depthDelta = 1.0f / (mNumLayers - 1);
        for (uint32_t d = 0; d < mNumLayers; ++d) {
            float depth = d * depthDelta;

            // clang-format off
            std::array<float, 36> rect = {     
                // position                   // tex coords
                 rectX,  rectY, depth, 1.0f,   1.0f, 0.0f,
                -rectX,  rectY, depth, 1.0f,   0.0f, 0.0f,
                -rectX, -rectY, depth, 1.0f,   0.0f, 1.0f,

                -rectX, -rectY, depth, 1.0f,   0.0f, 1.0f,
                 rectX, -rectY, depth, 1.0f,   1.0f, 1.0f,
                 rectX,  rectY, depth, 1.0f,   1.0f, 0.0f,
            };
            // clang-format on
            vertexData.insert(vertexData.end(), rect.begin(), rect.end());
        }

        uint32_t dataSize = ppx::SizeInBytesU32(vertexData);

        grfx::BufferCreateInfo bufferCreateInfo       = {};
        bufferCreateInfo.size                         = dataSize;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mVertexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, vertexData.data(), dataSize);
        mVertexBuffer->UnmapMemory();
    }

    // Descriptor pool
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sampledImage                   = 1;
        poolCreateInfo.sampler                        = 1;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_PS));

        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_PS));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));
    }

    // Texture image and view
    {
        std::string res = "1080p";
        if (mRenderTargetSize == uint2{3840, 2160}) {
            res = "4k";
        }

        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);

        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/bricks_" + res + ".png"), &mImage, options, false));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mImage);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo;
        grfx::Filter            filter       = mSamplerFilterType == "linear" ? grfx::FILTER_LINEAR : grfx::FILTER_NEAREST;
        grfx::SamplerMipmapMode mipmapFilter = mSamplerFilterType == "linear" ? grfx::SAMPLER_MIPMAP_MODE_LINEAR : grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.magFilter          = filter;
        samplerCreateInfo.minFilter          = filter;
        samplerCreateInfo.mipmapMode         = mipmapFilter;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
    }

    // Pipeline
    {
        std::string shaderName = mUseExplicitEarlyZShader ? "TextureSample_ExplicitEarlyZ" : "TextureSample";

        std::vector<char> bytecode = LoadShader("benchmarks/shaders", shaderName + ".vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", shaderName + ".ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32A32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        mVertexBinding.AppendAttribute({"TEXCOORD", 1, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mVertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_FRONT;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CW;
        gpCreateInfo.depthCompareOp                     = grfx::COMPARE_OP_LESS;
        gpCreateInfo.depthReadEnable                    = mEnableDepth;
        gpCreateInfo.depthWriteEnable                   = mEnableDepth;
        gpCreateInfo.blendModes[0]                      = blendModeStringToBlendMode.at(mBlendMode);
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
    }

    // Allocate descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

    // Write descriptors
    {
        grfx::WriteDescriptor write;
        write.binding    = 0;
        write.arrayIndex = 0;
        write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView = mSampledImageView;

        PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));

        grfx::WriteDescriptor sampler;
        sampler.binding  = 1;
        sampler.type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        sampler.pSampler = mSampler;

        PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &sampler));
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
        mGpuWorkDuration = data[1] - data[0];
    }
    // Reset queries
    frame.timestampQuery->Reset(0, 2);

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
            frame.cmd->SetScissors(1, &mScissorRect);
            frame.cmd->SetViewports(1, &mViewport);
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

            for (uint32_t d = 0; d < mNumLayers; ++d) {
                // If we're drawing back-to-front, start from mNumLayers - 1.
                uint32_t numLayer = mDrawFrontToBack ? d : (mNumLayers - 1 - d);
                frame.cmd->Draw(6, 1, numLayer * 6, 0);
            }

            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        }
        frame.cmd->EndRenderPass();
        frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(frame.cmd->End());

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
    if (GetFrameCount() > 0) {
        uint64_t frequency = 0;
        GetGraphicsQueue()->GetTimestampFrequency(&frequency);
        const float      gpuWorkDurationMs = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
        PerFrameRegister stats             = {};
        stats.frameNumber                  = GetFrameCount();
        stats.gpuWorkDurationMs            = gpuWorkDurationMs;
        stats.cpuFrameTimeMs               = GetPrevFrameTime();
        mFrameRegisters.push_back(stats);
    }
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);
    app.SaveResultsToFile();

    return res;
}
