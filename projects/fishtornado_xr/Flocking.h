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

#ifndef FLOCKING_H
#define FLOCKING_H

#include "ppx/grfx/grfx_mesh.h"
using namespace ppx;

#include "Buffer.h"

constexpr uint32_t kDefaultFishResX     = 128;
constexpr uint32_t kDefaultFishResY     = 128;
constexpr uint32_t kDefaultFishThreadsX = 8;
constexpr uint32_t kDefaultFishThreadsY = 8;

class FishTornadoApp;
struct FishTornadoSettings;

class Flocking
{
public:
    Flocking();
    ~Flocking();

    void Setup(uint32_t numFramesInFlight, const FishTornadoSettings& settings);
    void Shutdown();
    void Update(uint32_t frameIndex, uint32_t viewIndex);
    void CopyConstantsToGpu(uint32_t frameIndex, grfx::CommandBuffer* pCmd);

    void BeginCompute(uint32_t frameIndex, grfx::CommandBuffer* pCmd, bool asyncCompute);
    void Compute(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void EndCompute(uint32_t frameIndex, grfx::CommandBuffer* pCmd, bool asyncCompute);

    void BeginGraphics(uint32_t frameIndex, grfx::CommandBuffer* pCmd, bool asyncCompute);
    void DrawDebug(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawShadow(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawForward(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void EndGraphics(uint32_t frameIndex, grfx::CommandBuffer* pCmd, bool asyncCompute);

private:
    void SetupSetLayouts();
    void SetupSets();
    void SetupPipelineInterfaces();
    void SetupPipelines();

private:
    struct PerFrame
    {
        ConstantBuffer         modelConstants;
        ConstantBuffer         flockingConstants;
        grfx::TexturePtr       positionTexture;
        grfx::TexturePtr       velocityTexture;
        grfx::DescriptorSetPtr modelSet;
        grfx::DescriptorSetPtr positionSet;
        grfx::DescriptorSetPtr velocitySet;
        grfx::DescriptorSetPtr renderSet;

        bool renderedWithAsyncCompute = false;
    };

    // Tracked as a pointer as this value can be live-updated in the debug menu.
    const bool* pUseTracking = nullptr;
    uint32_t    mResX        = kDefaultFishResX;
    uint32_t    mResY        = kDefaultFishResY;
    uint32_t    mThreadsX    = kDefaultFishThreadsX;
    uint32_t    mThreadsY    = kDefaultFishThreadsY;
    float    mMinThresh;
    float    mMaxThresh;
    float    mMinSpeed;
    float    mMaxSpeed;
    float    mZoneRadius;

    grfx::DescriptorSetLayoutPtr mFlockingPositionSetLayout;
    grfx::DescriptorSetLayoutPtr mFlockingVelocitySetLayout;
    grfx::PipelineInterfacePtr   mFlockingPositionPipelineInterface;
    grfx::PipelineInterfacePtr   mFlockingVelocityPipelineInterface;
    grfx::ComputePipelinePtr     mFlockingPositionPipeline;
    grfx::ComputePipelinePtr     mFlockingVelocityPipeline;
    grfx::DescriptorSetLayoutPtr mRenderSetLayout;
    grfx::PipelineInterfacePtr   mForwardPipelineInterface;
    grfx::GraphicsPipelinePtr    mForwardPipeline;
    grfx::GraphicsPipelinePtr    mShadowPipeline;
    std::vector<PerFrame>        mPerFrame;
    ConstantBuffer               mMaterialConstants;
    grfx::DescriptorSetPtr       mMaterialSet;
    grfx::MeshPtr                mMesh;
    grfx::TexturePtr             mAlbedoTexture;
    grfx::TexturePtr             mRoughnessTexture;
    grfx::TexturePtr             mNormalMapTexture;
};

#endif // FLOCKING_H
