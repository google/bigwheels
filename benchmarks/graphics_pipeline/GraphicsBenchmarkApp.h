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

#ifndef BENCHMARKS_GRAPHICS_PIPELINE_GRAPHICS_BENCHMARK_APP_H
#define BENCHMARKS_GRAPHICS_PIPELINE_GRAPHICS_BENCHMARK_APP_H

#include "FreeCamera.h"
#include "MultiDimensionalIndexer.h"

#include "ppx/grfx/grfx_config.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/ppx.h"

#include <array>
#include <vector>
#include <unordered_map>

static constexpr uint32_t kMaxSphereInstanceCount  = 3000;
static constexpr uint32_t kSeed                    = 89977;
static constexpr uint32_t kMaxFullscreenQuadsCount = 1000;

static constexpr const char* kShaderBaseDir = "benchmarks/shaders";

static constexpr std::array<const char*, 2> kAvailableVsShaders = {
    "Benchmark_VsSimple",
    "Benchmark_VsAluBound"};

static constexpr std::array<const char*, 3> kAvailablePsShaders = {
    "Benchmark_PsSimple",
    "Benchmark_PsAluBound",
    "Benchmark_PsMemBound"};

enum class SpherePS
{
    SPHERE_PS_SIMPLE,
    SPHERE_PS_ALU_BOUND,
    SPHERE_PS_MEM_BOUND
};

static constexpr std::array<const char*, 2> kAvailableVbFormats = {
    "Low_Precision",
    "High_Precision"};

static constexpr std::array<const char*, 2> kAvailableVertexAttrLayouts = {
    "Interleaved",
    "Position_Planar"};

static constexpr size_t kPipelineCount = kAvailablePsShaders.size() * kAvailableVsShaders.size() * kAvailableVbFormats.size() * kAvailableVertexAttrLayouts.size();

static constexpr std::array<const char*, 3> kAvailableLODs = {
    "LOD_0",
    "LOD_1",
    "LOD_2"};

static constexpr size_t kMeshCount = kAvailableVbFormats.size() * kAvailableVertexAttrLayouts.size() * kAvailableLODs.size();

static constexpr std::array<const char*, 3> kFullscreenQuadsTypes = {
    "Noise",
    "Solid_Color",
    "Texture"};

enum class FullscreenQuadsType
{
    FULLSCREEN_QUADS_TYPE_NOISE,
    FULLSCREEN_QUADS_TYPE_SOLID_COLOR,
    FULLSCREEN_QUADS_TYPE_TEXTURE
};

static constexpr std::array<const char*, 4> kFullscreenQuadsColors = {
    "Red",
    "Blue",
    "Green",
    "White"};

static constexpr std::array<float3, 4> kFullscreenQuadsColorsValues = {
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(1.0f, 1.0f, 1.0f)};

