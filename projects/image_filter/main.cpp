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
#include "ppx/ppx.h"

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

protected:
    virtual void DrawGui() override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
        grfx::QueryPtr         timestampQuery;
    };

    std::vector<PerFrame>      mPerFrame;
    grfx::DescriptorPoolPtr    mDescriptorPool;
    grfx::ShaderModulePtr      mVS;
    grfx::ShaderModulePtr      mPS;
    grfx::PipelineInterfacePtr mPipelineInterface;
    grfx::GraphicsPipelinePtr  mPipeline;
    grfx::BufferPtr            mVertexBuffer;
    grfx::BufferPtr            mUniformBuffer;
    grfx::VertexBinding        mVertexBinding;

    // Compute shader
    std::string                  mShaderFile;
    grfx::ShaderModulePtr        mCS;
    grfx::DescriptorSetLayoutPtr mComputeDescriptorSetLayout;
    grfx::DescriptorSetPtr       mComputeDescriptorSet;
    grfx::PipelineInterfacePtr   mComputePipelineInterface;
    grfx::ComputePipelinePtr     mComputePipeline;
    grfx::SamplerPtr             mComputeSampler;
    grfx::BufferPtr              mComputeUniformBuffer;

    // Options
    uint32_t mFilterOption = 0;
    uint32_t mImageOption  = 0;

    // Stats
    uint64_t mGpuWorkDuration = 0;
    float    mCSDurationMs;

    // Textures
    std::vector<grfx::ImagePtr>            mOriginalImages;
    std::vector<grfx::ImagePtr>            mFilteredImages;
    std::vector<grfx::SampledImageViewPtr> mPresentImageViews;
    std::vector<grfx::SampledImageViewPtr> mSampledImageViews;
    std::vector<grfx::StorageImageViewPtr> mStorageImageViews;

    // For drawing into the swapchain
    grfx::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
    grfx::DescriptorSetPtr       mDrawToSwapchainSet;
    grfx::FullscreenQuadPtr      mDrawToSwapchain;
    grfx::SamplerPtr             mSampler;

    void SetupDrawToSwapchain();
    void SetupComputeShaderPass();

    float4x4 calculateTransform(float2 imgSize);
    void     changeImages();
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                        = "image_filter";
    settings.enableImGui                    = true;
    settings.grfx.api                       = kApi;
    settings.grfx.device.graphicsQueueCount = 1;
    settings.grfx.numFramesInFlight         = 1;
}

void ProjApp::Setup()
{
    // Create descriptor pool (for both pipelines)
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 2;
        createInfo.sampledImage                   = 2;
        createInfo.uniformBuffer                  = 2;
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
}

void ProjApp::SetupComputeShaderPass()
{
    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mComputeUniformBuffer));
    }

    // Texture images, views, and sampler
    {
        std::vector<std::string> imageFiles = {"basic/textures/hanging_lights.jpg", "basic/textures/chinatown.jpg", "basic/textures/box_panel.jpg", "benchmarks/textures/test_image_1280x720.jpg"};

        for (size_t i = 0; i < imageFiles.size(); ++i) {
            grfx_util::ImageOptions options = grfx_util::ImageOptions().AdditionalUsage(grfx::IMAGE_USAGE_STORAGE).MipLevelCount(1);
            grfx::ImagePtr          originalImage;
            grfx::ImagePtr          filteredImage;
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(imageFiles[i]), &originalImage, options, false));
            mOriginalImages.push_back(originalImage);
            // Create Filtered image
            {
                grfx::ImageCreateInfo ci       = {};
                ci.type                        = grfx::IMAGE_TYPE_2D;
                ci.width                       = originalImage->GetWidth();
                ci.height                      = originalImage->GetHeight();
                ci.depth                       = 1;
                ci.format                      = originalImage->GetFormat();
                ci.sampleCount                 = grfx::SAMPLE_COUNT_1;
                ci.mipLevelCount               = originalImage->GetMipLevelCount();
                ci.arrayLayerCount             = 1;
                ci.usageFlags.bits.transferDst = true;
                ci.usageFlags.bits.transferSrc = true; // For CS
                ci.usageFlags.bits.sampled     = true;
                ci.usageFlags.bits.storage     = true; // For CS
                ci.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
                ci.initialState                = grfx::RESOURCE_STATE_SHADER_RESOURCE;

                PPX_CHECKED_CALL(GetDevice()->CreateImage(&ci, &filteredImage));
                mFilteredImages.push_back(filteredImage);
            }
            grfx::SampledImageViewPtr        sampledImageView;
            grfx::SampledImageViewCreateInfo sampledViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mOriginalImages[i]);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sampledViewCreateInfo, &sampledImageView));
            mSampledImageViews.push_back(sampledImageView);

            grfx::StorageImageViewPtr        storageImageView;
            grfx::StorageImageViewCreateInfo storageViewCreateInfo = grfx::StorageImageViewCreateInfo::GuessFromImage(mFilteredImages[i]);
            PPX_CHECKED_CALL(GetDevice()->CreateStorageImageView(&storageViewCreateInfo, &storageImageView));
            mStorageImageViews.push_back(storageImageView);
        }

        // Sampler
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.minFilter               = grfx::FILTER_NEAREST;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
        write.pImageView            = mStorageImageViews[mImageOption];
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write              = {};
        write.binding      = 1;
        write.type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset = 0;
        write.bufferRange  = PPX_WHOLE_SIZE;
        write.pBuffer      = mComputeUniformBuffer;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write          = {};
        write.binding  = 2;
        write.type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler = mComputeSampler;
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));

        write            = {};
        write.binding    = 3;
        write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView = mSampledImageViews[mImageOption];
        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));
    }

    // Compute pipeline
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "ImageFilter.cs");
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
}

