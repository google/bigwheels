// Copyright 2023 Google LLC
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

// TODO Several meshes on top of each other (including opaque ones)
// TODO Choice of cubemaps as background
// TODO Add WeightedAverage with depth
// TODO Add Depth peeling (basic and dual)
// TODO Add buffer-based algorithms

#include "OITDemoApplication.h"
#include "ppx/graphics_util.h"
#include "shaders/Common.hlsli"

OITDemoApp::GuiParameters::GuiParameters()
    : meshOpacity(1.0f), algorithmDataIndex(0), displayBackground(true), faceMode(FACE_MODE_ALL), weightedAverageType(WEIGHTED_AVERAGE_TYPE_FRAGMENT_COUNT)
{
    backgroundColor[0] = 0.51f;
    backgroundColor[1] = 0.71f;
    backgroundColor[2] = 0.85f;
}

void OITDemoApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName = "OIT demo";

    settings.enableImGui      = true;
    settings.grfx.enableDebug = false;

    settings.grfx.swapchain.colorFormat = grfx::FORMAT_B8G8R8A8_UNORM;

#if defined(USE_DX12)
    settings.grfx.api = grfx::API_DX_12_0;
#elif defined(USE_VK)
    settings.grfx.api = grfx::API_VK_1_1;
#else
#error
#endif
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

