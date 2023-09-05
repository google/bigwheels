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

#include <functional>
#include <utility>
#include <queue>
#include <unordered_set>
#include <array>
#include <random>
#include <cmath>

#include "ppx/ppx.h"
#include "ppx/knob.h"
#include "ppx/timer.h"
#include "ppx/camera.h"
#include "ppx/math_util.h"
#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_scope.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "glm/gtc/type_ptr.hpp"

using namespace ppx;

static constexpr float kCameraSpeed = 0.2f;

class FreeCamera
    : public ppx::PerspCamera
{
public:
    enum class MovementDirection
    {
        FORWARD,
        LEFT,
        RIGHT,
        BACKWARD
    };

    // Initializes a FreeCamera located at `eyePosition` and looking at the
    // spherical coordinates in world space defined by `theta` and `phi`.
    // `mTheta` (longitude) is an angle in the range [0, 2pi].
    // `mPhi` (latitude) is an angle in the range [0, pi].
    FreeCamera(float3 eyePosition, float theta, float phi)
    {
        mEyePosition = eyePosition;
        mTheta       = theta;
        mPhi         = phi;
        mTarget      = eyePosition + SphericalToCartesian(theta, phi);
    }

    // Moves the location of the camera in dir direction for distance units.
    void Move(MovementDirection dir, float distance);

    // Changes the location where the camera is looking at by turning `deltaTheta`
    // (longitude) radians and looking up `deltaPhi` (latitude) radians.
    void Turn(float deltaTheta, float deltaPhi);

private:
    // Spherical coordinates in world space where the camera is looking at.
    // `mTheta` (longitude) is an angle in the range [0, 2pi].
    // `mPhi` (latitude) is an angle in the range [0, pi].
    float mTheta;
    float mPhi;
};

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

static constexpr uint32_t kMaxSphereInstanceCount = 3000;
static constexpr uint32_t kSeed                   = 89977;
static constexpr uint32_t kMaxNoiseQuadsCount     = 1000;

static constexpr std::array<const char*, 2> kAvailableVsShaders = {
    "Benchmark_VsSimple",
    "Benchmark_VsAluBound"};

static constexpr std::array<const char*, 3> kAvailablePsShaders = {
    "Benchmark_PsSimple",
    "Benchmark_PsAluBound",
    "Benchmark_PsMemBound"};

static constexpr uint32_t kPipelineCount = kAvailablePsShaders.size() * kAvailableVsShaders.size();

class ProjApp
    : public ppx::Application
{
public:
    ProjApp()
        : mCamera(float3(0, 0, -5), pi<float>() / 2.0f, pi<float>() / 2.0f) {}
    virtual void InitKnobs() override;
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;
    virtual void KeyDown(ppx::KeyCode key) override;
    virtual void KeyUp(ppx::KeyCode key) override;
    virtual void Render() override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
        grfx::QueryPtr         timestampQuery;
    };

    struct Texture
    {
        grfx::ImagePtr            image;
        grfx::SampledImageViewPtr sampledImageView;
        grfx::SamplerPtr          sampler;
    };

    struct Entity
    {
        grfx::MeshPtr                mesh;
        grfx::BufferPtr              uniformBuffer;
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::PipelineInterfacePtr   pipelineInterface;
        grfx::GraphicsPipelinePtr    pipeline;
    };

    struct Entity2D
    {
        grfx::BufferPtr            vertexBuffer;
        grfx::VertexBinding        vertexBinding;
        grfx::PipelineInterfacePtr pipelineInterface;
        grfx::GraphicsPipelinePtr  pipeline;
    };

    struct Grid
    {
        uint32_t xSize;
        uint32_t ySize;
        uint32_t zSize;
        float    step;
    };

    std::vector<PerFrame>                                         mPerFrame;
    FreeCamera                                                    mCamera;
    float3                                                        mLightPosition = float3(10, 250, 10);
    std::array<bool, TOTAL_KEY_COUNT>                             mPressedKeys   = {0};
    uint64_t                                                      mGpuWorkDuration;
    grfx::ShaderModulePtr                                         mVS;
    grfx::ShaderModulePtr                                         mPS;
    grfx::ShaderModulePtr                                         mVSNoise;
    grfx::ShaderModulePtr                                         mPSNoise;
    Texture                                                       mSkyBoxTexture;
    Texture                                                       mAlbedoTexture;
    Texture                                                       mNormalMapTexture;
    Texture                                                       mMetalRoughnessTexture;
    Entity                                                        mSkyBox;
    Entity                                                        mSphere;
    Entity2D                                                      mNoiseQuads;
    bool                                                          mEnableMouseMovement = true;
    std::vector<grfx::BufferPtr>                                  mDrawCallUniformBuffers;
    std::array<grfx::GraphicsPipelinePtr, kPipelineCount>         mPipelines;
    std::array<grfx::ShaderModulePtr, kAvailableVsShaders.size()> mVsShaders;
    std::array<grfx::ShaderModulePtr, kAvailablePsShaders.size()> mPsShaders;
    uint32_t                                                      mSphereIndexCount;

