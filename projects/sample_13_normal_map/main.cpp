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
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
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
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDrawObjectSetLayout;
    grfx::PipelineInterfacePtr   mDrawObjectPipelineInterface;
    grfx::GraphicsPipelinePtr    mDrawObjectPipeline;
    grfx::ImagePtr               mAlbedoTexture;
    grfx::ImagePtr               mNormalMap;
    grfx::SampledImageViewPtr    mAlbedoTextureView;
    grfx::SampledImageViewPtr    mNormalMapView;
    grfx::SamplerPtr             mSampler;
    Entity                       mCube;
    Entity                       mSphere;
    std::vector<Entity*>         mEntities;
    uint32_t                     mEntityIndex = 0;
    PerspCamera                  mCamera;

    std::vector<const char*> mEntityNames = {
        "Cube",
        "Sphere",
    };

    grfx::DescriptorSetLayoutPtr mLightSetLayout;
    grfx::PipelineInterfacePtr   mLightPipelineInterface;
    grfx::GraphicsPipelinePtr    mLightPipeline;
    Entity                       mLight;
    float3                       mLightPosition = float3(0, 5, 5);

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
    settings.appName                    = "normal_map";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
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

    // Draw descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

    // Update draw descriptor set
    grfx::WriteDescriptor writes[4] = {};
    writes[0].binding               = 0; // Uniform buffer
    writes[0].type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].bufferOffset          = 0;
    writes[0].bufferRange           = PPX_WHOLE_SIZE;
    writes[0].pBuffer               = pEntity->drawUniformBuffer;
    writes[1].binding               = 1; // Albedo texture
    writes[1].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[1].pImageView            = mAlbedoTextureView;
    writes[2].binding               = 2; // Normal map
    writes[2].type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[2].pImageView            = mNormalMapView;
    writes[3].binding               = 3; // Sampler
    writes[3].type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
    writes[3].pSampler              = mSampler;

    PPX_CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(4, writes));
}

void ProjApp::Setup()
{
    // Cameras
    {
        mCamera = PerspCamera(60.0f, GetWindowAspect());
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
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{2, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{3, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_PS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDrawObjectSetLayout));
    }

    // Textures, views, and samplers
    {
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/textures/normal_map/albedo.jpg"), &mAlbedoTexture, options, false));
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/textures/normal_map/normal.jpg"), &mNormalMap, options, false));

        grfx::SampledImageViewCreateInfo sivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mAlbedoTexture);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sivCreateInfo, &mAlbedoTextureView));

        sivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mNormalMap);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sivCreateInfo, &mNormalMapView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
    }

    // Setup entities
    {
        TriMeshOptions options = TriMeshOptions().Indices().Normals().TexCoords().Tangents();

        TriMesh mesh = TriMesh::CreateCube(float3(2, 2, 2), TriMeshOptions(options).ObjectColor(float3(0.7f)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mDrawObjectSetLayout, &mCube);
        mEntities.push_back(&mCube);

        mesh = TriMesh::CreateSphere(2, 16, 8, TriMeshOptions(options).ObjectColor(float3(0.7f)).TexCoordScale(float2(3)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mDrawObjectSetLayout, &mSphere);
        mEntities.push_back(&mSphere);
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

        std::vector<char> bytecode = LoadShader("basic/shaders", "NormalMap.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;

        bytecode = LoadShader("basic/shaders", "NormalMap.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = CountU32(mCube.mesh->GetDerivedVertexBindings());
        gpCreateInfo.vertexInputState.bindings[0]       = mCube.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mCube.mesh->GetDerivedVertexBindings()[1];
        gpCreateInfo.vertexInputState.bindings[2]       = mCube.mesh->GetDerivedVertexBindings()[2];
        gpCreateInfo.vertexInputState.bindings[3]       = mCube.mesh->GetDerivedVertexBindings()[3];
        gpCreateInfo.vertexInputState.bindings[4]       = mCube.mesh->GetDerivedVertexBindings()[4];
        gpCreateInfo.vertexInputState.bindings[5]       = mCube.mesh->GetDerivedVertexBindings()[5];
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
        std::vector<char>     bytecode = LoadShader("basic/shaders", "VertexColors.vs");
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

        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        mPerFrame.push_back(frame);
    }
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Update light position
    float t        = GetElapsedSeconds() / 2.0f;
    float r        = 3.0f;
    mLightPosition = float3(2, 2, 2);

    // Update camera(s)
    mCamera.LookAt(float3(0, 0, 5), float3(0, 0, 0));

    // Update uniform buffer(s)
    {
        Entity* pEntity = mEntities[mEntityIndex];

        pEntity->translate = float3(0, 0, -10 * (1 + sin(t / 2)));
        pEntity->rotate    = float3(t, t, 2 * t);

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
            float4   EyePosition;                // Eye position
        };

        Scene scene                      = {};
        scene.ModelMatrix                = M;
        scene.NormalMatrix               = glm::inverseTranspose(M);
        scene.Ambient                    = float4(0.3f);
        scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        scene.LightPosition              = float4(mLightPosition, 0);
        scene.EyePosition                = float4(mCamera.GetEyePosition(), 1);

        pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);
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
        //  Render scene
        // =====================================================================
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            // Draw entities
            frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
            frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &mEntities[mEntityIndex]->drawDescriptorSet);
            frame.cmd->BindIndexBuffer(mEntities[mEntityIndex]->mesh);
            frame.cmd->BindVertexBuffers(mEntities[mEntityIndex]->mesh);
            frame.cmd->DrawIndexed(mEntities[mEntityIndex]->mesh->GetIndexCount());

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

    grfx::Semaphore* presentationReadySemaphore = GetSwapchain()->GetPresentationReadySemaphore(imageIndex);

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &presentationReadySemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &presentationReadySemaphore));
}

void ProjApp::DrawGui()
{
    ImGui::Separator();

    static const char* currentEntity = mEntityNames[0];

    if (ImGui::BeginCombo("Geometry", currentEntity)) {
        for (size_t i = 0; i < mEntityNames.size(); ++i) {
            bool isSelected = (currentEntity == mEntityNames[i]);
            if (ImGui::Selectable(mEntityNames[i], isSelected)) {
                currentEntity = mEntityNames[i];
                mEntityIndex  = static_cast<uint32_t>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

SETUP_APPLICATION(ProjApp)
