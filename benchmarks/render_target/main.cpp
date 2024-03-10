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
#include "ppx/grfx/grfx_enums.h"
#include "ppx/log.h"
#include "ppx/ppx.h"
#include "ppx/csv_file_log.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

    void SaveResultsToFile();

private:
    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
        ppx::grfx::FencePtr         renderCompleteFence;
        ppx::grfx::QueryPtr         timestampQuery;
    };

    std::vector<PerFrame>   mPerFrame;
    grfx::DescriptorPoolPtr mDescriptorPool;

    // Draw to texture(s) pass
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;
    ppx::grfx::BufferPtr            mVertexBuffer;
    grfx::DrawPassPtr               mDrawPass;
    grfx::VertexBinding             mVertexBinding;

    // Options
    uint2    mRenderTargetSize;
    uint32_t mRenderTargetCount;

    // Stats
    uint64_t                 mGpuWorkDuration    = 0;
    grfx::PipelineStatistics mPipelineStatistics = {};
    std::string              mCSVFileName;

    // For drawing into the swapchain
    grfx::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
    grfx::DescriptorSetPtr       mDrawToSwapchainSet;
    grfx::FullscreenQuadPtr      mDrawToSwapchain;
    grfx::SamplerPtr             mSampler;

    void SetupDrawToSwapchain();
    void SetupDrawToTexturePass();

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
    settings.appName                        = "render_target";
    settings.enableImGui                    = false;
    settings.grfx.api                       = kApi;
    settings.grfx.device.graphicsQueueCount = 1;
    settings.grfx.numFramesInFlight         = 1;
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

    // Name of the CSV output file.
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
    }

    // Render target(s) resolution
    std::string resolution = cl_options.GetExtraOptionValueOrDefault<std::string>("render-target-resolution", "1080p");
    if (resolution != "1080p" && resolution != "4K") {
        resolution = "1080p";
        PPX_LOG_WARN("Render Target resolution must be either \"1080p\" or \"4K\", defaulting to: " + resolution);
    }
    mRenderTargetSize = (resolution == "1080p") ? uint2(1920, 1080) : uint2(3840, 2160);

    // Number of render targets to use
    mRenderTargetCount = cl_options.GetExtraOptionValueOrDefault<uint32_t>("render-target-count", 1);
    if (mRenderTargetCount != 1 && mRenderTargetCount != 4) {
        mRenderTargetCount = 1;
        PPX_LOG_WARN("Render Target count must be either 1 or 4, defaulting to: " + std::to_string(mRenderTargetCount));
    }

    // Create descriptor pool (for both pipelines)
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 1;
        createInfo.sampledImage                   = 1;
        createInfo.uniformBuffer                  = 0;
        createInfo.storageImage                   = 0;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // To write the render targets
    SetupDrawToTexturePass();
    // To present the image on screen
    SetupDrawToSwapchain();

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

        // Create the timestamp queries
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        mPerFrame.push_back(frame);
    }
}

