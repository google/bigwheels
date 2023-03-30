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

#include "ppx/ppx.h"

using namespace ppx;

class OITDemoApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    enum Algorithm : int32_t
    {
        ALGORITHM_UNSORTED_OVER,
        ALGORITHM_WEIGHTED_SUM,
        ALGORITHM_WEIGHTED_AVERAGE,
        ALGORITHM_WEIGHTED_AVERAGE_WITH_COVERAGE,
        ALGORITHMS_COUNT,
    };

    enum FaceMode : int32_t
    {
        FACE_MODE_ALL,
        FACE_MODE_ALL_BACK_THEN_FRONT,
        FACE_MODE_BACK_ONLY,
        FACE_MODE_FRONT_ONLY,
        FACE_MODES_COUNT,
    };

    struct GuiParameters
    {
        GuiParameters()
            : meshOpacity(1.0f), algorithmDataIndex(0), faceMode(FACE_MODE_ALL), displayBackground(true)
        {
            backgroundColor[0] = 0.51f;
            backgroundColor[1] = 0.71f;
            backgroundColor[2] = 0.85f;
        }

        float    meshOpacity;
        int32_t  algorithmDataIndex;
        FaceMode faceMode;
        float    backgroundColor[3];
        bool     displayBackground;
    };

    std::vector<const char*> mSupportedAlgorithmNames;
    std::vector<Algorithm>   mSupportedAlgorithmIds;

private:
    void SetupCommon();

    void SetupUnsortedOver();
    void SetupWeightedSum();
    void SetupWeightedAverage();
    void SetupWeightedAverageWithCoverage();

    void      FillSupportedAlgorithmData();
    void      AddSupportedAlgorithm(const char* name, Algorithm algorithm);
    Algorithm GetSelectedAlgorithm() const;

    void Update();

    void RecordOpaque();
    void RecordUnsortedOver();
    void RecordWeightedSum();
    void RecordWeightedAverage();
    void RecordWeightedAverageWithCoverage();
    void RecordTransparency();
    void RecordComposite(grfx::RenderPassPtr renderPass);

private:
    GuiParameters mGuiParameters;

    grfx::SemaphorePtr mImageAcquiredSemaphore;
    grfx::FencePtr     mImageAcquiredFence;
    grfx::SemaphorePtr mRenderCompleteSemaphore;
    grfx::FencePtr     mRenderCompleteFence;

    grfx::CommandBufferPtr  mCommandBuffer;
    grfx::DescriptorPoolPtr mDescriptorPool;

    grfx::MeshPtr mBackgroundMesh;
    grfx::MeshPtr mMonkeyMesh;

    grfx::BufferPtr mShaderGlobalsBuffer;

    grfx::DrawPassPtr            mOpaquePass;
    grfx::DescriptorSetLayoutPtr mOpaqueDescriptorSetLayout;
    grfx::DescriptorSetPtr       mOpaqueDescriptorSet;
    grfx::PipelineInterfacePtr   mOpaquePipelineInterface;
    grfx::GraphicsPipelinePtr    mOpaquePipeline;

    grfx::TexturePtr  mTransparencyTexture;
    grfx::DrawPassPtr mTransparencyPass;

    grfx::DescriptorSetLayoutPtr mCompositeDescriptorSetLayout;
    grfx::DescriptorSetPtr       mCompositeDescriptorSet;
    grfx::PipelineInterfacePtr   mCompositePipelineInterface;
    grfx::GraphicsPipelinePtr    mCompositePipeline;

    struct
    {
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::DescriptorSetPtr       descriptorSet;

        grfx::PipelineInterfacePtr pipelineInterface;
        grfx::GraphicsPipelinePtr  meshAllFacesPipeline;
        grfx::GraphicsPipelinePtr  meshBackFacesPipeline;
        grfx::GraphicsPipelinePtr  meshFrontFacesPipeline;
    } mUnsortedOver;

    struct
    {
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::DescriptorSetPtr       descriptorSet;

        grfx::PipelineInterfacePtr pipelineInterface;
        grfx::GraphicsPipelinePtr  pipeline;
    } mWeightedSum;

    struct
    {
        grfx::TexturePtr  colorTexture;
        grfx::TexturePtr  countTexture;
        grfx::DrawPassPtr gatherPass;

        grfx::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
        grfx::DescriptorSetPtr       gatherDescriptorSet;
        grfx::PipelineInterfacePtr   gatherPipelineInterface;
        grfx::GraphicsPipelinePtr    gatherPipeline;

        grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
        grfx::DescriptorSetPtr       combineDescriptorSet;
        grfx::PipelineInterfacePtr   combinePipelineInterface;
        grfx::GraphicsPipelinePtr    combinePipeline;
    } mWeightedAverage;

    struct
    {
        grfx::TexturePtr  colorTexture;
        grfx::TexturePtr  coverageTexture;
        grfx::DrawPassPtr gatherPass;

        grfx::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
        grfx::DescriptorSetPtr       gatherDescriptorSet;
        grfx::PipelineInterfacePtr   gatherPipelineInterface;
        grfx::GraphicsPipelinePtr    gatherPipeline;

        grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
        grfx::DescriptorSetPtr       combineDescriptorSet;
        grfx::PipelineInterfacePtr   combinePipelineInterface;
        grfx::GraphicsPipelinePtr    combinePipeline;
    } mWeightedAverageWithCoverage;
};
