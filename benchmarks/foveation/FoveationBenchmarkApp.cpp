// Copyright 2024 Google LLC
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

#include "FoveationBenchmarkApp.h"

void FoveationBenchmarkApp::InitKnobs()
{
    GetKnobManager().InitKnob(&mKnobs.pRenderPattern, "render-pattern", "");
    mKnobs.pRenderPattern->SetFlagDescription("Set the foveation pattern used for rendering ('clear', 'constant', 'static-noise', 'dynamic-noise').");
    mKnobs.pRenderPattern->SetValidator([](const std::string& res) {
        return res == "" ||
               res == "clear" ||
               res == "constant" ||
               res == "static-noise" ||
               res == "dynamic-noise";
    });

    GetKnobManager().InitKnob(&mKnobs.pFoveationPattern, "foveation-pattern", "");
    mKnobs.pFoveationPattern->SetFlagDescription("Set the foveation pattern used for rendering ('1x1', '2x2', '4x4', 'radial', 'anisotropic').");
    mKnobs.pFoveationPattern->SetValidator([](const std::string& res) {
        return res == "" ||
               res == "1x1" ||
               res == "2x2" ||
               res == "4x4" ||
               res == "radial" ||
               res == "anisotropic";
    });

    GetKnobManager().InitKnob(&mKnobs.pSubsampledImage, "enable-subsampled-image", kDefaultEnableSubsampledImage);
    mKnobs.pSubsampledImage->SetFlagDescription("Enable the subsampled image flag on the render target.");

    GetKnobManager().InitKnob(&mKnobs.pRenderResolution, "render-resolution", std::make_pair(kDefaultWidth, kDefaultHeight));
    mKnobs.pRenderResolution->SetFlagDescription("Width and height of render target in pixels.");
    mKnobs.pRenderResolution->SetFlagParameters("<width>x<height>");

    GetKnobManager().InitKnob(&mKnobs.pPostResolution, "post-resolution", std::make_pair(kDefaultWidth, kDefaultHeight));
    mKnobs.pPostResolution->SetFlagDescription("Width and height of post render target in pixels.");
    mKnobs.pPostResolution->SetFlagParameters("<width>x<height>");

    GetKnobManager().InitKnob(&mKnobs.pShowFragmentSize, "show-fragment-size", kDefaultShowFragmentSize);
    mKnobs.pShowFragmentSize->SetFlagDescription("Show the fragment width and height in the G and B color channels.");

    GetKnobManager().InitKnob(&mKnobs.pRenderScreenshotPath, "render-screenshot-path", "");
    mKnobs.pRenderScreenshotPath->SetFlagDescription("Set the path to save a copy of the render image when a screenshot is triggered. By default the render target image will not be saved.");

    GetKnobManager().InitKnob(&mKnobs.pPostScreenshotPath, "post-screenshot-path", "");
    mKnobs.pPostScreenshotPath->SetFlagDescription("Set the path to save a copy of the post image when a screenshot is triggered. By default the render target image will not be saved.");

    GetKnobManager().InitKnob(&mKnobs.pExtraHashRounds, "extra-hash-rounds", 0);
    mKnobs.pExtraHashRounds->SetFlagDescription("Number of extra hash rounds to execute in the fragment shader.");
}

void FoveationBenchmarkApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                            = "foveation_benchmark";
    settings.enableImGui                        = false;
    settings.grfx.api                           = kApi;
    settings.grfx.swapchain.depthFormat         = grfx::FORMAT_D32_FLOAT;
    settings.grfx.device.supportShadingRateMode = kDefaultShadingRateMode;
}

void FoveationBenchmarkApp::SetupSync()
{
    grfx::SemaphoreCreateInfo semaCreateInfo = {};
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &mSync.imageAcquiredSemaphore));
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &mSync.renderCompleteSemaphore));
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &mSync.postCompleteSemaphore));

    grfx::FenceCreateInfo fenceCreateInfo = {};
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &mSync.imageAcquiredFence));

    fenceCreateInfo = {true}; // Create signaled
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &mSync.postCompleteFence));
}

