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
#include "ppx/grfx/grfx_format.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/ui_util.h"
#include "ppx/ppx.h"
#include "ppx/random.h"

#include <array>
#include <vector>
#include <unordered_map>

static constexpr uint32_t kMaxSphereInstanceCount     = 3000;
static constexpr uint32_t kDefaultSphereInstanceCount = 50;
static constexpr uint32_t kSeed                       = 89977;
static constexpr uint32_t kMaxFullscreenQuadsCount    = 1000;
static constexpr uint32_t kMaxTextureCount            = 10;

static constexpr float4   kDefaultDrawCallColor        = float4(1.0f, 0.175f, 0.365f, 0.5f);
static constexpr uint32_t kDebugColorPushConstantCount = sizeof(float4) / sizeof(uint32_t);

static constexpr const char* kShaderBaseDir   = "benchmarks/shaders";
static constexpr const char* kQuadTextureFile = "benchmarks/textures/tiger.jpg";

enum class DebugView
{
    DISABLED = 0,
    SHOW_DRAWCALLS,
    WIREFRAME_MODE
};

inline const char* ToString(DebugView dv)
{
    switch (dv) {
        case DebugView::DISABLED: return "Disabled";
        case DebugView::SHOW_DRAWCALLS: return "Show Drawcalls";
        case DebugView::WIREFRAME_MODE: return "Wireframe Mode";
        default: return "?";
    }
}

static constexpr std::array<DebugView, 3> kAvailableDebugViews = {
    DebugView::DISABLED,
    DebugView::SHOW_DRAWCALLS,
    DebugView::WIREFRAME_MODE};

enum class SphereVS
{
    SPHERE_VS_SIMPLE = 0,
    SPHERE_VS_ALU_BOUND
};

inline const char* ToString(SphereVS vs)
{
    switch (vs) {
        case SphereVS::SPHERE_VS_SIMPLE: return "Benchmark_VsSimple";
        case SphereVS::SPHERE_VS_ALU_BOUND: return "Benchmark_VsAluBound";
        default: return "?";
    }
}

static constexpr std::array<SphereVS, 2> kAvailableVsShaders = {
    SphereVS::SPHERE_VS_SIMPLE,
    SphereVS::SPHERE_VS_ALU_BOUND};

enum class SpherePS
{
    SPHERE_PS_SIMPLE = 0,
    SPHERE_PS_ALU_BOUND,
    SPHERE_PS_MEM_BOUND
};

inline const char* ToString(SpherePS ps)
{
    switch (ps) {
        case SpherePS::SPHERE_PS_SIMPLE: return "Benchmark_PsSimple";
        case SpherePS::SPHERE_PS_ALU_BOUND: return "Benchmark_PsAluBound";
        case SpherePS::SPHERE_PS_MEM_BOUND: return "Benchmark_PsMemBound";
        default: return "?";
    }
}

static constexpr std::array<SpherePS, 3> kAvailablePsShaders = {
    SpherePS::SPHERE_PS_SIMPLE,
    SpherePS::SPHERE_PS_ALU_BOUND,
    SpherePS::SPHERE_PS_MEM_BOUND};

static constexpr std::array<const char*, 2> kAvailableVbFormats = {
    "Low_Precision",
    "High_Precision"};

static constexpr std::array<const char*, 2> kAvailableVertexAttrLayouts = {
    "Interleaved",
    "Position_Planar"};

static constexpr size_t kPipelineCount = kAvailablePsShaders.size() * kAvailableVsShaders.size() * kAvailableVbFormats.size() * kAvailableVertexAttrLayouts.size();

struct SphereLOD
{
    uint32_t longitudeSegments;
    uint32_t latitudeSegments;
};

static constexpr std::array<DropdownEntry<SphereLOD>, 3> kAvailableLODs = {{
    {"LOD_0", {50, 50}},
    {"LOD_1", {20, 20}},
    {"LOD_2", {10, 10}},
}};

static constexpr size_t kMeshCount = kAvailableVbFormats.size() * kAvailableVertexAttrLayouts.size() * kAvailableLODs.size();

enum class FullscreenQuadsType
{
    FULLSCREEN_QUADS_TYPE_NOISE,
    FULLSCREEN_QUADS_TYPE_SOLID_COLOR,
    FULLSCREEN_QUADS_TYPE_TEXTURE
};

static constexpr std::array<DropdownEntry<FullscreenQuadsType>, 3> kFullscreenQuadsTypes = {{
    {"Noise", FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_NOISE},
    {"Solid_Color", FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_SOLID_COLOR},
    {"Texture", FullscreenQuadsType::FULLSCREEN_QUADS_TYPE_TEXTURE},
}};

