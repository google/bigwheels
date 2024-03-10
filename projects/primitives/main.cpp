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
    grfx::GraphicsPipelinePtr    mTrianglePipeline;
    Entity                       mCube;
    Entity                       mSphere;
    Entity                       mPlane;
    grfx::GraphicsPipelinePtr    mWirePipeline;
    Entity                       mWireCube;
    Entity                       mWireSphere;
    Entity                       mWirePlane;

private:
    void SetupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
    void SetupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "primitives";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
}

void ProjApp::SetupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity)
{
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromTriMesh(GetGraphicsQueue(), &mesh, &pEntity->mesh));

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

void ProjApp::SetupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity)
{
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromWireMesh(GetGraphicsQueue(), &mesh, &pEntity->mesh));

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
        GeometryCreateInfo geometryCreateInfo = GeometryCreateInfo::Planar().AddColor();
        TriMeshOptions     triMeshOptions     = TriMeshOptions().Indices().VertexColors();
        WireMeshOptions    wireMeshOptions    = WireMeshOptions().Indices().VertexColors();

        TriMesh triMesh = TriMesh::CreateCube(float3(2, 2, 2), triMeshOptions);
        SetupEntity(triMesh, geometryCreateInfo, &mCube);

        triMesh = TriMesh::CreateSphere(1.0f, 16, 8, triMeshOptions);
        SetupEntity(triMesh, geometryCreateInfo, &mSphere);

        triMesh = TriMesh::CreatePlane(TRI_MESH_PLANE_POSITIVE_Y, float2(2, 2), 1, 1, triMeshOptions);
        SetupEntity(triMesh, geometryCreateInfo, &mPlane);

        WireMesh wireMesh = WireMesh::CreateCube(float3(2, 2, 2), wireMeshOptions);
        SetupEntity(wireMesh, geometryCreateInfo, &mWireCube);

        wireMesh = WireMesh::CreateSphere(1.0f, 16, 8, wireMeshOptions);
        SetupEntity(wireMesh, geometryCreateInfo, &mWireSphere);

        wireMesh = WireMesh::CreatePlane(WIRE_MESH_PLANE_POSITIVE_Y, float2(2, 2), 4, 4, wireMeshOptions);
        SetupEntity(wireMesh, geometryCreateInfo, &mWirePlane);
    }

    // Pipelines
    {
        std::vector<char> bytecode = LoadShader("basic/shaders", "VertexColors.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("basic/shaders", "VertexColors.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 2;
        gpCreateInfo.vertexInputState.bindings[0]       = mSphere.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mSphere.mesh->GetDerivedVertexBindings()[1];
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
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;

        // Triange pipeline
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mTrianglePipeline));

        // Wire pipeline
        gpCreateInfo.topology = grfx::PRIMITIVE_TOPOLOGY_LINE_LIST;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mWirePipeline));
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

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Update uniform buffer
    {
        float    t = GetElapsedSeconds();
        float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
        float4x4 V = glm::lookAt(float3(0, 0, 8), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));

        float4x4 T   = glm::translate(float3(-4, 2, 0));
        float4x4 mat = P * V * T * M;
        mCube.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(0, 2, 0));
        mat = P * V * T * M;
        mSphere.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(4, 2, 0));
        mat = P * V * T * M;
        mPlane.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(-4, -2, 0));
        mat = P * V * T * M;
        mWireCube.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(0, -2, 0));
        mat = P * V * T * M;
        mWireSphere.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

        T   = glm::translate(float3(4, -2, 0));
        mat = P * V * T * M;
        mWirePlane.uniformBuffer->CopyFromSource(sizeof(mat), &mat);
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

            // Triangle pipeline
            frame.cmd->BindGraphicsPipeline(mTrianglePipeline);

            // Cube
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mCube.descriptorSet);
            frame.cmd->BindIndexBuffer(mCube.mesh);
            frame.cmd->BindVertexBuffers(mCube.mesh);
            frame.cmd->DrawIndexed(mCube.mesh->GetIndexCount());

            // Sphere
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mSphere.descriptorSet);
            frame.cmd->BindIndexBuffer(mSphere.mesh);
            frame.cmd->BindVertexBuffers(mSphere.mesh);
            frame.cmd->DrawIndexed(mSphere.mesh->GetIndexCount());

            // Plane
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlane.descriptorSet);
            frame.cmd->BindIndexBuffer(mPlane.mesh);
            frame.cmd->BindVertexBuffers(mPlane.mesh);
            frame.cmd->DrawIndexed(mPlane.mesh->GetIndexCount());

            // -------------------------------------------------------------------------------------
            //
            // Wire pipeline
            frame.cmd->BindGraphicsPipeline(mWirePipeline);

            // Wire cube
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWireCube.descriptorSet);
            frame.cmd->BindIndexBuffer(mWireCube.mesh);
            frame.cmd->BindVertexBuffers(mWireCube.mesh);
            frame.cmd->DrawIndexed(mWireCube.mesh->GetIndexCount());

            // Wire sphere
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWireSphere.descriptorSet);
            frame.cmd->BindIndexBuffer(mWireSphere.mesh);
            frame.cmd->BindVertexBuffers(mWireSphere.mesh);
            frame.cmd->DrawIndexed(mWireSphere.mesh->GetIndexCount());

            // Wire plane
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWirePlane.descriptorSet);
            frame.cmd->BindIndexBuffer(mWirePlane.mesh);
            frame.cmd->BindVertexBuffers(mWirePlane.mesh);
            frame.cmd->DrawIndexed(mWirePlane.mesh->GetIndexCount());

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