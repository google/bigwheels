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

#include <memory>

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

    // Creates a modified version of the render pass create info which supports
    // the required shading rate mode.
    //
    // The shared_ptr also manages the memory of all referenced pointers and
    // arrays in the VkRenderPassCreateInfo2 struct.
    std::shared_ptr<const VkRenderPassCreateInfo2>        GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci);
    std::shared_ptr<const VkRenderPassCreateInfo2>        GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo2& vkci);
    static std::shared_ptr<const VkRenderPassCreateInfo2> GetModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode, const VkRenderPassCreateInfo& vkci);
    static std::shared_ptr<const VkRenderPassCreateInfo2> GetModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode, const VkRenderPassCreateInfo2& vkci);

protected:
    // ShadingRatePattern::ModifiedRenderPassCreateInfo
    //
    // Handles modification of VkRenderPassCreateInfo/VkRenderPassCreateInfo2
    // to add support for a ShadingRatePattern.
    //
    // The ModifiedRenderPassCreateInfo object handles the lifetimes of the
    // pointers and arrays referenced in the modified VkRenderPassCreateInfo2.
    class ModifiedRenderPassCreateInfo : public std::enable_shared_from_this<ModifiedRenderPassCreateInfo>
    {
    public:
        ModifiedRenderPassCreateInfo()          = default;
        virtual ~ModifiedRenderPassCreateInfo() = default;

        // Initializes the modified VkRenderPassCreateInfo2, based on the
        // values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2,
        // with appropriate modifications for the shading rate implementation.
        ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo& vkci);
        ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo2& vkci);

        // Returns the modified VkRenderPassCreateInfo2.
        //
        // The returned pointer, as well as pointers and arrays inside the
        // VkRenderPassCreateInfo2 struct, point to memory owned by this
        /// ModifiedRenderPassCreateInfo object, and so cannot be used after
        // this object is destroyed.
        std::shared_ptr<const VkRenderPassCreateInfo2> Get()
        {
            return std::shared_ptr<const VkRenderPassCreateInfo2>(shared_from_this(), &mVkRenderPassCreateInfo2);
        }

    protected:
        // Initializes the internal VkRenderPassCreateInfo2, based on the
        // values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2.
        // All arrays are copied to internal vectors, and the internal
        // VkRenderPassCreateInfo2 references the data in these vectors, rather
        // than the poitners in the input VkRenderPassCreateInfo.
        void LoadVkRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci);
        void LoadVkRenderPassCreateInfo2(const VkRenderPassCreateInfo2& vkci);

        // Modifies the internal VkRenderPassCreateInfo2 to enable the shading
        // rate implementation.
        virtual void UpdateRenderPassForShadingRateImplementation() = 0;

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

    // Creates a ModifiedRenderPassCreateInfo that will modify
    // VkRenderPassCreateInfo/VkRenderPassCreateInfo2  to support the given
    // ShadingRateMode on the given device.
    static std::shared_ptr<ModifiedRenderPassCreateInfo> CreateModifiedRenderPassCreateInfo(vk::Device* device, ShadingRateMode mode);

    // Creates a ModifiedRenderPassCreateInfo that will modify
    // VkRenderPassCreateInfo/VkRenderPassCreateInfo2 to support this
    // ShadingRatePattern.
    std::shared_ptr<ModifiedRenderPassCreateInfo> CreateModifiedRenderPassCreateInfo() const
    {
        return CreateModifiedRenderPassCreateInfo(ToApi(GetDevice()), GetShadingRateMode());
    }

    // ShadingRatePattern::FDMModifiedRenderPassCreateInfo
    //
    // Handles modification of VkRenderPassCreateInfo(2) to add support for FDM.
    class FDMModifiedRenderPassCreateInfo : public ModifiedRenderPassCreateInfo
    {
    protected:
        void UpdateRenderPassForShadingRateImplementation() override;

    private:
        VkRenderPassFragmentDensityMapCreateInfoEXT mFdmInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT};
    };

    // ShadingRatePattern::VRSModifiedRenderPassCreateInfo
    //
    // Handles modification of VkRenderPassCreateInfo(2) to add support for VRS.
    class VRSModifiedRenderPassCreateInfo : public ModifiedRenderPassCreateInfo
    {
    public:
        VRSModifiedRenderPassCreateInfo(const ShadingRateCapabilities& capabilities)
            : mCapabilities(capabilities) {}

    protected:
        void UpdateRenderPassForShadingRateImplementation() override;

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
