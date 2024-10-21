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

#include "GltfLoadObjects.h"
#include "ppx/scene/scene_gltf_loader.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

void GltfLoadObjectsApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "gltf_load_objects";
    settings.enableImGui                = false;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = false;
    settings.window.resizable           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.allowThirdPartyAssets      = true;
}

void GltfLoadObjectsApp::Setup()
{
    // Load GLTF objects
    {
        scene::GltfLoader* pLoader = nullptr;
        //
        PPX_CHECKED_CALL(scene::GltfLoader::Create(
            GetAssetPath("scene_renderer/scenes/tests/gltf_test_load_objects.gltf"),
            nullptr,
            &pLoader));

        scene::LoadOptions loadOptions = scene::LoadOptions().SetRequiredAttributes(scene::VertexAttributeFlags::All());

        PPX_CHECKED_CALL(pLoader->LoadNode(GetDevice(), "Camera", &mCameraNode));

        PPX_CHECKED_CALL(pLoader->LoadNodeTransformOnly("NoMaterial", &mNoMaterialTransform));
        PPX_CHECKED_CALL(pLoader->LoadMesh(GetDevice(), "NoMaterial", &mNoMaterialMesh, loadOptions));

        PPX_CHECKED_CALL(pLoader->LoadNodeTransformOnly("BlueMaterial", &mBlueMaterialTransform));
        PPX_CHECKED_CALL(pLoader->LoadMesh(GetDevice(), "BlueMaterial", &mBlueMaterialMesh, loadOptions));

        PPX_CHECKED_CALL(pLoader->LoadNodeTransformOnly("DrawImage", &mDrawImageTransform));
        PPX_CHECKED_CALL(pLoader->LoadMesh(GetDevice(), "DrawImage", &mDrawImageMesh, loadOptions));

        PPX_CHECKED_CALL(pLoader->LoadNodeTransformOnly("DrawTexture", &mDrawTextureTransform));
        PPX_CHECKED_CALL(pLoader->LoadMesh(GetDevice(), "DrawTexture", &mDrawTextureMesh, loadOptions));

        PPX_CHECKED_CALL(pLoader->LoadNode(GetDevice(), "Text", &mTextNode, loadOptions));

        auto GetMaterials = [this](const scene::Mesh* pMesh) {
            const auto meshMaterials = pMesh->GetMaterials();
            std::for_each(meshMaterials.begin(), meshMaterials.end(), [this](const scene::Material* pMaterial) {
                PPX_ASSERT_NULL_ARG(pMaterial);
                this->mMaterials.push_back(pMaterial);
            });
        };

        GetMaterials(mNoMaterialMesh);
        GetMaterials(mBlueMaterialMesh);
        GetMaterials(mDrawImageMesh);
        GetMaterials(mDrawTextureMesh);

        auto pMeshNode = static_cast<scene::MeshNode*>(mTextNode);
        GetMaterials(pMeshNode->GetMesh());

        delete pLoader;
    }

    // Pipeline args
    {
        PPX_CHECKED_CALL(scene::MaterialPipelineArgs::Create(GetDevice(), &mPipelineArgs));

        uint32_t textureIndex = 0;
        for (uint32_t index = 0; index < CountU32(mMaterials); ++index) {
            auto pMaterial = mMaterials[index];

            // Map material index
            mMaterialIndexMap[pMaterial] = index;

            // We only care about textures Unlit materials for this sample
            if (pMaterial->GetIdentString() == PPX_MATERIAL_IDENT_UNLIT) {
                auto pUnlitMaterial = static_cast<const scene::UnlitMaterial*>(pMaterial);

                // Populate image
                mPipelineArgs->SetMaterialTexture(textureIndex, pUnlitMaterial->GetBaseColorTextureView().GetTexture()->GetImage());

                // Populate material params
                auto pMaterialParams                            = mPipelineArgs->GetMaterialParams(index);
                pMaterialParams->baseColorFactor                = pUnlitMaterial->GetBaseColorFactor();
                pMaterialParams->baseColorTex.samplerIndex      = 0; // Use prepopulated sampler;
                pMaterialParams->baseColorTex.textureIndex      = textureIndex;
                pMaterialParams->baseColorTex.texCoordTransform = pUnlitMaterial->GetBaseColorTextureView().GetTexCoordTransform();

                // Increment after populating the material
                ++textureIndex;
            }
        }
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

        // Get vertex bindings - every mesh hould have the same attributes
        auto vertexBindings = mNoMaterialMesh->GetMeshData()->GetAvailableVertexBindings();

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

        // Create a pipeline for each material
        for (uint32_t index = 0; index < CountU32(mMaterials); ++index) {
            auto              pMaterial       = mMaterials[index];
            const std::string ident           = pMaterial->GetIdentString();
            auto              pMaterialParams = mPipelineArgs->GetMaterialParams(index);

            grfx::GraphicsPipelinePtr pipeline = nullptr;
            if (ident == PPX_MATERIAL_IDENT_ERROR) {
                CreatePipeline("MaterialVertex.vs", "ErrorMaterial.ps", &pipeline);
            }
            else if (ident == PPX_MATERIAL_IDENT_UNLIT) {
                CreatePipeline("MaterialVertex.vs", "UnlitMaterial.ps", &pipeline);
            }
            else {
                CreatePipeline("MaterialVertex.vs", "DebugMaterial.ps", &pipeline);
            }
            mMaterialPipelineMap[pMaterial] = pipeline;
        }
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

void GltfLoadObjectsApp::Shutdown()
{
    delete mPipelineArgs;
    delete mCameraNode;
    delete mNoMaterialTransform;
    delete mNoMaterialMesh;
    delete mBlueMaterialTransform;
    delete mBlueMaterialMesh;
    delete mDrawImageTransform;
    delete mDrawImageMesh;
    delete mDrawTextureTransform;
    delete mDrawTextureMesh;
    delete mTextureTransform;
    delete mTextureMesh;
    delete mTextNode;
}

void GltfLoadObjectsApp::Render()
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
    mPipelineArgs->SetCameraParams(static_cast<scene::CameraNode*>(mCameraNode)->GetCamera());

    // Update instance params
    {
        // NoMaterialPlane
        auto pInstanceParams         = mPipelineArgs->GetInstanceParams(0);
        pInstanceParams->modelMatrix = mNoMaterialTransform->GetEvaluatedMatrix();
        // BlueMaterialPlane
        pInstanceParams              = mPipelineArgs->GetInstanceParams(1);
        pInstanceParams->modelMatrix = mBlueMaterialTransform->GetEvaluatedMatrix();
        // DrawImage
        pInstanceParams              = mPipelineArgs->GetInstanceParams(2);
        pInstanceParams->modelMatrix = mDrawImageTransform->GetEvaluatedMatrix();
        // DrawTexture
        pInstanceParams              = mPipelineArgs->GetInstanceParams(3);
        pInstanceParams->modelMatrix = mDrawTextureTransform->GetEvaluatedMatrix();
        // Text
        pInstanceParams              = mPipelineArgs->GetInstanceParams(4);
        pInstanceParams->modelMatrix = mTextNode->GetEvaluatedMatrix();
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

            // Set DrawParams::dbgVtxAttrIndex
            const uint32_t dbgVtxAttrIndex = 0; // Position
            frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &dbgVtxAttrIndex, scene::MaterialPipelineArgs::DBG_VTX_ATTR_INDEX_CONSTANT_OFFSET);

            // Draw objects
            auto DrawMesh = [&frame, this](uint32_t instanceIndex, const scene::Mesh* pMesh) {
                // Set DrawParams::instanceIndex
                frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &instanceIndex, scene::MaterialPipelineArgs::INSTANCE_INDEX_CONSTANT_OFFSET);

                // Draw batches
                for (auto& batch : pMesh->GetBatches()) {
                    auto pMaterial = batch.GetMaterial();

                    // Pipeline
                    auto pipeline = mMaterialPipelineMap[pMaterial];
                    frame.cmd->BindGraphicsPipeline(pipeline);

                    // Set DrawParams::materialIndex
                    uint32_t materialIndex = mMaterialIndexMap[batch.GetMaterial()];
                    frame.cmd->PushGraphicsConstants(mPipelineInterface, 1, &materialIndex, scene::MaterialPipelineArgs::MATERIAL_INDEX_CONSTANT_OFFSET);

                    // Index buffer
                    frame.cmd->BindIndexBuffer(&batch.GetIndexBufferView());

                    // Vertex buffers
                    std::vector<grfx::VertexBufferView> vertexBufferViews = {
                        batch.GetPositionBufferView(),
                        batch.GetAttributeBufferView()};
                    frame.cmd->BindVertexBuffers(CountU32(vertexBufferViews), DataPtr(vertexBufferViews));

                    // Draw!
                    frame.cmd->DrawIndexed(batch.GetIndexCount(), 1, 0, 0, 0);
                }
            };

            // NoMaterialPlane
            DrawMesh(0, mNoMaterialMesh);
            // BlueMaterialPlane
            DrawMesh(1, mBlueMaterialMesh);
            // DrawImage
            DrawMesh(2, mDrawImageMesh);
            // DrawTexture
            DrawMesh(3, mDrawTextureMesh);
            // Text
            DrawMesh(4, static_cast<scene::MeshNode*>(mTextNode)->GetMesh());

            // Draw ImGui
            DrawDebugInfo();
#if defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
            DrawProfilerGrfxApiFunctions();
#endif // defined(PPX_ENABLE_PROFILE_GRFX_API_FUNCTIONS)
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