static constexpr std::array<DropdownEntry<float3>, 4> kFullscreenQuadsColors = {{
    {"Red", float3(1.0f, 0.0f, 0.0f)},
    {"Blue", float3(0.0f, 0.0f, 1.0f)},
    {"Green", float3(0.0f, 1.0f, 0.0f)},
    {"White", float3(1.0f, 1.0f, 1.0f)},
}};

static constexpr std::array<grfx::Format, 13> kFramebufferFormatTypes = {
    // Some format may not be supported on all devices.
    // We may want to filter based on device capability.
    grfx::Format::FORMAT_R8G8B8A8_UNORM,
    grfx::Format::FORMAT_B8G8R8A8_UNORM,
    grfx::Format::FORMAT_R8G8B8A8_SRGB,
    grfx::Format::FORMAT_B8G8R8A8_SRGB,
    grfx::Format::FORMAT_R16G16B16A16_UNORM,
    grfx::Format::FORMAT_R16G16B16A16_FLOAT,
    grfx::Format::FORMAT_R32G32B32A32_FLOAT,
    grfx::Format::FORMAT_R10G10B10A2_UNORM,
    grfx::Format::FORMAT_R11G11B10_FLOAT,
    grfx::Format::FORMAT_R8_UNORM,
    grfx::Format::FORMAT_R16_UNORM,
    grfx::Format::FORMAT_R8G8_UNORM,
    grfx::Format::FORMAT_R16G16_UNORM,
};

static constexpr std::array<std::pair<int, int>, 1 + 8> kSimpleResolutions = {{
    // 1x1
    {1, 1},
    // 2^n square
    {64, 64},
    {128, 128},
    {256, 256},
    {512, 512},
    {1024, 1024},
    {2048, 2048},
    {4096, 4096},
    {8192, 8192},
}};

static constexpr std::array<std::pair<int, int>, 6 + 5 + 2> kCommonResolutions = {{
    // 16:9 Wide screen
    {1280, 720},  // 720p
    {1920, 1080}, // 1080p
    {2560, 1440}, // 1440p
    {3840, 2160}, // 4K
    {5120, 2880}, // 5K
    {7680, 4320}, // 8K
    // VGA
    {320, 240},  // QVGA
    {480, 320},  // HVGA
    {640, 480},  // VGA
    {800, 600},  // SVGA
    {1024, 768}, // XGA
    // Other common display resolution
    {3840, 1600}, // 4K ultrawide
    {5120, 2160}, // 5K ultrawide
}};

static constexpr std::array<std::pair<int, int>, 8 + 9> kVRPerEyeResolutions = {{
    {720, 720},
    {960, 960},
    {1440, 1440},
    {1920, 1920},
    {2880, 2880},
    {3840, 3840},
    {5760, 5760},
    {7680, 7680},
    {1800, 1920}, // Quest Pro
    {1832, 1920}, // Quest 2
    {2064, 2208}, // Quest 3
    {1440, 1600}, // Valve Index
    {1080, 1200}, // HTC Vive
    {2448, 2448}, // HTC Vive Pro 2
    {960, 1080},  // PlayStation VR
    {2000, 2040}, // PlayStation VR 2
    {3680, 3140}, // Vision Pro, estimation from Wikipedia
}};

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

    struct SkyBoxPipelineKey
    {
        grfx::Format renderFormat;

        bool operator==(const SkyBoxPipelineKey& rhs) const
        {
            return renderFormat == rhs.renderFormat;
        }

        struct Hash
        {
            size_t operator()(const SkyBoxPipelineKey& key) const
            {
                return static_cast<size_t>(key.renderFormat);
            }
        };
    };

    struct QuadPipelineKey
    {
        grfx::Format        renderFormat;
        FullscreenQuadsType quadType;

        bool operator==(const QuadPipelineKey& rhs) const
        {
            return renderFormat == rhs.renderFormat && quadType == rhs.quadType;
        }

        struct Hash
        {
            size_t operator()(const QuadPipelineKey& key) const
            {
                return (static_cast<size_t>(key.renderFormat) * kFullscreenQuadsTypes.size()) | static_cast<size_t>(key.quadType);
            }
        };
    };

    struct SpherePipelineKey
    {
        uint8_t      ps;
        uint8_t      vs;
        uint8_t      vertexFormat;
        uint8_t      vertexAttributeLayout;
        bool         enableDepth;
        bool         enableAlphaBlend;
        grfx::Format renderFormat;
        bool         enablePolygonModeLine;

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
                   enableAlphaBlend == rhs.enableAlphaBlend &&
                   renderFormat == rhs.renderFormat &&
                   enablePolygonModeLine == rhs.enablePolygonModeLine;
        }

        struct Hash
        {
            // Not a good hash function, but good enough.
            size_t operator()(const SpherePipelineKey& key) const
            {
                size_t res = static_cast<size_t>(key.renderFormat);

                res = (res * kAvailablePsShaders.size()) | key.ps;
                res = (res * kAvailableVsShaders.size()) | key.vs;
                res = (res * kAvailableVbFormats.size()) | key.vertexFormat;
                res = (res * kAvailableVertexAttrLayouts.size()) | key.vertexAttributeLayout;
                res = (res << 1) | (key.enableDepth ? 1 : 0);
                res = (res << 1) | (key.enableAlphaBlend ? 1 : 0);
                res = (res << 1) | (key.enablePolygonModeLine ? 1 : 0);
                return res;
            }
        };
    };

    struct SubmissionTime
    {
        double min;
        double max;
        double average;
    };

    struct OffscreenFrame
    {
        uint32_t width;
        uint32_t height;

        grfx::Format colorFormat;
        grfx::Format depthFormat;

        grfx::RenderPassPtr loadRenderPass;
        grfx::RenderPassPtr clearRenderPass;
        grfx::RenderPassPtr noloadRenderPass;

        grfx::RenderTargetViewPtr renderTargetViews[3];
        grfx::DepthStencilViewPtr depthStencilView;
        // The actual image
        grfx::ImagePtr depthImage;
        grfx::ImagePtr colorImage;

        grfx::TexturePtr       blitSource;
        grfx::DescriptorSetPtr blitDescriptorSet;
    };

    struct RenderPasses
    {
        grfx::RenderPassPtr   loadRenderPass;
        grfx::RenderPassPtr   clearRenderPass;
        grfx::RenderPassPtr   noloadRenderPass;
        grfx::RenderPassPtr   uiRenderPass;
        grfx::RenderPassPtr   uiClearRenderPass;
        grfx::RenderPassPtr   blitRenderPass;
        const OffscreenFrame* offscreen = nullptr;
    };

    struct BlitContext
    {
        grfx::ShaderModulePtr vs;
        grfx::ShaderModulePtr ps;

        // Note: the blit implementated here does not perserve aspect ratio.
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::FullscreenQuadPtr      quad;
    };

    // Needs to match with the definition at assets/benchmarks/shaders/Benchmark_Quad.hlsli
    struct QuadPushConstant
    {
        uint32_t InstCount;
        uint32_t RandomSeed;
        uint32_t TextureCount;
        float3   ColorValue;
    };