void FoveationBenchmarkApp::SetupRender()
{
    PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&mRender.cmd));

    // Uniform buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = RoundUp<size_t>(sizeof(ShaderParams), PPX_UNIFORM_BUFFER_ALIGNMENT);
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mRender.uniformBuffer));
    }

    // Descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.uniformBuffer                  = 1;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mRender.descriptorPool));
    }

    // Descriptor set
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mRender.descriptorSetLayout));
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mRender.descriptorPool, mRender.descriptorSetLayout, &mRender.descriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mRender.uniformBuffer;
        PPX_CHECKED_CALL(mRender.descriptorSet->UpdateDescriptors(1, &write));
    }

    // Pipeline
    {
        grfx::ShaderModulePtr VS;
        grfx::ShaderModulePtr PS;
        const char*           vsMainName = "vsmain";
        const char*           psMainName = "psmain";
        const char*           vsFileName = "FoveationBenchmark.vs";
        const char*           psFileName = "FoveationBenchmark.ps";

        if (mRender.showFragmentSize) {
            if (mRender.shadingRateMode == grfx::SHADING_RATE_FDM) {
                vsMainName = "main";
                psMainName = "main";
                vsFileName = "FoveationBenchmarkFragSizeEXT.vs";
                psFileName = "FoveationBenchmarkFragSizeEXT.ps";
            }
            else if (mRender.shadingRateMode == grfx::SHADING_RATE_VRS) {
                psFileName = "FoveationBenchmarkShadingRateKHR.ps";
            }
        }
        std::vector<char>
            bytecode = LoadShader("benchmarks/shaders", vsFileName);
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        bytecode = LoadShader("benchmarks/shaders", psFileName);
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mRender.descriptorSetLayout;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mRender.pipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), vsMainName};
        gpCreateInfo.PS                                 = {PS.Get(), psMainName};
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mRender.pipelineInterface;
        gpCreateInfo.shadingRateMode                    = mRender.shadingRateMode;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mRender.pipeline));
    }

    // Foveation pattern
    if (mRender.shadingRateMode != grfx::SHADING_RATE_NONE) {
        grfx::ShadingRatePatternCreateInfo createInfo = {};

        createInfo.framebufferSize.width  = mRender.width;
        createInfo.framebufferSize.height = mRender.height;
        createInfo.shadingRateMode        = mRender.shadingRateMode;

        PPX_CHECKED_CALL(GetDevice()->CreateShadingRatePattern(&createInfo, &mRender.shadingRatePattern));

        auto bitmap = mRender.shadingRatePattern->CreateBitmap();

        switch (mRender.foveationPattern) {
            case FoveationPattern::UNIFORM_1X1:
                FillShadingRateUniformFragmentSize(mRender.shadingRatePattern, 1, 1, bitmap.get());
                break;
            case FoveationPattern::UNIFORM_2X2:
                FillShadingRateUniformFragmentSize(mRender.shadingRatePattern, 2, 2, bitmap.get());
                break;
            case FoveationPattern::UNIFORM_4X4:
                FillShadingRateUniformFragmentSize(mRender.shadingRatePattern, 4, 4, bitmap.get());
                break;
            case FoveationPattern::RADIAL:
                FillShadingRateRadial(mRender.shadingRatePattern, 3.5, bitmap.get());
                break;
            case FoveationPattern::ANISOTROPIC:
                FillShadingRateAnisotropic(mRender.shadingRatePattern, 3.5, bitmap.get());
                break;
        }

        PPX_CHECKED_CALL(mRender.shadingRatePattern->LoadFromBitmap(bitmap.get()));
    }

    // Draw pass
    {
        grfx::DrawPassCreateInfo createInfo        = {};
        createInfo.width                           = mRender.width;
        createInfo.height                          = mRender.height;
        createInfo.renderTargetCount               = 1;
        createInfo.renderTargetFormats[0]          = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat              = GetSwapchain()->GetDepthFormat();
        createInfo.renderTargetUsageFlags[0]       = grfx::IMAGE_USAGE_SAMPLED;
        grfx::RenderTargetClearValue rtvClearValue = {0, 1, 1, 1};
        grfx::DepthStencilClearValue dsvClearValue = {1.0f, 0xFF};
        createInfo.renderTargetClearValues[0]      = rtvClearValue;
        createInfo.depthStencilClearValue          = dsvClearValue;
        createInfo.pShadingRatePattern             = mRender.shadingRatePattern;
        createInfo.imageCreateFlags                = mRender.subsampledImage;
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mRender.drawPass));
    }
}

