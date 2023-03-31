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
    virtual void Render() override; // Renders a single frame

    void SaveResultsToFile();

private:
    struct Payload
    {
        uint32_t value;
    };

    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::FencePtr         renderCompleteFence;
        ppx::grfx::QueryPtr         timestampQuery;
    };

    std::vector<PerFrame>           mPerFrame;
    grfx::DescriptorPoolPtr         mDescriptorPool;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;

    // Compute shader
    std::string                  mShaderFile;
    grfx::ShaderModulePtr        mCS;
    grfx::DescriptorSetLayoutPtr mComputeDescriptorSetLayout;
    grfx::DescriptorSetPtr       mComputeDescriptorSet;
    grfx::PipelineInterfacePtr   mComputePipelineInterface;
    grfx::ComputePipelinePtr     mComputePipeline;
    grfx::BufferPtr              mStorageBuffer;
    grfx::BufferPtr              mReadbackBuffer;

    // Stats
    uint64_t                 mGpuWorkDuration    = 0;
    grfx::PipelineStatistics mPipelineStatistics = {};
    std::string              mCSVFileName;

    void SetupComputeShaderPass();

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
    settings.appName                        = "compute_operations";
    settings.headless                       = true;
    settings.enableImGui                    = false;
    settings.grfx.api                       = kApi;
    settings.grfx.enableDebug               = false;
    settings.grfx.device.graphicsQueueCount = 1;
    settings.grfx.numFramesInFlight         = 1;
    settings.grfx.pacedFrameRate            = 0; // Go as fast as possible
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger = {mCSVFileName};
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

    mShaderFile = "ComputeBufferIncrement";

    // Create descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.rawStorageBuffer               = 1;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // To run the compute shader.
    SetupComputeShaderPass();

    // Per frame data
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::FenceCreateInfo fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        // Create the timestamp queries
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2; // Before and after the work of a frame is performed.
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        mPerFrame.push_back(frame);
    }
}

void ProjApp::SetupComputeShaderPass()
{
    // Storage buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo           = {};
        bufferCreateInfo.size                             = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.rawStorageBuffer = true;
        bufferCreateInfo.usageFlags.bits.transferDst      = true;
        bufferCreateInfo.usageFlags.bits.transferSrc      = true;
        bufferCreateInfo.memoryUsage                      = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mStorageBuffer));
    }

    // Readback buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferDst = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_GPU_TO_CPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mReadbackBuffer));
    }

    grfx::BufferPtr uploadBuffer;
    // Upload buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &uploadBuffer));
    }

    // Compute descriptors
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER));

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mComputeDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mComputeDescriptorSetLayout, &mComputeDescriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mStorageBuffer;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));
    }

    // Compute pipeline
    {
        std::vector<char> bytecode = LoadShader("benchmarks/shaders", mShaderFile + ".cs");
        PPX_ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mCS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mComputeDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mComputePipelineInterface));

        grfx::ComputePipelineCreateInfo cpCreateInfo = {};
        cpCreateInfo.CS                              = {mCS.Get(), "csmain"};
        cpCreateInfo.pPipelineInterface              = mComputePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateComputePipeline(&cpCreateInfo, &mComputePipeline));
    }

    // Populate storage buffer
    {
        Payload data;
        data.value = 12;
        PPX_CHECKED_CALL(uploadBuffer->CopyFromSource(sizeof(data), &data));

        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.size                         = sizeof(data);
        PPX_CHECKED_CALL(GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, uploadBuffer, mStorageBuffer, grfx::RESOURCE_STATE_UNORDERED_ACCESS, grfx::RESOURCE_STATE_UNORDERED_ACCESS));
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    uint32_t imageIndex = UINT32_MAX;

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
        // Write to the buffer with the compute shader
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
        frame.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, &mComputeDescriptorSet);
        frame.cmd->BindComputePipeline(mComputePipeline);
        frame.cmd->Dispatch(1, 1, 1); // One workgroup
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);

        // Resolve queries
        frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &frame.cmd;
    submitInfo.pFence             = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

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

    // Read the result back.
    PPX_CHECKED_CALL(frame.renderCompleteFence->Wait());

    Payload data;
    data.value = 0;

    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size                         = sizeof(data);
    GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mStorageBuffer, mReadbackBuffer, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_COPY_DST);

    PPX_CHECKED_CALL(mReadbackBuffer->CopyToDest(sizeof(data), &data));
    PPX_LOG_INFO("Data value is " + std::to_string(data.value) + ", frame count is " + std::to_string(GetFrameCount()));
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);
    app.SaveResultsToFile();

    return res;
}
