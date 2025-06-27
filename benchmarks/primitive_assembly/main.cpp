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

#include <deque>
#include <filesystem>

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
    void         SaveResultsToFile();

private:
    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::FencePtr         renderCompleteFence;
        ppx::grfx::QueryPtr         timestampQuery;
        ppx::grfx::QueryPtr         pipelineStatsQuery;
    };

    std::vector<PerFrame>           mPerFrame;
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
    std::string                     mCSVFileName;
    uint64_t                        mGpuWorkDuration    = 0;
    bool                            mUsePipelineQuery   = false;
    grfx::PipelineStatistics        mPipelineStatistics = {};

    void SetupTestParameters();

    struct PerFrameRegister
    {
        uint64_t frameNumber;
        float    gpuWorkDurationMs;
        float    cpuFrameTimeMs;
        uint64_t numVertices;
        uint64_t numPrimitives;
        uint64_t clipPrimitives;
        uint64_t clipInvocations;
        uint64_t vsInvocations;
        uint64_t psInvocations;
    };
    std::deque<PerFrameRegister> mFrameRegisters;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName     = "primitive_assembly";
    settings.enableImGui = false;
    settings.grfx.api    = kApi;
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger{std::filesystem::path(mCSVFileName)};
    for (const auto& row : mFrameRegisters) {
        fileLogger.LogField(row.frameNumber);
        fileLogger.LogField(row.gpuWorkDurationMs);
        if (!mUsePipelineQuery) {
            fileLogger.LastField(row.cpuFrameTimeMs);
        }
        else {
            fileLogger.LogField(row.cpuFrameTimeMs);
            fileLogger.LogField(row.numVertices);
            fileLogger.LogField(row.numPrimitives);
            fileLogger.LogField(row.clipPrimitives);
            fileLogger.LogField(row.clipInvocations);
            fileLogger.LogField(row.vsInvocations);
            fileLogger.LastField(row.psInvocations);
        }
    }
}

void ProjApp::SetupTestParameters()
{
    // Set render target size
    mRenderTargetSize = uint2(1, 1);

    auto cl_options = GetExtraOptions();

    // Number of triangles to draw
    mNumTriangles = cl_options.GetExtraOptionValueOrDefault<uint32_t>("triangles", 1000000);

    // Name of the CSV output file
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
    }

    // Whether to use pipeline statistics queries.
    mUsePipelineQuery = cl_options.HasExtraOption("use-pipeline-query");
}

void ProjApp::Setup()
{
    SetupTestParameters();

    // Draw pass
    {
        // Usage flags for render target and depth stencil will automatically
        // be added during create. So we only need to specify the additional
        // usage flags here.
        //
        grfx::ImageUsageFlags additionalUsageFlags = 0;

        grfx::DrawPassCreateInfo createInfo     = {};
        createInfo.width                        = mRenderTargetSize.x;
        createInfo.height                       = mRenderTargetSize.y;
        createInfo.renderTargetCount            = 1;
        createInfo.renderTargetFormats[0]       = grfx::FORMAT_R16G16B16A16_FLOAT;
        createInfo.depthStencilFormat           = grfx::FORMAT_D32_FLOAT;
        createInfo.renderTargetUsageFlags[0]    = additionalUsageFlags;
        createInfo.depthStencilUsageFlags       = additionalUsageFlags;
        createInfo.renderTargetInitialStates[0] = grfx::RESOURCE_STATE_RENDER_TARGET;
        createInfo.depthStencilInitialState     = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
        createInfo.renderTargetClearValues[0]   = {0, 0, 0, 0};
        createInfo.depthStencilClearValue       = {1.0f, 0xFF};

        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mDrawPass));
    }
    // Pipeline
    {
        std::vector<char> bytecode = LoadShader("benchmarks/shaders", "PassThroughPos.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", "PassThroughPos.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = nullptr;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32A32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mVertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mDrawPass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mDrawPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
    }

    // Per frame data
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));


        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        // Create the timestamp queries
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        // Pipeline statistics query pool
        if (mUsePipelineQuery) {
            queryCreateInfo       = {};
            queryCreateInfo.type  = grfx::QUERY_TYPE_PIPELINE_STATISTICS;
            queryCreateInfo.count = 1;
            PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.pipelineStatsQuery));
        }
        mPerFrame.push_back(frame);
    }

    // Buffer and geometry data
    {
        // clang-format off
        std::vector<float> vertexData = {
            // position           
             0.0f,  0.5f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f,
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

    mViewport    = {0, 0, float(mRenderTargetSize.x), float(mRenderTargetSize.y), 0, 1};
    mScissorRect = {0, 0, mRenderTargetSize.x, mRenderTargetSize.y};
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
        mGpuWorkDuration = data[1] - data[0];
        if (mUsePipelineQuery) {
            PPX_CHECKED_CALL(frame.pipelineStatsQuery->GetData(&mPipelineStatistics, sizeof(grfx::PipelineStatistics)));
        }
    }
    // Reset queries
    frame.timestampQuery->Reset(0, 2);
    if (mUsePipelineQuery) {
        frame.pipelineStatsQuery->Reset(0, 1);
    }
    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        // Render pass to texture
        frame.cmd->BeginRenderPass(mDrawPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);
        {
            // Write start timestamp
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
            frame.cmd->SetScissors(1, &mScissorRect);
            frame.cmd->SetViewports(1, &mViewport);
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            if (mUsePipelineQuery) {
                frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
            }
            frame.cmd->Draw(3, mNumTriangles, 0, 0);
            if (mUsePipelineQuery) {
                frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
            }
        }
        frame.cmd->EndRenderPass();
        // Write end timestamp
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        // Resolve queries
        frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
        if (mUsePipelineQuery) {
            frame.cmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
        }
        // Present the swapchain
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            // Presenting the swapchain without rendering anything (Test is made in the previous pass)
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
    submitInfo.ppSignalSemaphores   = &GetSwapchain()->GetPresentationReadySemaphore(imageIndex);
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &GetSwapchain()->GetPresentationReadySemaphore(imageIndex)));
    if (GetFrameCount() > 0) {
        // Get the gpu timestamp query
        uint64_t frequency = 0;
        GetGraphicsQueue()->GetTimestampFrequency(&frequency);
        const float gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
        // Store this frame stats in a register
        PerFrameRegister csvRow  = {};
        csvRow.frameNumber       = GetFrameCount();
        csvRow.gpuWorkDurationMs = gpuWorkDuration;
        csvRow.cpuFrameTimeMs    = GetPrevFrameTime();
        if (mUsePipelineQuery) {
            csvRow.numVertices     = mPipelineStatistics.IAVertices;
            csvRow.numPrimitives   = mPipelineStatistics.IAPrimitives;
            csvRow.clipPrimitives  = mPipelineStatistics.CPrimitives;
            csvRow.clipInvocations = mPipelineStatistics.CInvocations;
            csvRow.vsInvocations   = mPipelineStatistics.VSInvocations;
            csvRow.psInvocations   = mPipelineStatistics.PSInvocations;
        }
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
