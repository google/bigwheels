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

#include "ppx/grfx/grfx_helper.h"
#include "ppx/util.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// ColorComponentFlags
// -------------------------------------------------------------------------------------------------
ColorComponentFlags ColorComponentFlags::RGBA()
{
    ColorComponentFlags flags = ColorComponentFlags(COLOR_COMPONENT_R | COLOR_COMPONENT_G | COLOR_COMPONENT_B | COLOR_COMPONENT_A);
    return flags;
}

// -------------------------------------------------------------------------------------------------
// ImageUsageFlags
// -------------------------------------------------------------------------------------------------
ImageUsageFlags ImageUsageFlags::SampledImage()
{
    ImageUsageFlags flags = ImageUsageFlags(grfx::IMAGE_USAGE_SAMPLED);
    return flags;
}

// -------------------------------------------------------------------------------------------------
// VertexBinding
// -------------------------------------------------------------------------------------------------
void VertexBinding::SetBinding(uint32_t binding)
{
    mBinding = binding;
    for (auto& elem : mAttributes) {
        elem.binding = binding;
    }
}

void VertexBinding::SetStride(uint32_t stride)
{
    mStride = stride;
}

Result VertexBinding::GetAttribute(uint32_t index, const grfx::VertexAttribute** ppAttribute) const
{
    if (!IsIndexInRange(index, mAttributes)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    *ppAttribute = &mAttributes[index];
    return ppx::SUCCESS;
}

uint32_t VertexBinding::GetAttributeIndex(grfx::VertexSemantic semantic) const
{
    auto it = FindIf(
        mAttributes,
        [semantic](const grfx::VertexAttribute& elem) -> bool {
            bool isMatch = (elem.semantic == semantic);
            return isMatch; });
    if (it == std::end(mAttributes)) {
        return PPX_VALUE_IGNORED;
    }
    uint32_t index = static_cast<uint32_t>(std::distance(std::begin(mAttributes), it));
    return index;
}

VertexBinding& VertexBinding::AppendAttribute(const grfx::VertexAttribute& attribute)
{
    mAttributes.push_back(attribute);

    if (mInputRate == grfx::VertexBinding::kInvalidVertexInputRate) {
        mInputRate = attribute.inputRate;
    }

    // Caluclate offset for inserted attribute
    if (mAttributes.size() > 1) {
        size_t i1 = mAttributes.size() - 1;
        size_t i0 = i1 - 1;
        if (mAttributes[i1].offset == PPX_APPEND_OFFSET_ALIGNED) {
            uint32_t prevOffset    = mAttributes[i0].offset;
            uint32_t prevSize      = grfx::GetFormatDescription(mAttributes[i0].format)->bytesPerTexel;
            mAttributes[i1].offset = prevOffset + prevSize;
        }
    }
    // Size of mAttributes should be 1
    else {
        if (mAttributes[0].offset == PPX_APPEND_OFFSET_ALIGNED) {
            mAttributes[0].offset = 0;
        }
    }

    // Calculate stride
    mStride = 0;
    for (auto& elem : mAttributes) {
        uint32_t size = grfx::GetFormatDescription(elem.format)->bytesPerTexel;
        mStride += size;
    }

    return *this;
}

grfx::VertexBinding& VertexBinding::operator+=(const grfx::VertexAttribute& rhs)
{
    AppendAttribute(rhs);
    return *this;
}

// -------------------------------------------------------------------------------------------------
// VertexDescription
// -------------------------------------------------------------------------------------------------
Result VertexDescription::GetBinding(uint32_t index, const grfx::VertexBinding** ppBinding) const
{
    if (!IsIndexInRange(index, mBindings)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }
    if (!IsNull(ppBinding)) {
        *ppBinding = &mBindings[index];
    }
    return ppx::SUCCESS;
}

const grfx::VertexBinding* VertexDescription::GetBinding(uint32_t index) const
{
    const VertexBinding* ptr = nullptr;
    GetBinding(index, &ptr);
    return ptr;
}

uint32_t VertexDescription::GetBindingIndex(uint32_t binding) const
{
    auto it = FindIf(
        mBindings,
        [binding](const grfx::VertexBinding& elem) -> bool {
            bool isMatch = (elem.GetBinding() == binding);
            return isMatch; });
    if (it == std::end(mBindings)) {
        return PPX_VALUE_IGNORED;
    }
    uint32_t index = static_cast<uint32_t>(std::distance(std::begin(mBindings), it));
    return index;
}

Result VertexDescription::AppendBinding(const grfx::VertexBinding& binding)
{
    auto it = FindIf(
        mBindings,
        [binding](const grfx::VertexBinding& elem) -> bool {
            bool isSame = (elem.GetBinding() == binding.GetBinding());
            return isSame; });
    if (it != std::end(mBindings)) {
        return ppx::ERROR_DUPLICATE_ELEMENT;
    }
    mBindings.push_back(binding);
    return ppx::SUCCESS;
}

} // namespace grfx
} // namespace ppx