private:
    std::shared_ptr<KnobDropdown<std::string>> pKnobVs;
    std::shared_ptr<KnobDropdown<std::string>> pKnobPs;
    std::shared_ptr<KnobSlider<int>>           pSphereInstanceCount;
    std::shared_ptr<KnobSlider<int>>           pDrawCallCount;
    std::shared_ptr<KnobSlider<int>>           pNoiseQuadsCount;

private:
    void ProcessInput();

    void UpdateGUI();

    void DrawExtraInfo();

    void SetupNoiseQuads();
};

void FreeCamera::Move(MovementDirection dir, float distance)
{
    // Given that v = (1, mTheta, mPhi) is where the camera is looking at in the Spherical
    // coordinates and moving forward goes in this direction, we have to update the
    // camera location for each movement as follows:
    //      FORWARD:     distance * unitVectorOf(v)
    //      BACKWARD:    -distance * unitVectorOf(v)
    //      RIGHT:       distance * unitVectorOf(1, mTheta + pi/2, pi/2)
    //      LEFT:        -distance * unitVectorOf(1, mTheta + pi/2, pi/2)
    switch (dir) {
        case MovementDirection::FORWARD: {
            float3 unitVector = glm::normalize(SphericalToCartesian(mTheta, mPhi));
            mEyePosition += distance * unitVector;
            break;
        }
        case MovementDirection::LEFT: {
            float3 perpendicularUnitVector = glm::normalize(SphericalToCartesian(mTheta + pi<float>() / 2.0f, pi<float>() / 2.0f));
            mEyePosition -= distance * perpendicularUnitVector;
            break;
        }
        case MovementDirection::RIGHT: {
            float3 perpendicularUnitVector = glm::normalize(SphericalToCartesian(mTheta + pi<float>() / 2.0f, pi<float>() / 2.0f));
            mEyePosition += distance * perpendicularUnitVector;
            break;
        }
        case MovementDirection::BACKWARD: {
            float3 unitVector = glm::normalize(SphericalToCartesian(mTheta, mPhi));
            mEyePosition -= distance * unitVector;
            break;
        }
    }
    mTarget = mEyePosition + SphericalToCartesian(mTheta, mPhi);
    LookAt(mEyePosition, mTarget);
}

void FreeCamera::Turn(float deltaTheta, float deltaPhi)
{
    mTheta += deltaTheta;
    mPhi += deltaPhi;

    // Saturate mTheta values by making wrap around.
    if (mTheta < 0) {
        mTheta = 2 * pi<float>();
    }
    else if (mTheta > 2 * pi<float>()) {
        mTheta = 0;
    }

    // mPhi is saturated by making it stop, so the world doesn't turn upside down.
    mPhi = std::clamp(mPhi, 0.1f, pi<float>() - 0.1f);

    mTarget = mEyePosition + SphericalToCartesian(mTheta, mPhi);
    LookAt(mEyePosition, mTarget);
}

