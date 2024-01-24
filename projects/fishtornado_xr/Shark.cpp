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

#include "Shark.h"
#include "FishTornado.h"
#include "ShaderConfig.h"
#include "ppx/graphics_util.h"

Shark::Shark()
{
}

Shark::~Shark()
{
}

void Shark::Setup(uint32_t numFramesInFlight)
{
    FishTornadoApp*              pApp           = FishTornadoApp::GetThisApp();
    grfx::DevicePtr              device         = pApp->GetDevice();
    grfx::QueuePtr               queue          = pApp->GetGraphicsQueue();
    grfx::DescriptorPoolPtr      pool           = pApp->GetDescriptorPool();
    grfx::DescriptorSetLayoutPtr modelSetLayout = pApp->GetModelDataSetLayout();

    mPerFrame.resize(numFramesInFlight);
    for (uint32_t i = 0; i < numFramesInFlight; ++i) {
        PerFrame& frame = mPerFrame[i];

        PPX_CHECKED_CALL(mPerFrame[i].modelConstants.Create(device, PPX_MINIMUM_CONSTANT_BUFFER_SIZE));

        // Allocate descriptor set
        PPX_CHECKED_CALL(device->AllocateDescriptorSet(pool, modelSetLayout, &frame.modelSet));

        // Update descriptor
        PPX_CHECKED_CALL(frame.modelSet->UpdateUniformBuffer(RENDER_MODEL_DATA_REGISTER, 0, frame.modelConstants.GetGpuBuffer()));
    }

    mForwardPipeline = pApp->CreateForwardPipeline("fishtornado/shaders", "Shark.vs", "Shark.ps");
    mShadowPipeline  = pApp->CreateShadowPipeline("fishtornado/shaders", "SharkShadow.vs");

    TriMeshOptions options = TriMeshOptions().Indices().AllAttributes().InvertTexCoordsV().InvertWinding();
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromFile(queue, pApp->GetAssetPath("fishtornado/models/shark/shark.obj"), &mMesh, options));

    grfx_util::TextureOptions textureOptions = grfx_util::TextureOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
    PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(queue, pApp->GetAssetPath("fishtornado/textures/shark/sharkDiffuse.png"), &mAlbedoTexture, textureOptions));
    PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(queue, pApp->GetAssetPath("fishtornado/textures/shark/sharkRoughness.png"), &mRoughnessTexture, textureOptions));
    PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(queue, pApp->GetAssetPath("fishtornado/textures/shark/sharkNormal.png"), &mNormalMapTexture, textureOptions));

    PPX_CHECKED_CALL(mMaterialConstants.Create(device, PPX_MINIMUM_CONSTANT_BUFFER_SIZE));

    PPX_CHECKED_CALL(device->AllocateDescriptorSet(pool, pApp->GetMaterialSetLayout(), &mMaterialSet));
    PPX_CHECKED_CALL(mMaterialSet->UpdateUniformBuffer(RENDER_MATERIAL_DATA_REGISTER, 0, mMaterialConstants.GetGpuBuffer()));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampledImage(RENDER_ALBEDO_TEXTURE_REGISTER, 0, mAlbedoTexture));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampledImage(RENDER_ROUGHNESS_TEXTURE_REGISTER, 0, mRoughnessTexture));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampledImage(RENDER_NORMAL_MAP_TEXTURE_REGISTER, 0, mNormalMapTexture));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampledImage(RENDER_CAUSTICS_TEXTURE_REGISTER, 0, pApp->GetCausticsTexture()));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampler(RENDER_CLAMPED_SAMPLER_REGISTER, 0, pApp->GetClampedSampler()));
    PPX_CHECKED_CALL(mMaterialSet->UpdateSampler(RENDER_REPEAT_SAMPLER_REGISTER, 0, pApp->GetRepeatSampler()));
}

void Shark::Shutdown()
{
    for (size_t i = 0; i < mPerFrame.size(); ++i) {
        mPerFrame[i].modelConstants.Destroy();
    }

    mMaterialConstants.Destroy();
}

