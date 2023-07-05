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

#include "ppx/grfx/dx12/dx12_descriptor.h"
#include "ppx/grfx/dx12/dx12_buffer.h"
#include "ppx/grfx/dx12/dx12_device.h"
#include "ppx/grfx/dx12/dx12_image.h"

// *** Graphics API Note ***
//
// D3D12 doesn't have the following descriptor types:
//   DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
//   DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
//   DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
//   DESCRIPTOR_TYPE_INPUT_ATTACHMENT
//

namespace ppx {
namespace grfx {
namespace dx12 {

const uint32_t END_OF_HEAP_BINDING_ID = UINT32_MAX;

// -------------------------------------------------------------------------------------------------
// DescriptorPool
// -------------------------------------------------------------------------------------------------
Result DescriptorPool::CreateApiObjects(const grfx::DescriptorPoolCreateInfo* pCreateInfo)
{
    bool hasCombinedImageSampler = (pCreateInfo->combinedImageSampler > 0);
    bool hasUniformBufferDynamic = (pCreateInfo->uniformBufferDynamic > 0);
    bool hasStorageBufferDynamic = (pCreateInfo->storageBufferDynamic > 0);
    bool hasInputAtachment       = (pCreateInfo->inputAttachment > 0);
    bool hasUnsupported          = hasCombinedImageSampler || hasUniformBufferDynamic || hasStorageBufferDynamic || hasInputAtachment;
    if (hasUnsupported) {
        return ppx::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
    }

    // Get totals for each descriptor type
    mDescriptorCountCBVSRVUAV = pCreateInfo->sampledImage +
                                pCreateInfo->storageImage +
                                pCreateInfo->uniformTexelBuffer +
                                pCreateInfo->storageTexelBuffer +
                                pCreateInfo->uniformBuffer +
                                pCreateInfo->rawStorageBuffer +
                                pCreateInfo->structuredBuffer;
    mDescriptorCountSampler = pCreateInfo->sampler;

    // Paranoid zero out these values
    mAllocatedCountCBVSRVUAV = 0;
    mAllocatedCountSampler   = 0;

    return ppx::SUCCESS;
}

void DescriptorPool::DestroyApiObjects()
{
    mDescriptorCountCBVSRVUAV = 0;
    mDescriptorCountSampler   = 0;
    mAllocatedCountCBVSRVUAV  = 0;
    mAllocatedCountSampler    = 0;
}

Result DescriptorPool::AllocateDescriptorSet(uint32_t numDescriptorsCBVSRVUAV, uint32_t numDescriptorsSampler)
{
    if (numDescriptorsCBVSRVUAV > 0) {
        int32_t remaining = static_cast<int32_t>(mDescriptorCountCBVSRVUAV) - static_cast<int32_t>(mAllocatedCountCBVSRVUAV);
        if (remaining < static_cast<int32_t>(numDescriptorsCBVSRVUAV)) {
            PPX_ASSERT_MSG(false, "out of descriptors (CBRSRVUAV)");
            return ppx::ERROR_OUT_OF_MEMORY;
        }
    }

    if (numDescriptorsSampler > 0) {
        int32_t remaining = static_cast<int32_t>(mDescriptorCountSampler) - static_cast<int32_t>(mAllocatedCountSampler);
        if (remaining < static_cast<int32_t>(numDescriptorsSampler)) {
            PPX_ASSERT_MSG(false, "out of descriptors (Sampler)");
            return ppx::ERROR_OUT_OF_MEMORY;
        }
    }

    mAllocatedCountCBVSRVUAV += numDescriptorsCBVSRVUAV;
    mAllocatedCountSampler += numDescriptorsSampler;

    return ppx::SUCCESS;
}

void DescriptorPool::FreeDescriptorSet(uint32_t numDescriptorsCBVSRVUAV, uint32_t numDescriptorsSampler)
{
    int32_t remaining = static_cast<int32_t>(mAllocatedCountCBVSRVUAV) - static_cast<int32_t>(numDescriptorsCBVSRVUAV);
    if (remaining < 0) {
        // Isn't suppose to ever happen
        PPX_ASSERT_MSG(false, "pool is freeing more descriptors than it containers (CBVSRVUAV)");
    }

    remaining = static_cast<int32_t>(mAllocatedCountSampler) - static_cast<int32_t>(numDescriptorsSampler);
    if (remaining < 0) {
        // Isn't suppose to ever happen
        PPX_ASSERT_MSG(false, "pool is freeing more descriptors than it containers (CBVSRVUAV)");
    }

    mAllocatedCountCBVSRVUAV -= numDescriptorsCBVSRVUAV;
    mAllocatedCountSampler -= numDescriptorsSampler;
}

// -------------------------------------------------------------------------------------------------
// DescriptorSet
// -------------------------------------------------------------------------------------------------
Result DescriptorSet::CreateApiObjects(const grfx::internal::DescriptorSetCreateInfo* pCreateInfo)
{
    // Check to see if the pool has available descriptors for each type left. This
    // is to keep alignment with Vulkan, it may not be necessary at all.
    //
    uint32_t numDescriptorsCBVSRVUAV = ToApi(pCreateInfo->pLayout)->GetCountCBVSRVUAV();
    uint32_t numDescriptorsSampler   = ToApi(pCreateInfo->pLayout)->GetCountSampler();

    Result ppxres = ToApi(pCreateInfo->pPool)->AllocateDescriptorSet(numDescriptorsCBVSRVUAV, numDescriptorsSampler);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Get D3D12 device
    D3D12DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    // Heap sizes
    mNumDescriptorsCBVSRVUAV = static_cast<UINT>(numDescriptorsCBVSRVUAV);
    mNumDescriptorsSampler   = static_cast<UINT>(numDescriptorsSampler);

    // Allocate CBVSRVUAV heap
    if (mNumDescriptorsCBVSRVUAV > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors             = mNumDescriptorsCBVSRVUAV;
        desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask                   = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeapCBVSRVUAV));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateDescriptorHeap(CBVSRVUAV) failed");
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(D3D12DescriptorHeap(CBVSRVUAV), mHeapCBVSRVUAV.Get());
    }

