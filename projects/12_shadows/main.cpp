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

#define kShadowMapSize 1024

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
    };

    struct Entity
    {
        float3                 translate = float3(0, 0, 0);
        float3                 rotate    = float3(0, 0, 0);
        float3                 scale     = float3(1, 1, 1);
        grfx::MeshPtr          mesh;
        grfx::DescriptorSetPtr drawDescriptorSet;
        grfx::BufferPtr        drawUniformBuffer;
        grfx::DescriptorSetPtr shadowDescriptorSet;
        grfx::BufferPtr        shadowUniformBuffer;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDrawObjectSetLayout;
    grfx::PipelineInterfacePtr   mDrawObjectPipelineInterface;
    grfx::GraphicsPipelinePtr    mDrawObjectPipeline;
    Entity                       mGroundPlane;
    Entity                       mCube;
    Entity                       mKnob;
    std::vector<Entity*>         mEntities;
    PerspCamera                  mCamera;

    grfx::DescriptorSetLayoutPtr mShadowSetLayout;
    grfx::PipelineInterfacePtr   mShadowPipelineInterface;
    grfx::GraphicsPipelinePtr    mShadowPipeline;
    grfx::RenderPassPtr          mShadowRenderPass;
    grfx::SampledImageViewPtr    mShadowImageView;
    grfx::SamplerPtr             mShadowSampler;

    grfx::DescriptorSetLayoutPtr mLightSetLayout;
    grfx::PipelineInterfacePtr   mLightPipelineInterface;
    grfx::GraphicsPipelinePtr    mLightPipeline;
    Entity                       mLight;
    float3                       mLightPosition = float3(0, 5, 5);
    PerspCamera                  mLightCamera;
    bool                         mUsePCF = false;

private:
    void SetupEntity(
        const TriMesh&                   mesh,
        grfx::DescriptorPool*            pDescriptorPool,
        const grfx::DescriptorSetLayout* pDrawSetLayout,
        const grfx::DescriptorSetLayout* pShadowSetLayout,
        Entity*                          pEntity);
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "shadows";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
}

void ProjApp::SetupEntity(
    const TriMesh&                   mesh,
    grfx::DescriptorPool*            pDescriptorPool,
    const grfx::DescriptorSetLayout* pDrawSetLayout,
    const grfx::DescriptorSetLayout* pShadowSetLayout,
    Entity*                          pEntity)
{
    Geometry geo;
    PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &pEntity->mesh));

    // Draw uniform buffer
    grfx::BufferCreateInfo bufferCreateInfo        = {};
    bufferCreateInfo.size                          = RoundUp(512, PPX_CONSTANT_BUFFER_ALIGNMENT);
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &pEntity->drawUniformBuffer));

    // Shadow uniform buffer
    bufferCreateInfo                               = {};
    bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &pEntity->shadowUniformBuffer));

    // Draw descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

    // Shadow descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(pDescriptorPool, pShadowSetLayout, &pEntity->shadowDescriptorSet));

    // Update draw descriptor set
    grfx::WriteDescriptor write = {};
    write.binding               = 0;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = 0;
    write.bufferRange           = PPX_WHOLE_SIZE;
    write.pBuffer               = pEntity->drawUniformBuffer;
    PPX_CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(1, &write));

    // Update shadow descriptor set
    write              = {};
    write.binding      = 0;
    write.type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset = 0;
    write.bufferRange  = PPX_WHOLE_SIZE;
    write.pBuffer      = pEntity->shadowUniformBuffer;
    PPX_CHECKED_CALL(pEntity->shadowDescriptorSet->UpdateDescriptors(1, &write));
}

