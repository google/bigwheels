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

#ifndef ppx_grfx_dx12_descriptor_h
#define ppx_grfx_dx12_descriptor_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_descriptor.h"

// *** Graphics API Note ***
//
// To keep things simple, aliasing of descriptor binding ranges within
// descriptor set layouts are curently not permitted.
//
// D3D12 limits sampler heap size to 2048
//   See: https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support?redirectedfrom=MSDN
//
// Shader visible vs non-shader visible heaps:
//    See: https://docs.microsoft.com/en-us/windows/win32/direct3d12/non-shader-visible-descriptor-heaps
//
//

namespace ppx {
namespace grfx {
namespace dx12 {

class DescriptorPool
    : public grfx::DescriptorPool
{
public:
    struct Allocation
    {
        const grfx::DescriptorSet* pSet    = nullptr;
        uint32_t                   binding = UINT32_MAX;
        uint32_t                   offset  = 0;
        uint32_t                   count   = 0;
    };

    DescriptorPool() {}
    virtual ~DescriptorPool() {}

    Result AllocateDescriptorSet(uint32_t numDescriptorsCBVSRVUAV, uint32_t numDescriptorsSampler);
    void   FreeDescriptorSet(uint32_t numDescriptorsCBVSRVUAV, uint32_t numDescriptorsSampler);

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorPoolCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    uint32_t mDescriptorCountCBVSRVUAV = 0;
    uint32_t mDescriptorCountSampler   = 0;
    uint32_t mAllocatedCountCBVSRVUAV  = 0;
    uint32_t mAllocatedCountSampler    = 0;
};

// -------------------------------------------------------------------------------------------------

class DescriptorSet
    : public grfx::DescriptorSet
{
public:
    struct HeapOffset
    {
        UINT                        binding          = UINT32_MAX;
        UINT                        offset           = UINT32_MAX;
        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    };

    DescriptorSet() {}
    virtual ~DescriptorSet() {}

    // std::vector<HeapOffset>& GetHeapOffsets() { return mHeapOffsets; }

    UINT GetNumDescriptorsCBVSRVUAV() const { return mNumDescriptorsCBVSRVUAV; }
    UINT GetNumDescriptorsSampler() const { return mNumDescriptorsSampler; }

    typename D3D12DescriptorHeapPtr::InterfaceType* GetHeapCBVSRVUAV() const { return mHeapCBVSRVUAV.Get(); }
    typename D3D12DescriptorHeapPtr::InterfaceType* GetHeapSampler() const { return mHeapSampler.Get(); }

    virtual Result UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites) override;

protected:
    virtual Result CreateApiObjects(const grfx::internal::DescriptorSetCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    UINT                    mNumDescriptorsCBVSRVUAV = 0;
    UINT                    mNumDescriptorsSampler   = 0;
    D3D12DescriptorHeapPtr  mHeapCBVSRVUAV;
    D3D12DescriptorHeapPtr  mHeapSampler;
    std::vector<HeapOffset> mHeapOffsets;
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

    uint32_t GetCountCBVSRVUAV() const { return mCountCBVSRVUAV; }
    uint32_t GetCountSampler() const { return mCountSampler; }

    const std::vector<DescriptorRange>& GetRangesCBVSRVUAV() const { return mRangesCBVSRVUAV; }
    const std::vector<DescriptorRange>& GetRangesSampler() const { return mRangesSampler; }

protected:
    virtual Result CreateApiObjects(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;

private:
    uint32_t                     mCountCBVSRVUAV = 0;
    uint32_t                     mCountSampler   = 0;
    std::vector<DescriptorRange> mRangesCBVSRVUAV;
    std::vector<DescriptorRange> mRangesSampler;
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_descriptor_h
