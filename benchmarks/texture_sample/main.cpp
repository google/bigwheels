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
#include "ppx/grfx/grfx_constants.h"
#include "ppx/grfx/grfx_enums.h"
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

    // Textures
    uint32_t                                    mNumImages;
    std::vector<ppx::grfx::ImagePtr>            mImages;
    std::vector<ppx::grfx::SampledImageViewPtr> mSampledImageViews;
    ppx::grfx::SamplerPtr                       mSampler;
    std::string                                 mSamplerFilterType;
    std::string                                 mSamplerMipmapFilterType;

    // Drawn rectangle sizes (number of mipmaps of target resolution)
    uint32_t mNumRectSizes = 0;

    // If set, the mip level to render each frame.
    // If unset (-1), mip levels are cycled one per frame.
    int32_t mForcedMipLevel = -1;

    // Stats
    uint64_t    mGpuWorkDuration = 0;
    std::string mCSVFileName;
    struct PerFrameRegister
    {
        uint64_t frameNumber;
        float    gpuWorkDuration;
        float    cpuFrameTime;
        uint32_t mipLevel;
    };
    std::deque<PerFrameRegister> mFrameRegisters;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "texture_sample";
    settings.enableImGui      = false;
    settings.grfx.api         = kApi;
    settings.grfx.enableDebug = false;
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger = {mCSVFileName};
    for (const auto& row : mFrameRegisters) {
        fileLogger.LogField(row.frameNumber);
        fileLogger.LogField(row.gpuWorkDuration);
        fileLogger.LogField(row.cpuFrameTime);
        fileLogger.LastField(row.mipLevel);
    }
}

void ProjApp::Setup()
{
    auto cl_options = GetExtraOptions();

    // Number of images (textures) to load in the pixel shader. Can be either 1 or 4.
    mNumImages = cl_options.GetExtraOptionValueOrDefault<uint32_t>("num-images", 1);
    if (mNumImages != 1 && mNumImages != 4) {
        PPX_LOG_WARN("Number of images must be either 1 or 4, defaulting to: 1");
        mNumImages = 1;
    }

    // Name of the CSV output file.
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
    }

    // Sampler filter operations (both normal and for mipmap).
    mSamplerFilterType = cl_options.GetExtraOptionValueOrDefault<std::string>("filter-type", "linear");
    if (mSamplerFilterType != "linear" && mSamplerFilterType != "nearest") {
        mSamplerFilterType = "linear";
        PPX_LOG_WARN("Invalid sampler filter type (must be `linear` or `nearest`), defaulting to: " + mSamplerFilterType);
    }
    mSamplerMipmapFilterType = cl_options.GetExtraOptionValueOrDefault<std::string>("mipmap-filter-type", "linear");
    if (mSamplerMipmapFilterType != "linear" && mSamplerMipmapFilterType != "nearest") {
        mSamplerMipmapFilterType = "linear";
        PPX_LOG_WARN("Invalid sampler mipmap filter type (must be `linear` or `nearest`), defaulting to: " + mSamplerMipmapFilterType);
    }

    // Forced mip level to use for all frames (instead of cycling through all mip levels, one per frame).
    // This value is validated once the image is created and the mip level count is known.
    mForcedMipLevel = cl_options.GetExtraOptionValueOrDefault<int32_t>("force-mip-level", -1);

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

    // Vertex buffer for rectangles (one for each mip level).
    {
        auto rectSize = mRenderTargetSize;

        std::vector<float> vertexData;
        while (rectSize.x > 1 || rectSize.y > 1) {
            mNumRectSizes++;

            // Map rectangle sides length to [-1,+1] space.
            float rectX = (-1.0f + (2.0f / mRenderTargetSize.x) * rectSize.x) + 1.0f;
            float rectY = (-1.0f + (2.0f / mRenderTargetSize.y) * rectSize.y) + 1.0f;

            // Center rectangle on screen.
            rectX /= 2;
            rectY /= 2;

            // clang-format off
            std::array<float, 36> rect = {     
                // position                   // tex coords
                 rectX,  rectY, 0.0f, 1.0f,   1.0f, 0.0f,
                -rectX,  rectY, 0.0f, 1.0f,   0.0f, 0.0f,
                -rectX, -rectY, 0.0f, 1.0f,   0.0f, 1.0f,

                -rectX, -rectY, 0.0f, 1.0f,   0.0f, 1.0f,
                 rectX, -rectY, 0.0f, 1.0f,   1.0f, 1.0f,
                 rectX,  rectY, 0.0f, 1.0f,   1.0f, 0.0f,
            };
            // clang-format on
            vertexData.insert(vertexData.end(), rect.begin(), rect.end());

            rectSize.x = std::max(rectSize.x / 2, 1u);
            rectSize.y = std::max(rectSize.y / 2, 1u);
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
        poolCreateInfo.sampledImage                   = mNumImages;
        poolCreateInfo.sampler                        = 1;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    // Descriptor set layout
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        for (uint32_t i = 0; i < mNumImages; ++i) {
            layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(i, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_PS));
        }
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(mNumImages, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_PS));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));
    }

    // Texture image and view
    {
        std::string res = "1080p";
        if (mRenderTargetSize == uint2{3840, 2160}) {
            res = "4k";
        }

        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);

        for (uint32_t i = 0; i < mNumImages; ++i) {
            grfx::ImagePtr image;
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("benchmarks/textures/bricks_" + res + ".png"), &image, options, false));
            mImages.push_back(image);

            grfx::SampledImageViewPtr        imageView;
            grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(image);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &imageView));
            mSampledImageViews.push_back(imageView);

            if (mForcedMipLevel != -1 && static_cast<uint32_t>(mForcedMipLevel) >= imageView->GetMipLevelCount()) {
                mForcedMipLevel = -1;
                PPX_LOG_WARN("Invalid mip level, defaulting to all mip levels");
            }
        }
        grfx::SamplerCreateInfo samplerCreateInfo;
        grfx::Filter            filter       = mSamplerFilterType == "linear" ? grfx::FILTER_LINEAR : grfx::FILTER_NEAREST;
        grfx::SamplerMipmapMode mipmapFilter = mSamplerMipmapFilterType == "linear" ? grfx::SAMPLER_MIPMAP_MODE_LINEAR : grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.magFilter          = filter;
        samplerCreateInfo.minFilter          = filter;
        samplerCreateInfo.mipmapMode         = mipmapFilter;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
    }

    // Pipeline
    {
        std::string shaderName = mNumImages == 1 ? "TextureSample" : "TextureSample4Textures";

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

    // Allocate descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

    // Write descriptors
    {
        std::vector<grfx::WriteDescriptor> writes;
        for (uint32_t i = 0; i < mNumImages; ++i) {
            grfx::WriteDescriptor write;
            write.binding    = i;
            write.arrayIndex = 0;
            write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageView = mSampledImageViews[i];
            writes.push_back(write);
        }
        PPX_CHECKED_CALL(mDescriptorSet->UpdateDescriptors(CountU32(writes), writes.data()));

        grfx::WriteDescriptor sampler;
        sampler.binding  = mNumImages;
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

    int mipLevel = GetFrameCount() % mNumRectSizes;
    if (mForcedMipLevel != -1) {
        mipLevel = mForcedMipLevel;
    }

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
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->Draw(6, 1, mipLevel * 6, 0);
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
        stats.mipLevel                   = mipLevel;
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
