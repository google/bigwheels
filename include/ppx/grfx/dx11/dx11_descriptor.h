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

#ifndef ppx_grfx_dx11_descriptor_h
#define ppx_grfx_dx11_descriptor_h

#include "ppx/grfx/dx11/dx11_config.h"
#include "ppx/grfx/grfx_descriptor.h"

namespace ppx {
namespace grfx {
namespace dx11 {

class DescriptorPool
    : public grfx::DescriptorPool
{
public:
    DescriptorPool() {}
    virtual ~DescriptorPool() {}

    uint32_t GetRemainingCountCBV() const { return mRemainingCountCBV; }
    uint32_t GetRemainingCountSRV() const { return mRemainingCountSRV; }
    uint32_t GetRemainingCountUAV() const { return mRemainingCountUAV; }
    uint32_t GetRemainingCountSampler() const { return mRemainingCountSampler; }

    Result AllocateSet(
        uint32_t bindingCountCBV,
        uint32_t bindingCountSRV,
        uint32_t bindingCountUAV,
        uint32_t bindingCountSampler);
    void FreeSet(
        uint32_t bindingCountCBV,
        uint32_t bindingCountSRV,
        uint32_t bindingCountUAV,
        uint32_t bindingCountSampler);

private:
    void UpdateRemainingCount();

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorPoolCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    uint32_t mTotalCountCBV     = 0;
    uint32_t mTotalCountSRV     = 0;
    uint32_t mTotalCountUAV     = 0;
    uint32_t mTotalCountSampler = 0;

    uint32_t mAllocatedCountCBV     = 0;
    uint32_t mAllocatedCountSRV     = 0;
    uint32_t mAllocatedCountUAV     = 0;
    uint32_t mAllocatedCountSampler = 0;

    uint32_t mRemainingCountCBV     = 0;
    uint32_t mRemainingCountSRV     = 0;
    uint32_t mRemainingCountUAV     = 0;
    uint32_t mRemainingCountSampler = 0;
};

// -------------------------------------------------------------------------------------------------

class DescriptorSet
    : public grfx::DescriptorSet
{
public:
    DescriptorSet() {}
    virtual ~DescriptorSet() {}

    const std::vector<dx11::DescriptorArray>& GetDescriptorArrays() const { return mDescriptorArrays; }

    virtual Result UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites) override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::DescriptorSetCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    std::vector<dx11::DescriptorArray> mDescriptorArrays;
};

// -------------------------------------------------------------------------------------------------

class DescriptorSetLayout
    : public grfx::DescriptorSetLayout
{
public:
    struct DescriptorRange
    {
        uint32_t binding = UINT32_MAX;
        uint32_t count   = 0;
    };

    DescriptorSetLayout() {}
    virtual ~DescriptorSetLayout() {}

    uint32_t GetBindingCountCBV() const { return mBindingCountCBV; }
    uint32_t GetBindingCountSRV() const { return mBindingCountSRV; }
    uint32_t GetBindingCountUAV() const { return mBindingCountUAV; }
    uint32_t GetBindingCountSampler() const { return mBindingCountSampler; }

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    uint32_t mBindingCountCBV     = 0;
    uint32_t mBindingCountSRV     = 0;
    uint32_t mBindingCountUAV     = 0;
    uint32_t mBindingCountSampler = 0;
};

} // namespace dx11
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx11_descriptor_h
