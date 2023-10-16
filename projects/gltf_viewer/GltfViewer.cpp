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

#include "GltfViewer.h"
#include "ppx/scene/scene_gltf_loader.h"
#include "ppx/graphics_util.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

const uint32_t kNumFramesInFlight = 2;

void GltfViewerApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "gltf_viewer";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = true;
    settings.grfx.numFramesInFlight     = kNumFramesInFlight;
    settings.window.width               = 1920 * 1;
    settings.window.height              = 1080 * 1;
    settings.window.resizable           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.allowThirdPartyAssets      = true;
}

void GltfViewerApp::Setup()
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
            //GetAssetPath("scene_renderer/scenes/tests/gltf_test_basic_materials.glb"),
            GetAssetPath("scene_renderer/scenes/tests/gltf_test_materials.glb"),
            nullptr,
            &pLoader));

        PPX_CHECKED_CALL(pLoader->LoadScene(GetDevice(), 0, &mScene));
        PPX_ASSERT_MSG((mScene->GetCameraNodeCount() > 0), "scene doesn't have camera nodes");
        PPX_ASSERT_MSG((mScene->GetMeshNodeCount() > 0), "scene doesn't have mesh nodes");

        delete pLoader;
    }

    // Create renderer
    {
        scene::ForwardRenderer::Create(GetDevice(), kNumFramesInFlight, &mRenderer);
    }
}

void GltfViewerApp::Shutdown()
{
    delete mRenderer;
    delete mScene;
}

void GltfViewerApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
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

void GltfViewerApp::DrawGui()
{
    ImGui::Separator();
}