void FoveationBenchmarkApp::SetupPost()
{
    PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&mPost.cmd));

    // Sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = 0.0f;
        createInfo.createFlags             = mRender.subsampledImage;
        createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mPost.sampler));
    }

    // Create descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 1000;
        createInfo.combinedImageSampler           = 1000;
        createInfo.sampledImage                   = 1000;
        createInfo.uniformBuffer                  = 1000;
        createInfo.structuredBuffer               = 1000;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mPost.descriptorPool));
    }

    // Descriptor set layout
    {
        // Descriptor set layout
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        grfx::DescriptorBinding             binding(0, grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        binding.immutableSamplers.push_back(mPost.sampler);
        layoutCreateInfo.bindings.push_back(binding);
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mPost.descriptorSetLayout));
    }

    // Pipeline
    {
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "FullScreenTriangleCombined.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;

        bytecode = LoadShader("basic/shaders", "FullScreenTriangleCombined.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::FullscreenQuadCreateInfo createInfo = {};
        createInfo.VS                             = VS;
        createInfo.PS                             = PS;
        createInfo.setCount                       = 1;
        createInfo.sets[0].set                    = 0;
        createInfo.sets[0].pLayout                = mPost.descriptorSetLayout;
        createInfo.renderTargetCount              = 1;
        createInfo.renderTargetFormats[0]         = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat             = GetSwapchain()->GetDepthFormat();

        PPX_CHECKED_CALL(GetDevice()->CreateFullscreenQuad(&createInfo, &mPost.fullscreenQuad));
    }

    // Allocate descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mPost.descriptorPool, mPost.descriptorSetLayout, &mPost.descriptorSet));

    // Write descriptors
    {
        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageView            = mRender.drawPass->GetRenderTargetTexture(0)->GetSampledImageView();
        write.pSampler              = mPost.sampler;

        PPX_CHECKED_CALL(mPost.descriptorSet->UpdateDescriptors(1, &write));
    }

    // Draw pass
    {
        grfx::DrawPassCreateInfo createInfo        = {};
        createInfo.width                           = mPost.width;
        createInfo.height                          = mPost.height;
        createInfo.renderTargetCount               = 1;
        createInfo.renderTargetFormats[0]          = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat              = GetSwapchain()->GetDepthFormat();
        createInfo.renderTargetUsageFlags[0]       = grfx::IMAGE_USAGE_TRANSFER_SRC;
        grfx::RenderTargetClearValue rtvClearValue = {0, 1, 1, 1};
        grfx::DepthStencilClearValue dsvClearValue = {1.0f, 0xFF};
        createInfo.renderTargetClearValues[0]      = rtvClearValue;
        createInfo.depthStencilClearValue          = dsvClearValue;
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mPost.drawPass));
    }
}

void FoveationBenchmarkApp::Setup()
{
    const auto& capabilities = GetDevice()->GetShadingRateCapabilities();
    mRender.shadingRateMode  = capabilities.supportedShadingRateMode;

    std::string renderPatternString = mKnobs.pRenderPattern->GetValue();
    if (renderPatternString == "") {
        mRender.renderPattern = kDefaultRenderPattern;
    }
    else if (renderPatternString == "clear") {
        mRender.renderPattern = RenderPattern::CLEAR_ONLY;
    }
    else if (renderPatternString == "constant") {
        mRender.renderPattern = RenderPattern::CONSTANT;
    }
    else if (renderPatternString == "static-noise") {
        mRender.renderPattern = RenderPattern::STATIC_NOISE;
    }
    else if (renderPatternString == "dynamic-noise") {
        mRender.renderPattern = RenderPattern::DYNAMIC_NOISE;
    }

    std::string foveationPatternString = mKnobs.pFoveationPattern->GetValue();
    if (foveationPatternString == "") {
        mRender.foveationPattern = kDefaultFoveationPattern;
    }
    else if (foveationPatternString == "1x1") {
        mRender.foveationPattern = FoveationPattern::UNIFORM_1X1;
    }
    else if (foveationPatternString == "2x2") {
        mRender.foveationPattern = FoveationPattern::UNIFORM_2X2;
    }
    else if (foveationPatternString == "4x4") {
        mRender.foveationPattern = FoveationPattern::UNIFORM_4X4;
    }
    else if (foveationPatternString == "radial") {
        mRender.foveationPattern = FoveationPattern::RADIAL;
    }
    else if (foveationPatternString == "anisotropic") {
        mRender.foveationPattern = FoveationPattern::ANISOTROPIC;
    }

    mRender.width  = mKnobs.pRenderResolution->GetValue().first;
    mRender.height = mKnobs.pRenderResolution->GetValue().second;

    mPost.width  = mKnobs.pPostResolution->GetValue().first;
    mPost.height = mKnobs.pPostResolution->GetValue().second;

    mRender.subsampledImage = mKnobs.pSubsampledImage->GetValue();

    mRender.showFragmentSize = mKnobs.pShowFragmentSize->GetValue() && mRender.shadingRateMode != grfx::SHADING_RATE_NONE;

    mRender.extraHashRounds = mKnobs.pExtraHashRounds->GetValue();

    SetupSync();
    SetupRender();
    SetupPost();
}

