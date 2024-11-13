// Copyright 2024 Google LLC
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

#ifndef GLTF_BASIC_MATERIALS_H
#define GLTF_BASIC_MATERIALS_H

#include "ppx/ppx.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/scene/scene_pipeline_args.h"

#include <unordered_map>

class GltfBasicMaterialsApp
    : public ppx::Application
{
public:
    void Config(ppx::ApplicationSettings& settings) override;
    void Setup() override;
    void Shutdown() override;
    void Render() override;
    void InitKnobs() override;

private:
    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
        ppx::grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>           mPerFrame;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mStandardMaterialPipeline = nullptr;
    ppx::grfx::GraphicsPipelinePtr  mUnlitMaterialPipeline    = nullptr;
    ppx::grfx::GraphicsPipelinePtr  mErrorMaterialPipeline    = nullptr;

    ppx::scene::Scene*                mScene        = nullptr;
    ppx::scene::MaterialPipelineArgs* mPipelineArgs = nullptr;

    std::unordered_map<const ppx::scene::Material*, uint32_t>                     mMaterialIndexMap;
    std::unordered_map<const ppx::scene::Material*, ppx::grfx::GraphicsPipeline*> mMaterialPipelineMap;

    ppx::grfx::TexturePtr mIBLIrrMap;
    ppx::grfx::TexturePtr mIBLEnvMap;

    std::shared_ptr<ppx::KnobFlag<std::string>> mSceneAssetKnob;
};

#endif // GLTF_BASIC_MATERIALS_H