class GraphicsBenchmarkApp
    : public ppx::Application
{
public:
    GraphicsBenchmarkApp()
        : mCamera(float3(0, 0, -5), pi<float>() / 2.0f, pi<float>() / 2.0f) {}
    virtual void InitKnobs() override;
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;
    virtual void KeyDown(ppx::KeyCode key) override;
    virtual void KeyUp(ppx::KeyCode key) override;
    virtual void Render() override;

private:
    struct SkyBoxData
    {
        float4x4 MVP;
    };

    struct SphereData
    {
        float4x4 modelMatrix;                // Transforms object space to world space.
        float4x4 ITModelMatrix;              // Inverse transpose of the ModelMatrix.
        float4   ambient;                    // Object's ambient intensity.
        float4x4 cameraViewProjectionMatrix; // Camera's view projection matrix.
        float4   lightPosition;              // Light's position.
        float4   eyePosition;                // Eye (camera) position.
    };

    struct SceneData
    {
        float4x4 viewProjectionMatrix;
    };

    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
        grfx::QueryPtr         timestampQuery;

        grfx::CommandBufferPtr uiCmd;
        ppx::grfx::FencePtr    uiRenderCompleteFence;

        SceneData sceneData;
    };

    struct Entity
    {
        grfx::MeshPtr                       mesh;
        grfx::BufferPtr                     uniformBuffer;
        grfx::DescriptorSetLayoutPtr        descriptorSetLayout;
        grfx::PipelineInterfacePtr          pipelineInterface;
        grfx::GraphicsPipelinePtr           pipeline;
        std::vector<grfx::DescriptorSetPtr> descriptorSets;
    };

    struct Entity2D
    {
        grfx::BufferPtr                     vertexBuffer;
        grfx::VertexBinding                 vertexBinding;
        grfx::DescriptorSetLayoutPtr        descriptorSetLayout;
        std::vector<grfx::DescriptorSetPtr> descriptorSets;
    };

    struct LOD
    {
        uint32_t    longitudeSegments;
        uint32_t    latitudeSegments;
        std::string name;
    };

    struct SpherePipelineKey
    {
        uint8_t ps;
        uint8_t vs;
        uint8_t vertexFormat;
        uint8_t vertexAttributeLayout;
        bool    enableDepth;
        bool    enableAlphaBlend;

        static_assert(kAvailablePsShaders.size() < (1 << (8 * sizeof(ps))));
        static_assert(kAvailableVsShaders.size() < (1 << (8 * sizeof(vs))));
        static_assert(kAvailableVbFormats.size() < (1 << (8 * sizeof(vertexFormat))));
        static_assert(kAvailableVertexAttrLayouts.size() < (1 << (8 * sizeof(vertexAttributeLayout))));

        bool operator==(const SpherePipelineKey& rhs) const
        {
            return ps == rhs.ps &&
                   vs == rhs.vs &&
                   vertexFormat == rhs.vertexFormat &&
                   vertexAttributeLayout == rhs.vertexAttributeLayout &&
                   enableDepth == rhs.enableDepth &&
                   enableAlphaBlend == rhs.enableAlphaBlend;
        }

        struct Hash
        {
            // Not a good hash function, but good enough.
            size_t operator()(const SpherePipelineKey& key) const
            {
                size_t res = 0;

                res = (res * kAvailablePsShaders.size()) | key.ps;
                res = (res * kAvailableVsShaders.size()) | key.vs;
                res = (res * kAvailableVbFormats.size()) | key.vertexFormat;
                res = (res * kAvailableVertexAttrLayouts.size()) | key.vertexAttributeLayout;
                res = (res << 1) | (key.enableDepth ? 1 : 0);
                res = (res << 1) | (key.enableAlphaBlend ? 1 : 0);
                return res;
            }
        };
    };

private:
    using SpherePipelineMap = std::unordered_map<SpherePipelineKey, grfx::GraphicsPipelinePtr, SpherePipelineKey::Hash>;

    std::vector<PerFrame>             mPerFrame;
    FreeCamera                        mCamera;
    float3                            mLightPosition       = float3(10, 250, 10);
    std::array<bool, TOTAL_KEY_COUNT> mPressedKeys         = {0};
    bool                              mEnableMouseMovement = true;
    uint64_t                          mGpuWorkDuration;
    grfx::SamplerPtr                  mLinearSampler;
    grfx::DescriptorPoolPtr           mDescriptorPool;

    // SkyBox resources
    Entity                mSkyBox;
    grfx::ShaderModulePtr mVSSkyBox;
    grfx::ShaderModulePtr mPSSkyBox;
    grfx::TexturePtr      mSkyBoxTexture;

    // Spheres resources
    Entity                                                        mSphere;
    std::array<grfx::ShaderModulePtr, kAvailableVsShaders.size()> mVsShaders;
    std::array<grfx::ShaderModulePtr, kAvailablePsShaders.size()> mPsShaders;
    grfx::TexturePtr                                              mAlbedoTexture;
    grfx::TexturePtr                                              mNormalMapTexture;
    grfx::TexturePtr                                              mMetalRoughnessTexture;
    grfx::TexturePtr                                              mWhitePixelTexture;
    SpherePipelineMap                                             mPipelines;
    std::array<grfx::MeshPtr, kMeshCount>                         mSphereMeshes;
    std::vector<LOD>                                              mSphereLODs;
    MultiDimensionalIndexer                                       mMeshesIndexer;

    // Fullscreen quads resources
    Entity2D                                                             mFullscreenQuads;
    grfx::ShaderModulePtr                                                mVSQuads;
    grfx::TexturePtr                                                     mQuadsTexture;
    std::array<grfx::GraphicsPipelinePtr, kFullscreenQuadsTypes.size()>  mQuadsPipelines;
    std::array<grfx::PipelineInterfacePtr, kFullscreenQuadsTypes.size()> mQuadsPipelineInterfaces;
    std::array<grfx::ShaderModulePtr, kFullscreenQuadsTypes.size()>      mQuadsPs;