    // Allocate Sampler heap
    if (mNumDescriptorsSampler > 0) {
        // CPU
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors             = mNumDescriptorsSampler;
        desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask                   = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeapSampler));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateDescriptorHeap(Sampler) failed");
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(D3D12DescriptorHeap(Sampler), mHeapSampler.Get());
    }

    // Build CBVSRVUAV offsets
    if (mNumDescriptorsCBVSRVUAV > 0) {
        UINT                        incrementSize = ToApi(GetDevice())->GetHandleIncrementSizeCBVSRVUAV();
        D3D12_CPU_DESCRIPTOR_HANDLE heapStart     = mHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
        UINT                        offset        = 0;

        auto& ranges = ToApi(pCreateInfo->pLayout)->GetRangesCBVSRVUAV();
        for (size_t i = 0; i < ranges.size(); ++i) {
            const dx12::DescriptorSetLayout::DescriptorRange& range = ranges[i];

            // Fill out heap offset
            HeapOffset heapOffset           = {};
            heapOffset.binding              = range.binding;
            heapOffset.offset               = offset;
            heapOffset.descriptorHandle.ptr = heapStart.ptr + static_cast<SIZE_T>(offset * incrementSize);

            // Store heap offset
            mHeapOffsets.push_back(heapOffset);

            // Increment offset by the range count
            offset += static_cast<UINT>(range.count);
        }
    }

    // Build Sampler offsets
    if (mNumDescriptorsSampler > 0) {
        UINT                        incrementSize = ToApi(GetDevice())->GetHandleIncrementSizeSampler();
        D3D12_CPU_DESCRIPTOR_HANDLE heapStart     = mHeapSampler->GetCPUDescriptorHandleForHeapStart();
        UINT                        offset        = 0;

        auto& ranges = ToApi(pCreateInfo->pLayout)->GetRangesSampler();
        for (size_t i = 0; i < ranges.size(); ++i) {
            const dx12::DescriptorSetLayout::DescriptorRange& range = ranges[i];

            // Fill out heap offset
            HeapOffset heapOffset           = {};
            heapOffset.binding              = range.binding;
            heapOffset.offset               = offset;
            heapOffset.descriptorHandle.ptr = heapStart.ptr + static_cast<SIZE_T>(offset * incrementSize);

            // Store heap offset
            mHeapOffsets.push_back(heapOffset);

            // Increment offset by the range count
            offset += static_cast<UINT>(range.count);
        }
    }

    return ppx::SUCCESS;
}

