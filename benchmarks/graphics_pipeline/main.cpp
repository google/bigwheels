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
#include <nlohmann/json.hpp>

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

static constexpr uint32_t    kMaxSphereInstanceCount = 3000;
static constexpr const char* kSphereIndicesFilePath  = "assets/benchmarks/sphere_indices.json";

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

    struct Entity
    {
        grfx::MeshPtr                mesh;
        grfx::BufferPtr              uniformBuffer;
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::PipelineInterfacePtr   pipelineInterface;
        grfx::GraphicsPipelinePtr    pipeline;
    };

    struct Grid
    {
        uint32_t xSize;
        uint32_t ySize;
        uint32_t zSize;
        float    step;
    };

    std::vector<PerFrame>             mPerFrame;
    FreeCamera                        mCamera;
    float3                            mLightPosition = float3(10, 250, 10);
    std::array<bool, TOTAL_KEY_COUNT> mPressedKeys   = {0};
    uint64_t                          mGpuWorkDuration;
    grfx::ShaderModulePtr             mVS;
    grfx::ShaderModulePtr             mPS;
    grfx::ImagePtr                    mImage;
    grfx::SampledImageViewPtr         mSampledImageView;
    grfx::SamplerPtr                  mSampler;
    Entity                            mSkyBox;
    Entity                            mSphere;
    Grid                              mSphereGrid;
    std::vector<grfx::BufferPtr>      mSphereInstanceUniformBuffers;
    std::vector<uint32_t>             mSphereIndices;
    uint32_t                          mCurrentSphereCount;

private:
    std::shared_ptr<KnobSlider<int>> pSphereInstanceCount;

private:
    void ProcessInput();

    void UpdateGUI();

    void DrawExtraInfo();
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
    pSphereInstanceCount = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("sphere count", 50, 1, kMaxSphereInstanceCount);
    pSphereInstanceCount->SetDisplayName("Sphere Count");
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

void WriteSphereIndicesToJson(const std::vector<uint32_t>& sphereIndices, const std::string& filePath)
{
    nlohmann::json data;
    data["sphere_indices"] = sphereIndices;

    std::ofstream outfile(filePath);
    outfile << data.dump(4);
    outfile.close();
    PPX_LOG_INFO("Wrote sphere indices to " << filePath);
}

