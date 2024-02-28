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

#ifndef ppx_grfx_vk_render_pass_h
#define ppx_grfx_vk_render_pass_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_render_pass.h"

namespace ppx {
namespace grfx {
namespace vk {

class RenderPass
    : public grfx::RenderPass
{
public:
    RenderPass() {}
    virtual ~RenderPass() {}

    VkRenderPassPtr  GetVkRenderPass() const { return mRenderPass; }
    VkFramebufferPtr GetVkFramebuffer() const { return mFramebuffer; }

protected:
    virtual Result CreateApiObjects(const grfx::internal::RenderPassCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result CreateRenderPass(const grfx::internal::RenderPassCreateInfo* pCreateInfo);
    Result CreateFramebuffer(const grfx::internal::RenderPassCreateInfo* pCreateInfo);

private:
    VkRenderPassPtr  mRenderPass;
    VkFramebufferPtr mFramebuffer;
};

// -------------------------------------------------------------------------------------------------

VkResult CreateTransientRenderPass(
    vk::Device*           device,
    uint32_t              renderTargetCount,
    const VkFormat*       pRenderTargetFormats,
    VkFormat              depthStencilFormat,
    VkSampleCountFlagBits sampleCount,
    uint32_t              viewMask,
    uint32_t              correlationMask,
    VkRenderPass*         pRenderPass,
    grfx::ShadingRateMode shadingRateMode = grfx::SHADING_RATE_NONE);

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_render_pass_h
