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
#include "ppx/metrics.h"
#include "ppx/camera.h"

#include <filesystem>
#include <vector>

struct FishTornadoSettings
{
    bool     usePCF                   = true;
    bool     forceSingleCommandBuffer = false;
    bool     useAsyncCompute          = false;
    bool     renderFish               = true;
    bool     renderOcean              = true;
    bool     renderShark              = true;
    bool     useTracking              = true;
    uint32_t fishResX                 = kDefaultFishResX;
    uint32_t fishResY                 = kDefaultFishResY;
    uint32_t fishThreadsX             = kDefaultFishThreadsX;
    uint32_t fishThreadsY             = kDefaultFishThreadsY;
};

class FishTornadoApp
    : public ppx::Application
{
public:
    static FishTornadoApp* GetThisApp();
    float                  GetTime() const { return mTime; }
    float                  GetDt() const { return mDt; }

    const Camera* GetCamera() const;
    const Shark*  GetShark() const { return &mShark; }

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

protected:
    virtual void DrawGui() override;
    virtual void UpdateMetrics() override;

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

        // XR UI per frame elements.
        ppx::grfx::CommandBufferPtr uiCmd;
        ppx::grfx::FencePtr         uiRenderCompleteFence;
    };

    struct MetricsData
    {
        static constexpr size_t kTypeGpuFrameTime  = 0;
        static constexpr size_t kTypeIAVertices    = 1;
        static constexpr size_t kTypeIAPrimitives  = 2;
        static constexpr size_t kTypeVSInvocations = 3;
        static constexpr size_t kTypeCInvocations  = 4;
        static constexpr size_t kTypeCPrimitives   = 5;
        static constexpr size_t kTypePSInvocations = 6;
        static constexpr size_t kCount             = 7;

        ppx::metrics::MetricID metrics[kCount] = {};
        uint64_t               data[kCount]    = {};
    };

    grfx::DescriptorPoolPtr               mDescriptorPool;
    grfx::DescriptorSetLayoutPtr          mSceneDataSetLayout;
    grfx::DescriptorSetLayoutPtr          mModelDataSetLayout;
    grfx::DescriptorSetLayoutPtr          mMaterialSetLayout;
    std::vector<PerFrame>                 mPerFrame;
    grfx::TexturePtr                      mCausticsTexture;
    grfx::TexturePtr                      m1x1BlackTexture;
    grfx::SamplerPtr                      mClampedSampler;
    grfx::SamplerPtr                      mRepeatSampler;
    grfx::SamplerPtr                      mShadowSampler;
    grfx::PipelineInterfacePtr            mForwardPipelineInterface;
    grfx::GraphicsPipelinePtr             mDebugDrawPipeline;
    PerspCamera                           mCamera;
    PerspCamera                           mShadowCamera;
    Flocking                              mFlocking;
    Ocean                                 mOcean;
    Shark                                 mShark;
    FishTornadoSettings                   mSettings;
    float                                 mTime                     = 0;
    float                                 mDt                       = 0;
    bool                                  mLastFrameWasAsyncCompute = false;
    int                                   mViewCount                = 1;
    std::vector<uint64_t>                 mViewGpuFrameTime         = {};
    std::vector<grfx::PipelineStatistics> mViewPipelineStatistics   = {};
    MetricsData                           mMetricsData;

private:
    void SetupDescriptorPool();
    void SetupSetLayouts();
    void SetupPipelineInterfaces();
    void SetupTextures();
    void SetupSamplers();
    void SetupPerFrame();
    void SetupCaustics();
    void SetupDebug();
    void SetupFtMetrics();
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
};

#endif // FISHTORNADO_H