void DescriptorSet::DestroyApiObjects()
{
    ToApi(mCreateInfo.pPool)->FreeDescriptorSet(mNumDescriptorsCBVSRVUAV, mNumDescriptorsSampler);

    mNumDescriptorsCBVSRVUAV = 0;
    mNumDescriptorsSampler   = 0;

    mHeapOffsets.clear();

    if (mHeapCBVSRVUAV) {
        mHeapCBVSRVUAV.Reset();
    }

    if (mHeapSampler) {
        mHeapSampler.Reset();
    }
}

Result DescriptorSet::UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites)
{
    // Check descriptor types
    for (uint32_t writeIndex = 0; writeIndex < writeCount; ++writeIndex) {
        const grfx::WriteDescriptor& srcWrite               = pWrites[writeIndex];
        bool                         isCombinedImageSampler = (srcWrite.type == grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        bool                         isUniformBufferDynamic = (srcWrite.type == grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        bool                         isStorageBufferDynamic = (srcWrite.type == grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
        bool                         isInputAtachment       = (srcWrite.type == grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        bool                         isUnsupported          = isCombinedImageSampler || isUniformBufferDynamic || isStorageBufferDynamic || isInputAtachment;
        if (isUnsupported) {
            return ppx::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
        }
    }

    UINT           handleIncSizeCBVSRVUAV = ToApi(GetDevice())->GetHandleIncrementSizeCBVSRVUAV();
    UINT           handleIncSizeSampler   = ToApi(GetDevice())->GetHandleIncrementSizeSampler();
    D3D12DevicePtr device                 = ToApi(GetDevice())->GetDxDevice();

    for (uint32_t writeIndex = 0; writeIndex < writeCount; ++writeIndex) {
        const grfx::WriteDescriptor& srcWrite = pWrites[writeIndex];

        // Find heap offset
        auto it = FindIf(mHeapOffsets, [srcWrite](const HeapOffset& elem) -> bool { return elem.binding == srcWrite.binding; });
        if (it == std::end(mHeapOffsets)) {
            PPX_ASSERT_MSG(false, "attempted to update binding " << srcWrite.binding << " at array index " << srcWrite.arrayIndex << " but binding is not in set");
            return ppx::ERROR_GRFX_BINDING_NOT_IN_SET;
        }
        const HeapOffset& heapOffset = *it;

        switch (srcWrite.type) {
            default: {
                // This shouldn't happen unless there's a supported descriptor
                // type that isn't handled below.
                PPX_ASSERT_MSG(false, "unknown descriptor type: " << ToString(srcWrite.type) << "(" << srcWrite.type << ")");
                return ppx::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
            } break;

            case grfx::DESCRIPTOR_TYPE_SAMPLER: {
                const D3D12_SAMPLER_DESC& desc = ToApi(srcWrite.pSampler)->GetDesc();

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeSampler * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateSampler(&desc, handle);
            } break;

            case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
                const dx12::SampledImageView*          pView = static_cast<const dx12::SampledImageView*>(srcWrite.pImageView);
                const D3D12_SHADER_RESOURCE_VIEW_DESC& desc  = pView->GetDesc();

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateShaderResourceView(ToApi(pView->GetImage())->GetDxResource(), &desc, handle);
            } break;

            case grfx::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
                desc.Format                          = DXGI_FORMAT_UNKNOWN;
                desc.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
                desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Buffer.FirstElement             = 0;
                desc.Buffer.NumElements              = static_cast<UINT>(srcWrite.structuredElementCount);
                desc.Buffer.StructureByteStride      = static_cast<UINT>(srcWrite.pBuffer->GetStructuredElementStride());
                desc.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAG_NONE;

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateShaderResourceView(ToApi(srcWrite.pBuffer)->GetDxResource(), &desc, handle);
            } break;

            case grfx::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.Format                           = DXGI_FORMAT_UNKNOWN;
                desc.ViewDimension                    = D3D12_UAV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement              = 0;
                desc.Buffer.NumElements               = static_cast<UINT>(srcWrite.structuredElementCount);
                desc.Buffer.StructureByteStride       = static_cast<UINT>(srcWrite.pBuffer->GetStructuredElementStride());
                desc.Buffer.Flags                     = D3D12_BUFFER_UAV_FLAG_NONE;

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateUnorderedAccessView(ToApi(srcWrite.pBuffer)->GetDxResource(), nullptr, &desc, handle);
            } break;

            case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER: {
                uint64_t sizeInBytes = (srcWrite.bufferRange == PPX_WHOLE_SIZE) ? srcWrite.pBuffer->GetSize() : srcWrite.bufferRange;
                PPX_ASSERT_MSG(sizeInBytes % 4 == 0, "Size of storage buffer must be a multiple of 4");
                PPX_ASSERT_MSG(srcWrite.bufferOffset % 4 == 0, "Buffer offset for storage buffer must be a multiple of 4");

                D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
                desc.Format                           = DXGI_FORMAT_R32_TYPELESS;
                desc.ViewDimension                    = D3D12_UAV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement              = srcWrite.bufferOffset / 4;
                desc.Buffer.NumElements               = static_cast<UINT>(sizeInBytes / 4);
                desc.Buffer.StructureByteStride       = 0;
                desc.Buffer.Flags                     = D3D12_BUFFER_UAV_FLAG_RAW;

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateUnorderedAccessView(ToApi(srcWrite.pBuffer)->GetDxResource(), nullptr, &desc, handle);
            } break;

            case grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            case grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                const dx12::StorageImageView*           pView = static_cast<const dx12::StorageImageView*>(srcWrite.pImageView);
                const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc  = pView->GetDesc();

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateUnorderedAccessView(ToApi(pView->GetImage())->GetDxResource(), nullptr, &desc, handle);

            } break;

            case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                D3D12_GPU_VIRTUAL_ADDRESS baseAddress = ToApi(srcWrite.pBuffer)->GetDxResource()->GetGPUVirtualAddress();
                UINT                      sizeInBytes = static_cast<UINT>((srcWrite.bufferRange == PPX_WHOLE_SIZE) ? srcWrite.pBuffer->GetSize() : srcWrite.bufferRange);

                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
                desc.BufferLocation                  = baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(srcWrite.bufferOffset);
                desc.SizeInBytes                     = sizeInBytes;

                SIZE_T                      ptr    = heapOffset.descriptorHandle.ptr + static_cast<SIZE_T>(handleIncSizeCBVSRVUAV * srcWrite.arrayIndex);
                D3D12_CPU_DESCRIPTOR_HANDLE handle = D3D12_CPU_DESCRIPTOR_HANDLE{ptr};

                device->CreateConstantBufferView(&desc, handle);
            } break;
        }
    }

    return ppx::SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// DescriptorSetLayout