void ProjApp::InitKnobs()
{
    const auto& cl_options = GetExtraOptions();
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("vs-shader-index"), "--vs-shader-index flag has been replaced, instead use --vs and specify the name of the vertex shader");
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("ps-shader-index"), "--ps-shader-index flag has been replaced, instead use --ps and specify the name of the pixel shader");

    pKnobVs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vs", 0, kAvailableVsShaders);
    pKnobVs->SetDisplayName("Vertex Shader");
    pKnobVs->SetFlagDescription("Select the vertex shader for the graphics pipeline.");

    pKnobPs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("ps", 0, kAvailablePsShaders);
    pKnobPs->SetDisplayName("Pixel Shader");
    pKnobPs->SetFlagDescription("Select the pixel shader for the graphics pipeline.");

    pSphereInstanceCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sphere-count", /* defaultValue = */ 50, /* minValue = */ 1, kMaxSphereInstanceCount);
    pSphereInstanceCount->SetDisplayName("Sphere Count");
    pSphereInstanceCount->SetFlagDescription("Select the number of spheres to draw on the screen.");

    pDrawCallCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("drawcall-count", /* defaultValue = */ 1, /* minValue = */ 1, kMaxSphereInstanceCount);
    pDrawCallCount->SetDisplayName("DrawCall Count");
    pDrawCallCount->SetFlagDescription("Select the number of draw calls to be used to draw the `sphere-count` spheres.");

    pNoiseQuadsCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("noise-quads-count", /* defaultValue = */ 0, /* minValue = */ 0, kMaxNoiseQuadsCount);
    pNoiseQuadsCount->SetDisplayName("Number of Fullscreen Noise Quads");
    pNoiseQuadsCount->SetFlagDescription("Select the number of fullscreen noise quads to render.");
}

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "graphics_pipeline";
    settings.enableImGui                = true;
    settings.window.width               = 1920;
    settings.window.height              = 1080;
    settings.grfx.api                   = kApi;
    settings.grfx.enableDebug           = false;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
}

// Shuffles [`begin`, `end`) using function `f`.
template <class Iter, class F>
void Shuffle(Iter begin, Iter end, F&& f)
{
    size_t count = end - begin;
    for (size_t i = 0; i < count; i++) {
        std::swap(begin[i], begin[f() % (count - i) + i]);
    }
}

