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

#ifndef CUBEXR_H
#define CUBEXR_H

#include "ppx/math_config.h"
#include "ppx/ppx.h"
using namespace ppx;

class CubeXrApp
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

        // XR UI per frame elements.
        grfx::CommandBufferPtr uiCmd;
        grfx::FencePtr         uiRenderCompleteFence;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::ShaderModulePtr        mVS;
    grfx::ShaderModulePtr        mPS;
    grfx::PipelineInterfacePtr   mPipelineInterface;
    grfx::GraphicsPipelinePtr    mPipeline;
    grfx::BufferPtr              mVertexBuffer;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    grfx::DescriptorSetPtr       mDescriptorSet;
    grfx::BufferPtr              mUniformBuffer;
    grfx::Viewport               mViewport;
    grfx::Rect                   mScissorRect;
    grfx::VertexBinding          mVertexBinding;
};

#endif // CUBEXR_H