// -------------------------------------------------------------------------------------------------
Result DescriptorSetLayout::CreateApiObjects(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo)
{
    // Check descriptor types
    for (size_t i = 0; i < pCreateInfo->bindings.size(); ++i) {
        const grfx::DescriptorBinding& binding = pCreateInfo->bindings[i];

        bool isCombinedImageSampler = (binding.type == grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        bool isUniformBufferDynamic = (binding.type == grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        bool isStorageBufferDynamic = (binding.type == grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
        bool isInputAtachment       = (binding.type == grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        bool isUnsupported          = isCombinedImageSampler || isUniformBufferDynamic || isStorageBufferDynamic || isInputAtachment;
        if (isUnsupported) {
            return ppx::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
        }
    }

    // Build descriptor ranges
    for (size_t i = 0; i < pCreateInfo->bindings.size(); ++i) {
        const grfx::DescriptorBinding& binding = pCreateInfo->bindings[i];

        mCountCBVSRVUAV += (binding.type == grfx::DESCRIPTOR_TYPE_SAMPLER) ? 0 : binding.arrayCount;
        mCountSampler += (binding.type == grfx::DESCRIPTOR_TYPE_SAMPLER) ? binding.arrayCount : 0;

        std::vector<DescriptorRange>* pRanges = (binding.type == grfx::DESCRIPTOR_TYPE_SAMPLER) ? &mRangesSampler : &mRangesCBVSRVUAV;

        DescriptorRange info = {};
        info.binding         = binding.binding;
        info.count           = binding.arrayCount;

        pRanges->push_back(info);
    }

    return ppx::SUCCESS;
}

void DescriptorSetLayout::DestroyApiObjects()
{
    mCountCBVSRVUAV = 0;
    mCountSampler   = 0;
    mRangesCBVSRVUAV.clear();
    mRangesSampler.clear();
}

} // namespace dx12
} // namespace grfx
} // namespace ppx
