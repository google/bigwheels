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

#ifndef DYNAMIC_RENDERING_H
#define DYNAMIC_RENDERING_H

#include "ppx/ppx.h"
using namespace ppx;

class DynamicRenderingApp
    : public Application
{
public:
    virtual void Config(ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>               mPerFrame;
    std::vector<grfx::CommandBufferPtr> mPreRecordedCmds;
    grfx::PipelineInterfacePtr          mPipelineInterface;
    grfx::GraphicsPipelinePtr           mPipeline;
    grfx::MeshPtr                       mSphereMesh;
};

#endif // DYNAMIC_RENDERING_H