void ProjApp::SetupDrawToSwapchain()
{
    // Image and sampler
    {
        for (size_t i = 0; i < mFilteredImages.size(); ++i) {
            grfx::SampledImageViewPtr        presentImageView;
            grfx::SampledImageViewCreateInfo presentViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mFilteredImages[i]);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&presentViewCreateInfo, &presentImageView));
            mPresentImageViews.push_back(presentImageView);
        }

        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_NEAREST;
        createInfo.minFilter               = grfx::FILTER_NEAREST;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mSampler));
    }

    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mUniformBuffer));
    }

    // Descriptors
    {
        // Descriptor set layout
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDrawToSwapchainLayout));

        // Allocate descriptor set
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDrawToSwapchainLayout, &mDrawToSwapchainSet));

        grfx::WriteDescriptor writes[3] = {};
        writes[0].binding               = 0;
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset          = 0;
        writes[0].bufferRange           = PPX_WHOLE_SIZE;
        writes[0].pBuffer               = mUniformBuffer;

        writes[1].binding    = 1;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mPresentImageViews[mImageOption];

        writes[2].binding  = 2;
        writes[2].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[2].pSampler = mSampler;

        PPX_CHECKED_CALL(mDrawToSwapchainSet->UpdateDescriptors(3, writes));
    }

    // Pipeline
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "Texture.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "Texture.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDrawToSwapchainLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        mVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        mVertexBinding.AppendAttribute({"TEXCOORD", 1, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

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
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipeline));
    }

    // Vertex buffer and geometry data
    {
        // clang-format off
        std::vector<float> vertexData = {
            // position           // tex coords
            -0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,   0.0f, 1.0f,
             0.5f, -0.5f, 0.0f,   1.0f, 1.0f,

            -0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,   1.0f, 1.0f,
             0.5f,  0.5f, 0.0f,   1.0f, 0.0f,
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
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    int w = swapchain->GetWidth();
    int h = swapchain->GetHeight();

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
    }
    // Reset queries
    frame.timestampQuery->Reset(0, 2);

    changeImages();

    // Update Compute uniform buffer
    {
        struct alignas(16) ParamsData
        {
            float2 texel_size;
            int    filter;
        };
        ParamsData params;

        params.texel_size.x = 1.0f / float(mFilteredImages[mImageOption]->GetWidth());
        params.texel_size.y = 1.0f / float(mFilteredImages[mImageOption]->GetHeight());

        params.filter = mFilterOption;

        void* pData = nullptr;
        PPX_CHECKED_CALL(mComputeUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &params, sizeof(ParamsData));
        mComputeUniformBuffer->UnmapMemory();
    }

    // Update graphics uniform buffer
    {
        float4x4 mat = calculateTransform(float2(mFilteredImages[mImageOption]->GetWidth(), mFilteredImages[mImageOption]->GetHeight()));
        // This shader only takes a floa4x4 as uniform
        void* pData = nullptr;
        PPX_CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
        memcpy(pData, &mat, sizeof(float4x4));
        mUniformBuffer->UnmapMemory();
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};

        // Filter image with CS
        frame.cmd->TransitionImageLayout(mFilteredImages[mImageOption], PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_UNORDERED_ACCESS);
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
        frame.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, &mComputeDescriptorSet);
        frame.cmd->BindComputePipeline(mComputePipeline);
        uint32_t dispatchX = static_cast<uint32_t>(std::ceil(mFilteredImages[mImageOption]->GetWidth() / 32.0));
        uint32_t dispatchY = static_cast<uint32_t>(std::ceil(mFilteredImages[mImageOption]->GetHeight() / 32.0));
        frame.cmd->Dispatch(dispatchX, dispatchY, 1);
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
        frame.cmd->TransitionImageLayout(mFilteredImages[mImageOption], PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_UNORDERED_ACCESS, grfx::RESOURCE_STATE_SHADER_RESOURCE);

        frame.cmd->SetScissors(renderPass->GetScissor());
        frame.cmd->SetViewports(renderPass->GetViewport());

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            // Draw texture
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDrawToSwapchainSet);
            frame.cmd->BindGraphicsPipeline(mPipeline);
            frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
            frame.cmd->Draw(6, 1, 0, 0);

            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.cmd);
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
        mCSDurationMs = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
    }
}

