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

#include "OITDemoApplication.h"

void OITDemoApp::SetupBufferBuckets()
{
    mBuffer.buckets.countTextureNeedClear = true;

    // Count texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = mTransparencyTexture->GetWidth();
        createInfo.height                          = mTransparencyTexture->GetHeight();
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = grfx::FORMAT_R32_UINT;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.storage         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mBuffer.buckets.countTexture));
    }

    // Fragment texture
    {
        grfx::TextureCreateInfo createInfo = {};
        createInfo.imageType               = grfx::IMAGE_TYPE_2D;
        createInfo.width                   = mBuffer.buckets.countTexture->GetWidth();
        createInfo.height                  = mBuffer.buckets.countTexture->GetHeight() * BUFFER_BUCKET_SIZE_PER_PIXEL;
        createInfo.depth                   = 1;
        createInfo.imageFormat             = grfx::FORMAT_R32G32_UINT;
        createInfo.sampleCount             = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount           = 1;
        createInfo.arrayLayerCount         = 1;
        createInfo.usageFlags.bits.storage = true;
        createInfo.memoryUsage             = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState            = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mBuffer.buckets.fragmentTexture));
    }

    // Clear pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mBuffer.buckets.countTexture->GetWidth();
        createInfo.height                     = mBuffer.buckets.countTexture->GetHeight();
        createInfo.renderTargetCount          = 1;
        createInfo.pRenderTargetImages[0]     = mBuffer.buckets.countTexture->GetImage();
        createInfo.pDepthStencilImage         = nullptr;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.buckets.clearPass));
    }

    // Gather pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mBuffer.buckets.countTexture->GetWidth();
        createInfo.height                     = mBuffer.buckets.countTexture->GetHeight();
        createInfo.renderTargetCount          = 0;
        createInfo.pDepthStencilImage         = nullptr;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.buckets.gatherPass));
    }

    ////////////////////////////////////////
    // Gather
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.buckets.gatherDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.buckets.gatherDescriptorSetLayout, &mBuffer.buckets.gatherDescriptorSet));

        std::array<grfx::WriteDescriptor, 4> writes = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_TEXTURE_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

        writes[2].binding    = CUSTOM_UAV_0_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageView = mBuffer.buckets.countTexture->GetStorageImageView();

        writes[3].binding    = CUSTOM_UAV_1_REGISTER;
        writes[3].arrayIndex = 0;
        writes[3].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[3].pImageView = mBuffer.buckets.fragmentTexture->GetStorageImageView();

        PPX_CHECKED_CALL(mBuffer.buckets.gatherDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.buckets.gatherDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.buckets.gatherPipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsGather.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsGather.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
        gpCreateInfo.VS                                = {VS, "vsmain"};
        gpCreateInfo.PS                                = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount     = 1;
        gpCreateInfo.vertexInputState.bindings[0]      = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                          = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                   = false;
        gpCreateInfo.depthWriteEnable                  = false;
        gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount     = 0;
        gpCreateInfo.pPipelineInterface                = mBuffer.buckets.gatherPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.buckets.gatherPipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    ////////////////////////////////////////
    // Combine
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.buckets.combineDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.buckets.combineDescriptorSetLayout, &mBuffer.buckets.combineDescriptorSet));

        std::array<grfx::WriteDescriptor, 3> writes = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_UAV_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageView = mBuffer.buckets.countTexture->GetStorageImageView();

        writes[2].binding    = CUSTOM_UAV_1_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageView = mBuffer.buckets.fragmentTexture->GetStorageImageView();

        PPX_CHECKED_CALL(mBuffer.buckets.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.buckets.combineDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.buckets.combinePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferBucketsCombine.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS, "vsmain"};
        gpCreateInfo.PS                                 = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 0;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mBuffer.buckets.combinePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.buckets.combinePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::SetupBufferLinkedLists()
{
    // Linked list head texture
    {
        grfx::TextureCreateInfo createInfo         = {};
        createInfo.imageType                       = grfx::IMAGE_TYPE_2D;
        createInfo.width                           = mTransparencyTexture->GetWidth();
        createInfo.height                          = mTransparencyTexture->GetHeight();
        createInfo.depth                           = 1;
        createInfo.imageFormat                     = grfx::FORMAT_R32_UINT;
        createInfo.sampleCount                     = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevelCount                   = 1;
        createInfo.arrayLayerCount                 = 1;
        createInfo.usageFlags.bits.colorAttachment = true;
        createInfo.usageFlags.bits.storage         = true;
        createInfo.memoryUsage                     = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                    = grfx::RESOURCE_STATE_SHADER_RESOURCE;

        PPX_CHECKED_CALL(GetDevice()->CreateTexture(&createInfo, &mBuffer.lists.linkedListHeadTexture));
    }

    const uint32_t fragmentBufferElementCount = mTransparencyTexture->GetWidth() * mTransparencyTexture->GetHeight() * BUFFER_BUFFER_MAX_SCALE;
    const uint32_t fragmentBufferElementSize  = static_cast<uint32_t>(sizeof(uint4));
    const uint32_t fragmentBufferSize         = fragmentBufferElementCount * fragmentBufferElementSize;

    // Fragment buffer
    {
        grfx::BufferCreateInfo bufferCreateInfo           = {};
        bufferCreateInfo.size                             = fragmentBufferSize;
        bufferCreateInfo.structuredElementStride          = fragmentBufferElementSize;
        bufferCreateInfo.usageFlags.bits.structuredBuffer = true;
        bufferCreateInfo.memoryUsage                      = grfx::MEMORY_USAGE_GPU_ONLY;
        bufferCreateInfo.initialState                     = grfx::RESOURCE_STATE_GENERAL;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mBuffer.lists.fragmentBuffer));
    }

    // Atomic counter
    {
        grfx::BufferCreateInfo bufferCreateInfo           = {};
        bufferCreateInfo.size                             = std::max(sizeof(uint), static_cast<size_t>(PPX_MINIMUM_UNIFORM_BUFFER_SIZE));
        bufferCreateInfo.structuredElementStride          = sizeof(uint);
        bufferCreateInfo.usageFlags.bits.structuredBuffer = true;
        bufferCreateInfo.memoryUsage                      = grfx::MEMORY_USAGE_GPU_ONLY;
        bufferCreateInfo.initialState                     = grfx::RESOURCE_STATE_GENERAL;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mBuffer.lists.atomicCounter));
    }

    // Clear pass
    {
        constexpr uint            clearValueUint  = BUFFER_LINKED_LIST_INVALID_INDEX;
        const float               clearValueFloat = *reinterpret_cast<const float*>(&clearValueUint);
        grfx::DrawPassCreateInfo2 createInfo      = {};
        createInfo.width                          = mBuffer.lists.linkedListHeadTexture->GetWidth();
        createInfo.height                         = mBuffer.lists.linkedListHeadTexture->GetHeight();
        createInfo.renderTargetCount              = 1;
        createInfo.pRenderTargetImages[0]         = mBuffer.lists.linkedListHeadTexture->GetImage();
        createInfo.pDepthStencilImage             = nullptr;
        createInfo.renderTargetClearValues[0]     = {clearValueFloat, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.lists.clearPass));
    }

    // Gather pass
    {
        grfx::DrawPassCreateInfo2 createInfo  = {};
        createInfo.width                      = mBuffer.lists.linkedListHeadTexture->GetWidth();
        createInfo.height                     = mBuffer.lists.linkedListHeadTexture->GetHeight();
        createInfo.renderTargetCount          = 0;
        createInfo.pDepthStencilImage         = nullptr;
        createInfo.renderTargetClearValues[0] = {0, 0, 0, 0};
        PPX_CHECKED_CALL(GetDevice()->CreateDrawPass(&createInfo, &mBuffer.lists.gatherPass));
    }

    ////////////////////////////////////////
    // Gather
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_TEXTURE_0_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_2_REGISTER, grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.lists.gatherDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.lists.gatherDescriptorSetLayout, &mBuffer.lists.gatherDescriptorSet));

        grfx::WriteDescriptor writes[5] = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_TEXTURE_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[1].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

        writes[2].binding    = CUSTOM_UAV_0_REGISTER;
        writes[2].arrayIndex = 0;
        writes[2].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageView = mBuffer.lists.linkedListHeadTexture->GetStorageImageView();

        writes[3].binding                = CUSTOM_UAV_1_REGISTER;
        writes[3].type                   = grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER;
        writes[3].bufferOffset           = 0;
        writes[3].bufferRange            = PPX_WHOLE_SIZE;
        writes[3].structuredElementCount = fragmentBufferElementCount;
        writes[3].pBuffer                = mBuffer.lists.fragmentBuffer;

        writes[4].binding                = CUSTOM_UAV_2_REGISTER;
        writes[4].type                   = grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER;
        writes[4].bufferOffset           = 0;
        writes[4].bufferRange            = PPX_WHOLE_SIZE;
        writes[4].structuredElementCount = 1;
        writes[4].pBuffer                = mBuffer.lists.atomicCounter;

        PPX_CHECKED_CALL(mBuffer.lists.gatherDescriptorSet->UpdateDescriptors(sizeof(writes) / sizeof(writes[0]), writes));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.lists.gatherDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.lists.gatherPipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferLinkedListsGather.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferLinkedListsGather.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
        gpCreateInfo.VS                                = {VS, "vsmain"};
        gpCreateInfo.PS                                = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount     = 1;
        gpCreateInfo.vertexInputState.bindings[0]      = GetTransparentMesh()->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                          = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                       = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                          = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                         = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                   = false;
        gpCreateInfo.depthWriteEnable                  = false;
        gpCreateInfo.blendModes[0]                     = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount     = 0;
        gpCreateInfo.pPipelineInterface                = mBuffer.lists.gatherPipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.lists.gatherPipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }

    ////////////////////////////////////////
    // Combine
    ////////////////////////////////////////

    // Descriptor
    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{SHADER_GLOBALS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_0_REGISTER, grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_1_REGISTER, grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{CUSTOM_UAV_2_REGISTER, grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mBuffer.lists.combineDescriptorSetLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mBuffer.lists.combineDescriptorSetLayout, &mBuffer.lists.combineDescriptorSet));

        grfx::WriteDescriptor writes[4] = {};

        writes[0].binding      = SHADER_GLOBALS_REGISTER;
        writes[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].bufferOffset = 0;
        writes[0].bufferRange  = PPX_WHOLE_SIZE;
        writes[0].pBuffer      = mShaderGlobalsBuffer;

        writes[1].binding    = CUSTOM_UAV_0_REGISTER;
        writes[1].arrayIndex = 0;
        writes[1].type       = grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageView = mBuffer.lists.linkedListHeadTexture->GetStorageImageView();

        writes[2].binding                = CUSTOM_UAV_1_REGISTER;
        writes[2].type                   = grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER;
        writes[2].bufferOffset           = 0;
        writes[2].bufferRange            = PPX_WHOLE_SIZE;
        writes[2].structuredElementCount = fragmentBufferElementCount;
        writes[2].pBuffer                = mBuffer.lists.fragmentBuffer;

        writes[3].binding                = CUSTOM_UAV_2_REGISTER;
        writes[3].type                   = grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER;
        writes[3].bufferOffset           = 0;
        writes[3].bufferRange            = PPX_WHOLE_SIZE;
        writes[3].structuredElementCount = 1;
        writes[3].pBuffer                = mBuffer.lists.atomicCounter;

        PPX_CHECKED_CALL(mBuffer.lists.combineDescriptorSet->UpdateDescriptors(sizeof(writes) / sizeof(writes[0]), writes));
    }

    // Pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mBuffer.lists.combineDescriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mBuffer.lists.combinePipelineInterface));

        grfx::ShaderModulePtr VS, PS;
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferLinkedListsCombine.vs", &VS));
        PPX_CHECKED_CALL(CreateShader("oit_demo/shaders", "BufferLinkedListsCombine.ps", &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS, "vsmain"};
        gpCreateInfo.PS                                 = {PS, "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 0;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = false;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
        gpCreateInfo.outputState.depthStencilFormat     = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
        gpCreateInfo.pPipelineInterface                 = mBuffer.lists.combinePipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBuffer.lists.combinePipeline));

        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
    }
}