std::vector<uint32_t> ReadSphereIndicesFromJson(const std::string& filePath)
{
    std::ifstream  infile(filePath);
    nlohmann::json data;
    infile >> data;
    infile.close();

    std::vector<uint32_t> sphereIndices = data["sphere_indices"];
    PPX_LOG_INFO("Read sphere indices from " << filePath);
    return sphereIndices;
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
        {
            grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
            PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath("basic/models/spheres/basic-skybox.jpg"), &mImage, options, true));

            grfx::SampledImageViewCreateInfo viewCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(mImage);
            PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&viewCreateInfo, &mSampledImageView));
        }

        // Sampler
        grfx::SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
        samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = FLT_MAX;
        PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
    }

    // Meshes
    {
        // SkyBox
        TriMesh  mesh = TriMesh::CreateCube(float3(1, 1, 1), TriMeshOptions().TexCoords());
        Geometry geo;
        PPX_CHECKED_CALL(Geometry::Create(GeometryOptions::InterleavedU16().AddTexCoord(), mesh, &geo));
        PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSkyBox.mesh));
    }
    {
        // Sphere
        TriMesh  mesh = TriMesh::CreateSphere(/* radius = */ 1, /* longitudeSegments = */ 10, /* latitudeSegments = */ 10, TriMeshOptions().TexCoords().Normals().Tangents());
        Geometry geo;
        PPX_CHECKED_CALL(Geometry::Create(GeometryOptions::InterleavedU16().AddTexCoord().AddNormal().AddTangent(), mesh, &geo));
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

    // Grid for sphere instances
    {
        mSphereGrid.xSize = static_cast<uint32_t>(std::pow(kMaxSphereInstanceCount, 1.0f / 3.0f));
        mSphereGrid.ySize = mSphereGrid.xSize;
        mSphereGrid.zSize = static_cast<uint32_t>(std::ceil(kMaxSphereInstanceCount / static_cast<float>(mSphereGrid.xSize * mSphereGrid.ySize)));
        mSphereGrid.step  = 10.0f;
    }

    // Read/Generate sphere indices
    {
        const std::filesystem::path path = kSphereIndicesFilePath;
        if (std::filesystem::exists(path)) {
            mSphereIndices = ReadSphereIndicesFromJson(kSphereIndicesFilePath);
            PPX_ASSERT_MSG(mSphereIndices.size() == kMaxSphereInstanceCount, "Size of mSphereIndices must be " << kMaxSphereInstanceCount);
        }
        else {
            mSphereIndices.resize(kMaxSphereInstanceCount);
            std::iota(mSphereIndices.begin(), mSphereIndices.end(), 0);
            std::shuffle(mSphereIndices.begin(), mSphereIndices.end(), std::random_device{});
            WriteSphereIndicesToJson(mSphereIndices, kSphereIndicesFilePath);
        }
    }

    // Uniform buffers for sphere instances
    {
        mSphereInstanceUniformBuffers.resize(kMaxSphereInstanceCount);
        for (uint32_t i = 0; i < kMaxSphereInstanceCount; i++) {
            grfx::BufferCreateInfo bufferCreateInfo        = {};
            bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
            bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
            bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
            PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mSphereInstanceUniformBuffers[i]));
        }
    }

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

    // Sphere Pipeline
    {
        std::vector<char> bytecode = LoadShader("benchmarks/shaders", "Benchmark_VsSimple.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

        bytecode = LoadShader("benchmarks/shaders", "Benchmark_PsSimple.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mSphere.descriptorSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mSphere.pipelineInterface));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
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
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mSphere.pipeline));
    }

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

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
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
    mCurrentSphereCount = pSphereInstanceCount->GetValue();

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

        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        // =====================================================================
        //  Render scene
        // =====================================================================
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
                frame.cmd->PushGraphicsSampledImage(mSkyBox.pipelineInterface, /* binding = */ 1, /* set = */ 0, mSampledImageView);
                frame.cmd->PushGraphicsSampler(mSkyBox.pipelineInterface, /* binding = */ 2, /* set = */ 0, mSampler);
            }
            frame.cmd->DrawIndexed(mSkyBox.mesh->GetIndexCount());

            // Draw sphere instances
            frame.cmd->BindGraphicsPipeline(mSphere.pipeline);
            frame.cmd->BindIndexBuffer(mSphere.mesh);
            frame.cmd->BindVertexBuffers(mSphere.mesh);
            {
                for (uint32_t i = 0; i < mCurrentSphereCount; i++) {
                    uint32_t index = mSphereIndices[i];
                    uint32_t x     = (index % (mSphereGrid.xSize * mSphereGrid.ySize)) / mSphereGrid.ySize;
                    uint32_t y     = index % mSphereGrid.ySize;
                    uint32_t z     = index / (mSphereGrid.xSize * mSphereGrid.ySize);

                    SphereData data                 = {};
                    data.modelMatrix                = glm::translate(float3(x * mSphereGrid.step, y * mSphereGrid.step, z * mSphereGrid.step));
                    data.ITModelMatrix              = glm::inverse(glm::transpose(data.modelMatrix));
                    data.ambient                    = float4(0.3f);
                    data.cameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
                    data.lightPosition              = float4(mLightPosition, 0.0f);
                    data.eyePosition                = float4(mCamera.GetEyePosition(), 0.0f);
                    mSphereInstanceUniformBuffers[index]->CopyFromSource(sizeof(data), &data);

                    frame.cmd->PushGraphicsUniformBuffer(mSphere.pipelineInterface, /* binding = */ 0, /* set = */ 0, /* bufferOffset = */ 0, mSphereInstanceUniformBuffers[index]);
                    frame.cmd->DrawIndexed(mSphere.mesh->GetIndexCount());
                }
            }

            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);

        // Write end timestamp
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* queryIndex = */ 1);

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
