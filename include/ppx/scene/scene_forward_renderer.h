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

#ifndef ppx_scene_forward_renderer_h
#define ppx_scene_forward_renderer_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_renderer.h"

namespace ppx {
namespace scene {

class ForwardRenderer
    : public scene::Renderer
{
private:
    ForwardRenderer(grfx::Device* pDevice, uint32_t numInFlightFrames);

public:
    virtual ~ForwardRenderer();

    static ppx::Result Create(grfx::Device* pDevice, uint32_t numFramesInFlight, scene::Renderer** ppRenderer);

private:
    ppx::Result CreateObjects();
    void        DestroyObjects();

    ppx::Result RenderInternal(scene::RenderOutput* pOutput, grfx::Semaphore* pRenderCompleteSemaphore) override;

private:
    struct Frame;

    ppx::Result RenderToOutput(
        ForwardRenderer::Frame& frame,
        scene::RenderOutput*    pOutput,
        grfx::Semaphore*        pRenderCompleteSemaphore);

private:
    struct Frame
    {
        scene::RenderPass      depthPrePass;
        scene::RenderPass      shadowPass;
        scene::RenderPass      lightingPass;
        grfx::CommandBufferPtr renderOutputCmd;
        grfx::SemaphorePtr     timelineSemaphore;
        uint64_t               timelineValue = 0;
    };

    std::vector<Frame> mFrames;
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_forward_renderer_h
