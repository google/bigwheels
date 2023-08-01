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
        grfx::MeshPtr          mesh;
        grfx::DescriptorSetPtr descriptorSet;
        grfx::BufferPtr        uniformBuffer;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::ShaderModulePtr        mVS;
    grfx::ShaderModulePtr        mPS;
    grfx::PipelineInterfacePtr   mPipelineInterface;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    grfx::GraphicsPipelinePtr    mInterleavedPipeline;
    Entity                       mInterleavedU16;
    Entity                       mInterleavedU32;
    Entity                       mInterleaved;
    grfx::GraphicsPipelinePtr    mPlanarPipeline;
    Entity                       mPlanarU16;
    Entity                       mPlanarU32;
    Entity                       mPlanar;
    grfx::GraphicsPipelinePtr    mPositionPlanarPipeline;
    Entity                       mPositionPlanarU16;
    Entity                       mPositionPlanarU32;
    Entity                       mPositionPlanar;

private:
    void SetupEntity(const TriMesh& mesh, const GeometryOptions& createInfo, Entity* pEntity);
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "08_basic_geometry";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
}

void ProjApp::SetupEntity(const TriMesh& mesh, const GeometryOptions& createInfo, Entity* pEntity)
{
    Geometry geo;
    PPX_CHECKED_CALL(Geometry::Create(createInfo, mesh, &geo));
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &pEntity->mesh));

    grfx::BufferCreateInfo bufferCreateInfo        = {};
    bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &pEntity->uniformBuffer));

    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

    grfx::WriteDescriptor write = {};
    write.binding               = 0;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = 0;
    write.bufferRange           = PPX_WHOLE_SIZE;
    write.pBuffer               = pEntity->uniformBuffer;
    PPX_CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}

