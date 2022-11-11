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

#include "ppx/grfx/dx11/dx11_buffer.h"
#include "ppx/grfx/dx11/dx11_device.h"

namespace ppx {
namespace grfx {
namespace dx11 {

Result Buffer::CreateApiObjects(const grfx::BufferCreateInfo* pCreateInfo)
{
    bool dynamic = pCreateInfo->usageFlags.bits.uniformBuffer ||
                   pCreateInfo->usageFlags.bits.indexBuffer ||
                   pCreateInfo->usageFlags.bits.vertexBuffer;
    mUsage = ToD3D11Usage(pCreateInfo->memoryUsage, dynamic);

    mCpuAccessFlags = 0;
    if (pCreateInfo->memoryUsage == grfx::MEMORY_USAGE_CPU_ONLY) {
        mCpuAccessFlags |= D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }
    else if (pCreateInfo->memoryUsage == grfx::MEMORY_USAGE_CPU_TO_GPU) {
        mCpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        if (!dynamic) {
            mCpuAccessFlags |= D3D11_CPU_ACCESS_READ;
        }
    }
    else if (pCreateInfo->memoryUsage == grfx::MEMORY_USAGE_GPU_TO_CPU) {
        mCpuAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    UINT miscFlags = 0;
    if (pCreateInfo->usageFlags.bits.structuredBuffer) {
        miscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    }
    if (pCreateInfo->usageFlags.bits.storageBuffer) {
        miscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = static_cast<UINT>(pCreateInfo->size);
    desc.Usage               = mUsage;
    desc.BindFlags           = ToD3D11BindFlags(pCreateInfo->usageFlags);
    desc.CPUAccessFlags      = mCpuAccessFlags;
    desc.MiscFlags           = miscFlags;
    desc.StructureByteStride = pCreateInfo->structuredElementStride;

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem                = nullptr;
    initialData.SysMemPitch            = 0;
    initialData.SysMemSlicePitch       = 0;

    D3D11DevicePtr device = ToApi(GetDevice())->GetDxDevice();

    HRESULT hr = device->CreateBuffer(&desc, nullptr, &mBuffer);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    return ppx::SUCCESS;
}

void Buffer::DestroyApiObjects()
{
    if (mBuffer) {
        mBuffer.Reset();
    }
}

D3D11_MAP Buffer::GetMapType() const
{
    D3D11_MAP mapType = InvalidValue<D3D11_MAP>();
    if (mUsage == D3D11_USAGE_DYNAMIC) {
        mapType = D3D11_MAP_WRITE_DISCARD;
    }
    else if (mUsage == D3D11_USAGE_STAGING) {
        if (mCreateInfo.memoryUsage == grfx::MEMORY_USAGE_GPU_TO_CPU) {
            mapType = D3D11_MAP_READ;
        }
        else {
            mapType = D3D11_MAP_READ_WRITE;
        }
    }
    return mapType;
}

Result Buffer::MapMemory(uint64_t offset, void** ppMappedAddress)
{
    D3D11DeviceContextPtr context = ToApi(GetDevice())->GetDxDeviceContext();

    D3D11_MAP mapType = GetMapType();
    if (mapType == InvalidValue<D3D11_MAP>()) {
        PPX_ASSERT_MSG(false, "invalid maptype");
        return ppx::ERROR_API_FAILURE;
    }

    D3D11_MAPPED_SUBRESOURCE mappedSubres = {};
    HRESULT                  hr           = context->Map(mBuffer.Get(), 0, mapType, 0, &mappedSubres);
    if (FAILED(hr)) {
        return ppx::ERROR_API_FAILURE;
    }

    *ppMappedAddress = mappedSubres.pData;

    return ppx::SUCCESS;
}

void Buffer::UnmapMemory()
{
    D3D11DeviceContextPtr context = ToApi(GetDevice())->GetDxDeviceContext();
    context->Unmap(mBuffer.Get(), 0);
}

} // namespace dx11
} // namespace grfx
} // namespace ppx