void OITDemoApp::SetupBuffer()
{
    SetupBufferBuckets();
    SetupBufferLinkedLists();
}

void OITDemoApp::RecordBufferBuckets()
{
    if (mBuffer.buckets.countTextureNeedClear) {
        mCommandBuffer->TransitionImageLayout(
            mBuffer.buckets.clearPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->BeginRenderPass(mBuffer.buckets.clearPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

        mCommandBuffer->SetScissors(mBuffer.buckets.clearPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.buckets.clearPass->GetViewport());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mBuffer.buckets.clearPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);

        mBuffer.buckets.countTextureNeedClear = false;
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->BeginRenderPass(mBuffer.buckets.gatherPass, 0);

        mCommandBuffer->SetScissors(mBuffer.buckets.gatherPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.buckets.gatherPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.buckets.gatherPipelineInterface, 1, &mBuffer.buckets.gatherDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.buckets.gatherPipeline);
        mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
        mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
        mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(mTransparencyPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

        mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
        mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.buckets.combinePipelineInterface, 1, &mBuffer.buckets.combineDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.buckets.combinePipeline);
        mCommandBuffer->Draw(3);

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }
}

void OITDemoApp::RecordBufferLinkedLists()
{
    static bool sListsTextureNeedClear = true;
    if (sListsTextureNeedClear) {
        mCommandBuffer->TransitionImageLayout(
            mBuffer.lists.clearPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->BeginRenderPass(mBuffer.lists.clearPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

        mCommandBuffer->SetScissors(mBuffer.lists.clearPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.lists.clearPass->GetViewport());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mBuffer.lists.clearPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);

        sListsTextureNeedClear = false;
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->BeginRenderPass(mBuffer.lists.gatherPass, 0);

        mCommandBuffer->SetScissors(mBuffer.lists.gatherPass->GetScissor());
        mCommandBuffer->SetViewports(mBuffer.lists.gatherPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.lists.gatherPipelineInterface, 1, &mBuffer.lists.gatherDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.lists.gatherPipeline);
        mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
        mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
        mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }

    {
        mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_GENERAL);
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE);
        mCommandBuffer->BeginRenderPass(mTransparencyPass, grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

        mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
        mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

        mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.lists.combinePipelineInterface, 1, &mBuffer.lists.combineDescriptorSet);
        mCommandBuffer->BindGraphicsPipeline(mBuffer.lists.combinePipeline);
        mCommandBuffer->Draw(3);

        mCommandBuffer->EndRenderPass();
        mCommandBuffer->TransitionImageLayout(
            mTransparencyPass,
            grfx::RESOURCE_STATE_RENDER_TARGET,
            grfx::RESOURCE_STATE_SHADER_RESOURCE,
            grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE,
            grfx::RESOURCE_STATE_SHADER_RESOURCE);
        mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, grfx::RESOURCE_STATE_GENERAL, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }
}

void OITDemoApp::RecordBuffer()
{
    switch (mGuiParameters.buffer.type) {
        case BUFFER_ALGORITHM_BUCKETS: {
            RecordBufferBuckets();
            break;
        }
        case BUFFER_ALGORITHM_LINKED_LISTS: {
            RecordBufferLinkedLists();
            break;
        }
        default: {
            break;
        }
    }
}
