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

#ifndef FISHTORNADO_H
#define FISHTORNADO_H

#include "Buffer.h"
#include "Config.h"
#include "Flocking.h"
#include "Ocean.h"
#include "ShaderConfig.h"
#include "Shark.h"

#include "ppx/ppx.h"
#include "ppx/camera.h"

#include <filesystem>

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

class FishTornadoApp
    : public ppx::Application
{
public:
    static FishTornadoApp* GetThisApp();
    float                  GetTime() const { return mTime; }
    float                  GetDt() const { return mDt; }
    const PerspCamera*     GetCamera() const { return &mCamera; }
    const Shark*           GetShark() const { return &mShark; }

    grfx::DescriptorPoolPtr      GetDescriptorPool() const { return mDescriptorPool; }
    grfx::DescriptorSetLayoutPtr GetSceneDataSetLayout() const { return mSceneDataSetLayout; }
    grfx::DescriptorSetLayoutPtr GetModelDataSetLayout() const { return mModelDataSetLayout; }
    grfx::DescriptorSetLayoutPtr GetMaterialSetLayout() const { return mMaterialSetLayout; }
    grfx::DescriptorSetPtr       GetSceneSet(uint32_t frameIndex) const;
    grfx::DescriptorSetPtr       GetSceneShadowSet(uint32_t frameIndex) const;
    grfx::TexturePtr             GetCausticsTexture() const { return mCausticsTexture; }
    grfx::TexturePtr             GetShadowTexture(uint32_t frameIndex) const;
    grfx::SamplerPtr             GetClampedSampler() const { return mClampedSampler; }
    grfx::SamplerPtr             GetRepeatSampler() const { return mRepeatSampler; }
    grfx::PipelineInterfacePtr   GetForwardPipelineInterface() const { return mForwardPipelineInterface; }
    grfx::GraphicsPipelinePtr    GetDebugDrawPipeline() const { return mDebugDrawPipeline; }

    grfx::GraphicsPipelinePtr CreateForwardPipeline(
        const std::filesystem::path& baseDir,
        const std::string&           vsBaseName,
        const std::string&           psBaseName,
        grfx::PipelineInterface*     pPipelineInterface = nullptr);

    grfx::GraphicsPipelinePtr CreateShadowPipeline(
        const std::filesystem::path& baseDir,
        const std::string&           vsBaseName,
        grfx::PipelineInterface*     pPipelineInterface = nullptr);

    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Shutdown() override;
    virtual void Scroll(float dx, float dy) override;
    virtual void Render() override;

    bool WasLastFrameAsync() { return mLastFrameWasAsyncCompute; }

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::CommandBufferPtr gpuStartTimestampCmd;
        grfx::CommandBufferPtr gpuEndTimestampCmd;
        grfx::CommandBufferPtr copyConstantsCmd;
        grfx::CommandBufferPtr grfxFlockingCmd;
        grfx::CommandBufferPtr asyncFlockingCmd;
        grfx::CommandBufferPtr shadowCmd;
        grfx::SemaphorePtr     gpuStartTimestampSemaphore;
        grfx::SemaphorePtr     copyConstantsSemaphore;
        grfx::SemaphorePtr     flockingCompleteSemaphore;
        grfx::SemaphorePtr     shadowCompleteSemaphore;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     frameCompleteSemaphore;
        grfx::FencePtr         frameCompleteFence;
        ConstantBuffer         sceneConstants;
        grfx::DrawPassPtr      shadowDrawPass;
        grfx::DescriptorSetPtr sceneSet;
        grfx::DescriptorSetPtr sceneShadowSet; // See note in SetupSetLayouts()
        grfx::QueryPtr         startTimestampQuery;
        grfx::QueryPtr         endTimestampQuery;
        grfx::QueryPtr         pipelineStatsQuery;
    };

    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mSceneDataSetLayout;
    grfx::DescriptorSetLayoutPtr mModelDataSetLayout;
    grfx::DescriptorSetLayoutPtr mMaterialSetLayout;
    std::vector<PerFrame>        mPerFrame;
    grfx::TexturePtr             mCausticsTexture;
    grfx::TexturePtr             m1x1BlackTexture;
    grfx::SamplerPtr             mClampedSampler;
    grfx::SamplerPtr             mRepeatSampler;
    grfx::SamplerPtr             mShadowSampler;
    grfx::PipelineInterfacePtr   mForwardPipelineInterface;
    grfx::GraphicsPipelinePtr    mDebugDrawPipeline;
    PerspCamera                  mCamera;
    PerspCamera                  mShadowCamera;
    float                        mTime = 0;
    float                        mDt   = 0;
    Flocking                     mFlocking;
    Ocean                        mOcean;
    Shark                        mShark;
    bool                         mUsePCF                   = true;
    uint64_t                     mTotalGpuFrameTime        = 0;
    grfx::PipelineStatistics     mPipelineStatistics       = {};
    bool                         mForceSingleCommandBuffer = false;
    bool                         mUseAsyncCompute          = false;
    bool                         mLastFrameWasAsyncCompute = false;

private:
    void SetupDescriptorPool();
    void SetupSetLayouts();
    void SetupPipelineInterfaces();
    void SetupTextures();
    void SetupSamplers();
    void SetupPerFrame();
    void SetupCaustics();
    void SetupDebug();
    void SetupScene();
    void UploadCaustics();
    void UpdateTime();
    void UpdateScene(uint32_t frameIndex);
    void RenderSceneUsingSingleCommandBuffer(
        uint32_t            frameIndex,
        PerFrame&           frame,
        uint32_t            prevFrameIndex,
        PerFrame&           prevFrame,
        grfx::SwapchainPtr& swapchain,
        uint32_t            imageIndex);
    void RenderSceneUsingMultipleCommandBuffers(
        uint32_t            frameIndex,
        PerFrame&           frame,
        uint32_t            prevFrameIndex,
        PerFrame&           prevFrame,
        grfx::SwapchainPtr& swapchain,
        uint32_t            imageIndex);
    void DrawGui();
};

#endif // FISHTORNADO_H