void OITDemoApp::SetupCommon()
{
    ////////////////////////////////////////
    // Shared
    ////////////////////////////////////////

    // Synchronization objects
    {
        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &mImageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &mImageAcquiredFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &mRenderCompleteSemaphore));

        fenceCreateInfo.signaled = true;
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &mRenderCompleteFence));
    }

    // Command buffer
    {
        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&mCommandBuffer));
    }

    // Descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 16;
        createInfo.sampledImage                   = 16;
        createInfo.uniformBuffer                  = 16;
        createInfo.structuredBuffer               = 16;
        createInfo.storageTexelBuffer             = 16;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // Sampler
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_NEAREST;
        createInfo.minFilter               = grfx::FILTER_NEAREST;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&createInfo, &mNearestSampler));
    }

    // Meshes
    {
        grfx::QueuePtr queue   = this->GetGraphicsQueue();
        TriMeshOptions options = TriMeshOptions().Indices();
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromFile(queue, this->GetAssetPath("basic/models/cube.obj"), &mBackgroundMesh, options));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromFile(queue, this->GetAssetPath("basic/models/monkey.obj"), &mMonkeyMesh, options));
    }

    // Shader globals
    {
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = std::max(sizeof(ShaderGlobals), static_cast<size_t>(PPX_MINIMUM_UNIFORM_BUFFER_SIZE));
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mShaderGlobalsBuffer));
    }

    ////////////////////////////////////////
    // Opaque
    ////////////////////////////////////////

    // Pass
    {
        grfx::DrawPassCreateInfo createInfo     = {};
        createInfo.width                        = GetSwapchain()->GetWidth();
        createInfo.height                       = GetSwapchain()->GetHeight();
        createInfo.renderTargetCount            = 1;
        createInfo.renderTargetFormats[0]       = GetSwapchain()->GetColorFormat();
        createInfo.depthStencilFormat           = grfx::FORMAT_D32_FLOAT;
        createInfo.renderTargetUsageFlags[0]    = grfx::IMAGE_USAGE_SAMPLED;
        createInfo.depthStencilUsageFlags       = grfx::IMAGE_USAGE_SAMPLED;
        createInfo.renderTargetInitialStates[0] = grfx::RESOURCE_STATE_SHADER_RESOURCE;
        createInfo.depthStencilInitialState     = grfx::RESOURCE_STATE_SHADER_RESOURCE;
        createInfo.renderTargetClearValues[0]   = {0, 0, 0, 0};
        createInfo.depthStencilClearValue       = {1.0f, 0xFF};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mOpaquePass));
    }

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mOpaqueDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mOpaqueDescriptorSetLayout, &mOpaqueDescriptorSet));

        grfx::WriteDescriptor write = {};
        write.binding               = SHADER_GLOBALS_REGISTER;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mShaderGlobalsBuffer;
        PPX_CHECKED_CALL(mOpaqueDescriptorSet->UpdateDescriptors(1, &write));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mOpaqueDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mOpaquePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "Opaque.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "Opaque.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS, "vsmain"};
        gpCreateInfo.PS                                 = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mBackgroundMesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_FRONT;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mOpaquePass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mOpaquePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mOpaquePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    ////////////////////////////////////////
    // Transparency
    ////////////////////////////////////////

    // Texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = GetSwapchain()->GetWidth();
        createInfo.height                          = GetSwapchain()->GetHeight();
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = grfx::FORMAT_R16G16B16A16_FLOAT;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.sampled         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;
        createInfo.RTVClearValue                   = {0, 0, 0, 0};
        createInfo.DSVClearValue                   = {1.0f, 0xFF};
        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mTransparencyTexture));
    }

    // Pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = GetSwapchain()->GetWidth();
        createInfo.height                     = GetSwapchain()->GetHeight();
        createInfo.renderTargetCount          = 1;
        createInfo.pRenderTargetImages[0]     = mTransparencyTexture->GetImage();
        createInfo.pDepthStencilImage         = mOpaquePass->GetDepthStencilTexture()->GetImage();
        createInfo.depthStencilState          = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        createInfo.depthStencilClearValue     = {1.0f, 0xFF};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mTransparencyPass));
    }

    ////////////////////////////////////////
    // Composite
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{NEAREST_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{OPAQUE_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{TRANSPARENCY_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mCompositeDescriptorSetLayout));
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mCompositeDescriptorSetLayout, &mCompositeDescriptorSet));

        grfx::WriteDescriptor writes[3] = {};

        writes[0].binding  = NEAREST_SAMPLER_REGISTER;
        writes[0].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[0].pSampler = mNearestSampler;

        writes[1].binding    = OPAQUE_TEXTURE_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mOpaquePass->GetRenderTargetTexture(0)->GetSampledImageView();

        writes[2].binding    = TRANSPARENCY_TEXTURE_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2].pImageView = mTransparencyTexture->GetSampledImageView();

        PPX_CHECKED_CALL(mCompositeDescriptorSet->UpdateDescriptors(3, writes));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mCompositeDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mCompositePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "Composite.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "Composite.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS, "vsmain"};
        gpCreateInfo.PS                                 = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 0;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mCompositePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mCompositePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::AddSupportedAlgorithm(const char* name, Algorithm algorithm)
{
    mSupportedAlgorithmNames.push_back(name);
    mSupportedAlgorithmIds.push_back(algorithm);
    PPX_ASSERT_MSG(mSupportedAlgorithmNames.size() == mSupportedAlgorithmIds.size(), "supported algorithm data is out-of-sync");
}

OITDemoApp::Algorithm OITDemoApp::GetSelectedAlgorithm() const
{
    return mSupportedAlgorithmIds[mGuiParameters.algorithmDataIndex];
}

void OITDemoApp::FillSupportedAlgorithmData()
{
    AddSupportedAlgorithm("Unsorted over", ALGORITHM_UNSORTED_OVER);
    AddSupportedAlgorithm("Weighted sum", ALGORITHM_WEIGHTED_SUM);
    if (GetDevice()->IndependentBlendingSupported()) {
        AddSupportedAlgorithm("Weighted average", ALGORITHM_WEIGHTED_AVERAGE);
    }
}

void OITDemoApp::SetDefaultAlgorithmIndex(Algorithm defaultAlgorithm)
{
    for (size_t i = 0; i < mSupportedAlgorithmIds.size(); ++i) {
        if (mSupportedAlgorithmIds[i] == defaultAlgorithm) {
            mGuiParameters.algorithmDataIndex = static_cast<int32_t>(i);
            break;
        }
    }
}