void ProjApp::SetupDrawToTexturePass()
{
    // Draw pass
    {
        // Usage flags for render target and depth stencil will automatically
        // be added during create. So we only need to specify the additional
        // usage flags here.
        //
        grfx::ImageUsageFlags        additionalUsageFlags = grfx::IMAGE_USAGE_SAMPLED;
        grfx::RenderTargetClearValue rtvClearValue        = {0, 0, 0, 0};
        grfx::DepthStencilClearValue dsvClearValue        = {1.0f, 0xFF};

        grfx::DrawPassCreateInfo createInfo = {};
        createInfo.width                    = mRenderTargetSize.x;
        createInfo.height                   = mRenderTargetSize.y;
        createInfo.renderTargetCount        = mRenderTargetCount;
        for (uint32_t i = 0; i < mRenderTargetCount; i++) {
            createInfo.renderTargetFormats[i]       = grfx::FORMAT_R16G16B16A16_FLOAT;
            createInfo.renderTargetUsageFlags[i]    = additionalUsageFlags;
            createInfo.renderTargetInitialStates[i] = grfx::RESOURCE_STATE_SHADER_RESOURCE;
            createInfo.renderTargetClearValues[i]   = rtvClearValue;
        }
        createInfo.depthStencilFormat       = grfx::FORMAT_D32_FLOAT;
        createInfo.depthStencilUsageFlags   = additionalUsageFlags;
        createInfo.depthStencilInitialState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
        createInfo.depthStencilClearValue   = dsvClearValue;

        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mDrawPass));
    }

    // Pipeline
    {
        const std::string shaderSource = (mRenderTargetCount == 1) ? "PassThroughPos" : "MultipleRT";
        std::vector<char> bytecode     = LoadShader("benchmarks/shaders", shaderSource + ".vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", shaderSource + ".ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = nullptr;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32A32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
        gpCreateInfo.VS                                = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount     = 1;
        gpCreateInfo.vertexInputState.bindings[0]      = mVertexBinding;
        gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                          = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
        gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount     = mRenderTargetCount;
        for (uint32_t i = 0; i < mRenderTargetCount; ++i) {
            gpCreateInfo.outputState.renderTargetFormats[i] = mDrawPass->GetRenderTargetTexture(i)->GetImageFormat();
        }
        gpCreateInfo.depthReadEnable                = false;
        gpCreateInfo.depthWriteEnable               = false;
        gpCreateInfo.outputState.depthStencilFormat = mDrawPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface             = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
    }

    // Buffer and geometry data
    {
        // One triangle that covers the whole render target
        // clang-format off
        std::vector<float> vertexData = {
            // position           
             0.0f,  4.0f, 0.0f, 1.0f,
            -2.0f, -2.0f, 0.0f, 1.0f,
             2.0f, -2.0f, 0.0f, 1.0f,
        };
        // clang-format on
        uint32_t dataSize = ppx::SizeInBytesU32(vertexData);

        grfx::BufferCreateInfo bufferCreateInfo       = {};
        bufferCreateInfo.size                         = dataSize;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;
        bufferCreateInfo.initialState                 = grfx::RESOURCE_STATE_VERTEX_BUFFER;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mVertexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, vertexData.data(), dataSize);
        mVertexBuffer->UnmapMemory();
    }
}

void ProjApp::SetupDrawToSwapchain()
{
    // Image and sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_NEAREST;
        createInfo.minFilter               = grfx::FILTER_NEAREST;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mSampler));
    }

    // Descriptor set layout
    {
        // Descriptor set layout
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDrawToSwapchainLayout));
    }

    // Pipeline
    {
        grfx::ShaderModulePtr VS;
        std::vector<char>     bytecode = LoadShader("basic/shaders", "FullScreenTriangle.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;
        bytecode = LoadShader("basic/shaders", "FullScreenTriangle.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::FullscreenQuadCreateInfo createInfo = {};
        createInfo.VS                             = VS;
        createInfo.PS                             = PS;
        createInfo.setCount                       = 1;
        createInfo.sets[0].set                    = 0;
        createInfo.sets[0].pLayout                = mDrawToSwapchainLayout;
        createInfo.renderTargetCount              = 1;
        createInfo.renderTargetFormats[0]         = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat             = GetSwapchain()->GetDepthFormat();

        PPX_CHECKED_CALL(GetDevice()->CreateFullscreenQuad(&createInfo, &mDrawToSwapchain));
    }

    // Allocate descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDrawToSwapchainLayout, &mDrawToSwapchainSet));

    // Update descriptors
    {
        grfx::WriteDescriptor writes[2] = {};
        writes[0].binding               = 0;
        writes[0].arrayIndex            = 0;
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageView            = mDrawPass->GetRenderTargetTexture(0)->GetSampledImageView();

        writes[1].binding  = 1;
        writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pSampler = mSampler;

        PPX_CHECKED_CALL(mDrawToSwapchainSet->UpdateDescriptors(2, writes));
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
        frame.cmd->TransitionImageLayout(
            mDrawPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);

        // Draw to render target(s) pass
        frame.cmd->BeginRenderPass(mDrawPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);
        {
            frame.cmd->SetScissors(mDrawPass->GetScissor());
            frame.cmd->SetViewports(mDrawPass->GetViewport());
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
            frame.cmd->Draw(3, 1, 0, 0);
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        }
        frame.cmd->EndRenderPass();

        // Resolve queries
        frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);

        frame.cmd->TransitionImageLayout(
            mDrawPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);

        // Blit to swapchain pass
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            // Draw render target output to swapchain
            frame.cmd->Draw(mDrawToSwapchain, 1, &mDrawToSwapchainSet);
        }
        frame.cmd->EndRenderPass();

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
        const float      gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
        PerFrameRegister csvRow          = {};
        csvRow.frameNumber               = GetFrameCount();
        csvRow.gpuWorkDurationMs         = gpuWorkDuration;
        csvRow.cpuFrameTimeMs            = GetPrevFrameTime();
        mFrameRegisters.push_back(csvRow);
    }
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);
    app.SaveResultsToFile();

    return res;
}
