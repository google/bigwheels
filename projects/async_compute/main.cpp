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

#include "ppx/ppx.h"
#include "ppx/camera.h"
#include "ppx/graphics_util.h"

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
    void SetupComposition();
    void SetupCompute();
    void SetupDrawToSwapchain();
    void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;

    struct PerFrame
    {
        grfx::SemaphorePtr imageAcquiredSemaphore;
        grfx::FencePtr     imageAcquiredFence;
        grfx::SemaphorePtr renderCompleteSemaphore;
        grfx::FencePtr     renderCompleteFence;

        // Graphics pipeline objects.
        struct RenderData
        {
            grfx::CommandBufferPtr cmd;
            grfx::DescriptorSetPtr descriptorSet;
            grfx::BufferPtr        constants;
            grfx::DrawPassPtr      drawPass;
            grfx::SemaphorePtr     completeSemaphore;
        };

        std::array<RenderData, 4> renderData;

        // Compute pipeline objects.
        struct ComputeData
        {
            grfx::CommandBufferPtr    cmd;
            grfx::DescriptorSetPtr    descriptorSet;
            grfx::BufferPtr           constants;
            grfx::ImagePtr            outputImage;
            grfx::SampledImageViewPtr outputImageSampledView;
            grfx::StorageImageViewPtr outputImageStorageView;
            grfx::SemaphorePtr        completeSemaphore;
        };
        std::array<ComputeData, 4> computeData;

        // Final image composition objects.
        struct ComposeData
        {
            grfx::CommandBufferPtr cmd;
            grfx::DescriptorSetPtr descriptorSet;
            grfx::BufferPtr        quadVertexBuffer;
            grfx::SemaphorePtr     completeSemaphore;
        };
        std::array<ComposeData, 4> composeData;
        grfx::DrawPassPtr          composeDrawPass;

        // Draw to swapchain objects.
        struct DrawToSwapchainData
        {
            grfx::CommandBufferPtr cmd;
            grfx::DescriptorSetPtr descriptorSet;
        };
        DrawToSwapchainData drawToSwapchainData;
    };
    std::vector<PerFrame> mPerFrame;
    const uint32_t        mNumFramesInFlight = 2;

    void     UpdateTransforms(PerFrame& frame);
    uint32_t AcquireFrame(PerFrame& frame);
    void     BlitAndPresent(PerFrame& frame, uint32_t swapchainImageIndex);
    void     RunCompute(PerFrame& frame, size_t quadIndex);
    void     Compose(PerFrame& frame, size_t quadIndex);
    void     DrawScene(PerFrame& frame, size_t quadIndex);

    PerspCamera mCamera;

    grfx::MeshPtr    mModelMesh;
    grfx::TexturePtr mModelTexture;
    float            mModelRotation       = 45.0f;
    float            mModelTargetRotation = 45.0f;

    int mGraphicsLoad = 150;
    int mComputeLoad  = 5;

    grfx::SamplerPtr mLinearSampler;
    grfx::SamplerPtr mNearestSampler;

    // This will be a compute queue if async compute is enabled,
    // or a graphics queue otherwise.
    grfx::QueuePtr mComputeQueue;
    grfx::QueuePtr mGraphicsQueue;

    grfx::DescriptorPoolPtr mDescriptorPool;

    grfx::DescriptorSetLayoutPtr mRenderLayout;
    grfx::GraphicsPipelinePtr    mRenderPipeline;
    grfx::PipelineInterfacePtr   mRenderPipelineInterface;

    grfx::DescriptorSetLayoutPtr mComputeLayout;
    grfx::ComputePipelinePtr     mComputePipeline;
    grfx::PipelineInterfacePtr   mComputePipelineInterface;

    grfx::DescriptorSetLayoutPtr mComposeLayout;
    grfx::GraphicsPipelinePtr    mComposePipeline;
    grfx::PipelineInterfacePtr   mComposePipelineInterface;
    grfx::VertexBinding          mComposeVertexBinding;

    grfx::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
    grfx::FullscreenQuadPtr      mDrawToSwapchainPipeline;

    bool mAsyncComputeEnabled     = true;
    bool mUseQueueFamilyTransfers = true;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                       = "async_compute";
    settings.enableImGui                   = true;
    settings.grfx.api                      = kApi;
    settings.grfx.enableDebug              = false;
    settings.window.width                  = 1920;
    settings.window.height                 = 1080;
    settings.grfx.swapchain.imageCount     = mNumFramesInFlight;
    settings.grfx.device.computeQueueCount = 1;
    settings.grfx.numFramesInFlight        = mNumFramesInFlight;
}