private:
    using SpherePipelineMap = std::unordered_map<SpherePipelineKey, grfx::GraphicsPipelinePtr, SpherePipelineKey::Hash>;
    using SkyboxPipelineMap = std::unordered_map<SkyBoxPipelineKey, grfx::GraphicsPipelinePtr, SkyBoxPipelineKey::Hash>;
    using QuadPipelineMap   = std::unordered_map<QuadPipelineKey, grfx::GraphicsPipelinePtr, QuadPipelineKey::Hash>;

    std::vector<PerFrame>             mPerFrame;
    FreeCamera                        mCamera;
    float3                            mLightPosition       = float3(10, 250, 10);
    std::array<bool, TOTAL_KEY_COUNT> mPressedKeys         = {0};
    bool                              mEnableMouseMovement = true;
    RealtimeValue<uint64_t, float>    mGpuWorkDuration;
    grfx::SamplerPtr                  mLinearSampler;
    grfx::DescriptorPoolPtr           mDescriptorPool;
    std::vector<OffscreenFrame>       mOffscreenFrame;
    RealtimeValue<double>             mCPUSubmissionTime;

    // SkyBox resources
    Entity                mSkyBox;
    grfx::ShaderModulePtr mVSSkyBox;
    grfx::ShaderModulePtr mPSSkyBox;
    grfx::TexturePtr      mSkyBoxTexture;
    SkyboxPipelineMap     mSkyBoxPipelines;

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
    std::vector<float4>                                           mColorsForDrawCalls;
    Random                                                        mRandom;
    bool                                                          mSpheresAreSetUp    = false;
    uint32_t                                                      mInitializedSpheres = 0;

    // Fullscreen quads resources
    Entity2D                                                             mFullscreenQuads;
    grfx::ShaderModulePtr                                                mVSQuads;
    std::array<grfx::TexturePtr, kMaxTextureCount>                       mQuadsTextures;
    QuadPipelineMap                                                      mQuadsPipelines;
    std::array<grfx::PipelineInterfacePtr, kFullscreenQuadsTypes.size()> mQuadsPipelineInterfaces;
    std::array<grfx::ShaderModulePtr, kFullscreenQuadsTypes.size()>      mQuadsPs;

    BlitContext mBlit;
    // Metrics Data
    struct MetricsData
    {
        enum MetricsType : size_t
        {
            kTypeCPUSubmissionTime = 0,
            kTypeBandwidth,
            kCount
        };

        ppx::metrics::MetricID metrics[kCount] = {};
    };
    MetricsData mMetricsData;
    // This is used to skip first several frames after the knob of quad count being changed
    uint32_t mSkipRecordBandwidthMetricFrameCounter = 0;

    QuadPushConstant mQuadPushConstant;

