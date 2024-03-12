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

#ifndef ppx_grfx_vk_util_h
#define ppx_grfx_vk_util_h

#include "ppx/grfx/vk/vk_config_platform.h"
#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {
namespace vk {

const char* ToString(VkResult value);
const char* ToString(VkDescriptorType value);
const char* ToString(VkPresentModeKHR value);

VkAttachmentLoadOp         ToVkAttachmentLoadOp(grfx::AttachmentLoadOp value);
VkAttachmentStoreOp        ToVkAttachmentStoreOp(grfx::AttachmentStoreOp value);
VkBlendFactor              ToVkBlendFactor(grfx::BlendFactor value);
VkBlendOp                  ToVkBlendOp(grfx::BlendOp value);
VkBorderColor              ToVkBorderColor(grfx::BorderColor value);
VkBufferUsageFlags         ToVkBufferUsageFlags(const grfx::BufferUsageFlags& value);
VkClearColorValue          ToVkClearColorValue(const grfx::RenderTargetClearValue& value);
VkClearDepthStencilValue   ToVkClearDepthStencilValue(const grfx::DepthStencilClearValue& value);
VkColorComponentFlags      ToVkColorComponentFlags(const grfx::ColorComponentFlags& value);
VkCompareOp                ToVkCompareOp(grfx::CompareOp value);
VkComponentSwizzle         ToVkComponentSwizzle(grfx::ComponentSwizzle value);
VkComponentMapping         ToVkComponentMapping(const grfx::ComponentMapping& value);
VkCullModeFlagBits         ToVkCullMode(grfx::CullMode value);
VkDescriptorBindingFlags   ToVkDescriptorBindingFlags(const grfx::DescriptorBindingFlags& value);
VkDescriptorType           ToVkDescriptorType(grfx::DescriptorType value);
VkFilter                   ToVkFilter(grfx::Filter value);
VkFormat                   ToVkFormat(grfx::Format value);
VkFrontFace                ToVkFrontFace(grfx::FrontFace value);
VkImageType                ToVkImageType(grfx::ImageType value);
VkImageUsageFlags          ToVkImageUsageFlags(const grfx::ImageUsageFlags& value);
VkImageViewType            ToVkImageViewType(grfx::ImageViewType value);
VkIndexType                ToVkIndexType(grfx::IndexType value);
VkLogicOp                  ToVkLogicOp(grfx::LogicOp value);
VkPipelineStageFlagBits    ToVkPipelineStage(grfx::PipelineStage value);
VkPolygonMode              ToVkPolygonMode(grfx::PolygonMode value);
VkPresentModeKHR           ToVkPresentMode(grfx::PresentMode value);
VkPrimitiveTopology        ToVkPrimitiveTopology(grfx::PrimitiveTopology value);
VkQueryType                ToVkQueryType(grfx::QueryType value);
VkSamplerAddressMode       ToVkSamplerAddressMode(grfx::SamplerAddressMode value);
VkSamplerMipmapMode        ToVkSamplerMipmapMode(grfx::SamplerMipmapMode value);
VkSampleCountFlagBits      ToVkSampleCount(grfx::SampleCount value);
VkShaderStageFlags         ToVkShaderStageFlags(const grfx::ShaderStageFlags& value);
VkStencilOp                ToVkStencilOp(grfx::StencilOp value);
VkTessellationDomainOrigin ToVkTessellationDomainOrigin(grfx::TessellationDomainOrigin value);
VkVertexInputRate          ToVkVertexInputRate(grfx::VertexInputRate value);

Result ToVkBarrierSrc(
    ResourceState                   state,
    grfx::CommandType               commandType,
    const VkPhysicalDeviceFeatures& features,
    VkPipelineStageFlags&           stageMask,
    VkAccessFlags&                  accessMask,
    VkImageLayout&                  layout);
Result ToVkBarrierDst(
    ResourceState                   state,
    grfx::CommandType               commandType,
    const VkPhysicalDeviceFeatures& features,
    VkPipelineStageFlags&           stageMask,
    VkAccessFlags&                  accessMask,
    VkImageLayout&                  layout);

VkImageAspectFlags DetermineAspectMask(VkFormat format);

VmaMemoryUsage ToVmaMemoryUsage(grfx::MemoryUsage value);

// Inserts nextStruct into the pNext chain of baseStruct.
template <typename TVkStruct1, typename TVkStruct2>
void InsertPNext(TVkStruct1& baseStruct, TVkStruct2& nextStruct)
{
    nextStruct.pNext = baseStruct.pNext;
    baseStruct.pNext = &nextStruct;
}

} // namespace vk
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_vk_util_h
