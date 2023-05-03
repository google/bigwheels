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
#include <fstream>

#include "ppx/grfx/grfx_scope.h"

#include "ppx/ppx.h"
#include "ppx/graphics_util.h"
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
        ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
        ppx::grfx::FencePtr         renderCompleteFence;
    };

    grfx::DescriptorPoolPtr         mDescriptorPool;
    std::vector<PerFrame>           mPerFrame;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;

    // For drawing into the swapchain
    grfx::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
    grfx::DescriptorSetPtr       mDrawToSwapchainSet;
    grfx::FullscreenQuadPtr      mDrawToSwapchain;
    grfx::SamplerPtr             mSampler;

    // Test parameters
    std::vector<std::string> mTextureNames;
    std::string              mCSVFileName;

    // Textures
    std::vector<ppx::grfx::SampledImageViewPtr> mSampledImageViews;

    void SetupTestParameters();
    void TransferTexture(const std::string fileName);
    void SetupDrawToSwapchain();

    struct PerFrameRegister
    {
        uint64_t frameNumber;
        float    cpuTransferTimeMs;
        uint2    textureSize;
    };
    std::deque<PerFrameRegister> mFrameRegisters;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "texture_transfer_cpu_to_gpu";
    settings.enableImGui      = false;
    settings.grfx.api         = kApi;
    settings.grfx.enableDebug = false;
}

void ProjApp::SaveResultsToFile()
{
    CSVFileLog fileLogger = {mCSVFileName};
    for (const auto& row : mFrameRegisters) {
        fileLogger.LogField(row.frameNumber);
        fileLogger.LogField(row.cpuTransferTimeMs);
        fileLogger.LogField(row.textureSize.x);
        fileLogger.LastField(row.textureSize.y);
    }
}

void ProjApp::SetupTestParameters()
{
    // List of the textures used in this benchmark
    const std::string basepath = {"benchmarks/textures/Wood/"};
    mTextureNames.push_back(basepath + "WoodHD720p1280x720.png");
    mTextureNames.push_back(basepath + "WoodFullHD1080p1920x1080.png");
    mTextureNames.push_back(basepath + "Wood4KUHD-3840x2160.png");
    mTextureNames.push_back(basepath + "Wood4KUHD3840x2160.png");
    mTextureNames.push_back(basepath + "WoodWQXGA2560x1600.png");
    mTextureNames.push_back(basepath + "WoodWUXGA1920x1200.png");
    mTextureNames.push_back(basepath + "WoodHD1366x768.png");
    mTextureNames.push_back(basepath + "Wood640x480.png");
    mTextureNames.push_back(basepath + "Wood800x600.png");
    mTextureNames.push_back(basepath + "Wood1024x1024.jpg");
    mTextureNames.push_back(basepath + "Wood512x512.jpg");
    mTextureNames.push_back(basepath + "Wood256x256.jpg");
    mTextureNames.push_back(basepath + "Wood128x128.jpg");
    mTextureNames.push_back(basepath + "Wood64x64.jpg");
    mTextureNames.push_back(basepath + "Wood32x32.jpg");
    mTextureNames.push_back(basepath + "Wood16x16.jpg");
    mTextureNames.push_back(basepath + "Wood8x8.jpg");
    mTextureNames.push_back(basepath + "Wood4x4.jpg");
    mTextureNames.push_back(basepath + "Wood2x2.jpg");
    mTextureNames.push_back(basepath + "Wood1x1.jpg");

    const CliOptions& cl_options = GetExtraOptions();

    uint32_t imageIndex = 0;
    if (cl_options.HasExtraOption("use-image")) {
        imageIndex = cl_options.GetExtraOptionValueOrDefault<uint32_t>("use-image", 0);
        if (imageIndex == 0 || imageIndex > mTextureNames.size()) {
            PPX_LOG_WARN("Invalid --use-image index, value must be between [1," + std::to_string(mTextureNames.size()) + "]");
        }
        else {
            std::string fileName = mTextureNames[imageIndex - 1];
            mTextureNames.clear();
            mTextureNames.push_back(fileName);
        }
    }

    // Name of the CSV output file
    mCSVFileName = cl_options.GetExtraOptionValueOrDefault<std::string>("stats-file", "stats.csv");
    if (mCSVFileName.empty()) {
        mCSVFileName = "stats.csv";
        PPX_LOG_WARN("Invalid name for CSV log file, defaulting to: " + mCSVFileName);
    }
}

