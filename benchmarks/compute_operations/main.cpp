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

#if defined(USE_DX11)
const grfx::Api kApi = grfx::API_DX_11_1;
#elif defined(USE_DX12)
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
    grfx::DescriptorPoolPtr         mDescriptorPool;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;
    ppx::grfx::BufferPtr            mVertexBuffer;
    grfx::Viewport                  mViewport;
    grfx::Rect                      mScissorRect;
    grfx::VertexBinding             mVertexBinding;
    uint2                           mRenderTargetSize;

    // Compute shader
    std::string                  mShaderFile;
    grfx::ShaderModulePtr        mCS;
    grfx::DescriptorSetLayoutPtr mComputeDescriptorSetLayout;
    grfx::DescriptorSetPtr       mComputeDescriptorSet;
    grfx::PipelineInterfacePtr   mComputePipelineInterface;
    grfx::ComputePipelinePtr     mComputePipeline;
    grfx::SampledImageViewPtr    mSampledImageView;
    grfx::StorageImageViewPtr    mStorageImageView;
    grfx::SamplerPtr             mComputeSampler;
    grfx::BufferPtr              mUniformBuffer;
    grfx::ImagePtr               mFilteredImage;

    // Options
    uint32_t mFilterOption;

    // Stats
    uint64_t                 mGpuWorkDuration    = 0;
    grfx::PipelineStatistics mPipelineStatistics = {};
    std::string              mCSVFileName;

    // Textures
    grfx::ImagePtr            mOriginalImage;
    grfx::SampledImageViewPtr mPresentImageView;

    // For drawing into the swapchain
    grfx::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
    grfx::DescriptorSetPtr       mDrawToSwapchainSet;
    grfx::FullscreenQuadPtr      mDrawToSwapchain;
    grfx::SamplerPtr             mSampler;

    void SetupDrawToSwapchain();
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
    settings.enableImGui                    = false;
    settings.grfx.api                       = kApi;
    settings.grfx.enableDebug               = false;
    settings.grfx.device.graphicsQueueCount = 1;
    settings.grfx.numFramesInFlight         = 1;
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

    // Filter size
    uint32_t filter_size = cl_options.GetExtraOptionValueOrDefault<uint32_t>("filter-size", 3);
    if (filter_size != 3 && filter_size != 5 && filter_size != 7) {
        filter_size = 3;
        PPX_LOG_WARN("The filter-size must be 3, 5 or 7, defaulting to: " + std::to_string(filter_size));
    }
    mShaderFile = "ComputeFilter" + std::to_string(filter_size);

    // Create descriptor pool (for both pipelines)
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 2;
        createInfo.sampledImage                   = 2;
        createInfo.uniformBuffer                  = 1;
        createInfo.storageImage                   = 1;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // To filter the image
    SetupComputeShaderPass();
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

    mViewport    = {0, 0, float(GetWindowWidth()), float(GetWindowHeight()), 0, 1};
    mScissorRect = {0, 0, GetWindowWidth(), GetWindowHeight()};
}

void ProjApp::SetupComputeShaderPass()
{
    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mUniformBuffer));
    }

    // Texture image, view, and sampler
    {
        grfx_util::ImageOptions options = grfx_util::ImageOptions().AdditionalUsage(grfx::IMAGE_USAGE_STORAGE).MipLevelCount(1);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/test_image_1280x720.jpg"), &mOriginalImage, options, false));
        // Create Filtered image
        {
            grfx::ImageCreateInfo ci       = {};
            ci.type                        = grfx::IMAGE_TYPE_2D;
            ci.width                       = mOriginalImage->GetWidth();
            ci.height                      = mOriginalImage->GetHeight();
            ci.depth                       = 1;
            ci.format                      = mOriginalImage->GetFormat();
            ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
            ci.mipLevelCount               = mOriginalImage->GetMipLevelCount();
            ci.arrayLayerCount             = 1;
            ci.usageFlags.bits.transferDst = true;
            ci.usageFlags.bits.transferSrc = true; // For CS
            ci.usageFlags.bits.sampled     = true;
            ci.usageFlags.bits.storage     = true; // For CS
            ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
            ci.initialState                = grfx::RESOURCE_STATE_SHADER_RESOURCE;

            PPX_CHECKED_CALL(GetDevice()->CreateImage(&ci, &mFilteredImage));
        }

        grfx::SampledImageViewCreateInfo sampledViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mOriginalImage);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sampledViewCreateInfo, &mSampledImageView));

        grfx::StorageImageViewCreateInfo storageViewCreateInfo = grfx::StorageImageViewCreateInfo::GuessFromImage(mFilteredImage);
        PPX_CHECKED_CALL(GetDevice()->CreateStorageImageView(&storageViewCreateInfo, &mStorageImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.minFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.minLod                  = 0.0f;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mComputeSampler));
    }

    // Compute descriptors
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(3, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mComputeDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mComputeDescriptorSetLayout, &mComputeDescriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageView            = mStorageImageView;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write              = {};
        write.binding      = 1;
        write.type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset = 0;
        write.bufferRange  = PPX_WHOLE_SIZE;
        write.pBuffer      = mUniformBuffer;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write          = {};
        write.binding  = 2;
        write.type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler = mComputeSampler;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write            = {};
        write.binding    = 3;
        write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView = mSampledImageView;
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

    // Update uniform buffer
    {
        struct alignas(16) ParamsData
        {
            float2 texel_size;
        };
        ParamsData params;

        params.texel_size.x = 1.0f / float(mFilteredImage->GetWidth());
        params.texel_size.y = 1.0f / float(mFilteredImage->GetHeight());

        void* pData = nullptr;
        PPX_CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &params, sizeof(ParamsData));
        mUniformBuffer->UnmapMemory();
    }
}

void ProjApp::SetupDrawToSwapchain()
{
    // Image and sampler
    {
        grfx::SampledImageViewCreateInfo presentViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mFilteredImage);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&presentViewCreateInfo, &mPresentImageView));

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
        writes[0].pImageView            = mPresentImageView;

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

    int w = swapchain->GetWidth();
    int h = swapchain->GetHeight();

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

        // Filter image with CS
        frame.cmd->TransitionImageLayout(mFilteredImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_UNORDERED_ACCESS);
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
        frame.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, &mComputeDescriptorSet);
        frame.cmd->BindComputePipeline(mComputePipeline);
        frame.cmd->Dispatch(mFilteredImage->GetWidth(), mFilteredImage->GetHeight(), 1);
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        frame.cmd->TransitionImageLayout(mFilteredImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_UNORDERED_ACCESS, grfx::RESOURCE_STATE_SHADER_RESOURCE);

        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            // Draw render target output to swapchain
            frame.cmd->Draw(mDrawToSwapchain, 1, &mDrawToSwapchainSet);
        }
        frame.cmd->EndRenderPass();
        // Resolve queries
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