float4x4 ProjApp::calculateTransform(float2 imgSize)
{
    float4x4 P;
    float2   spanRange;
    if (GetWindowWidth() < GetWindowHeight()) {
        P           = glm::ortho(-1.0f, 1.0f, (-1.0f / GetWindowAspect()), (1.0f / GetWindowAspect()));
        spanRange.x = 2.0f;
        spanRange.y = 2.0f / GetWindowAspect();
    }
    else {
        P           = glm::ortho(-GetWindowAspect(), GetWindowAspect(), -1.0f, 1.0f);
        spanRange.x = 2.0f * GetWindowAspect();
        spanRange.y = 2.0f;
    }
    float2 scaleFactors = float2(1, 1);
    float  imgAspect    = imgSize.x / imgSize.y;
    if (imgSize.x <= imgSize.y) {
        scaleFactors.x = spanRange.y * imgAspect;
        scaleFactors.y = spanRange.y;
    }
    else {
        scaleFactors.x = spanRange.x;
        scaleFactors.y = spanRange.x / imgAspect;
    }

    float4x4 V = glm::lookAt(float3(0, 0, 1), float3(0, 0, 0), float3(0, 1, 0));
    float4x4 M = glm::scale(float3(scaleFactors, 1));

    return P * V * M;
}

void ProjApp::changeImages()
{
    // Update compute descriptors
    {
        grfx::WriteDescriptor writes[2] = {};

        writes[0].binding    = 0;
        writes[0].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].pImageView = mStorageImageViews[mImageOption];

        writes[1]            = {};
        writes[1].binding    = 3;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mSampledImageViews[mImageOption];

        PPX_CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(2, writes));
    }

    // Update present descriptor
    {
        grfx::WriteDescriptor write = {};
        write.binding               = 1;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = mPresentImageViews[mImageOption];

        PPX_CHECKED_CALL(mDrawToSwapchainSet->UpdateDescriptors(1, &write));
    }
}

void ProjApp::DrawGui()
{
    ImGui::Separator();
    ImGui::Text("Filter time: %fms", mCSDurationMs);
    ImGui::Separator();
    const std::vector<const char*> filterNames = {"No filter", "Blur", "Sharpen", "Desaturate", "Sobel"};

    if (ImGui::BeginCombo("Filter", filterNames[mFilterOption])) {
        for (size_t i = 0; i < filterNames.size(); ++i) {
            bool isSelected = (i == mFilterOption);
            if (ImGui::Selectable(filterNames[i], isSelected)) {
                mFilterOption = static_cast<int>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const std::vector<const char*> imageNames = {"Lights", "Chinatown", "Box", "San Francisco"};
    if (ImGui::BeginCombo("Image", imageNames[mImageOption])) {
        for (size_t i = 0; i < imageNames.size(); ++i) {
            bool isSelected = (i == mImageOption);
            if (ImGui::Selectable(imageNames[i], isSelected)) {
                mImageOption = static_cast<int>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

SETUP_APPLICATION(ProjApp)