void FoveationBenchmarkApp::RecordRenderCommands()
{
    // Build render command buffer
    PPX_CHECKED_CALL(mRender.cmd->Begin());
    {
        mRender.cmd->BufferResourceBarrier(
            mRender.uniformBuffer,
            ppx::grfx::RESOURCE_STATE_GENERAL,
            ppx::grfx::RESOURCE_STATE_CONSTANT_BUFFER,
            GetGraphicsQueue(),
            GetGraphicsQueue());
        mRender.cmd->BeginRenderPass(mRender.drawPass);
        if (mRender.renderPattern != RenderPattern::CLEAR_ONLY) {
            mRender.cmd->SetScissors(mRender.drawPass->GetScissor());
            mRender.cmd->SetViewports(mRender.drawPass->GetViewport());

            mRender.cmd->BindGraphicsPipeline(mRender.pipeline);
            mRender.cmd->BindGraphicsDescriptorSets(mRender.pipelineInterface, 1, &mRender.descriptorSet);
            mRender.cmd->Draw(3, 1, 0, 0);
        }
        mRender.cmd->EndRenderPass();
    }
    PPX_CHECKED_CALL(mRender.cmd->End());
}
void FoveationBenchmarkApp::UpdateRenderShaderParams()
{
    ShaderParams params    = {};
    params.seed            = 0u;
    params.noiseWeights    = float3(1.0f, 1.0f, 1.0f);
    params.color           = float3(1.0f, 0.0f, 1.0f);
    params.extraHashRounds = mRender.extraHashRounds;
    if (mRender.renderPattern == RenderPattern::DYNAMIC_NOISE) {
        params.seed = static_cast<uint32_t>(GetFrameCount());
    }
    else if (mRender.renderPattern == RenderPattern::CONSTANT) {
        params.noiseWeights = float3(0.001f, 0.001f, 0.001f);
    }

    void* pData = nullptr;
    PPX_CHECKED_CALL(mRender.uniformBuffer->MapMemory(0, &pData));
    memcpy(pData, &params, sizeof(ShaderParams));
    mRender.uniformBuffer->UnmapMemory();
}

