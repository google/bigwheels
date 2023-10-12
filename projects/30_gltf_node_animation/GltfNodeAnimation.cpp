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

#include "GltfNodeanimation.h"
#include "ppx/scene/scene_gltf_loader.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

uint32_t                 gDbgVtxAttrIndex = 0;
std::vector<const char*> gDbgVtxAttrNames = {
    "Positions",
    "Tex Coords",
    "Normals",
    "Tangents",
};

void GltfNodeAnimationApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "gltf_load_scene";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = false;
    settings.window.resizable           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.allowThirdPartyAssets      = true;
}

void GltfNodeAnimationApp::Setup()
{
    // Load GLTF scene
    {
        scene::GltfLoader* pLoader = nullptr;
        //
        PPX_CHECKED_CALL(scene::GltfLoader::Create(
            GetAssetPath("scene_renderer/scenes/tests/gltf_test_node_animation.gltf"),
            nullptr,
            &pLoader));

        PPX_CHECKED_CALL(pLoader->LoadScene(GetDevice(), 0, &mScene));
        PPX_ASSERT_MSG((mScene->GetCameraNodeCount() > 0), "scene doesn't have camera nodes");
        PPX_ASSERT_MSG((mScene->GetMeshNodeCount() > 0), "scene doesn't have mesh nodes");

        delete pLoader;
    }

    // Pipeline args
    PPX_CHECKED_CALL(scene::MaterialPipelineArgs::Create(GetDevice(), &mPipelineArgs));

    // Pipelines
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.pushConstants.count               = 32;
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mPipelineArgs->GetDescriptorSetLayout();
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

        // Get vertex bindings - every mesh hould have the same attributes
        auto vertexBindings = mScene->GetMeshNode(0)->GetMesh()->GetMeshData()->GetGpuMesh()->GetDerivedVertexBindings();

        auto CreatePipeline = [this, &vertexBindings](const std::string& vsName, const std::string& psName, grfx::GraphicsPipeline** ppPipeline) {
            std::vector<char> bytecode = LoadShader("scene_renderer/shaders", vsName);
            PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
            grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

            bytecode = LoadShader("scene_renderer/shaders", psName);
            PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
            shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

            grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
            gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
            gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
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

            gpCreateInfo.vertexInputState.bindingCount = CountU32(vertexBindings);
            for (uint32_t i = 0; i < gpCreateInfo.vertexInputState.bindingCount; ++i) {
                gpCreateInfo.vertexInputState.bindings[i] = vertexBindings[i];
            }

            PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, ppPipeline));
        };

        CreatePipeline("MaterialVertex.vs", "DebugMaterial.ps", &mPipeline);
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

void GltfNodeAnimationApp::Shutdown()
{
    delete mPipelineArgs;
    delete mScene;
}

void GltfNodeAnimationApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Do some simple animations
    {
        float t = GetElapsedSeconds();

        mScene->FindNode("TopLevelSphere")->SetRotation(float3(0, t, 0));

        mScene->FindNode("Sphere_L2_1")->SetRotation(float3(0, t * 1.25f, 0));
        mScene->FindNode("Sphere_L2_2")->SetRotation(float3(0, t * 1.25f, 0));
        mScene->FindNode("Sphere_L2_3")->SetRotation(float3(0, t * 1.25f, 0));
        mScene->FindNode("Sphere_L2_4")->SetRotation(float3(0, t * 1.25f, 0));

        mScene->FindNode("Sphere_L2_1_L3_1")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_1_L3_2")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_1_L3_3")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_1_L3_4")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_2_L3_1")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_2_L3_2")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_2_L3_3")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_2_L3_4")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_3_L3_1")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_3_L3_2")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_3_L3_3")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_3_L3_4")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_4_L3_1")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_4_L3_2")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_4_L3_3")->SetRotation(float3(0, t * 1.5f, 0));
        mScene->FindNode("Sphere_L2_4_L3_4")->SetRotation(float3(0, t * 1.5f, 0));
    }

    // Update camera params
    mPipelineArgs->SetCameraParams(mScene->GetCameraNode(0)->GetCamera());

    // Update instance params
    for (uint32_t index = 0; index < mScene->GetMeshNodeCount(); ++index) {
        auto pInstanceParams         = mPipelineArgs->GetInstanceParams(index);
        pInstanceParams->modelMatrix = mScene->GetMeshNode(index)->GetEvaluatedMatrix();
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        // Copy pipeline args buffers
        mPipelineArgs->CopyBuffers(frame.cmd);

        // Set descriptor set from pipeline args
        auto pDescriptorSets = mPipelineArgs->GetDescriptorSet();
        frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &pDescriptorSets);

        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0.2f, 0.2f, 0.3f, 1}};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            // Use the same pipeline for everything
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);
            frame.cmd->BindGraphicsPipeline(mPipeline);

            // Set DrawParams::dbgVtxAttrIndex
            const uint32_t dbgVtxAttrIndex = gDbgVtxAttrIndex;
            frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &dbgVtxAttrIndex, scene::MaterialPipelineArgs::DBG_VTX_ATTR_INDEX_CONSTANT_OFFSET);

            // Draw scene
            auto DrawMesh = [&frame, this](uint32_t instanceIndex, const scene::Mesh* pMesh) {
                // Index buffers
                frame.cmd->BindIndexBuffer(&pMesh->GetMeshData()->GetIndexBufferView());

                // Vertex buffers
                std::vector<grfx::VertexBufferView> vertexBufferViews = {
                    pMesh->GetMeshData()->GetPositionBufferView(),
                    pMesh->GetMeshData()->GetAttributeBufferView()};
                frame.cmd->BindVertexBuffers(CountU32(vertexBufferViews), DataPtr(vertexBufferViews));

                frame.cmd->BindVertexBuffers(CountU32(vertexBufferViews), DataPtr(vertexBufferViews));

                // Set DrawParams::instanceIndex
                frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &instanceIndex, scene::MaterialPipelineArgs::INSTANCE_INDEX_CONSTANT_OFFSET);

                // Draw batches
                for (auto& batch : pMesh->GetBatches()) {
                    frame.cmd->DrawIndexed(batch.GetIndexCount(), 1, batch.GetIndexOffset(), batch.GetVertexOffset(), 0);
                }
            };

            const uint32_t numMeshNodes = mScene->GetMeshNodeCount();
            for (uint32_t nodeIdx = 0; nodeIdx < numMeshNodes; ++nodeIdx) {
                auto pNode = mScene->GetMeshNode(nodeIdx);
                auto pMesh = pNode->GetMesh();

                DrawMesh(nodeIdx, pMesh);
            }

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

void GltfNodeAnimationApp::DrawGui()
{
    ImGui::Separator();

    static const char* sCurrentPipeline = gDbgVtxAttrNames[0];
    if (ImGui::BeginCombo("Vertex Attribute", sCurrentPipeline)) {
        for (size_t i = 0; i < gDbgVtxAttrNames.size(); ++i) {
            bool isSelected = (sCurrentPipeline == gDbgVtxAttrNames[i]);
            if (ImGui::Selectable(gDbgVtxAttrNames[i], isSelected)) {
                sCurrentPipeline = gDbgVtxAttrNames[i];
                gDbgVtxAttrIndex = static_cast<int>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}