void ProjApp::Setup()
{
    auto cl_options = GetExtraOptions();

    // Whether async compute is used or not.
    mAsyncComputeEnabled = cl_options.GetExtraOptionValueOrDefault<bool>("enable-async-compute", true);

    // Whether to use queue family transfers in Vulkan (not required in DX12).
    mUseQueueFamilyTransfers = cl_options.GetExtraOptionValueOrDefault<bool>("use-queue-family-transfers", true);

    mCamera = PerspCamera(60.0f, GetWindowAspect());

    mGraphicsQueue = GetGraphicsQueue();
    mComputeQueue  = mAsyncComputeEnabled ? GetComputeQueue() : mGraphicsQueue;

    // Per frame data
    for (uint32_t i = 0; i < mNumFramesInFlight; ++i) {
        PerFrame                  frame          = {};
        grfx::SemaphoreCreateInfo semaCreateInfo = {};

        for (uint32_t d = 0; d < frame.renderData.size(); ++d) {
            PPX_CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.renderData[d].cmd));
            PPX_CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.composeData[d].cmd));
            PPX_CHECKED_CALL(mComputeQueue->CreateCommandBuffer(&frame.computeData[d].cmd));

            PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderData[d].completeSemaphore));
            PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.computeData[d].completeSemaphore));
            PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.composeData[d].completeSemaphore));
        }

        // Use the graphics queue for drawing to the swapchain.
        PPX_CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.drawToSwapchainData.cmd));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));
        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

        mPerFrame.push_back(frame);
    }

    // Descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 200;
        createInfo.sampledImage                   = 200;
        createInfo.uniformBuffer                  = 200;
        createInfo.storageImage                   = 200;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // Descriptor layout for graphics pipeline (Texture.hlsl)
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mRenderLayout));
    }

    // Mesh
    {
        Geometry geo;
        TriMesh  mesh = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/altimeter/altimeter.obj"), TriMeshOptions().Indices().TexCoords().Scale(float3(1.5f)));
        PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mModelMesh));
    }

    // Texture.
    {
        grfx_util::TextureOptions options = grfx_util::TextureOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("materials/textures/altimeter/albedo.jpg"), &mModelTexture, options));
    }

    // Samplers.
    {
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0.0f;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mLinearSampler));

        samplerCreateInfo.magFilter  = grfx::FILTER_NEAREST;
        samplerCreateInfo.minFilter  = grfx::FILTER_NEAREST;
        samplerCreateInfo.mipmapMode = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mNearestSampler));
    }

    // Pipeline for graphics rendering.
    {
        grfx::ShaderModulePtr VS;
        grfx::ShaderModulePtr PS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "Texture.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        bytecode = LoadShader("basic/shaders", "Texture.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mRenderLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mRenderPipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 2;
        gpCreateInfo.vertexInputState.bindings[0]       = mModelMesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mModelMesh->GetDerivedVertexBindings()[1];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = grfx::FORMAT_D32_FLOAT;
        gpCreateInfo.pPipelineInterface                 = mRenderPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mRenderPipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    for (auto& frameData : mPerFrame) {
        for (auto& renderData : frameData.renderData) {
            // Descriptor set.
            {
                PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mRenderLayout, &renderData.descriptorSet));
            }
            {
                grfx::WriteDescriptor write = {};
                write.binding               = 1;
                write.arrayIndex            = 0;
                write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                write.pImageView            = mModelTexture->GetSampledImageView();

                PPX_CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
            }

            // Sampler.
            {
                grfx::WriteDescriptor write = {};
                write.binding               = 2;
                write.arrayIndex            = 0;
                write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
                write.pSampler              = mLinearSampler;

                PPX_CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
            }

            // Uniform buffer (contains transformation matrix).
            {
                grfx::BufferCreateInfo bufferCreateInfo        = {};
                bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
                bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
                bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

                PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &renderData.constants));

                grfx::WriteDescriptor write = {};
                write.binding               = 0;
                write.arrayIndex            = 0;
                write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.bufferOffset          = 0;
                write.bufferRange           = PPX_WHOLE_SIZE;
                write.pBuffer               = renderData.constants;

                PPX_CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
            }

            // Graphics render pass
            {
                grfx::DrawPassCreateInfo dpCreateInfo     = {};
                dpCreateInfo.width                        = GetSwapchain()->GetWidth();
                dpCreateInfo.height                       = GetSwapchain()->GetHeight();
                dpCreateInfo.depthStencilFormat           = grfx::FORMAT_D32_FLOAT;
                dpCreateInfo.depthStencilClearValue       = {1.0f, 0};
                dpCreateInfo.depthStencilInitialState     = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
                dpCreateInfo.renderTargetCount            = 1;
                dpCreateInfo.renderTargetFormats[0]       = GetSwapchain()->GetColorFormat();
                dpCreateInfo.renderTargetClearValues[0]   = {100.0f / 255, 149.0f / 255, 237.0f / 255, 1.0f};
                dpCreateInfo.renderTargetInitialStates[0] = grfx::RESOURCE_STATE_SHADER_RESOURCE;
                dpCreateInfo.renderTargetUsageFlags[0]    = grfx::IMAGE_USAGE_SAMPLED;

                PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&dpCreateInfo, &renderData.drawPass));
            }
        }
    }

    SetupCompute();
    SetupComposition();
    SetupDrawToSwapchain();

    mCamera.LookAt(float3(0, 2, 7), float3(0, 0, 0));
}