void OITDemoApp::Setup()
{
    SetupCommon();
    FillSupportedAlgorithmData();

    {
        const CliOptions& cliOptions       = GetExtraOptions();
        const Algorithm   defaultAlgorithm = static_cast<Algorithm>(cliOptions.GetExtraOptionValueOrDefault("algorithm", static_cast<int32_t>(ALGORITHM_UNSORTED_OVER)));
        SetDefaultAlgorithmIndex(defaultAlgorithm);
    }

    SetupUnsortedOver();
    SetupWeightedSum();
    SetupWeightedAverage();
}

void OITDemoApp::Update()
{
    const float time = GetElapsedSeconds();

    // Shader globals
    {
        const float4x4 VP =
            glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f) *
            glm::lookAt(float3(0, 0, 8), float3(0, 0, 0), float3(0, 1, 0));

        ShaderGlobals shaderGlobals = {};
        {
            const float4x4 M            = glm::scale(float3(20.0));
            shaderGlobals.backgroundMVP = VP * M;

            shaderGlobals.backgroundColor.r = mGuiParameters.backgroundColor[0];
            shaderGlobals.backgroundColor.g = mGuiParameters.backgroundColor[1];
            shaderGlobals.backgroundColor.b = mGuiParameters.backgroundColor[2];
            shaderGlobals.backgroundColor.a = 1.0f;
        }
        {
            const float4x4 M =
                glm::rotate(time, float3(0, 0, 1)) *
                glm::rotate(2 * time, float3(0, 1, 0)) *
                glm::rotate(time, float3(1, 0, 0)) *
                glm::scale(float3(2));
            shaderGlobals.meshMVP = VP * M;
        }
        shaderGlobals.meshOpacity = mGuiParameters.meshOpacity;
        mShaderGlobalsBuffer->CopyFromSource(sizeof(shaderGlobals), &shaderGlobals);
    }

    UpdateGUI();
}

void OITDemoApp::UpdateGUI()
{
    if (!GetSettings()->enableImGui) {
        return;
    }

    // GUI
    if (ImGui::Begin("Parameters")) {
        ImGui::Combo("Algorithm", &mGuiParameters.algorithmDataIndex, mSupportedAlgorithmNames.data(), static_cast<int>(mSupportedAlgorithmNames.size()));

        ImGui::SliderFloat("Opacity", &mGuiParameters.meshOpacity, 0.0f, 1.0f, "%.2f");
        ImGui::Checkbox("Display background", &mGuiParameters.displayBackground);
        if (mGuiParameters.displayBackground) {
            ImGui::ColorPicker3(
                "Background color", mGuiParameters.backgroundColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB);
        }

        ImGui::Separator();

        switch (GetSelectedAlgorithm()) {
            case ALGORITHM_UNSORTED_OVER: {
                const char* faceModeChoices[] =
                    {
                        "All",
                        "Back first, then front",
                        "Back only",
                        "Front only",
                    };
                static_assert(IM_ARRAYSIZE(faceModeChoices) == FACE_MODES_COUNT, "Face modes count mismatch");
                ImGui::Combo("Face draw mode", reinterpret_cast<int32_t*>(&mGuiParameters.faceMode), faceModeChoices, IM_ARRAYSIZE(faceModeChoices));
                break;
            }
            case ALGORITHM_WEIGHTED_AVERAGE: {
                const char* typeChoices[] =
                    {
                        "Fragment count",
                        "Exact coverage",
                    };
                static_assert(IM_ARRAYSIZE(typeChoices) == WEIGHTED_AVERAGE_TYPES_COUNT, "Weighted average types count mismatch");
                ImGui::Combo("Type", reinterpret_cast<int32_t*>(&mGuiParameters.weightedAverageType), typeChoices, IM_ARRAYSIZE(typeChoices));
                break;
            }
            default: {
                break;
            }
        }
    }
    ImGui::End();
}

