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

#ifndef ppx_grfx_vk_device_h
#define ppx_grfx_vk_device_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace grfx {
namespace vk {

class Device
    : public grfx::Device
{
public:
    Device() {}
    virtual ~Device() {}

    VkDevicePtr     GetVkDevice() const { return mDevice; }
    VmaAllocatorPtr GetVmaAllocator() const { return mVmaAllocator; }

    const VkPhysicalDeviceFeatures& GetDeviceFeatures() const { return mDeviceFeatures; }

    bool HasDescriptorIndexingFeatures() const { return mHasDescriptorIndexingFeatures; }
    bool HasTimelineSemaphore() const { return mHasTimelineSemaphore; }
    bool HasExtendedDynamicState() const { return mHasExtendedDynamicState; }
    bool HasMultiView() const { return mHasMultiView; }
    virtual Result WaitIdle() override;

    virtual bool PipelineStatsAvailable() const override;
    virtual bool MultiViewAvailable() const override;
    virtual bool DynamicRenderingSupported() const override;
    virtual bool IndependentBlendingSupported() const override;
    virtual bool FragmentStoresAndAtomicsSupported() const override;
    virtual bool PartialDescriptorBindingsSupported() const override;

    void ResetQueryPoolEXT(
        VkQueryPool queryPool,
        uint32_t    firstQuery,
        uint32_t    queryCount) const;

    VkResult WaitSemaphores(const VkSemaphoreWaitInfo* pWaitInfo, uint64_t timeout) const;
    VkResult SignalSemaphore(const VkSemaphoreSignalInfo* pSignalInfo);
    VkResult GetSemaphoreCounterValue(VkSemaphore semaphore, uint64_t* pValue);

    uint32_t                GetGraphicsQueueFamilyIndex() const { return mGraphicsQueueFamilyIndex; }
    uint32_t                GetComputeQueueFamilyIndex() const { return mComputeQueueFamilyIndex; }
    uint32_t                GetTransferQueueFamilyIndex() const { return mTransferQueueFamilyIndex; }
    std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

    uint32_t GetMaxPushDescriptors() const { return mMaxPushDescriptors; }

protected:
    virtual Result AllocateObject(grfx::Buffer** ppObject) override;
    virtual Result AllocateObject(grfx::CommandBuffer** ppObject) override;
    virtual Result AllocateObject(grfx::CommandPool** ppObject) override;
    virtual Result AllocateObject(grfx::ComputePipeline** ppObject) override;
    virtual Result AllocateObject(grfx::DepthStencilView** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorPool** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorSet** ppObject) override;
    virtual Result AllocateObject(grfx::DescriptorSetLayout** ppObject) override;
    virtual Result AllocateObject(grfx::Fence** ppObject) override;
    virtual Result AllocateObject(grfx::GraphicsPipeline** ppObject) override;
    virtual Result AllocateObject(grfx::Image** ppObject) override;
    virtual Result AllocateObject(grfx::PipelineInterface** ppObject) override;
    virtual Result AllocateObject(grfx::Queue** ppObject) override;
    virtual Result AllocateObject(grfx::Query** ppObject) override;
    virtual Result AllocateObject(grfx::RenderPass** ppObject) override;
    virtual Result AllocateObject(grfx::RenderTargetView** ppObject) override;
    virtual Result AllocateObject(grfx::SampledImageView** ppObject) override;
    virtual Result AllocateObject(grfx::Sampler** ppObject) override;
    virtual Result AllocateObject(grfx::SamplerYcbcrConversion** ppObject) override;
    virtual Result AllocateObject(grfx::Semaphore** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderModule** ppObject) override;
    virtual Result AllocateObject(grfx::ShaderProgram** ppObject) override;
    virtual Result AllocateObject(grfx::ShadingRatePattern** ppObject) override;
    virtual Result AllocateObject(grfx::StorageImageView** ppObject) override;
    virtual Result AllocateObject(grfx::Swapchain** ppObject) override;

protected:
    virtual Result CreateApiObjects(const grfx::DeviceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result ConfigureQueueInfo(const grfx::DeviceCreateInfo* pCreateInfo, std::vector<float>& queuePriorities, std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos);
    Result ConfigureExtensions(const grfx::DeviceCreateInfo* pCreateInfo);
    Result ConfigureFeatures(const grfx::DeviceCreateInfo* pCreateInfo, VkPhysicalDeviceFeatures& features);
    Result ConfigureDescriptorIndexingFeatures(const grfx::DeviceCreateInfo* pCreateInfo, VkPhysicalDeviceDescriptorIndexingFeatures& diFeatures);
    void   ConfigureShadingRateCapabilities(
          const grfx::DeviceCreateInfo*  pCreateInfo,
          grfx::ShadingRateCapabilities* pShadingRateCapabilities);
    void ConfigureFDMShadingRateCapabilities(
        VkPhysicalDevice               physicalDevice,
        grfx::ShadingRateCapabilities* pShadingRateCapabilities);
    void ConfigureVRSShadingRateCapabilities(
        VkPhysicalDevice               physicalDevice,
        grfx::ShadingRateCapabilities* pShadingRateCapabilities);
    Result CreateQueues(const grfx::DeviceCreateInfo* pCreateInfo);

private:
    std::vector<std::string>                       mFoundExtensions;
    std::vector<std::string>                       mExtensions;
    VkDevicePtr                                    mDevice;
    VkPhysicalDeviceFeatures                       mDeviceFeatures             = {};
    VkPhysicalDeviceDescriptorIndexingFeatures     mDescriptorIndexingFeatures = {};
    VmaAllocatorPtr                                mVmaAllocator;
    bool                                           mHasDescriptorIndexingFeatures              = false;
    bool                                           mHasTimelineSemaphore                       = false;
    bool                                           mHasExtendedDynamicState                    = false;
    bool                                           mHasDepthClipEnabled                        = false;
    bool                                           mHasMultiView                               = false;
    bool                                           mHasDynamicRendering                        = false;
    PFN_vkResetQueryPoolEXT                        mFnResetQueryPoolEXT                        = nullptr;
    PFN_vkWaitSemaphores                           mFnWaitSemaphores                           = nullptr;
    PFN_vkSignalSemaphore                          mFnSignalSemaphore                          = nullptr;
    PFN_vkGetSemaphoreCounterValue                 mFnGetSemaphoreCounterValue                 = nullptr;
    uint32_t                                       mGraphicsQueueFamilyIndex                   = 0;
    uint32_t                                       mComputeQueueFamilyIndex                    = 0;
    uint32_t                                       mTransferQueueFamilyIndex                   = 0;
    uint32_t                                       mMaxPushDescriptors                         = 0;
    PFN_vkGetPhysicalDeviceFeatures2               mFnGetPhysicalDeviceFeatures2               = nullptr;
    PFN_vkGetPhysicalDeviceProperties2             mFnGetPhysicalDeviceProperties2             = nullptr;
    PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR mFnGetPhysicalDeviceFragmentShadingRatesKHR = nullptr;
};

extern PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR;

#if defined(VK_KHR_dynamic_rendering)
extern PFN_vkCmdBeginRenderingKHR CmdBeginRenderingKHR;
extern PFN_vkCmdEndRenderingKHR   CmdEndRenderingKHR;
#endif

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_device_h
