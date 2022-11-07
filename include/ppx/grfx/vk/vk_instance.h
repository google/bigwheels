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

#ifndef ppx_grfx_vk_instance_h
#define ppx_grfx_vk_instance_h

#include "ppx/grfx/vk/vk_config.h"
#include "ppx/grfx/grfx_instance.h"

namespace ppx {
namespace grfx {
namespace vk {

class Instance
    : public grfx::Instance
{
public:
    Instance() {}
    virtual ~Instance() {}

    VkInstancePtr GetVkInstance() const { return mInstance; }

#if defined(PPX_BUILD_XR)
    // Get the graphics binding header for session creation.
    virtual const XrBaseInStructure* XrGetGraphicsBinding() const override;
    virtual bool                     XrIsGraphicsBindingValid() const override;
    virtual void                     XrUpdateDeviceInGraphicsBinding() override;
#endif

protected:
    virtual Result AllocateObject(grfx::Device** ppDevice) override;
    virtual Result AllocateObject(grfx::Gpu** ppGpu) override;
    virtual Result AllocateObject(grfx::Surface** ppSurface) override;

protected:
    virtual Result CreateApiObjects(const grfx::InstanceCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    Result ConfigureLayersAndExtensions(const grfx::InstanceCreateInfo* pCreateInfo);
    Result CreateDebugUtils(const grfx::InstanceCreateInfo* pCreateInfo);
    Result EnumerateAndCreateeGpus();

private:
    std::vector<std::string> mFoundLayers;
    std::vector<std::string> mFoundExtensions;
    std::vector<std::string> mLayers;
    std::vector<std::string> mExtensions;
    VkInstancePtr            mInstance;
    VkDebugUtilsMessengerPtr mMessenger;

    PFN_vkCreateDebugUtilsMessengerEXT  mFnCreateDebugUtilsMessengerEXT  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT mFnDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSubmitDebugUtilsMessageEXT    mFnSubmitDebugUtilsMessageEXT    = nullptr;
#if defined(PPX_BUILD_XR)
    XrGraphicsBindingVulkan2KHR mXrGraphicsBinding;
#endif
};

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_instance_h