void ProjApp::SetupDrawToSwapchain()
{
    // Create descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 1;
        createInfo.sampledImage                   = 1;
        createInfo.uniformBuffer                  = 0;
        createInfo.structuredBuffer               = 0;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // Sampler
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
}

void ProjApp::TransferTexture(const std::string fileName)
{
    Timer timer;
    PPX_ASSERT_MSG(timer.Start() == ppx::TIMER_RESULT_SUCCESS, "timer start failed");

    // Scoped destroy
    grfx::ScopeDestroyer SCOPED_DESTROYER(GetDevice());

    Bitmap bitmap;
    // Load bitmap to CPU from file
    PPX_CHECKED_CALL(Bitmap::LoadFile(GetAssetPath(fileName), &bitmap));
    // Create target image
    grfx::ImagePtr image;
    {
        grfx::ImageCreateInfo ci       = {};
        ci.type                        = grfx::IMAGE_TYPE_2D;
        ci.width                       = bitmap.GetWidth();
        ci.height                      = bitmap.GetHeight();
        ci.depth                       = 1;
        ci.format                      = grfx_util::ToGrfxFormat(bitmap.GetFormat());
        ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
        ci.mipLevelCount               = 1;
        ci.arrayLayerCount             = 1;
        ci.usageFlags.bits.transferDst = true;
        ci.usageFlags.bits.sampled     = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.initialState                = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateImage(&ci, &image));
    }
    // This is the actual test, we timer it on the CPU
    // Since we time in CPU we put a hard barrier to ensure GPU finishes
    GetDevice()->WaitIdle();
    double transferStartTimeMs = timer.MillisSinceStart();
    PPX_CHECKED_CALL(grfx_util::CopyBitmapToImage(GetDevice()->GetGraphicsQueue(), &bitmap, image, 0, 0, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_SHADER_RESOURCE));
    GetDevice()->WaitIdle();
    double transferEndTimeMs = timer.MillisSinceStart();
    float  elapsedTimeMs     = static_cast<float>(transferEndTimeMs - transferStartTimeMs);
    // Save the result to a register
    PerFrameRegister stats  = {};
    stats.frameNumber       = GetFrameCount();
    stats.cpuTransferTimeMs = elapsedTimeMs;
    stats.textureSize       = uint2(image->GetWidth(), image->GetHeight());
    mFrameRegisters.push_back(stats);

    if (mSampledImageViews.size() < mTextureNames.size()) {
        // Since we later render the texture, we keep a copy of the view
        grfx::SampledImageViewPtr        imageView;
        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &imageView));
        mSampledImageViews.push_back(imageView);
    }
}

void ProjApp::Setup()
{
    SetupTestParameters();

    // After the benchmark, we prepare to render the textures
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

        mPerFrame.push_back(frame);
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    // The benchmark happens inside this call
    TransferTexture(mTextureNames[GetFrameCount() % mTextureNames.size()]);

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Change texture every second
    float    seconds        = GetElapsedSeconds();
    uint32_t currentTexture = static_cast<size_t>(seconds) % mSampledImageViews.size();

    // Update descriptors
    {
        grfx::WriteDescriptor writes[2] = {};
        writes[0].binding               = 0;
        writes[0].arrayIndex            = 0;
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageView            = mSampledImageViews[currentTexture];

        writes[1].binding  = 1;
        writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pSampler = mSampler;

        PPX_CHECKED_CALL(mDrawToSwapchainSet->UpdateDescriptors(2, writes));
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
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);
    app.SaveResultsToFile();

    return res;
}
