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

    std::vector<PerFrame>           mPerFrame;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;
    ppx::grfx::BufferPtr            mVertexBuffer;
    grfx::Viewport                  mViewport;
    grfx::Rect                      mScissorRect;
    grfx::VertexBinding             mVertexBinding;
    uint2                           mRenderTargetSize;

    // Options
    uint32_t mNumTriangles;
    bool     mUseInstancedDraw;

    // Stats
    uint64_t                 mGpuWorkDuration    = 0;
    grfx::PipelineStatistics mPipelineStatistics = {};
    std::string              mCSVFileName;
    struct PerFrameRegister
    {
        uint64_t frameNumber;
        float    gpuWorkDuration;
        float    cpuFrameTime;
    };
    std::deque<PerFrameRegister> mFrameRegisters;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                        = "draw_call";
    settings.enableImGui                    = false;
    settings.grfx.api                       = kApi;
    settings.grfx.enableDebug               = false;
    settings.grfx.device.graphicsQueueCount = 1;
    settings.grfx.numFramesInFlight         = 1;
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger = {mCSVFileName};
    for (const auto& row : mFrameRegisters) {
        fileLogger.LogField(row.frameNumber);
        fileLogger.LogField(row.gpuWorkDuration);
        fileLogger.LastField(row.cpuFrameTime);
    }
}

void ProjApp::Setup()
{
    auto cl_options = GetExtraOptions();

    // Number of triangles to draw.
    mNumTriangles = cl_options.GetExtraOptionValueOrDefault<uint32_t>("num-triangles", 10000);
    if (mNumTriangles == 0) {
        mNumTriangles = 10000;
        PPX_LOG_WARN("Number of triangles must be greater than zero, defaulting to: " + std::to_string(mNumTriangles));
    }

    // Whether to make an instanced call for all triangles or use separate draw calls.
    mUseInstancedDraw = cl_options.GetExtraOptionValueOrDefault<bool>("instanced-draw", false);

    // Name of the CSV output file
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
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

    // Vertex buffer for triangle.
    {
        // Size of half the triangle's side.
        // Use a very small triangle since we are not interested in anything
        // but draw call load.
        float triangleHalfSize = 0.00001f;

        // clang-format off
        std::vector<float> vertexData = {     
             triangleHalfSize,  triangleHalfSize, 0.0f, 1.0f, 
            -triangleHalfSize,  triangleHalfSize, 0.0f, 1.0f, 
            -triangleHalfSize, -triangleHalfSize, 0.0f, 1.0f,
        };
        // clang-format on
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

    // Pipeline
    {
        std::string shaderName = "PassThroughPos";

        std::vector<char> bytecode = LoadShader("benchmarks/shaders", shaderName + ".vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", shaderName + ".ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
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
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
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

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
            frame.cmd->SetScissors(1, &mScissorRect);
            frame.cmd->SetViewports(1, &mViewport);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            if (mUseInstancedDraw) {
                frame.cmd->Draw(3, mNumTriangles, 0, 0);
            }
            else {
                for (uint32_t i = 0; i < mNumTriangles; ++i) {
                    frame.cmd->Draw(3, 1, 0, 0);
                }
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
        const float      gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
        PerFrameRegister stats           = {};
        stats.frameNumber                = GetFrameCount();
        stats.gpuWorkDuration            = gpuWorkDuration;
        stats.cpuFrameTime               = GetPrevFrameTime();
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
