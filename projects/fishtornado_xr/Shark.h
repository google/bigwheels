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

#ifndef SHARK_H
#define SHARK_H

#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_mesh.h"
using namespace ppx;

#include "Buffer.h"

class FishTornadoApp;

class Shark
{
public:
    Shark();
    ~Shark();

    const float3& GetPosition() const { return mPos; }

    void Setup(uint32_t numFramesInFlight);
    void Shutdown();
    void Update(uint32_t frameIndex, uint32_t currentViewIndex);
    void CopyConstantsToGpu(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawDebug(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawShadow(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawForward(uint32_t frameIndex, grfx::CommandBuffer* pCmd);

private:
    struct PerFrame
    {
        ConstantBuffer         modelConstants;
        grfx::DescriptorSetPtr modelSet;
    };

    std::vector<PerFrame>     mPerFrame;
    ConstantBuffer            mMaterialConstants;
    grfx::DescriptorSetPtr    mMaterialSet;
    grfx::GraphicsPipelinePtr mForwardPipeline;
    grfx::GraphicsPipelinePtr mShadowPipeline;
    grfx::MeshPtr             mMesh;
    grfx::TexturePtr          mAlbedoTexture;
    grfx::TexturePtr          mRoughnessTexture;
    grfx::TexturePtr          mNormalMapTexture;

    float3 mPos = float3(3000.0f, 100.0f, 0.0f);
    float3 mVel;
    float3 mDir;
};

#endif // SHARK_H