void ProjApp::SetupCompute()
{
    // Descriptor layout for compute pipeline (ImageFilter.hlsl)
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(3, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mComputeLayout));
    }

    // Compute pipeline
    {
        grfx::ShaderModulePtr CS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "ImageFilter.cs");
        PPX_ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &CS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mComputeLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mComputePipelineInterface));

        grfx::ComputePipelineCreateInfo cpCreateInfo = {};
        cpCreateInfo.CS                              = {CS.Get(), "csmain"};
        cpCreateInfo.pPipelineInterface              = mComputePipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateComputePipeline(&cpCreateInfo, &mComputePipeline));

        GetDevice()->DestroyShaderModule(CS);
    }

    for (auto& frameData : mPerFrame) {
        for (size_t i = 0; i < frameData.computeData.size(); ++i) {
            PerFrame::ComputeData& computeData   = frameData.computeData[i];
            grfx::Texture*         sourceTexture = frameData.renderData[i].drawPass->GetRenderTargetTexture(0);

            // Descriptor set.
            {
                PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mComputeLayout, &computeData.descriptorSet));
            }

            // Output image and views.
            {
                grfx::ImageCreateInfo ci   = {};
                ci.type                    = grfx::IMAGE_TYPE_2D;
                ci.width                   = sourceTexture->GetWidth();
                ci.height                  = sourceTexture->GetHeight();
                ci.depth                   = 1;
                ci.format                  = GetSwapchain()->GetColorFormat();
                ci.usageFlags.bits.sampled = true;
                ci.usageFlags.bits.storage = true;
                ci.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
                ci.initialState            = grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

                PPX_CHECKED_CALL(GetDevice()->CreateImage(&ci, &computeData.outputImage));

                grfx::SampledImageViewCreateInfo sampledViewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(computeData.outputImage);
                PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sampledViewCreateInfo, &computeData.outputImageSampledView));

                grfx::StorageImageViewCreateInfo storageViewCreateInfo = grfx::StorageImageViewCreateInfo::GuessFromImage(computeData.outputImage);
                PPX_CHECKED_CALL(GetDevice()->CreateStorageImageView(&storageViewCreateInfo, &computeData.outputImageStorageView));
            }

            // Uniform buffer (contains filter selection flag).
            {
                grfx::BufferCreateInfo bufferCreateInfo        = {};
                bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
                bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
                bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;

                PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &computeData.constants));

                struct alignas(16) ParamsData
                {
                    float2 texel_size;
                    int    filter;
                };
                ParamsData params;

                params.texel_size.x = 1.0f / sourceTexture->GetWidth();
                params.texel_size.y = 1.0f / sourceTexture->GetHeight();
                params.filter       = static_cast<int>(i + 1); // Apply a different filter to each quad.

                void* pMappedAddress = nullptr;
                PPX_CHECKED_CALL(computeData.constants->MapMemory(0, &pMappedAddress));
                memcpy(pMappedAddress, &params, sizeof(ParamsData));
                computeData.constants->UnmapMemory();
            }

            // Descriptors.
            {
                grfx::WriteDescriptor write = {};
                write.binding               = 0;
                write.arrayIndex            = 0;
                write.type                  = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
                write.pImageView            = computeData.outputImageStorageView;
                PPX_CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

                write              = {};
                write.binding      = 1;
                write.arrayIndex   = 0;
                write.type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.bufferOffset = 0;
                write.bufferRange  = PPX_WHOLE_SIZE;
                write.pBuffer      = computeData.constants;

                PPX_CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

                write            = {};
                write.binding    = 2;
                write.arrayIndex = 0;
                write.type       = grfx::DESCRIPTOR_TYPE_SAMPLER;
                write.pSampler   = mNearestSampler;
                PPX_CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

                write            = {};
                write.binding    = 3;
                write.arrayIndex = 0;
                write.type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                write.pImageView = sourceTexture->GetSampledImageView();
                PPX_CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));
            }
        }
    }
}

