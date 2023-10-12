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

#include "GltfBasicMaterials.h"
#include "ppx/scene/scene_gltf_loader.h"
#include "ppx/graphics_util.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void GltfBasicMaterialsApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "gltf_basic_materials";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = false;
    settings.window.width               = 1920 * 1;
    settings.window.height              = 1080 * 1;
    settings.window.resizable           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.allowThirdPartyAssets      = true;
}

void GltfBasicMaterialsApp::Setup()
{
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

    // Load GLTF scene
    {
        scene::GltfLoader* pLoader = nullptr;
        //
        PPX_CHECKED_CALL(scene::GltfLoader::Create(
            GetAssetPath("scene_renderer/scenes/tests/gltf_test_basic_materials.glb"),
            nullptr,
            &pLoader));

        PPX_CHECKED_CALL(pLoader->LoadScene(GetDevice(), 0, &mScene));
        PPX_ASSERT_MSG((mScene->GetCameraNodeCount() > 0), "scene doesn't have camera nodes");
        PPX_ASSERT_MSG((mScene->GetMeshNodeCount() > 0), "scene doesn't have mesh nodes");

        delete pLoader;
    }

    // IBL Textures
    {
        PPX_CHECKED_CALL(grfx_util::CreateIBLTexturesFromFile(
            GetDevice()->GetGraphicsQueue(),
            GetAssetPath("poly_haven/ibl/old_depot_4k.ibl"),
            &mIBLIrrMap,
            &mIBLEnvMap));
    }

    // Pipeline args
    {
        PPX_CHECKED_CALL(scene::MaterialPipelineArgs::Create(GetDevice(), &mPipelineArgs));

        // Populate material samplers
        auto  samplersArrayIndexMap = mScene->GetSamplersArrayIndexMap();
        auto& samplersIndexMap      = samplersArrayIndexMap.second;
        auto& samplersArray         = samplersArrayIndexMap.first;
        for (uint32_t index = 0; index < CountU32(samplersArray); ++index) {
            mPipelineArgs->SetMaterialSampler(index, samplersArray[index]);
        }

        // Populate material images
        auto  imagesArrayIndexMap = mScene->GetImagesArrayIndexMap();
        auto& imagesIndexMap      = imagesArrayIndexMap.second;
        auto& imagesArray         = imagesArrayIndexMap.first;
        for (uint32_t index = 0; index < CountU32(imagesArray); ++index) {
            mPipelineArgs->SetMaterialTexture(index, imagesArray[index]);
        }

        // Populate material params
        auto  materialsArrayIndexMap = mScene->GetMaterialsArrayIndexMap();
        auto& materialsArray         = materialsArrayIndexMap.first;
        for (uint32_t index = 0; index < CountU32(materialsArray); ++index) {
            auto pMaterial = materialsArray[index];

            auto pMaterialParams = mPipelineArgs->GetMaterialParams(index);

            if (pMaterial->GetIdentString() == PPX_MATERIAL_IDENT_STANDARD) {
                auto pStandardMaterial = static_cast<const scene::StandardMaterial*>(pMaterial);

                pMaterialParams->baseColorFactor   = pStandardMaterial->GetBaseColorFactor();
                pMaterialParams->metallicFactor    = pStandardMaterial->GetMetallicFactor();
                pMaterialParams->roughnessFactor   = pStandardMaterial->GetRoughnessFactor();
                pMaterialParams->occlusionStrength = pStandardMaterial->GetOcclusionStrength();
                pMaterialParams->emissiveFactor    = pStandardMaterial->GetEmissiveFactor();
                pMaterialParams->emissiveStrength  = pStandardMaterial->GetEmissiveStrength();

                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pStandardMaterial->GetBaseColorTextureView(), pMaterialParams->baseColorTex);
                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pStandardMaterial->GetMetallicRoughnessTextureView(), pMaterialParams->metallicRoughnessTex);
                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pStandardMaterial->GetNormalTextureView(), pMaterialParams->normalTex);
                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pStandardMaterial->GetOcclusionTextureView(), pMaterialParams->occlusionTex);
                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pStandardMaterial->GetEmissiveTextureView(), pMaterialParams->emssiveTex);
            }
            else if (pMaterial->GetIdentString() == PPX_MATERIAL_IDENT_UNLIT) {
                auto pUnlitMaterial              = static_cast<const scene::UnlitMaterial*>(pMaterial);
                pMaterialParams->baseColorFactor = pUnlitMaterial->GetBaseColorFactor();
                scene::CopyMaterialTextureParams(samplersIndexMap, imagesIndexMap, pUnlitMaterial->GetBaseColorTextureView(), pMaterialParams->baseColorTex);
            }
        }

        // Populate IBL textures
        mPipelineArgs->SetIBLTextures(0, mIBLIrrMap->GetSampledImageView(), mIBLEnvMap->GetSampledImageView());

        // Save material index map
        mMaterialIndexMap = materialsArrayIndexMap.second;
    }

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

        // Get vertex bindings - every mesh in the test scene should have the same attributes
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

        // Pipelines
        CreatePipeline("MaterialVertex.vs", "StandardMaterial.ps", &mStandardMaterialPipeline);
        CreatePipeline("MaterialVertex.vs", "UnlitMaterial.ps", &mUnlitMaterialPipeline);
        CreatePipeline("MaterialVertex.vs", "ErrorMaterial.ps", &mErrorMaterialPipeline);

        // Compile pipelines for mmaterials
        for (auto it : mMaterialIndexMap) {
            auto pMaterial = it.first;
            auto ident     = pMaterial->GetIdentString();

            if (ident == PPX_MATERIAL_IDENT_STANDARD) {
                mMaterialPipelineMap[pMaterial] = mStandardMaterialPipeline;
            }
            else if (ident == PPX_MATERIAL_IDENT_UNLIT) {
                mMaterialPipelineMap[pMaterial] = mUnlitMaterialPipeline;
            }
            else {
                mMaterialPipelineMap[pMaterial] = mErrorMaterialPipeline;
            }
        }
    }
}