void FoveationBenchmarkApp::RecordPostCommands(uint32_t imageIndex)
{
    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(mSync.imageAcquiredFence->WaitAndReset());

    // Build render command buffer
    PPX_CHECKED_CALL(mPost.cmd->Begin());
    {
        mPost.cmd->TransitionImageLayout(mRender.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        mPost.cmd->BeginRenderPass(mPost.drawPass);
        {
            mPost.cmd->SetScissors(mPost.drawPass->GetScissor());
            mPost.cmd->SetViewports(mPost.drawPass->GetViewport());
            mPost.cmd->Draw(mPost.fullscreenQuad, 1, &mPost.descriptorSet);
        }
        mPost.cmd->EndRenderPass();
        mPost.cmd->TransitionImageLayout(mRender.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);

        grfx::ImagePtr postImage      = mPost.drawPass->GetRenderTargetTexture(0)->GetImage();
        grfx::ImagePtr swapchainImage = GetSwapchain()->GetColorImage(imageIndex);

        mPost.cmd->TransitionImageLayout(postImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_COPY_SRC);
        mPost.cmd->TransitionImageLayout(swapchainImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_COPY_DST);

        grfx::ImageBlitInfo blitInfo = {};
        blitInfo.srcImage.offsets[1] = {postImage->GetWidth(), postImage->GetHeight(), 1};
        blitInfo.dstImage.offsets[1] = {swapchainImage->GetWidth(), swapchainImage->GetHeight(), 1};

        mPost.cmd->BlitImage(&blitInfo, postImage, swapchainImage);

        mPost.cmd->TransitionImageLayout(postImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_COPY_SRC, grfx::RESOURCE_STATE_RENDER_TARGET);
        mPost.cmd->TransitionImageLayout(swapchainImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(mPost.cmd->End());
}

void FoveationBenchmarkApp::SaveImage(grfx::ImagePtr image, const std::string& filepath, grfx::ResourceState resourceState) const
{
    auto queue = GetDevice()->GetGraphicsQueue();

    const grfx::FormatDesc* formatDesc = grfx::GetFormatDescription(image->GetFormat());
    const uint32_t          width      = image->GetWidth();
    const uint32_t          height     = image->GetHeight();

    // Create a buffer that will hold the swapchain image's texels.
    // Increase its size by a factor of 2 to ensure that a larger-than-needed
    // row pitch will not overflow the buffer.
    uint64_t bufferSize = 2ull * formatDesc->bytesPerTexel * width * height;

    grfx::BufferPtr        screenshotBuf = nullptr;
    grfx::BufferCreateInfo bufferCi      = {};
    bufferCi.size                        = bufferSize;
    bufferCi.initialState                = grfx::RESOURCE_STATE_COPY_DST;
    bufferCi.usageFlags.bits.transferDst = 1;
    bufferCi.memoryUsage                 = grfx::MEMORY_USAGE_GPU_TO_CPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCi, &screenshotBuf));

    // We wait for idle so that we can avoid tracking swapchain fences.
    // It's not ideal, but we won't be taking screenshots in
    // performance-critical scenarios.
    PPX_CHECKED_CALL(queue->WaitIdle());

    // Copy the swapchain image into the buffer.
    grfx::CommandBufferPtr cmdBuf;
    PPX_CHECKED_CALL(queue->CreateCommandBuffer(&cmdBuf, 0, 0));

    grfx::ImageToBufferOutputPitch outPitch;
    cmdBuf->Begin();
    {
        cmdBuf->TransitionImageLayout(image, PPX_ALL_SUBRESOURCES, resourceState, grfx::RESOURCE_STATE_COPY_SRC);

        grfx::ImageToBufferCopyInfo bufCopyInfo = {};
        bufCopyInfo.extent                      = {width, height, 0};
        outPitch                                = cmdBuf->CopyImageToBuffer(&bufCopyInfo, image, screenshotBuf);

        cmdBuf->TransitionImageLayout(image, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_COPY_SRC, resourceState);
    }
    cmdBuf->End();

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &cmdBuf;
    queue->Submit(&submitInfo);

    // Wait for the copy to be finished.
    queue->WaitIdle();

    // Export to PPM.
    unsigned char* texels = nullptr;
    screenshotBuf->MapMemory(0, (void**)&texels);

    PPX_CHECKED_CALL(ExportToPPM(filepath, image->GetFormat(), texels, width, height, outPitch.rowPitch));

    screenshotBuf->UnmapMemory();

    // Clean up temporary resources.
    GetDevice()->DestroyBuffer(screenshotBuf);
    queue->DestroyCommandBuffer(cmdBuf);
}

void FoveationBenchmarkApp::Render()
{
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(mSync.postCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(GetSwapchain()->AcquireNextImage(UINT64_MAX, mSync.imageAcquiredSemaphore, mSync.imageAcquiredFence, &imageIndex));

    PPX_LOG_INFO("FoveationBenchmarkApp::Render imageIndex:" << imageIndex);

    UpdateRenderShaderParams();
    RecordRenderCommands();
    RecordPostCommands(imageIndex);

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &mRender.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &mSync.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &mSync.renderCompleteSemaphore;
    submitInfo.pFence               = nullptr;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    submitInfo                      = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &mPost.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &mSync.renderCompleteSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &mSync.postCompleteSemaphore;
    submitInfo.pFence               = mSync.postCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(GetSwapchain()->Present(imageIndex, 1, &mSync.postCompleteSemaphore));

    if (GetFrameCount() == static_cast<uint64_t>(GetStandardOptions().pScreenshotFrameNumber->GetValue())) {
        if (mKnobs.pRenderScreenshotPath->GetValue().size() > 0) {
            SaveImage(mRender.drawPass->GetRenderTargetTexture(0)->GetImage(), mKnobs.pRenderScreenshotPath->GetValue());
        }
        if (mKnobs.pPostScreenshotPath->GetValue().size() > 0) {
            SaveImage(mPost.drawPass->GetRenderTargetTexture(0)->GetImage(), mKnobs.pPostScreenshotPath->GetValue());
        }
    }
}