void ProjApp::SetupComposition()
{
    // Descriptor set layout
    {
        // Descriptor set layout
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLER));

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mComposeLayout));
    }

    // Pipeline
    {
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "StaticTexture.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;

        bytecode = LoadShader("basic/shaders", "StaticTexture.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mComposeLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mComposePipelineInterface));

        mComposeVertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32A32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        mComposeVertexBinding.AppendAttribute({"TEXCOORD", 1, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mComposeVertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = grfx::FORMAT_D32_FLOAT;
        gpCreateInfo.pPipelineInterface                 = mComposePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mComposePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    for (auto& frameData : mPerFrame) {
        // Graphics render pass
        {
            grfx::DrawPassCreateInfo dpCreateInfo     = {};
            dpCreateInfo.width                        = GetSwapchain()->GetWidth();
            dpCreateInfo.height                       = GetSwapchain()->GetHeight();
            dpCreateInfo.depthStencilFormat           = grfx::FORMAT_D32_FLOAT;
            dpCreateInfo.renderTargetCount            = 1;
            dpCreateInfo.renderTargetFormats[0]       = GetSwapchain()->GetColorFormat();
            dpCreateInfo.renderTargetClearValues[0]   = {0.0f, 0.0f, 0.0f, 0.0f};
            dpCreateInfo.renderTargetInitialStates[0] = grfx::RESOURCE_STATE_RENDER_TARGET;
            dpCreateInfo.renderTargetUsageFlags[0]    = grfx::IMAGE_USAGE_SAMPLED;

            PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&dpCreateInfo, &frameData.composeDrawPass));
        }

        for (size_t i = 0; i < frameData.composeData.size(); ++i) {
            PerFrame::ComposeData& composeData = frameData.composeData[i];
            PerFrame::ComputeData& computeData = frameData.computeData[i];

            // Descriptor set.
            {
                PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mComposeLayout, &composeData.descriptorSet));
            }

            // Quad vertex buffer.
            {
                // Split the screen into four quads.
                float offsetX = i < 2 ? 0.0f : 1.0f;
                float offsetY = i % 2 ? 0.0f : -1.0f;

                // clang-format off
                std::vector<float> vertexData = {  
                     // Position.                                    // Texture coordinates.
                     offsetX +  0.0f,  offsetY + 1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
                     offsetX + -1.0f,  offsetY + 1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
                     offsetX + -1.0f,  offsetY + 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

                     offsetX + -1.0f,  offsetY + 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
                     offsetX +  0.0f,  offsetY + 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
                     offsetX +  0.0f,  offsetY + 1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
                };
                // clang-format on

                uint32_t dataSize = ppx::SizeInBytesU32(vertexData);

                grfx::BufferCreateInfo bufferCreateInfo       = {};
                bufferCreateInfo.size                         = dataSize;
                bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
                bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;

                PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &composeData.quadVertexBuffer));

                void* pAddr = nullptr;
                PPX_CHECKED_CALL(composeData.quadVertexBuffer->MapMemory(0, &pAddr));
                memcpy(pAddr, vertexData.data(), dataSize);
                composeData.quadVertexBuffer->UnmapMemory();
            }

            // Descriptors.
            {
                grfx::WriteDescriptor writes[2] = {};
                writes[0].binding               = 0;
                writes[0].arrayIndex            = 0;
                writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;

                writes[1].binding  = 1;
                writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
                writes[1].pSampler = mLinearSampler;

                writes[0].pImageView = computeData.outputImageSampledView;
                PPX_CHECKED_CALL(composeData.descriptorSet->UpdateDescriptors(2, writes));
            }
        }
    }
}