private:
    std::shared_ptr<KnobDropdown<std::string>> pKnobVs;
    std::shared_ptr<KnobDropdown<std::string>> pKnobPs;
    std::shared_ptr<KnobDropdown<std::string>> pKnobLOD;
    std::shared_ptr<KnobDropdown<std::string>> pKnobVbFormat;
    std::shared_ptr<KnobDropdown<std::string>> pKnobVertexAttrLayout;
    std::shared_ptr<KnobSlider<int>>           pSphereInstanceCount;
    std::shared_ptr<KnobSlider<int>>           pDrawCallCount;
    std::shared_ptr<KnobSlider<int>>           pFullscreenQuadsCount;
    std::shared_ptr<KnobDropdown<std::string>> pFullscreenQuadsType;
    std::shared_ptr<KnobDropdown<std::string>> pFullscreenQuadsColor;
    std::shared_ptr<KnobCheckbox>              pFullscreenQuadsSingleRenderpass;
    std::shared_ptr<KnobCheckbox>              pAlphaBlend;
    std::shared_ptr<KnobCheckbox>              pDepthTestWrite;
    std::shared_ptr<KnobCheckbox>              pEnableSkyBox;
    std::shared_ptr<KnobCheckbox>              pEnableSpheres;
    std::shared_ptr<KnobCheckbox>              pAllTexturesTo1x1;

private:
    // =====================================================================
    // SETUP (One-time setup for objects)
    // =====================================================================

    // Setup resources:
    // - Images (images, image views, samplers)
    // - Uniform buffers
    // - Descriptor set layouts
    // - Shaders
    void SetupSkyBoxResources();
    void SetupSphereResources();
    void SetupFullscreenQuadsResources();

    // Setup vertex data:
    // - Geometries (or raw vertices & bindings), meshes
    void SetupSkyBoxMeshes();
    void SetupSphereMeshes();
    void SetupFullscreenQuadsMeshes();

    // Setup pipelines:
    // Note: Pipeline creation can be re-triggered within rendering loop
    void SetupSkyBoxPipelines();
    void SetupSpheresPipelines();
    void SetupFullscreenQuadsPipelines();

    Result CompileSpherePipeline(const SpherePipelineKey& key);

    // Update descriptors
    // Note: Descriptors can be updated within rendering loop
    void UpdateSkyBoxDescriptors();
    void UpdateSphereDescriptors();
    void UpdateFullscreenQuadsDescriptors();

    // =====================================================================
    // RENDERING LOOP (Called every frame)
    // =====================================================================

    // Processing changed state
    void ProcessInput();
    void ProcessKnobs();
    void ProcessQuadsKnobs();

    // Drawing GUI
    void UpdateGUI();
    void DrawExtraInfo();

    // Record this frame's command buffer with multiple renderpasses
    void RecordCommandBuffer(PerFrame& frame, grfx::SwapchainPtr swapchain, uint32_t imageIndex);

    // Records commands to render * in this frame's command buffer, with the current renderpass
    void RecordCommandBufferSkyBox(PerFrame& frame);
    void RecordCommandBufferSpheres(PerFrame& frame);
    void RecordCommandBufferFullscreenQuad(PerFrame& frame, size_t seed);

#if defined(PPX_BUILD_XR)
    // Records and submits commands for UI for XR
    void RecordAndSubmitCommandBufferGUIXR(PerFrame& frame);
#endif

    // =====================================================================
    // UTILITY
    // =====================================================================

    // Loads shader at shaderBaseDir/fileName and creates it at ppShaderModule
    void SetupShader(const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule);

    // Compile or load from cache currently required pipeline.
    grfx::GraphicsPipelinePtr GetSpherePipeline();
};

#endif // BENCHMARKS_GRAPHICS_PIPELINE_GRAPHICS_BENCHMARK_APP_H