void ProjApp::Setup()
{
    // Cameras
    {
        mCamera.LookAt(mCamera.GetEyePosition(), mCamera.GetTarget());
        mCamera.SetPerspective(60.f, GetWindowAspect());
    }

    // Texture image, view, and sampler
    {
        // SkyBox
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/spheres/basic-skybox.jpg"), &mSkyBoxTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mSkyBoxTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSkyBoxTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSkyBoxTexture.sampler));
    }
    {
        // Albedo
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/albedo.png"), &mAlbedoTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mAlbedoTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mAlbedoTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mAlbedoTexture.sampler));
    }
    {
        // NormalMap
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/normal.png"), &mNormalMapTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mNormalMapTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mNormalMapTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mNormalMapTexture.sampler));
    }
    {
        // MetalRoughness
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/altimeter/metalness-roughness.png"), &mMetalRoughnessTexture.image, options, true));

        grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mMetalRoughnessTexture.image);
        PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mMetalRoughnessTexture.sampledImageView));

        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mMetalRoughnessTexture.sampler));
    }

    // SkyBox mesh
    {
        TriMesh  mesh = TriMesh::CreateCube(float3(1, 1, 1), TriMeshOptions().TexCoords());
        Geometry geo;
        PPX_CHECKED_CALL(Geometry::Create(GeometryOptions::InterleavedU16().AddTexCoord(), mesh, &geo));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSkyBox.mesh));
    }

    // Meshes for sphere instances
    {
        // 3D grid
        Grid grid;
        grid.xSize = static_cast<uint32_t>(std::cbrt(kMaxSphereInstanceCount));
        grid.ySize = grid.xSize;
        grid.zSize = static_cast<uint32_t>(std::ceil(kMaxSphereInstanceCount / static_cast<float>(grid.xSize * grid.ySize)));
        grid.step  = 10.0f;

        // Get sphere indices
        std::vector<uint32_t> sphereIndices(kMaxSphereInstanceCount);
        std::iota(sphereIndices.begin(), sphereIndices.end(), 0);
        // Shuffle using the `mersenne_twister` deterministic random number generator to obtain
        // the same sphere indices for a given `kMaxSphereInstanceCount`.
        Shuffle(sphereIndices.begin(), sphereIndices.end(), std::mt19937(kSeed));

        TriMesh mesh                     = TriMesh::CreateSphere(/* radius = */ 1, /* longitudeSegments = */ 10, /* latitudeSegments = */ 10, TriMeshOptions().Indices().TexCoords().Normals().Tangents());
        mSphereIndexCount                = mesh.GetCountIndices();
        const uint32_t sphereVertexCount = mesh.GetCountPositions();
        const uint32_t sphereTriCount    = mesh.GetCountTriangles();

        Geometry geo;
        PPX_CHECKED_CALL(Geometry::Create(GeometryOptions::InterleavedU32().AddTexCoord().AddNormal().AddTangent(), &geo));

        for (uint32_t i = 0; i < kMaxSphereInstanceCount; i++) {
            uint32_t index = sphereIndices[i];
            uint32_t x     = (index % (grid.xSize * grid.ySize)) / grid.ySize;
            uint32_t y     = index % grid.ySize;
            uint32_t z     = index / (grid.xSize * grid.ySize);

            // Model matrix to be applied to the sphere mesh
            float4x4 modelMatrix = glm::translate(float3(x * grid.step, y * grid.step, z * grid.step));

            // Copy a sphere mesh to create a giant vertex buffer
            // Iterate through the meshes vertx data and add it to the geometry
            for (uint32_t vertexIndex = 0; vertexIndex < sphereVertexCount; ++vertexIndex) {
                TriMeshVertexData vertexData = {};
                mesh.GetVertexData(vertexIndex, &vertexData);
                vertexData.position = modelMatrix * float4(vertexData.position, 1);
                geo.AppendVertexData(vertexData);
            }
            // Iterate the meshes triangles and add the vertex indices
            for (uint32_t triIndex = 0; triIndex < sphereTriCount; ++triIndex) {
                uint32_t v0 = PPX_VALUE_IGNORED;
                uint32_t v1 = PPX_VALUE_IGNORED;
                uint32_t v2 = PPX_VALUE_IGNORED;
                mesh.GetTriangle(triIndex, v0, v1, v2);
                geo.AppendIndicesTriangle(v0 + i * sphereVertexCount, v1 + i * sphereVertexCount, v2 + i * sphereVertexCount);
            }
        }
        // Create a giant vertex buffer to accommodate all copies of the sphere mesh
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSphere.mesh));
    }

    // Uniform buffers
    {
        // SkyBox
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSkyBox.uniformBuffer));
    }
    {
        // Sphere
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSphere.uniformBuffer));
    }

    // Descriptor set layout
    {
        // SkyBox
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.flags.bits.pushable                 = true;
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSkyBox.descriptorSetLayout));
    }
    {
        // Sphere
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.flags.bits.pushable                 = true;
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(2, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(3, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(4, grfx::DESCRIPTOR_TYPE_SAMPLER));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(5, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding(6, grfx::DESCRIPTOR_TYPE_SAMPLER));
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSphere.descriptorSetLayout));
    }

    // Uniform buffers for draw calls
    {
        mDrawCallUniformBuffers.resize(kMaxSphereInstanceCount);
        for (uint32_t i = 0; i < kMaxSphereInstanceCount; i++) {
            grfx::BufferCreateInfo bufferCreateInfo        = {};
            bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
            bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
            bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
            PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mDrawCallUniformBuffers[i]));
        }
    }

    // Scene drawpass

    // SkyBox Pipeline
    {
        std::vector<char> bytecode = LoadShader("benchmarks/shaders", "Benchmark_SkyBox.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", "Benchmark_SkyBox.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mSkyBox.descriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSkyBox.pipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mSkyBox.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_FRONT;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mSkyBox.pipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mSkyBox.pipeline));
    }

    // Vertex Shaders
    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        const std::string vsShaderBaseName = kAvailableVsShaders[i];
        std::vector<char> bytecode         = LoadShader("benchmarks/shaders", vsShaderBaseName + ".vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVsShaders[i]));
    }
    // Pixel Shaders
    for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
        const std::string psShaderBaseName = kAvailablePsShaders[j];
        std::vector<char> bytecode         = LoadShader("benchmarks/shaders", psShaderBaseName + ".ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPsShaders[j]));
    }

    // Sphere Pipelines
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mSphere.descriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSphere.pipelineInterface));

        uint32_t pipeline_index = 0;
        for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
            for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
                grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
                gpCreateInfo.VS                                 = {mVsShaders[i].Get(), "vsmain"};
                gpCreateInfo.PS                                 = {mPsShaders[j].Get(), "psmain"};
                gpCreateInfo.vertexInputState.bindingCount      = 1;
                gpCreateInfo.vertexInputState.bindings[0]       = mSphere.mesh->GetDerivedVertexBindings()[0];
                gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
                gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
                gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
                gpCreateInfo.depthReadEnable                    = true;
                gpCreateInfo.depthWriteEnable                   = true;
                gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
                gpCreateInfo.outputState.renderTargetCount      = 1;
                gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
                gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
                gpCreateInfo.pPipelineInterface                 = mSphere.pipelineInterface;
                PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPipelines[pipeline_index++]));
            }
        }
    }

    SetupNoiseQuads();

    // Per frame data
    {
        PerFrame frame = {};

        PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

        grfx::SemaphoreCreateInfo semaCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

        grfx::FenceCreateInfo fenceCreateInfo = {};
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

        PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

        fenceCreateInfo = {true}; // Create signaled
        PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

        // Timestamp query
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        mPerFrame.push_back(frame);
    }
}