private:
    std::shared_ptr<KnobCheckbox>              pEnableSkyBox;
    std::shared_ptr<KnobCheckbox>              pEnableSpheres;
    std::shared_ptr<KnobDropdown<DebugView>>   pDebugViews;
    std::shared_ptr<KnobDropdown<SphereVS>>    pKnobVs;
    std::shared_ptr<KnobDropdown<SpherePS>>    pKnobPs;
    std::shared_ptr<KnobDropdown<SphereLOD>>   pKnobLOD;
    std::shared_ptr<KnobDropdown<std::string>> pKnobVbFormat;
    std::shared_ptr<KnobDropdown<std::string>> pKnobVertexAttrLayout;
    std::shared_ptr<KnobSlider<int>>           pSphereInstanceCount;
    std::shared_ptr<KnobSlider<int>>           pDrawCallCount;
    std::shared_ptr<KnobCheckbox>              pAlphaBlend;
    std::shared_ptr<KnobCheckbox>              pDepthTestWrite;
    std::shared_ptr<KnobCheckbox>              pAllTexturesTo1x1;

    std::shared_ptr<KnobSlider<int>>                   pFullscreenQuadsCount;
    std::shared_ptr<KnobDropdown<FullscreenQuadsType>> pFullscreenQuadsType;
    std::shared_ptr<KnobDropdown<float3>>              pFullscreenQuadsColor;
    std::shared_ptr<KnobCheckbox>                      pFullscreenQuadsSingleRenderpass;
    std::shared_ptr<KnobFlag<std::string>>             pQuadTextureFile;

    std::shared_ptr<KnobCheckbox>                      pRenderOffscreen;
    std::shared_ptr<KnobCheckbox>                      pBlitOffscreen;
    std::shared_ptr<KnobDropdown<grfx::Format>>        pFramebufferFormat;
    std::shared_ptr<KnobDropdown<std::pair<int, int>>> pResolution;

    std::shared_ptr<KnobFlag<int>> pKnobAluCount;
    std::shared_ptr<KnobFlag<int>> pKnobTextureCount;

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

    // Setup everything:
    // - Resources, vertex data and pipelines
    void SetupSpheres();

    // Metrics related functions
    virtual void SetupMetrics() override;
    virtual void UpdateMetrics() override;

    Result CompileSpherePipeline(const SpherePipelineKey& key);
    Result CompilePipeline(const SkyBoxPipelineKey& key);
    Result CompilePipeline(const SpherePipelineKey& key);
    Result CompilePipeline(const QuadPipelineKey& key);

    // Update descriptors
    // Note: Descriptors can be updated within rendering loop
    void UpdateSkyBoxDescriptors();
    void UpdateSphereDescriptors();
    void UpdateFullscreenQuadsDescriptors();

    void UpdateOffscreenBuffer(grfx::Format format, int w, int h);

    // =====================================================================
    // RENDERING LOOP (Called every frame)
    // =====================================================================

    // Processing changed state
    void ProcessInput();
    void ProcessKnobs();
    // Process quad/sphere knobs, return if any of them has been updated.
    bool ProcessSphereKnobs();
    bool ProcessQuadsKnobs();
    bool ProcessOffscreenRenderKnobs();

    // Drawing GUI
    void UpdateGUI();
    void DrawExtraInfo();

    // Record this frame's command buffer with multiple renderpasses
    void RecordCommandBuffer(PerFrame& frame, const RenderPasses& renderPasses, uint32_t imageIndex);

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

    const Camera& GetCamera() const;

    // Loads shader at shaderBaseDir/fileName and creates it at ppShaderModule
    void SetupShader(const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule);
    void SetupShader(const char* baseDir, const std::filesystem::path& fileName, grfx::ShaderModule** ppShaderModule);

    // Compile or load from cache currently required pipeline.
    grfx::GraphicsPipelinePtr GetSpherePipeline();
    grfx::GraphicsPipelinePtr GetFullscreenQuadPipeline();
    grfx::GraphicsPipelinePtr GetSkyBoxPipeline();

    Result       CreateOffscreenFrame(OffscreenFrame&, grfx::Format colorFormat, grfx::Format depthFormat, uint32_t width, uint32_t height);
    void         DestroyOffscreenFrame(OffscreenFrame&);
    Result       CreateBlitContext(BlitContext& blit);
    RenderPasses SwapchainRenderPasses(grfx::SwapchainPtr swapchain, uint32_t imageIndex);
    RenderPasses OffscreenRenderPasses(const OffscreenFrame&);
    grfx::Format RenderFormat();
    void         CreateColorsForDrawCalls();
};

#endif // BENCHMARKS_GRAPHICS_PIPELINE_GRAPHICS_BENCHMARK_APP_H
