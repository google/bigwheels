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
    ShadingRatePattern() {}
    virtual ~ShadingRatePattern() {}

    VkImageViewPtr GetAttachmentImageView() const
    {
        return mAttachmentView;
    }

    // ShadingRatePattern::RenderPassModifier
    //
    // Handles modification of VkRenderPassCreateInfo(2) to add support for a ShadingRatePattern.
    class RenderPassModifier
    {
    public:
        RenderPassModifier()          = default;
        virtual ~RenderPassModifier() = default;

        // Initializes the modified VkRenderPassCreateInfo2, based on the values
        // in VkRenderPassCreateInfo.
        void Initialize(const VkRenderPassCreateInfo& vkci);

        // Initializes the modified VkRenderPassCreateInfo2, based on the values
        // in VkRenderPassCreateInfo2.
        void Initialize(const VkRenderPassCreateInfo2& vkci);

        // Returns the modified VkRenderPassCreateInfo2.
        const VkRenderPassCreateInfo2* GetVkRenderPassCreateInfo2()
        {
            return &mVkRenderPassCreateInfo2;
        }

    protected:
        virtual void InitializeImpl() = 0;

        VkRenderPassCreateInfo2               mVkRenderPassCreateInfo2 = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2};
        std::vector<VkAttachmentDescription2> mAttachments;
        std::vector<VkSubpassDescription2>    mSubpasses;
        struct SubpassAttachments
        {
            std::vector<VkAttachmentReference2> inputAttachments;
            std::vector<VkAttachmentReference2> colorAttachments;
            std::vector<VkAttachmentReference2> resolveAttachments;
            VkAttachmentReference2              depthStencilAttachment;
            std::vector<uint32_t>               preserveAttachments;
        };
        std::vector<SubpassAttachments>   mSubpassAttachments;
        std::vector<VkSubpassDependency2> mDependencies;
    };

    // Creates a RenderPassModifier that will modify VkRenderPassCreateInfo(2)
    // to support this ShadingRatePattern.
    std::unique_ptr<RenderPassModifier> CreateRenderPassModifier() const
    {
        return CreateRenderPassModifier(ToApi(GetDevice()), GetShadingRateMode());
    }

    // Creates a RenderPassModifier that will modify VkRenderPassCreateInfo(2)
    // to support the given ShadingRateMode on the given device.
    static std::unique_ptr<RenderPassModifier> CreateRenderPassModifier(vk::Device* device, ShadingRateMode mode);

protected:
    // ShadingRatePattern::FDMRenderPassModifier
    //
    // Handles modification of VkRenderPassCreateInfo(2) to add support for FDM.
    class FDMRenderPassModifier : public RenderPassModifier
    {
    protected:
        void InitializeImpl() override;

    private:
        VkRenderPassFragmentDensityMapCreateInfoEXT mFdmInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT};
    };

    // ShadingRatePattern::VRSRenderPassModifier
    //
    // Handles modification of VkRenderPassCreateInfo(2) to add support for VRS.
    class VRSRenderPassModifier : public RenderPassModifier
    {
    public:
        VRSRenderPassModifier(const ShadingRateCapabilities& capabilities)
            : mCapabilities(capabilities) {}

    protected:
        void InitializeImpl() override;

    private:
        ShadingRateCapabilities                mCapabilities;
        VkFragmentShadingRateAttachmentInfoKHR mVrsAttachmentInfo = {VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR};
        VkAttachmentReference2                 mVrsAttachmentRef  = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR};
    };

    Result         CreateApiObjects(const ShadingRatePatternCreateInfo* pCreateInfo) override;
    void           DestroyApiObjects() override;
    VkImageViewPtr mAttachmentView;
};

} // namespace vk
} // namespace grfx
} // namespace ppx
#endif // ppx_grfx_vk_shading_rate_h