void Shark::Update(uint32_t frameIndex, uint32_t currentViewIndex)
{
    // Update the shark's position for the zeroth frame since this update method is called twice in XR mode.
    if (currentViewIndex == 0) {
        const float t = FishTornadoApp::GetThisApp()->GetTime();

        // Calculate position
        float3 prevPos = mPos;
        mPos.x         = sin(t * -0.0205f) * 100.0f;
        mPos.z         = sin(t * -0.0205f) * 900.0f; // 800.0f
        mPos.y         = 100.0f;
        // mPos.y         = 100.0f * 0.675f + (sin(t * 0.00125f) * 0.5f + 0.5f) * 100.0f * 0.25f;

        // Calculate velocity
        mVel = mPos - prevPos;

        // Find direction of travel
        mDir = normalize(mVel);
    }

    // Calculate rotation matrix for orientation
    quat     q           = glm::rotation(float3(0, 0, 1), mDir);
    float4x4 rotMat      = glm::toMat4(q);
    float4x4 modelMatrix = glm::translate(mPos) * rotMat;

    // Write to CPU constants buffer
    {
        PerFrame& frame = mPerFrame[frameIndex];

        hlsl::ModelData* pData = static_cast<hlsl::ModelData*>(frame.modelConstants.GetMappedAddress());
        pData->modelMatrix     = modelMatrix;
    }
}

void Shark::CopyConstantsToGpu(uint32_t frameIndex, grfx::CommandBuffer* pCmd)
{
    PerFrame& frame = mPerFrame[frameIndex];

    pCmd->BufferResourceBarrier(frame.modelConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_COPY_DST);

    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size                         = frame.modelConstants.GetSize();

    pCmd->CopyBufferToBuffer(
        &copyInfo,
        frame.modelConstants.GetCpuBuffer(),
        frame.modelConstants.GetGpuBuffer());

    pCmd->BufferResourceBarrier(frame.modelConstants.GetGpuBuffer(), grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
}

void Shark::DrawDebug(uint32_t frameIndex, grfx::CommandBuffer* pCmd)
{
    grfx::PipelineInterfacePtr pipelineInterface = FishTornadoApp::GetThisApp()->GetForwardPipelineInterface();
    grfx::GraphicsPipelinePtr  pipeline          = FishTornadoApp::GetThisApp()->GetDebugDrawPipeline();

    grfx::DescriptorSet* sets[2] = {nullptr};
    sets[0]                      = FishTornadoApp::GetThisApp()->GetSceneSet(frameIndex);
    sets[1]                      = mPerFrame[frameIndex].modelSet;

    pCmd->BindGraphicsDescriptorSets(pipelineInterface, 2, sets);

    pCmd->BindGraphicsPipeline(pipeline);

    pCmd->BindIndexBuffer(mMesh);
    pCmd->BindVertexBuffers(mMesh);
    pCmd->DrawIndexed(mMesh->GetIndexCount());
}

void Shark::DrawShadow(uint32_t frameIndex, grfx::CommandBuffer* pCmd)
{
    FishTornadoApp* pApp = FishTornadoApp::GetThisApp();

    grfx::DescriptorSet* sets[2] = {nullptr};
    sets[0]                      = FishTornadoApp::GetThisApp()->GetSceneShadowSet(frameIndex);
    sets[1]                      = mPerFrame[frameIndex].modelSet;

    pCmd->BindGraphicsDescriptorSets(pApp->GetForwardPipelineInterface(), 2, sets);

    pCmd->BindGraphicsPipeline(mShadowPipeline);

    pCmd->BindIndexBuffer(mMesh);
    pCmd->BindVertexBuffers(mMesh);
    pCmd->DrawIndexed(mMesh->GetIndexCount());
}

void Shark::DrawForward(uint32_t frameIndex, grfx::CommandBuffer* pCmd)
{
    grfx::PipelineInterfacePtr pipelineInterface = FishTornadoApp::GetThisApp()->GetForwardPipelineInterface();

    grfx::DescriptorSet* sets[3] = {nullptr};
    sets[0]                      = FishTornadoApp::GetThisApp()->GetSceneSet(frameIndex);
    sets[1]                      = mPerFrame[frameIndex].modelSet;
    sets[2]                      = mMaterialSet;

    pCmd->BindGraphicsDescriptorSets(pipelineInterface, 3, sets);

    pCmd->BindGraphicsPipeline(mForwardPipeline);

    pCmd->BindIndexBuffer(mMesh);
    pCmd->BindVertexBuffers(mMesh);
    pCmd->DrawIndexed(mMesh->GetIndexCount());
}