void ProjApp::Setup()
{
    // Cameras
    {
        mCamera      = PerspCamera(60.0f, GetWindowAspect());
        mLightCamera = PerspCamera(60.0f, 1.0f, 1.0f, 100.0f);
    }

    // Create descriptor pool large enough for this project
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 512;
        poolCreateInfo.sampledImage                   = 512;
        poolCreateInfo.sampler                        = 512;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    // Descriptor set layouts
    {
        // Draw objects
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{2, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_PS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDrawObjectSetLayout));

        // Shadow
        layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mShadowSetLayout));
    }

    // Setup entities
    {
        TriMeshOptions options = TriMeshOptions().Indices().VertexColors().Normals();
        TriMesh        mesh    = TriMesh::CreatePlane(TRI_MESH_PLANE_POSITIVE_Y, float2(50, 50), 1, 1, TriMeshOptions(options).ObjectColor(float3(0.7f)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mGroundPlane);
        mEntities.push_back(&mGroundPlane);

        mesh = TriMesh::CreateCube(float3(2, 2, 2), TriMeshOptions(options).ObjectColor(float3(0.5f, 0.5f, 0.7f)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mCube);
        mCube.translate = float3(-2, 1, 0);
        mEntities.push_back(&mCube);

        mesh = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/material_sphere.obj"), TriMeshOptions(options).ObjectColor(float3(0.7f, 0.2f, 0.2f)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mKnob);
        mKnob.translate = float3(2, 1, 0);
        mKnob.rotate    = float3(0, glm::radians(180.0f), 0);
        mKnob.scale     = float3(2, 2, 2);
        mEntities.push_back(&mKnob);
    }

    // Draw object pipeline interface and pipeline
    {
        // Pipeline interface
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDrawObjectSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mDrawObjectPipelineInterface));

        // Pipeline
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "DiffuseShadow.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;

        bytecode = LoadShader("basic/shaders", "DiffuseShadow.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 3;
        gpCreateInfo.vertexInputState.bindings[0]       = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mGroundPlane.mesh->GetDerivedVertexBindings()[1];
        gpCreateInfo.vertexInputState.bindings[2]       = mGroundPlane.mesh->GetDerivedVertexBindings()[2];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mDrawObjectPipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mDrawObjectPipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    // Shadow pipeline interface and pipeline
    {
        // Pipeline interface
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mShadowSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mShadowPipelineInterface));

        // Pipeline
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "Depth.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
        gpCreateInfo.VS                                = {VS.Get(), "vsmain"};
        gpCreateInfo.vertexInputState.bindingCount     = 1;
        gpCreateInfo.vertexInputState.bindings[0]      = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                          = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                   = true;
        gpCreateInfo.depthWriteEnable                  = true;
        gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount     = 0;
        gpCreateInfo.outputState.depthStencilFormat    = grfx::FORMAT_D32_FLOAT;
        gpCreateInfo.pPipelineInterface                = mShadowPipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mShadowPipeline));
        GetDevice()->DestroyShaderModule(VS);
    }

    // Shadow render pass
    {
        grfx::RenderPassCreateInfo2 createInfo                        = {};
        createInfo.width                                              = kShadowMapSize;
        createInfo.height                                             = kShadowMapSize;
        createInfo.depthStencilFormat                                 = grfx::FORMAT_D32_FLOAT;
        createInfo.depthStencilUsageFlags.bits.depthStencilAttachment = true;
        createInfo.depthStencilUsageFlags.bits.sampled                = true;
        createInfo.depthStencilClearValue                             = {1.0f, 0xFF};
        createInfo.depthLoadOp                                        = grfx::ATTACHMENT_LOAD_OP_CLEAR;
        createInfo.depthStoreOp                                       = grfx::ATTACHMENT_STORE_OP_STORE;
        createInfo.depthStencilInitialState                           = grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateRenderPass(&createInfo, &mShadowRenderPass));
    }

    // Update draw objects with shadow information
    {
        grfx::SampledImageViewCreateInfo ivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mShadowRenderPass->GetDepthStencilImage());
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&ivCreateInfo, &mShadowImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.compareEnable           = true;
        samplerCreateInfo.compareOp               = grfx::COMPARE_OP_LESS_OR_EQUAL;
        samplerCreateInfo.borderColor             = grfx::BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mShadowSampler));

        grfx::WriteDescriptor writes[2] = {};
        writes[0].binding               = 1; // Shadow texture
        writes[0].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageView            = mShadowImageView;
        writes[1].binding               = 2; // Shadow sampler
        writes[1].type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pSampler              = mShadowSampler;

        for (size_t i = 0; i < mEntities.size(); ++i) {
            Entity* pEntity = mEntities[i];
            PPX_CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(2, writes));
        }
    }

    // Light
    {
        // Descriptor set layt
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mLightSetLayout));

        // Model
        TriMeshOptions options = TriMeshOptions().Indices().ObjectColor(float3(1, 1, 1));
        TriMesh        mesh    = TriMesh::CreateCube(float3(0.25f, 0.25f, 0.25f), options);

        Geometry geo;
        PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mLight.mesh));

        // Uniform buffer
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mLight.drawUniformBuffer));

        // Descriptor set
        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mLightSetLayout, &mLight.drawDescriptorSet));

        // Update descriptor set
        grfx::WriteDescriptor write = {};
        write.binding               = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mLight.drawUniformBuffer;
        PPX_CHECKED_CALL(mLight.drawDescriptorSet->UpdateDescriptors(1, &write));

        // Pipeline interface
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mLightSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mLightPipelineInterface));

        // Pipeline
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "VertexColors.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;
        bytecode = LoadShader("basic/shaders", "VertexColors.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 2;
        gpCreateInfo.vertexInputState.bindings[0]       = mLight.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mLight.mesh->GetDerivedVertexBindings()[1];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = true;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mLightPipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mLightPipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
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

        mPerFrame.push_back(frame);
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

    // Update light position
    float t        = GetElapsedSeconds() / 2.0f;
    float r        = 7.0f;
    mLightPosition = float3(r * cos(t), 5.0f, r * sin(t));

    // Update camera(s)
    mCamera.LookAt(float3(5, 7, 7), float3(0, 1, 0));
    mLightCamera.LookAt(mLightPosition, float3(0, 0, 0));

    // Update uniform buffers
    for (size_t i = 0; i < mEntities.size(); ++i) {
        Entity* pEntity = mEntities[i];

        float4x4 T = glm::translate(pEntity->translate);
        float4x4 R = glm::rotate(pEntity->rotate.z, float3(0, 0, 1)) *
                     glm::rotate(pEntity->rotate.y, float3(0, 1, 0)) *
                     glm::rotate(pEntity->rotate.x, float3(1, 0, 0));
        float4x4 S = glm::scale(pEntity->scale);
        float4x4 M = T * R * S;

        // Draw uniform buffers
        struct Scene
        {
            float4x4 ModelMatrix;                // Transforms object space to world space
            float4x4 NormalMatrix;               // Transforms object space to normal space
            float4   Ambient;                    // Object's ambient intensity
            float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
            float4   LightPosition;              // Light's position
            float4x4 LightViewProjectionMatrix;  // Light's view projection matrix
            uint4    UsePCF;                     // Enable/disable PCF
        };

        Scene scene                      = {};
        scene.ModelMatrix                = M;
        scene.NormalMatrix               = glm::inverseTranspose(M);
        scene.Ambient                    = float4(0.3f);
        scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        scene.LightPosition              = float4(mLightPosition, 0);
        scene.LightViewProjectionMatrix  = mLightCamera.GetViewProjectionMatrix();
        scene.UsePCF                     = uint4(mUsePCF);

        pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);

        // Shadow uniform buffers
        float4x4 PV  = mLightCamera.GetViewProjectionMatrix();
        float4x4 MVP = PV * M; // Yes - the other is reversed

        pEntity->shadowUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
    }

    // Update light uniform buffer
    {
        float4x4        T   = glm::translate(mLightPosition);
        const float4x4& PV  = mCamera.GetViewProjectionMatrix();
        float4x4        MVP = PV * T; // Yes - the other is reversed

        mLight.drawUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        // =====================================================================
        //  Render shadow pass
        // =====================================================================
        frame.cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        frame.cmd->BeginRenderPass(mShadowRenderPass);
        {
            frame.cmd->SetScissors(mShadowRenderPass->GetScissor());
            frame.cmd->SetViewports(mShadowRenderPass->GetViewport());

            // Draw entities
            frame.cmd->BindGraphicsPipeline(mShadowPipeline);
            for (size_t i = 0; i < mEntities.size(); ++i) {
                Entity* pEntity = mEntities[i];

                frame.cmd->BindGraphicsDescriptorSets(mShadowPipelineInterface, 1, &pEntity->shadowDescriptorSet);
                frame.cmd->BindIndexBuffer(pEntity->mesh);
                frame.cmd->BindVertexBuffers(pEntity->mesh);
                frame.cmd->DrawIndexed(pEntity->mesh->GetIndexCount());
            }
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE, grfx::RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // =====================================================================
        //  Render scene
        // =====================================================================
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            // Draw entities
            frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
            for (size_t i = 0; i < mEntities.size(); ++i) {
                Entity* pEntity = mEntities[i];

                frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &pEntity->drawDescriptorSet);
                frame.cmd->BindIndexBuffer(pEntity->mesh);
                frame.cmd->BindVertexBuffers(pEntity->mesh);
                frame.cmd->DrawIndexed(pEntity->mesh->GetIndexCount());
            }

            // Draw light
            frame.cmd->BindGraphicsPipeline(mLightPipeline);
            frame.cmd->BindGraphicsDescriptorSets(mLightPipelineInterface, 1, &mLight.drawDescriptorSet);
            frame.cmd->BindIndexBuffer(mLight.mesh);
            frame.cmd->BindVertexBuffers(mLight.mesh);
            frame.cmd->DrawIndexed(mLight.mesh->GetIndexCount());

            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.cmd);
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

void ProjApp::DrawGui()
{
    ImGui::Separator();

    ImGui::Checkbox("Use PCF Shadows", &mUsePCF);
}

SETUP_APPLICATION(ProjApp)