void OITDemoApp::RecordOpaque()
{
    mCommandBuffer->TransitionImageLayout(
        mOpaquePass,
        grfx::RESOURCE_STATE_SHADER_RESOURCE,
        grfx::RESOURCE_STATE_RENDER_TARGET,
        grfx::RESOURCE_STATE_SHADER_RESOURCE,
        grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
    mCommandBuffer->BeginRenderPass(mOpaquePass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

    mCommandBuffer->SetScissors(mOpaquePass->GetScissor());
    mCommandBuffer->SetViewports(mOpaquePass->GetViewport());

    if (mGuiParameters.displayBackground) {
        mCommandBuffer->BindGraphicsDescriptorSets(mOpaquePipelineInterface, 1, &mOpaqueDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mOpaquePipeline);
        mCommandBuffer->BindIndexBuffer(mBackgroundMesh);
        mCommandBuffer->BindVertexBuffers(mBackgroundMesh);
        mCommandBuffer->DrawIndexed(mBackgroundMesh->GetIndexCount());
    }

    mCommandBuffer->EndRenderPass();
    mCommandBuffer->TransitionImageLayout(
        mOpaquePass,
        grfx::RESOURCE_STATE_RENDER_TARGET,
        grfx::RESOURCE_STATE_SHADER_RESOURCE,
        grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
        grfx::RESOURCE_STATE_SHADER_RESOURCE);
}

void OITDemoApp::RecordTransparency()
{
    switch (GetSelectedAlgorithm()) {
        case ALGORITHM_UNSORTED_OVER:
            RecordUnsortedOver();
            break;
        case ALGORITHM_WEIGHTED_SUM:
            RecordWeightedSum();
            break;
        case ALGORITHM_WEIGHTED_AVERAGE:
            RecordWeightedAverage();
            break;
        default:
            PPX_ASSERT_MSG(false, "unknown algorithm");
            break;
    }
}

void OITDemoApp::RecordComposite(grfx::RenderPassPtr renderPass)
{
    PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

    mCommandBuffer->TransitionImageLayout(
        renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);

    grfx::RenderPassBeginInfo beginInfo = {};
    beginInfo.pRenderPass               = renderPass;
    beginInfo.renderArea                = renderPass->GetRenderArea();
    beginInfo.RTVClearCount             = 1;
    beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
    beginInfo.DSVClearValue             = {1.0f, 0xFF};
    mCommandBuffer->BeginRenderPass(&beginInfo);

    mCommandBuffer->SetScissors(renderPass->GetScissor());
    mCommandBuffer->SetViewports(renderPass->GetViewport());

    mCommandBuffer->BindGraphicsDescriptorSets(mCompositePipelineInterface, 1, &mCompositeDescriptorSet);
    mCommandBuffer->BindGraphicsPipeline(mCompositePipeline);
    mCommandBuffer->Draw(3);

    DrawImGui(mCommandBuffer);

    mCommandBuffer->EndRenderPass();
    mCommandBuffer->TransitionImageLayout(
        renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
}

void OITDemoApp::Render()
{
    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(GetSwapchain()->AcquireNextImage(UINT64_MAX, mImageAcquiredSemaphore, mImageAcquiredFence, &imageIndex));
    PPX_CHECKED_CALL(mImageAcquiredFence->WaitAndReset());
    PPX_CHECKED_CALL(mRenderCompleteFence->WaitAndReset());

    // Update state
    Update();

    // Record command buffer
    PPX_CHECKED_CALL(mCommandBuffer->Begin());
    RecordOpaque();
    RecordTransparency();
    RecordComposite(GetSwapchain()->GetRenderPass(imageIndex));
    PPX_CHECKED_CALL(mCommandBuffer->End());

    // Submit and present
    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &mCommandBuffer;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &mImageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &mRenderCompleteSemaphore;
    submitInfo.pFence               = mRenderCompleteFence;
    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    PPX_CHECKED_CALL(GetSwapchain()->Present(imageIndex, 1, &mRenderCompleteSemaphore));
}