void ProjApp::SetupDrawToSwapchain()
{
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

        std::vector<char> bytecode = LoadShader("basic/shaders", "FullScreenTriangle.vs");
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

        PPX_CHECKED_CALL(GetDevice()->CreateFullscreenQuad(&createInfo, &mDrawToSwapchainPipeline));
    }

    // Allocate descriptor set
    for (auto& frameData : mPerFrame) {
        PerFrame::DrawToSwapchainData& drawData = frameData.drawToSwapchainData;
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDrawToSwapchainLayout, &drawData.descriptorSet));

        // Write descriptors
        {
            grfx::WriteDescriptor writes[2] = {};
            writes[0].binding               = 0;
            writes[0].arrayIndex            = 0;
            writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[0].pImageView            = frameData.composeDrawPass->GetRenderTargetTexture(0)->GetSampledImageView();

            writes[1].binding  = 1;
            writes[1].type     = grfx::DESCRIPTOR_TYPE_SAMPLER;
            writes[1].pSampler = mLinearSampler;

            PPX_CHECKED_CALL(drawData.descriptorSet->UpdateDescriptors(2, writes));
        }
    }
}

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    if (buttons & ppx::MOUSE_BUTTON_LEFT) {
        mModelTargetRotation += 0.25f * dx;
    }
}

