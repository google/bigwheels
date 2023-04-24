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

#include "shaders/Common.hlsli"

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
        ALGORITHM_DEPTH_PEELING,
        ALGORITHM_BUFFER,
        ALGORITHMS_COUNT,
    };

    enum MeshType : int32_t
    {
        MESH_TYPE_MONKEY,
        MESH_TYPE_HORSE,
        MESH_TYPE_MEGAPHONE,
        MESH_TYPE_CANNON,
        MESH_TYPES_COUNT,
    };

    enum FaceMode : int32_t
    {
        FACE_MODE_ALL,
        FACE_MODE_ALL_BACK_THEN_FRONT,
        FACE_MODE_BACK_ONLY,
        FACE_MODE_FRONT_ONLY,
        FACE_MODES_COUNT,
    };

    enum WeightAverageType : int32_t
    {
        WEIGHTED_AVERAGE_TYPE_FRAGMENT_COUNT,
        WEIGHTED_AVERAGE_TYPE_EXACT_COVERAGE,
        WEIGHTED_AVERAGE_TYPES_COUNT,
    };

    enum BufferAlgorithmType : int32_t
    {
        BUFFER_ALGORITHM_BUCKETS,
        BUFFER_ALGORITHM_LINKED_LISTS,
        BUFFER_ALGORITHMS_COUNT,
    };

    struct GuiParameters
    {
        int32_t algorithmDataIndex;

        struct
        {
            float color[3];
            bool  display;
        } background;

        struct
        {
            MeshType type;
            float    opacity;
            float    scale;
            bool     auto_rotate;
        } mesh;

        struct
        {
            FaceMode faceMode;
        } unsortedOver;

        struct
        {
            WeightAverageType type;
        } weightedAverage;

        struct
        {
            int32_t startLayer;
            int32_t layersCount;
        } depthPeeling;

        struct
        {
            BufferAlgorithmType type;
            int32_t             bucketsFragmentsMaxCount;
            int32_t             listsFragmentBufferScale;
            int32_t             listsSortedFragmentMaxCount;
        } buffer;
    };

    std::vector<const char*> mSupportedAlgorithmNames;
    std::vector<Algorithm>   mSupportedAlgorithmIds;

private:
    void SetupCommon();
    void SetupUnsortedOver();
    void SetupWeightedSum();
    void SetupWeightedAverage();
    void SetupDepthPeeling();
    void SetupBuffer();
    void SetupBufferBuckets();
    void SetupBufferLinkedLists();

    void FillSupportedAlgorithmData();
    void ParseCommandLineOptions();

    Algorithm     GetSelectedAlgorithm() const;
    grfx::MeshPtr GetTransparentMesh() const;

    void Update();
    void UpdateGUI();

    void RecordOpaque();
    void RecordTransparency();
    void RecordComposite(grfx::RenderPassPtr renderPass);

    void RecordUnsortedOver();
    void RecordWeightedSum();
    void RecordWeightedAverage();
    void RecordDepthPeeling();
    void RecordBuffer();
    void RecordBufferBuckets();
    void RecordBufferLinkedLists();

private:
    GuiParameters mGuiParameters = {};

    float mPreviousElapsedSeconds;
    float mMeshAnimationSeconds;

    grfx::SemaphorePtr mImageAcquiredSemaphore;
    grfx::FencePtr     mImageAcquiredFence;
    grfx::SemaphorePtr mRenderCompleteSemaphore;
    grfx::FencePtr     mRenderCompleteFence;

    grfx::CommandBufferPtr  mCommandBuffer;
    grfx::DescriptorPoolPtr mDescriptorPool;

    grfx::SamplerPtr mNearestSampler;

    grfx::MeshPtr mBackgroundMesh;
    grfx::MeshPtr mTransparentMeshes[MESH_TYPES_COUNT];

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
        grfx::TexturePtr colorTexture;
        grfx::TexturePtr extraTexture;

        grfx::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
        grfx::DescriptorSetPtr       gatherDescriptorSet;
        grfx::PipelineInterfacePtr   gatherPipelineInterface;

        grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
        grfx::DescriptorSetPtr       combineDescriptorSet;
        grfx::PipelineInterfacePtr   combinePipelineInterface;

        struct
        {
            grfx::DrawPassPtr         gatherPass;
            grfx::GraphicsPipelinePtr gatherPipeline;

            grfx::GraphicsPipelinePtr combinePipeline;
        } count;

        struct
        {
            grfx::DrawPassPtr         gatherPass;
            grfx::GraphicsPipelinePtr gatherPipeline;

            grfx::GraphicsPipelinePtr combinePipeline;
        } coverage;
    } mWeightedAverage;

    struct
    {
        grfx::TexturePtr  layerTextures[DEPTH_PEELING_LAYERS_COUNT];
        grfx::TexturePtr  depthTextures[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
        grfx::DrawPassPtr layerPasses[DEPTH_PEELING_LAYERS_COUNT];

        grfx::DescriptorSetLayoutPtr layerDescriptorSetLayout;
        grfx::DescriptorSetPtr       layerDescriptorSets[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
        grfx::PipelineInterfacePtr   layerPipelineInterface;
        grfx::GraphicsPipelinePtr    layerPipeline_OtherLayers;
        grfx::GraphicsPipelinePtr    layerPipeline_FirstLayer;

        grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
        grfx::DescriptorSetPtr       combineDescriptorSet;
        grfx::PipelineInterfacePtr   combinePipelineInterface;
        grfx::GraphicsPipelinePtr    combinePipeline;
    } mDepthPeeling;

    struct
    {
        struct
        {
            grfx::TexturePtr  countTexture;
            grfx::TexturePtr  fragmentTexture;
            grfx::DrawPassPtr clearPass;
            grfx::DrawPassPtr gatherPass;

            grfx::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
            grfx::DescriptorSetPtr       gatherDescriptorSet;
            grfx::PipelineInterfacePtr   gatherPipelineInterface;
            grfx::GraphicsPipelinePtr    gatherPipeline;

            grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
            grfx::DescriptorSetPtr       combineDescriptorSet;
            grfx::PipelineInterfacePtr   combinePipelineInterface;
            grfx::GraphicsPipelinePtr    combinePipeline;

            bool countTextureNeedClear;
        } buckets;

        struct
        {
            grfx::TexturePtr  linkedListHeadTexture;
            grfx::BufferPtr   fragmentBuffer;
            grfx::BufferPtr   atomicCounter;
            grfx::DrawPassPtr clearPass;
            grfx::DrawPassPtr gatherPass;

            grfx::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
            grfx::DescriptorSetPtr       gatherDescriptorSet;
            grfx::PipelineInterfacePtr   gatherPipelineInterface;
            grfx::GraphicsPipelinePtr    gatherPipeline;

            grfx::DescriptorSetLayoutPtr combineDescriptorSetLayout;
            grfx::DescriptorSetPtr       combineDescriptorSet;
            grfx::PipelineInterfacePtr   combinePipelineInterface;
            grfx::GraphicsPipelinePtr    combinePipeline;

            bool linkedListHeadTextureNeedClear;
        } lists;
    } mBuffer;
};
