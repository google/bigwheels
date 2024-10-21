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

#ifndef GLTF_NODE_ANIMATION_H
#define GLTF_NODE_ANIMATION_H

#include "ppx/ppx.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/scene/scene_pipeline_args.h"

class GltfNodeAnimationApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Shutdown() override;
    virtual void Render() override;

protected:
    virtual void DrawGui() override;

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
    ppx::grfx::GraphicsPipelinePtr  mPipeline;

    ppx::scene::Scene*                mScene        = nullptr;
    ppx::scene::MaterialPipelineArgs* mPipelineArgs = nullptr;
};

#endif // GLTF_NODE_ANIMATION_H