void ProjApp::SetupNoiseQuads()
{
    // Noise Quad drawpass

    // Vertex buffer
    {
        // clang-format off
        std::vector<float> vertexData = {
            // position       
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        };
        // clang-format on
        uint32_t dataSize = SizeInBytesU32(vertexData);

        grfx::BufferCreateInfo bufferCreateInfo       = {};
        bufferCreateInfo.size                         = dataSize;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_CPU_TO_GPU;
        bufferCreateInfo.initialState                 = grfx::RESOURCE_STATE_VERTEX_BUFFER;

        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mNoiseQuads.vertexBuffer));

        void* pAddr = nullptr;
        PPX_CHECKED_CALL(mNoiseQuads.vertexBuffer->MapMemory(0, &pAddr));
        memcpy(pAddr, vertexData.data(), dataSize);
        mNoiseQuads.vertexBuffer->UnmapMemory();
    }

    // Load shaders
    {
        std::vector<char> bytecode = LoadShader("benchmarks/shaders", "Benchmark_RandomNoise.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVSNoise));

        bytecode = LoadShader("benchmarks/shaders", "Benchmark_RandomNoise.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPSNoise));
    }

    // Noise quads: pipeline
    {
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 0;
        piCreateInfo.pushConstants.count               = 1;
        piCreateInfo.pushConstants.binding             = 0;
        piCreateInfo.pushConstants.set                 = 0;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mNoiseQuads.pipelineInterface));

        mNoiseQuads.vertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVSNoise.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPSNoise.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 1;
        gpCreateInfo.vertexInputState.bindings[0]       = mNoiseQuads.vertexBinding;
        gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        gpCreateInfo.cullMode                           = grfx::CULL_MODE_NONE;
        gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CW;
        gpCreateInfo.depthReadEnable                    = true;
        gpCreateInfo.depthWriteEnable                   = false;
        gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
        gpCreateInfo.outputState.renderTargetCount      = 1;
        gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
        gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
        gpCreateInfo.pPipelineInterface                 = mNoiseQuads.pipelineInterface;
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mNoiseQuads.pipeline));
    }
}

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    if (!mEnableMouseMovement) {
        return;
    }

    float2 prevPos  = GetNormalizedDeviceCoordinates(x - dx, y - dy);
    float2 currPos  = GetNormalizedDeviceCoordinates(x, y);
    float2 deltaPos = currPos - prevPos;

    // In the NDC: -1 <= x, y <= 1, so the maximum value for dx and dy is 2
    // which turns the camera by pi/2 radians, so for a specific dx and dy
    // we turn (dx * pi / 4, dy * pi / 4) respectively.
    float deltaTheta = deltaPos[0] * pi<float>() / 4.0f;
    float deltaPhi   = deltaPos[1] * pi<float>() / 4.0f;
    mCamera.Turn(deltaTheta, -deltaPhi);
}

void ProjApp::KeyDown(ppx::KeyCode key)
{
    mPressedKeys[key] = true;
}

void ProjApp::KeyUp(ppx::KeyCode key)
{
    mPressedKeys[key] = false;
    if (key == KEY_SPACE) {
        mEnableMouseMovement = !mEnableMouseMovement;
    }
}

