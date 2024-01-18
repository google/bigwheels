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

#ifndef OCEAN_H
#define OCEAN_H

#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/grfx/grfx_mesh.h"
using namespace ppx;

#include "Buffer.h"

class FishTornadoApp;

class Ocean
{
public:
    Ocean();
    ~Ocean();

    void Setup(uint32_t numFramesInFlight);
    void Shutdown();
    void Render(uint32_t frameIndex);
    void CopyConstantsToGpu(uint32_t frameIndex, grfx::CommandBuffer* pCmd);
    void DrawForward(uint32_t frameIndex, grfx::CommandBuffer* pCmd);

private:
    struct PerFrame
    {
        ConstantBuffer         floorModelConstants;
        grfx::DescriptorSetPtr floorModelSet;
        ConstantBuffer         surfaceModelConstants;
        grfx::DescriptorSetPtr surfaceModelSet;
        ConstantBuffer         beamModelConstants;
        grfx::DescriptorSetPtr beamModelSet;
    };

    std::vector<PerFrame> mPerFrame;

    // Floor
    grfx::GraphicsPipelinePtr mFloorForwardPipeline;
    ConstantBuffer            mFloorMaterialConstants;
    grfx::DescriptorSetPtr    mFloorMaterialSet;
    grfx::MeshPtr             mFloorMesh;
    grfx::TexturePtr          mFloorAlbedoTexture;
    grfx::TexturePtr          mFloorRoughnessTexture;
    grfx::TexturePtr          mFloorNormalMapTexture;

    // Surface
    grfx::GraphicsPipelinePtr mSurfaceForwardPipeline;
    ConstantBuffer            mSurfaceMaterialConstants;
    grfx::DescriptorSetPtr    mSurfaceMaterialSet;
    grfx::MeshPtr             mSurfaceMesh;
    grfx::TexturePtr          mSurfaceAlbedoTexture;
    grfx::TexturePtr          mSurfaceRoughnessTexture;
    grfx::TexturePtr          mSurfaceNormalMapTexture;

    // Beam
    grfx::GraphicsPipelinePtr mBeamForwardPipeline;
    grfx::MeshPtr             mBeamMesh;
};

#endif // OCEAN_H