void ProjApp::UpdateTransforms(PerFrame& frame)
{
    for (PerFrame::RenderData& renderData : frame.renderData) {
        grfx::BufferPtr buf = renderData.constants;

        mModelRotation += (mModelTargetRotation - mModelRotation) * 0.1f;

        void* pMappedAddress = nullptr;
        PPX_CHECKED_CALL(buf->MapMemory(0, &pMappedAddress));

        const float4x4& PV  = mCamera.GetViewProjectionMatrix();
        float4x4        M   = glm::rotate(glm::radians(mModelRotation + 180.0f), float3(0, 1, 0));
        float4x4        mat = PV * M;
        memcpy(pMappedAddress, &mat, sizeof(mat));

        buf->UnmapMemory();
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[GetInFlightFrameIndex()];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = AcquireFrame(frame);

    UpdateTransforms(frame);

    for (size_t quadIndex = 0; quadIndex < 4; ++quadIndex) {
        DrawScene(frame, quadIndex);
        RunCompute(frame, quadIndex);
    }

    // We have to record all composition command buffers after we
    // have recorded rendering and compute first.
    // This is because we are using a single logical graphics queue,
    // and due to DX12 requirements on command list execution order,
    // recording composition commands along rendering and compute
    // would preclude async compute from being possible.
    for (size_t quadIndex = 0; quadIndex < 4; ++quadIndex) {
        Compose(frame, quadIndex);
    }

    BlitAndPresent(frame, imageIndex);
}

uint32_t ProjApp::AcquireFrame(PerFrame& frame)
{
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(GetSwapchain()->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    return imageIndex;
}

void ProjApp::DrawScene(PerFrame& frame, size_t quadIndex)
{
    PerFrame::RenderData& renderData = frame.renderData[quadIndex];
    PPX_CHECKED_CALL(renderData.cmd->Begin());
    {
        renderData.cmd->SetScissors(renderData.drawPass->GetScissor());
        renderData.cmd->SetViewports(renderData.drawPass->GetViewport());

        // Draw model.
        renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);
        renderData.cmd->BeginRenderPass(renderData.drawPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS | grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH);
        {
            grfx::DescriptorSet* sets[1] = {nullptr};
            sets[0]                      = renderData.descriptorSet;
            renderData.cmd->BindGraphicsDescriptorSets(mRenderPipelineInterface, 1, sets);

            renderData.cmd->BindGraphicsPipeline(mRenderPipeline);

            renderData.cmd->BindIndexBuffer(mModelMesh);
            renderData.cmd->BindVertexBuffers(mModelMesh);
            renderData.cmd->DrawIndexed(mModelMesh->GetIndexCount(), mGraphicsLoad);
        }
        renderData.cmd->EndRenderPass();
        renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_SHADER_RESOURCE);

        // Release from graphics queue to compute queue.
        if (mUseQueueFamilyTransfers) {
            renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_SHADER_RESOURCE, mGraphicsQueue, mComputeQueue);
        }
    }
    PPX_CHECKED_CALL(renderData.cmd->End());

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &renderData.cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &renderData.completeSemaphore;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
}

void ProjApp::RunCompute(PerFrame& frame, size_t quadIndex)
{
    PerFrame::ComputeData& computeData = frame.computeData[quadIndex];
    PerFrame::RenderData&  renderData  = frame.renderData[quadIndex];

    PPX_CHECKED_CALL(computeData.cmd->Begin());
    {
        // Acquire from graphics queue to compute queue.
        if (mUseQueueFamilyTransfers) {
            computeData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_SHADER_RESOURCE, mGraphicsQueue, mComputeQueue);
        }

        computeData.cmd->TransitionImageLayout(computeData.outputImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_UNORDERED_ACCESS);
        {
            grfx::DescriptorSet* sets[1] = {nullptr};
            sets[0]                      = computeData.descriptorSet;
            computeData.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, sets);
            computeData.cmd->BindComputePipeline(mComputePipeline);
            uint32_t dispatchX = static_cast<uint32_t>(std::ceil(computeData.outputImage->GetWidth() / 32.0));
            uint32_t dispatchY = static_cast<uint32_t>(std::ceil(computeData.outputImage->GetHeight() / 32.0));
            for (int i = 0; i < mComputeLoad; ++i)
                computeData.cmd->Dispatch(dispatchX, dispatchY, 1);
        }
        computeData.cmd->TransitionImageLayout(computeData.outputImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_UNORDERED_ACCESS, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        // Release from compute queue to graphics queue.
        if (mUseQueueFamilyTransfers) {
            computeData.cmd->TransitionImageLayout(computeData.outputImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, mComputeQueue, mGraphicsQueue);
        }
    }
    PPX_CHECKED_CALL(computeData.cmd->End());

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &computeData.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &renderData.completeSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &computeData.completeSemaphore;

    if (mAsyncComputeEnabled) {
        PPX_CHECKED_CALL(GetComputeQueue()->Submit(&submitInfo));
    }
    else {
        PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
    }
}