void ProjApp::Setup()
{
    // Descriptor stuff
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 6;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));

        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));
    }

    // Entities
    {
        TriMesh mesh = TriMesh::CreateCube(float3(2, 2, 2), TriMeshOptions().VertexColors().Normals());
        SetupEntity(mesh, GeometryOptions::InterleavedU16().AddColor().AddNormal(), &mInterleavedU16);
        SetupEntity(mesh, GeometryOptions::InterleavedU32().AddColor().AddNormal(), &mInterleavedU32);
        SetupEntity(mesh, GeometryOptions::Interleaved().AddColor().AddNormal(), &mInterleaved);
        SetupEntity(mesh, GeometryOptions::PlanarU16().AddColor().AddNormal(), &mPlanarU16);
        SetupEntity(mesh, GeometryOptions::PlanarU32().AddColor().AddNormal(), &mPlanarU32);
        SetupEntity(mesh, GeometryOptions::Planar().AddColor().AddNormal(), &mPlanar);
        SetupEntity(mesh, GeometryOptions::PositionPlanarU16().AddColor().AddNormal(), &mPositionPlanarU16);
        SetupEntity(mesh, GeometryOptions::PositionPlanarU32().AddColor().AddNormal(), &mPositionPlanarU32);
        SetupEntity(mesh, GeometryOptions::PositionPlanar().AddColor().AddNormal(), &mPositionPlanar);
    }

    // Pipelines
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "VertexLayoutTest.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "VertexLayoutTest.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        // -----------------------------------------------------------------------------------------
        // Interleaved pipeline

        auto interleavedVertexBindings = mInterleavedU16.mesh->GetDerivedVertexBindings();

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = interleavedVertexBindings[0];
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
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mInterleavedPipeline));

        // -----------------------------------------------------------------------------------------
        // Planar pipeline

        auto planarVertexBindings = mPlanarU16.mesh->GetDerivedVertexBindings();

        PPX_ASSERT_MSG(mPlanarU16.mesh->GetVertexBufferCount() == 3, "vertex buffer count should be 3: position, color, normal");
        gpCreateInfo.vertexInputState.bindingCount = 3;
        gpCreateInfo.vertexInputState.bindings[0]  = planarVertexBindings[0];
        gpCreateInfo.vertexInputState.bindings[1]  = planarVertexBindings[1];
        gpCreateInfo.vertexInputState.bindings[2]  = planarVertexBindings[2];
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPlanarPipeline));

        // -----------------------------------------------------------------------------------------
        // Planar pipeline

        auto positionPlanarVertexBindings = mPositionPlanarU16.mesh->GetDerivedVertexBindings();

        PPX_ASSERT_MSG(mPositionPlanarU16.mesh->GetVertexBufferCount() == 2, "vertex buffer count should be 2: position, non-position");
        gpCreateInfo.vertexInputState.bindingCount = 2;
        gpCreateInfo.vertexInputState.bindings[0]  = positionPlanarVertexBindings[0];
        gpCreateInfo.vertexInputState.bindings[1]  = positionPlanarVertexBindings[1];
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPositionPlanarPipeline));
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

    // Update uniform buffer
    {
        float    t = GetElapsedSeconds();
        float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V = glm::lookAt(float3(0, 0, 8), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));

        // Top 3 cubes are interleaved
        float4x4 T   = glm::translate(float3(-4, 2, 0));
        float4x4 mat = P * V * T * M;
        mInterleavedU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(0, 2, 0));
        mat = P * V * T * M;
        mInterleavedU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(4, 2, 0));
        mat = P * V * T * M;
        mInterleaved.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        // Middle 3 cubes are planar
        T   = glm::translate(float3(-4, 0, 0));
        mat = P * V * T * M;
        mPlanarU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(0, 0, 0));
        mat = P * V * T * M;
        mPlanarU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(4, 0, 0));
        mat = P * V * T * M;
        mPlanar.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        // Bottom 3 cubes are position planar
        T   = glm::translate(float3(-4, -2, 0));
        mat = P * V * T * M;
        mPositionPlanarU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(0, -2, 0));
        mat = P * V * T * M;
        mPositionPlanarU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(4, -2, 0));
        mat = P * V * T * M;
        mPositionPlanar.uniformBuffer->CopyFromSource(sizeof(mat), &mat);
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
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            // Interleaved pipeline
            frame.cmd->BindGraphicsPipeline(mInterleavedPipeline);

            // Interleaved U16
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleavedU16.descriptorSet);
            frame.cmd->BindIndexBuffer(mInterleavedU16.mesh);
            frame.cmd->BindVertexBuffers(mInterleavedU16.mesh);
            frame.cmd->DrawIndexed(mInterleavedU16.mesh->GetIndexCount());

            // Interleaved U32
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleavedU32.descriptorSet);
            frame.cmd->BindIndexBuffer(mInterleavedU32.mesh);
            frame.cmd->BindVertexBuffers(mInterleavedU32.mesh);
            frame.cmd->DrawIndexed(mInterleavedU32.mesh->GetIndexCount());

            // Interleaved
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleaved.descriptorSet);
            frame.cmd->BindVertexBuffers(mInterleaved.mesh);
            frame.cmd->Draw(mInterleaved.mesh->GetVertexCount());

            // -------------------------------------------------------------------------------------

            // Planar pipeline
            frame.cmd->BindGraphicsPipeline(mPlanarPipeline);

            // Planar U16
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanarU16.descriptorSet);
            frame.cmd->BindIndexBuffer(mPlanarU16.mesh);
            frame.cmd->BindVertexBuffers(mPlanarU16.mesh);
            frame.cmd->DrawIndexed(mPlanarU16.mesh->GetIndexCount());

            // Planar U32
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanarU32.descriptorSet);
            frame.cmd->BindIndexBuffer(mPlanarU32.mesh);
            frame.cmd->BindVertexBuffers(mPlanarU32.mesh);
            frame.cmd->DrawIndexed(mPlanarU32.mesh->GetIndexCount());

            // Planar
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanar.descriptorSet);
            frame.cmd->BindVertexBuffers(mPlanar.mesh);
            frame.cmd->Draw(mPlanar.mesh->GetVertexCount());

            // -------------------------------------------------------------------------------------

            // Position Planar pipeline
            frame.cmd->BindGraphicsPipeline(mPositionPlanarPipeline);

            // Position Planar U16
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanarU16.descriptorSet);
            frame.cmd->BindIndexBuffer(mPositionPlanarU16.mesh);
            frame.cmd->BindVertexBuffers(mPositionPlanarU16.mesh);
            frame.cmd->DrawIndexed(mPositionPlanarU16.mesh->GetIndexCount());

            // Position Planar U32
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanarU32.descriptorSet);
            frame.cmd->BindIndexBuffer(mPositionPlanarU32.mesh);
            frame.cmd->BindVertexBuffers(mPositionPlanarU32.mesh);
            frame.cmd->DrawIndexed(mPositionPlanarU32.mesh->GetIndexCount());

            // Position Planar
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanar.descriptorSet);
            frame.cmd->BindVertexBuffers(mPositionPlanar.mesh);
            frame.cmd->Draw(mPositionPlanar.mesh->GetVertexCount());

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

SETUP_APPLICATION(ProjApp)