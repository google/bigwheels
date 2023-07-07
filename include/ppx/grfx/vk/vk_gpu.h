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

#ifndef ppx_grfx_vk_gpu_h
#define ppx_grfx_vk_gpu_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_gpu.h"

namespace ppx {
namespace grfx {
namespace vk {

class Gpu
    : public grfx::Gpu
{
public:
    Gpu() {}
    virtual ~Gpu() {}

    VkPhysicalDevicePtr GetVkGpu() const { return mGpu; }

    const VkPhysicalDeviceLimits& GetLimits() const { return mGpuProperties.limits; }

    float GetTimestampPeriod() const;

    uint32_t GetQueueFamilyCount() const;

    uint32_t GetGraphicsQueueFamilyIndex() const;
    uint32_t GetComputeQueueFamilyIndex() const;
    uint32_t GetTransferQueueFamilyIndex() const;

    virtual uint32_t GetGraphicsQueueCount() const override;
    virtual uint32_t GetComputeQueueCount() const override;
    virtual uint32_t GetTransferQueueCount() const override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::GpuCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    VkPhysicalDevicePtr                  mGpu;
    VkPhysicalDeviceProperties           mGpuProperties;
    VkPhysicalDeviceFeatures             mGpuFeatures;
    std::vector<VkQueueFamilyProperties> mQueueFamilies;
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_gpu_h