void ProjApp::ProcessInput()
{
    float deltaTime = GetPrevFrameTime();

    if (mPressedKeys[KEY_W]) {
        mCamera.Move(FreeCamera::MovementDirection::FORWARD, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_A]) {
        mCamera.Move(FreeCamera::MovementDirection::LEFT, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_S]) {
        mCamera.Move(FreeCamera::MovementDirection::BACKWARD, kCameraSpeed * deltaTime);
    }

    if (mPressedKeys[KEY_D]) {
        mCamera.Move(FreeCamera::MovementDirection::RIGHT, kCameraSpeed * deltaTime);
    }
}

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

void ProjApp::Render()
{
    uint32_t currentSphereCount   = pSphereInstanceCount->GetValue();
    uint32_t currentDrawCallCount = pDrawCallCount->GetValue();
    // TODO: Ideally, the `maxValue` of the drawcall-count slider knob should be changed at runtime.
    // Currently, the value of the drawcall-count is adjusted to the sphere-count in case the
    // former exceeds the value of the sphere-count.
    if (currentDrawCallCount > currentSphereCount) {
        pDrawCallCount->SetValue(currentSphereCount);
        currentDrawCallCount = currentSphereCount;
    }

    PerFrame&          frame      = mPerFrame[0];
    grfx::SwapchainPtr swapchain  = GetSwapchain();
    uint32_t           imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0, 0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, sizeof(data)));
        mGpuWorkDuration = data[1] - data[0];
    }
    // Reset query
    frame.timestampQuery->Reset(/* firstQuery= */ 0, frame.timestampQuery->GetCount());

    ProcessInput();

    UpdateGUI();

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        // Write start timestamp
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 0);

        // =====================================================================
        // Scene renderpass
        // =====================================================================
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(renderPass);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            // Draw SkyBox
            frame.cmd->BindGraphicsPipeline(mSkyBox.pipeline);
            frame.cmd->BindIndexBuffer(mSkyBox.mesh);
            frame.cmd->BindVertexBuffers(mSkyBox.mesh);
            {
                SkyBoxData data = {};
                data.MVP        = mCamera.GetViewProjectionMatrix() * glm::scale(float3(500.0f, 500.0f, 500.0f));
                mSkyBox.uniformBuffer->CopyFromSource(sizeof(data), &data);

                frame.cmd->PushGraphicsUniformBuffer(mSkyBox.pipelineInterface, /* binding = */ 0, /* set = */ 0, /* bufferOffset = */ 0, mSkyBox.uniformBuffer);
                frame.cmd->PushGraphicsSampledImage(mSkyBox.pipelineInterface, /* binding = */ 1, /* set = */ 0, mSkyBoxTexture.sampledImageView);
                frame.cmd->PushGraphicsSampler(mSkyBox.pipelineInterface, /* binding = */ 2, /* set = */ 0, mSkyBoxTexture.sampler);
            }
            frame.cmd->DrawIndexed(mSkyBox.mesh->GetIndexCount());

            // Draw sphere instances
            uint32_t pipeline_index = pKnobVs->GetIndex() * kAvailablePsShaders.size() + pKnobPs->GetIndex();
            frame.cmd->BindGraphicsPipeline(mPipelines[pipeline_index]);
            frame.cmd->BindIndexBuffer(mSphere.mesh);
            frame.cmd->BindVertexBuffers(mSphere.mesh);
            {
                uint32_t indicesPerDrawCall = (currentSphereCount * mSphereIndexCount) / currentDrawCallCount;
                // Make `indicesPerDrawCall` multiple of 3 given that each consecutive three vertices (3*i + 0, 3*i + 1, 3*i + 2)
                // defines a single triangle primitive (PRIMITIVE_TOPOLOGY_TRIANGLE_LIST).
                indicesPerDrawCall -= indicesPerDrawCall % 3;
                for (uint32_t i = 0; i < currentDrawCallCount; i++) {
                    SphereData data                 = {};
                    data.modelMatrix                = float4x4(1.0f);
                    data.ITModelMatrix              = glm::inverse(glm::transpose(data.modelMatrix));
                    data.ambient                    = float4(0.3f);
                    data.cameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
                    data.lightPosition              = float4(mLightPosition, 0.0f);
                    data.eyePosition                = float4(mCamera.GetEyePosition(), 0.0f);
                    mDrawCallUniformBuffers[i]->CopyFromSource(sizeof(data), &data);

                    frame.cmd->PushGraphicsUniformBuffer(mSphere.pipelineInterface, /* binding = */ 0, /* set = */ 0, /* bufferOffset = */ 0, mDrawCallUniformBuffers[i]);
                    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 1, /* set = */ 0, mAlbedoTexture.sampledImageView);
                    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 2, /* set = */ 0, mAlbedoTexture.sampler);
                    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 3, /* set = */ 0, mNormalMapTexture.sampledImageView);
                    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 4, /* set = */ 0, mNormalMapTexture.sampler);
                    frame.cmd->PushGraphicsSampledImage(mSphere.pipelineInterface, /* binding = */ 5, /* set = */ 0, mMetalRoughnessTexture.sampledImageView);
                    frame.cmd->PushGraphicsSampler(mSphere.pipelineInterface, /* binding = */ 6, /* set = */ 0, mMetalRoughnessTexture.sampler);

                    uint32_t indexCount = indicesPerDrawCall;
                    // Add the remaining indices to the last drawcall
                    if (i == currentDrawCallCount - 1) {
                        indexCount += (currentSphereCount * mSphereIndexCount - currentDrawCallCount * indicesPerDrawCall);
                    }
                    uint32_t firstIndex = i * indicesPerDrawCall;
                    frame.cmd->DrawIndexed(indexCount, /* instanceCount = */ 1, firstIndex);
                }
            }
        }
        frame.cmd->EndRenderPass();

        // =====================================================================
        // Fullscreen quads renderpasses
        // =====================================================================

        if (pNoiseQuadsCount->GetValue() > 0) {
            for (size_t i = 0; i < pNoiseQuadsCount->GetValue(); ++i) {
                renderPass = swapchain->GetRenderPass(imageIndex);
                PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

                frame.cmd->BeginRenderPass(renderPass);
                {
                    // Draw noise quads
                    frame.cmd->BindGraphicsPipeline(mNoiseQuads.pipeline);
                    frame.cmd->BindVertexBuffers(1, &mNoiseQuads.vertexBuffer, &mNoiseQuads.vertexBinding.GetStride());

                    uint32_t noiseQuadRandomSeed = (uint32_t)i;
                    frame.cmd->PushGraphicsConstants(mNoiseQuads.pipelineInterface, 1, &noiseQuadRandomSeed);
                    frame.cmd->Draw(4, 1, 0, 0);
                }
                frame.cmd->EndRenderPass();

                // Force resolve by transitioning image layout
                frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_SHADER_RESOURCE);
                frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_RENDER_TARGET);
            }
        }

        // Write end timestamp
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 1);

        // =====================================================================
        // ImGui renderpass
        // =====================================================================

        renderPass = swapchain->GetRenderPass(imageIndex, grfx::ATTACHMENT_LOAD_OP_LOAD);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        frame.cmd->BeginRenderPass(renderPass);
        {
            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);

        // Resolve queries
        frame.cmd->ResolveQueryData(frame.timestampQuery, /* startIndex= */ 0, frame.timestampQuery->GetCount());
    }
    PPX_CHECKED_CALL(frame.cmd->End());

    grfx::SubmitInfo submitInfo     = {};
    submitInfo.commandBufferCount   = 1;
    submitInfo.ppCommandBuffers     = &frame.cmd;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
    submitInfo.pFence               = frame.renderCompleteFence;

    PPX_CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

    PPX_CHECKED_CALL(swapchain->Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void ProjApp::UpdateGUI()
{
    if (!GetSettings()->enableImGui) {
        return;
    }

    // GUI
    ImGui::Begin("Debug Window");
    GetKnobManager().DrawAllKnobs(true);
    ImGui::Separator();
    DrawExtraInfo();
    ImGui::End();
}

void ProjApp::DrawExtraInfo()
{
    uint64_t frequency = 0;
    GetGraphicsQueue()->GetTimestampFrequency(&frequency);

    ImGui::Columns(2);
    const float gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
    ImGui::Text("GPU Work Duration");
    ImGui::NextColumn();
    ImGui::Text("%f ms ", gpuWorkDuration);
    ImGui::NextColumn();

    ImGui::Columns(2);
    const float gpuFPS = static_cast<float>(frequency / static_cast<double>(mGpuWorkDuration));
    ImGui::Text("GPU FPS");
    ImGui::NextColumn();
    ImGui::Text("%f fps ", gpuFPS);
    ImGui::NextColumn();
}
SETUP_APPLICATION(ProjApp)
