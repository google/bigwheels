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

#ifndef ppx_grfx_vk_shading_rate_h
#define ppx_grfx_vk_shading_rate_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_shading_rate.h"

namespace ppx {
namespace grfx {
namespace vk {

// ShadingRatePattern
//
// An image defining the shading rate of regions of a render pass.
class ShadingRatePattern
    : public grfx::ShadingRatePattern
{
public:
    virtual ~ShadingRatePattern() = default;

    VkImageViewPtr GetAttachmentImageView() const
    {
        return mAttachmentView;
    }

protected:
    Result CreateApiObjects(const ShadingRatePatternCreateInfo* pCreateInfo) override;
    void   DestroyApiObjects() override;

    VkImageViewPtr mAttachmentView;
};

} // namespace vk
} // namespace grfx
} // namespace ppx
#endif // ppx_grfx_vk_shading_rate_h