void ProjApp::Compose(PerFrame& frame, size_t quadIndex)
{
    PerFrame::ComposeData& composeData = frame.composeData[quadIndex];
    PerFrame::ComputeData& computeData = frame.computeData[quadIndex];

    PPX_CHECKED_CALL(composeData.cmd->Begin());
    {
        grfx::DrawPassPtr renderPass = frame.composeDrawPass;

        composeData.cmd->SetScissors(renderPass->GetScissor());
        composeData.cmd->SetViewports(renderPass->GetViewport());

        // Acquire from compute queue to graphics queue.
        if (mUseQueueFamilyTransfers) {
            composeData.cmd->TransitionImageLayout(computeData.outputImage, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, mComputeQueue, mGraphicsQueue);
        }

        composeData.cmd->BeginRenderPass(renderPass, 0 /* do not clear render target */);
        {
            grfx::DescriptorSet* sets[1] = {nullptr};
            sets[0]                      = composeData.descriptorSet;
            composeData.cmd->BindGraphicsDescriptorSets(mComposePipelineInterface, 1, sets);

            composeData.cmd->BindGraphicsPipeline(mComposePipeline);

            composeData.cmd->BindVertexBuffers(1, &composeData.quadVertexBuffer, &mComposeVertexBinding.GetStride());
            composeData.cmd->Draw(6);
        }
        composeData.cmd->EndRenderPass();
    }
    PPX_CHECKED_CALL(composeData.cmd->End());

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &composeData.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &computeData.completeSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &composeData.completeSemaphore;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));
}

void ProjApp::BlitAndPresent(PerFrame& frame, uint32_t swapchainImageIndex)
{
    grfx::RenderPassPtr renderPass = GetSwapchain()->GetRenderPass(swapchainImageIndex);
    PPX_ASSERT_MSG(!renderPass.IsNull(), "swapchain render pass object is null");

    grfx::CommandBufferPtr cmd = frame.drawToSwapchainData.cmd;

    PPX_CHECKED_CALL(cmd->Begin());
    {
        cmd->SetScissors(renderPass->GetScissor());
        cmd->SetViewports(renderPass->GetViewport());
        cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        cmd->TransitionImageLayout(frame.composeDrawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmd->BeginRenderPass(renderPass);
        {
            // Draw composed image to swapchain.
            cmd->Draw(mDrawToSwapchainPipeline, 1, &frame.drawToSwapchainData.descriptorSet);

            // Draw ImGui.
            DrawDebugInfo();
            DrawImGui(cmd);
        }
        cmd->EndRenderPass();
        cmd->TransitionImageLayout(frame.composeDrawPass->GetRenderTargetTexture(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);
        cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
    }
    PPX_CHECKED_CALL(cmd->End());

    const grfx::Semaphore* ppWaitSemaphores[] = {
        frame.composeData[0].completeSemaphore,
        frame.composeData[1].completeSemaphore,
        frame.composeData[2].completeSemaphore,
        frame.composeData[3].completeSemaphore,
        frame.imageAcquiredSemaphore};

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &cmd;
    submitInfo.waitSemaphoreCount   = sizeof(ppWaitSemaphores) / sizeof(ppWaitSemaphores[0]);
    submitInfo.ppWaitSemaphores     = ppWaitSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(GetSwapchain()->Present(swapchainImageIndex, 1, &frame.renderCompleteSemaphore));
}

void ProjApp::DrawGui()
{
    ImGui::Separator();

    ImGui::SliderInt("Graphics Load", &mGraphicsLoad, 1, 500);
    ImGui::SliderInt("Compute Load", &mComputeLoad, 1, 20);
}

SETUP_APPLICATION(ProjApp)