void GltfBasicMaterialsApp::Shutdown()
{
    delete mScene;
    delete mPipelineArgs;
}

void GltfBasicMaterialsApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update camera params
    mPipelineArgs->SetCameraParams(mScene->GetCameraNode(0)->GetCamera());

    // Update instance params
    {
        const uint32_t numMeshNodes = mScene->GetMeshNodeCount();
        for (uint32_t instanceIdx = 0; instanceIdx < numMeshNodes; ++instanceIdx) {
            auto pNode                   = mScene->GetMeshNode(instanceIdx);
            auto pInstanceParmas         = mPipelineArgs->GetInstanceParams(instanceIdx);
            pInstanceParmas->modelMatrix = pNode->GetEvaluatedMatrix();
        }
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
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);

            // Set DrawParams::iblIndex and DrawParams::iblLevelCount
            const uint32_t iblIndex      = 0;
            const uint32_t iblLevelCount = mIBLEnvMap->GetMipLevelCount();
            frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &iblIndex, 2);
            frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &iblLevelCount, 3);

            // Draw scene
            const uint32_t numMeshNodes = mScene->GetMeshNodeCount();
            for (uint32_t instanceIdx = 0; instanceIdx < numMeshNodes; ++instanceIdx) {
                auto pNode = mScene->GetMeshNode(instanceIdx);
                auto pMesh = pNode->GetMesh();

                // Index buffer
                frame.cmd->BindIndexBuffer(&pMesh->GetMeshData()->GetIndexBufferView());

                // Vertex buffers
                std::vector<grfx::VertexBufferView> vertexBufferViews = {
                    pMesh->GetMeshData()->GetPositionBufferView(),
                    pMesh->GetMeshData()->GetAttributeBufferView()};
                frame.cmd->BindVertexBuffers(CountU32(vertexBufferViews), DataPtr(vertexBufferViews));

                // Set DrawParams::instanceIndex
                frame.cmd->PushGraphicsConstants(
                    mPipelineInterface,
                    1,
                    &instanceIdx,
                    scene::MaterialPipelineArgs::INSTANCE_INDEX_CONSTANT_OFFSET);

                // Draw batches
                auto& batches = pMesh->GetBatches();
                for (auto& batch : batches) {
                    // Set pipeline
                    auto pipeline = mMaterialPipelineMap[batch.GetMaterial()];
                    frame.cmd->BindGraphicsPipeline(pipeline);

                    // Set DrawParams::materialIndex
                    uint32_t materialIndex = mMaterialIndexMap[batch.GetMaterial()];
                    frame.cmd->PushGraphicsConstants(
                        mPipelineInterface,
                        1,
                        &materialIndex,
                        scene::MaterialPipelineArgs::MATERIAL_INDEX_CONSTANT_OFFSET);

                    frame.cmd->DrawIndexed(batch.GetIndexCount(), 1, batch.GetIndexOffset(), batch.GetVertexOffset(), 0);
                }
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
