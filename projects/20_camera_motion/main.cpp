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

#include <cmath>
#include <cstdlib>
#include <set>

#include "ppx/ppx.h"
#include "ppx/camera.h"
#include "ppx/graphics_util.h"
#include "ppx/random.h"
#include "ppx/math_util.h"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

// Number of entities. Add one more for the floor.
#define kNumEntities (45 + 1)

// Size of the world grid.
#define kGridDepth 100
#define kGridWidth 100

namespace {

class Entity
{
public:
    enum class EntityKind
    {
        INVALID  = -1,
        FLOOR    = 0,
        TRI_MESH = 1,
        OBJECT   = 2
    };

    Entity()
        : mesh(nullptr),
          descriptorSet(nullptr),
          uniformBuffer(nullptr),
          pipeline(nullptr),
          location(0, 0, 0),
          dimension(0, 0, 0),
          kind(EntityKind::INVALID) {}

    Entity(float3 location, float3 dimension, EntityKind kind)
        : mesh(nullptr),
          descriptorSet(nullptr),
          uniformBuffer(nullptr),
          pipeline(nullptr),
          location(location),
          dimension(dimension),
          kind(kind) {}

    // Place this entity in a random location within the given sub-grid index.
    // @param subGridIx - Index identifying the sub-grid where the object should be randomly positioned.
    // @param subGridWidth - Width of the sub-grid.
    // @param subGridDepth - Depth of the sub-grid.
    void Place(int32_t subGridIx, ppx::Random& random, const int2& gridDim, const int2& subGridDim);

    EntityKind Kind() const { return kind; }
    bool       IsMesh() const { return Kind() == EntityKind::TRI_MESH; }
    bool       IsFloor() const { return Kind() == EntityKind::FLOOR; }
    bool       IsObject() const { return Kind() == EntityKind::OBJECT; }

    grfx::DescriptorSet**        DescriptorSetPtr() { return &descriptorSet; }
    const grfx::DescriptorSetPtr DescriptorSet() const { return descriptorSet; }

    const grfx::GraphicsPipelinePtr Pipeline() const { return pipeline; }
    grfx::GraphicsPipeline**        PipelinePtr() { return &pipeline; }

    const grfx::MeshPtr& Mesh() const { return mesh; }
    grfx::Mesh**         MeshPtr() { return &mesh; }

    const grfx::BufferPtr& UniformBuffer() const { return uniformBuffer; }
    grfx::Buffer**         UniformBufferPtr() { return &uniformBuffer; }

    const float3& Location() const { return location; }

private:
    grfx::MeshPtr             mesh;
    grfx::DescriptorSetPtr    descriptorSet;
    grfx::BufferPtr           uniformBuffer;
    grfx::GraphicsPipelinePtr pipeline;
    float3                    location;
    float3                    dimension;
    EntityKind                kind;
};

class Person
{
public:
    enum class MovementDirection
    {
        FORWARD,
        LEFT,
        RIGHT,
        BACKWARD
    };

    Person()
    {
        Setup();
    }

    // Initialize the location of this person.
    void Setup()
    {
        location   = float3(0, 1, 0);
        azimuth    = pi<float>() / 2.0f;
        altitude   = pi<float>() / 2.0f;
        rateOfMove = 0.2f;
        rateOfTurn = 0.02f;
    }

    // Move the location of this person in @param dir direction for @param distance units.
    // All the symbolic directions are computed using the current direction where the person
    // is looking at (azimuth).
    void Move(MovementDirection dir, float distance);

    // Change the location where the person is looking at by turning @param deltaAzimuth
    // radians and looking up @param deltaAltitude radians. @param deltaAzimuth is an angle in
    // the range [0, 2pi].  @param deltaAltitude is an angle in the range [0, pi].
    void Turn(float deltaAzimuth, float deltaAltitude);

    // Return the coordinates in world space that the person is looking at.
    const float3 GetLookAt() const { return location + SphericalToCartesian(azimuth, altitude); }

    // Return the location of the person in world space.
    const float3& GetLocation() const { return location; }

    float GetAzimuth() const { return azimuth; }
    float GetAltitude() const { return altitude; }
    float GetRateOfMove() const { return rateOfMove; }
    float GetRateOfTurn() const { return rateOfTurn; }

private:
    // Coordinate in world space where the person is standing.
    float3 location;

    // Spherical coordinates in world space where the person is looking at.
    // azimuth is an angle in the range [0, 2pi].
    // altitude is an angle in the range [0, pi].
    float azimuth;
    float altitude;

    // Rate of motion (grid units) and turning (radians).
    float rateOfMove;
    float rateOfTurn;
};

} // namespace

class ProjApp
    : public ppx::Application
{
public:
    ProjApp()
        : mPerFrame(),
          mVS(nullptr),
          mPS(nullptr),
          mPipelineInterface(nullptr),
          mDescriptorPool(nullptr),
          mDescriptorSetLayout(nullptr),
          mEntities(),
          mPerspCamera(),
          mArcballCamera(),
          mCurrentCamera(nullptr),
          mPressedKeys(),
          mPerson() {}
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;
    virtual void Render() override;
    virtual void KeyDown(ppx::KeyCode key) override;
    virtual void KeyUp(ppx::KeyCode key) override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::ShaderModulePtr        mVS;
    grfx::ShaderModulePtr        mPS;
    grfx::PipelineInterfacePtr   mPipelineInterface;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    std::vector<Entity>          mEntities;
    PerspCamera                  mPerspCamera;
    ArcballCamera                mArcballCamera;
    PerspCamera*                 mCurrentCamera;
    std::set<ppx::KeyCode>       mPressedKeys;
    Person                       mPerson;

    void SetupDescriptors();
    void SetupPipelines();
    void SetupPerFrameData();
    void SetupCamera();
    void UpdateCamera(PerspCamera* camera);
    void SetupEntities();
    void SetupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
    void SetupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
    void ProcessInput();
    void DrawCameraInfo();
    void DrawInstructions();

protected:
    virtual void DrawGui() override;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "20_camera_motion";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
}

void ProjApp::SetupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity)
{
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromTriMesh(GetGraphicsQueue(), &mesh, pEntity->MeshPtr()));

    grfx::BufferCreateInfo bufferCreateInfo        = {};
    bufferCreateInfo.size                          = RoundUp(512, PPX_CONSTANT_BUFFER_ALIGNMENT);
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, pEntity->UniformBufferPtr()));

    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, pEntity->DescriptorSetPtr()));

    grfx::WriteDescriptor write = {};
    write.binding               = 0;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = 0;
    write.bufferRange           = PPX_WHOLE_SIZE;
    write.pBuffer               = pEntity->UniformBuffer();
    PPX_CHECKED_CALL(pEntity->DescriptorSet()->UpdateDescriptors(1, &write));
}

void ProjApp::SetupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity)
{
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromWireMesh(GetGraphicsQueue(), &mesh, pEntity->MeshPtr()));

    grfx::BufferCreateInfo bufferCreateInfo        = {};
    bufferCreateInfo.size                          = PPX_MINIMUM_UNIFORM_BUFFER_SIZE;
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, pEntity->UniformBufferPtr()));

    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, pEntity->DescriptorSetPtr()));

    grfx::WriteDescriptor write = {};
    write.binding               = 0;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = 0;
    write.bufferRange           = PPX_WHOLE_SIZE;
    write.pBuffer               = pEntity->UniformBuffer();
    PPX_CHECKED_CALL(pEntity->DescriptorSet()->UpdateDescriptors(1, &write));
}

void Entity::Place(int32_t subGridIx, ppx::Random& random, const int2& gridDim, const int2& subGridDim)
{
    // The original grid has been split in equal-sized sub-grids that preserve the same ratio.  There are
    // as many sub-grids as entities to place.  The entity will be placed at random inside the sub-grid
    // with index @param subGridIx.
    //
    // Each sub-grid is assumed to have its origin at top-left.
    int32_t sgx = random.UInt32() % subGridDim[0];
    int32_t sgz = random.UInt32() % subGridDim[1];
    PPX_LOG_INFO("Object location in grid #" << subGridIx << ": (" << sgx << ", " << sgz << ")");

    // Translate the location relative to the sub-grid location to the main grid coordinates.
    int32_t xDisplacement = subGridDim[0] * subGridIx;
    int32_t x             = (xDisplacement + sgx) % gridDim[0];
    int32_t z             = sgz + (subGridDim[1] * (xDisplacement / gridDim[0]));
    PPX_LOG_INFO("xDisplacement: " << xDisplacement);
    PPX_LOG_INFO("Object location in main grid: (" << x << ", " << z << ")");

    // All the calculations above assume that the main grid has its origin at the top-left corner,
    // but grids have their origin in the center.  So, we need to shift the location accordingly.
    int32_t adjX = x - gridDim[0] / 2;
    int32_t adjZ = z - gridDim[1] / 2;
    PPX_LOG_INFO("Adjusted object location in main grid: (" << adjX << ", " << adjZ << ")\n\n");
    location = float3(adjX, 1, adjZ);
}

void ProjApp::SetupEntities()
{
    GeometryCreateInfo geometryCreateInfo = GeometryCreateInfo::Planar().AddColor();

    // Each object will live in a square region on the grid.  The size of each grid
    // depends on how many objects we need to place.  Note that since the first
    // entity is the grid itself, we ignore it here.
    int numObstacles = (kNumEntities > 1) ? kNumEntities - 1 : 0;
    PPX_ASSERT_MSG(numObstacles > 0, "There should be at least 1 obstacle in the grid");

    // Using the total area of the main grid and the grid ratio, compute the height and
    // width of each sub-grid where each object will be placed at random. Each sub-grid
    // has the same ratio as the original grid.
    //
    // To compute the depth (SGD) and width (SGW) of each sub-grid, we start with:
    //
    // Grid area:  A = kGridWidth * kGridDepth
    // Grid ratio: R = kGridWidth / kGridDepth
    // Number of objects: N
    // Sub-grid area: SGA = A / N
    //
    // SGA = SGW * SGD
    // R = SGW / SGD
    //
    // Solving for SGW and SGD, we get:
    //
    // SGD = sqrt(SGA / R)
    // SGW = SGA / SGD
    float gridRatio    = static_cast<float>(kGridWidth) / static_cast<float>(kGridDepth);
    float subGridArea  = (kGridWidth * kGridDepth) / static_cast<float>(numObstacles);
    float subGridDepth = std::sqrt(subGridArea / gridRatio);
    float subGridWidth = subGridArea / subGridDepth;

    ppx::Random random;
    for (int32_t i = 0; i < kNumEntities; ++i) {
        float3 location, dimension;

        if (i == 0) {
            // The first object is the mesh plane where all the other entities are placed.
            WireMeshOptions wireMeshOptions = WireMeshOptions().Indices().VertexColors();
            WireMesh        wireMesh        = WireMesh::CreatePlane(WIRE_MESH_PLANE_POSITIVE_Y, float2(kGridWidth, kGridDepth), 100, 100, wireMeshOptions);
            dimension                       = float3(kGridWidth, 0, kGridDepth);
            location                        = float3(0, 0, 0);
            auto& entity                    = mEntities.emplace_back(location, dimension, Entity::EntityKind::FLOOR);
            SetupEntity(wireMesh, geometryCreateInfo, &entity);
        }
        else {
            TriMesh            triMesh;
            uint32_t           distribution = random.UInt32() % 100;
            Entity::EntityKind kind         = Entity::EntityKind::INVALID;

            // NOTE: TriMeshOptions added here must match the number of bindings when creating this entity's pipeline.
            // See the handling of different entities in ProjApp::SetupPipelines.
            if (distribution <= 60) {
                dimension              = float3(2, 2, 2);
                TriMeshOptions options = TriMeshOptions().Indices().VertexColors();
                triMesh                = (distribution <= 30) ? TriMesh::CreateCube(dimension, options) : TriMesh::CreateSphere(dimension[0] / 2, 100, 100, options);
                kind                   = Entity::EntityKind::TRI_MESH;
            }
            else {
                float3         lb      = {0, 0, 0};
                float3         ub      = {1, 1, 1};
                TriMeshOptions options = TriMeshOptions().Indices().ObjectColor(random.Float3(lb, ub));
                triMesh                = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/monkey.obj"), options);
                kind                   = Entity::EntityKind::OBJECT;
                dimension              = triMesh.GetBoundingBoxMax();
                PPX_LOG_INFO("Object dimension: (" << dimension[0] << ", " << dimension[1] << ", " << dimension[2] << ")");
            }

            // Create the entity and compute a random location for it.  The location is computed within the
            // boundaries of the object's home grid.
            auto& entity = mEntities.emplace_back(float3(0, 0, 0), dimension, kind);
            entity.Place(i - 1, random, int2(kGridWidth, kGridDepth), int2(subGridWidth, subGridDepth));
            SetupEntity(triMesh, geometryCreateInfo, &entity);
        }
    }
}

void ProjApp::SetupDescriptors()
{
    grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.uniformBuffer                  = kNumEntities;
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));

    grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDescriptorSetLayout));
}

void ProjApp::SetupPipelines()
{
    std::vector<char> bytecode = LoadShader("basic/shaders", "VertexColors.vs");
    PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVS));

    bytecode = LoadShader("basic/shaders", "VertexColors.ps");
    PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
    shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPS));

    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = mDescriptorSetLayout;
    PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mPipelineInterface));

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
    gpCreateInfo.VS                                 = {mVS.Get(), "vsmain"};
    gpCreateInfo.PS                                 = {mPS.Get(), "psmain"};
    gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
    gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
    gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                    = true;
    gpCreateInfo.depthWriteEnable                   = true;
    gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
    gpCreateInfo.outputState.renderTargetCount      = 1;
    gpCreateInfo.outputState.renderTargetFormats[0] = GetSwapchain()->GetColorFormat();
    gpCreateInfo.outputState.depthStencilFormat     = GetSwapchain()->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = mPipelineInterface;

    for (auto& entity : mEntities) {
        // NOTE: Number of vertex input bindings here must match the number of options added to each entity in ProjApp::SetupEntities.
        if (entity.IsFloor() || entity.IsMesh()) {
            gpCreateInfo.topology                      = (entity.IsFloor()) ? grfx::PRIMITIVE_TOPOLOGY_LINE_LIST : grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            gpCreateInfo.vertexInputState.bindingCount = 2;
            gpCreateInfo.vertexInputState.bindings[0]  = entity.Mesh()->GetDerivedVertexBindings()[0];
            gpCreateInfo.vertexInputState.bindings[1]  = entity.Mesh()->GetDerivedVertexBindings()[1];
        }
        else if (entity.IsObject()) {
            gpCreateInfo.topology                      = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            gpCreateInfo.vertexInputState.bindingCount = 2;
            gpCreateInfo.vertexInputState.bindings[0]  = entity.Mesh()->GetDerivedVertexBindings()[0];
            gpCreateInfo.vertexInputState.bindings[1]  = entity.Mesh()->GetDerivedVertexBindings()[1];
        }
        else {
            PPX_ASSERT_MSG(false, "Unrecognized entity kind: " << static_cast<int>(entity.Kind()));
        }
        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, entity.PipelinePtr()));
    }
}

void ProjApp::SetupPerFrameData(void)
{
    PerFrame frame = {};

    PPX_CHECKED_CALL(GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

    grfx::SemaphoreCreateInfo semaCreateInfo = {};
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.imageAcquiredSemaphore));

    grfx::FenceCreateInfo fenceCreateInfo = {};
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.imageAcquiredFence));

    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&semaCreateInfo, &frame.renderCompleteSemaphore));

    fenceCreateInfo = {true};
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fenceCreateInfo, &frame.renderCompleteFence));

    mPerFrame.push_back(frame);
}

void ProjApp::SetupCamera()
{
    mPerson.Setup();
    mCurrentCamera = &mPerspCamera;
    UpdateCamera(&mPerspCamera);
    UpdateCamera(&mArcballCamera);
}

void ProjApp::Setup()
{
    SetupDescriptors();
    SetupEntities();
    SetupPipelines();
    SetupPerFrameData();
    SetupCamera();
}

void ProjApp::UpdateCamera(PerspCamera* camera)
{
    float3 cameraPosition(0, 0, 0);
    if (camera == &mPerspCamera) {
        cameraPosition = mPerson.GetLocation();
    }
    else {
        cameraPosition = mPerson.GetLocation() + float3(0, 1, -5);
    }
    camera->LookAt(cameraPosition, mPerson.GetLookAt(), ppx::PPX_CAMERA_DEFAULT_WORLD_UP);
    camera->SetPerspective(60.f, GetWindowAspect());
}

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    float2 prevPos       = GetNormalizedDeviceCoordinates(x - dx, y - dy);
    float2 curPos        = GetNormalizedDeviceCoordinates(x, y);
    float2 deltaPos      = prevPos - curPos;
    float  deltaAzimuth  = deltaPos[0] * pi<float>() / 4.0f;
    float  deltaAltitude = deltaPos[1] * pi<float>() / 2.0f;
    mPerson.Turn(-deltaAzimuth, deltaAltitude);
    UpdateCamera(mCurrentCamera);
}

void ProjApp::KeyDown(ppx::KeyCode key)
{
    mPressedKeys.insert(key);
}

void ProjApp::KeyUp(ppx::KeyCode key)
{
    mPressedKeys.erase(key);
}

void Person::Move(MovementDirection dir, float distance)
{
    if (dir == MovementDirection::FORWARD) {
        location += float3(distance * std::cos(azimuth), 0, distance * std::sin(azimuth));
    }
    else if (dir == MovementDirection::LEFT) {
        float perpendicularDir = azimuth - pi<float>() / 2.0f;
        location += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
    }
    else if (dir == MovementDirection::RIGHT) {
        float perpendicularDir = azimuth + pi<float>() / 2.0f;
        location += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
    }
    else if (dir == MovementDirection::BACKWARD) {
        location += float3(-distance * std::cos(azimuth), 0, -distance * std::sin(azimuth));
    }
    else {
        PPX_ASSERT_MSG(false, "unhandled direction code " << static_cast<int>(dir));
    }
}

void Person::Turn(float deltaAzimuth, float deltaAltitude)
{
    azimuth += deltaAzimuth;
    altitude += deltaAltitude;

    // Saturate azimuth values by making wrap around.
    if (azimuth < 0) {
        azimuth = 2 * pi<float>();
    }
    else if (azimuth > 2 * pi<float>()) {
        azimuth = 0;
    }

    // Altitude is saturated by making it stop, so the world doesn't turn upside down.
    if (altitude < 0) {
        altitude = 0;
    }
    else if (altitude > pi<float>()) {
        altitude = pi<float>();
    }
}

void ProjApp::ProcessInput()
{
    if (mPressedKeys.empty()) {
        return;
    }

    if (mPressedKeys.count(ppx::KEY_W) > 0) {
        mPerson.Move(Person::MovementDirection::FORWARD, mPerson.GetRateOfMove());
    }

    if (mPressedKeys.count(ppx::KEY_A) > 0) {
        mPerson.Move(Person::MovementDirection::LEFT, mPerson.GetRateOfMove());
    }

    if (mPressedKeys.count(ppx::KEY_S) > 0) {
        mPerson.Move(Person::MovementDirection::BACKWARD, mPerson.GetRateOfMove());
    }

    if (mPressedKeys.count(ppx::KEY_D) > 0) {
        mPerson.Move(Person::MovementDirection::RIGHT, mPerson.GetRateOfMove());
    }

    if (mPressedKeys.count(ppx::KEY_SPACE) > 0) {
        SetupCamera();
        return;
    }

    if (mPressedKeys.count(ppx::KEY_1) > 0) {
        mCurrentCamera = &mPerspCamera;
    }

    if (mPressedKeys.count(ppx::KEY_2) > 0) {
        mCurrentCamera = &mArcballCamera;
    }

    if (mPressedKeys.count(ppx::KEY_LEFT) > 0) {
        mPerson.Turn(-mPerson.GetRateOfTurn(), 0);
    }

    if (mPressedKeys.count(ppx::KEY_RIGHT) > 0) {
        mPerson.Turn(mPerson.GetRateOfTurn(), 0);
    }

    if (mPressedKeys.count(ppx::KEY_UP) > 0) {
        mPerson.Turn(0, -mPerson.GetRateOfTurn());
    }

    if (mPressedKeys.count(ppx::KEY_DOWN) > 0) {
        mPerson.Turn(0, mPerson.GetRateOfTurn());
    }

    UpdateCamera(mCurrentCamera);
}

void ProjApp::DrawInstructions()
{
    if (ImGui::Begin("Instructions")) {
        ImGui::Columns(2);

        ImGui::Text("Movement keys");
        ImGui::NextColumn();
        ImGui::Text("W, A, S, D ");
        ImGui::NextColumn();

        ImGui::Text("Turn and look");
        ImGui::NextColumn();
        ImGui::Text("Arrow keys and mouse");
        ImGui::NextColumn();

        ImGui::Text("Cameras");
        ImGui::NextColumn();
        ImGui::Text("1 (perspective), 2 (arcball)");
        ImGui::NextColumn();

        ImGui::Text("Reset view");
        ImGui::NextColumn();
        ImGui::Text("space");
        ImGui::NextColumn();
    }
    ImGui::End();
}

void ProjApp::DrawCameraInfo()
{
    ImGui::Separator();

    ImGui::Columns(2);
    ImGui::Text("Camera position");
    ImGui::NextColumn();
    ImGui::Text("(%.4f, %.4f, %.4f)", mCurrentCamera->GetEyePosition()[0], mCurrentCamera->GetEyePosition()[1], mCurrentCamera->GetEyePosition()[2]);
    ImGui::NextColumn();

    ImGui::Columns(2);
    ImGui::Text("Camera looking at");
    ImGui::NextColumn();
    ImGui::Text("(%.4f, %.4f, %.4f)", mCurrentCamera->GetTarget()[0], mCurrentCamera->GetTarget()[1], mCurrentCamera->GetTarget()[2]);
    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::Columns(2);
    ImGui::Text("Person location");
    ImGui::NextColumn();
    ImGui::Text("(%.4f, %.4f, %.4f)", mPerson.GetLocation()[0], mPerson.GetLocation()[1], mPerson.GetLocation()[2]);
    ImGui::NextColumn();

    ImGui::Columns(2);
    ImGui::Text("Person looking at");
    ImGui::NextColumn();
    ImGui::Text("(%.4f, %.4f, %.4f)", mPerson.GetLookAt()[0], mPerson.GetLookAt()[1], mPerson.GetLookAt()[2]);
    ImGui::NextColumn();

    ImGui::Columns(2);
    ImGui::Text("Azimuth");
    ImGui::NextColumn();
    ImGui::Text("%.4f", mPerson.GetAzimuth());
    ImGui::NextColumn();

    ImGui::Columns(2);
    ImGui::Text("Altitude");
    ImGui::NextColumn();
    ImGui::Text("%.4f", mPerson.GetAltitude());
    ImGui::NextColumn();
}

void ProjApp::Render()
{
    PerFrame& frame = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update uniform buffers
    {
        ProcessInput();

        const float4x4& P = mCurrentCamera->GetProjectionMatrix();
        const float4x4& V = mCurrentCamera->GetViewMatrix();

        for (auto& entity : mEntities) {
            float4x4 T   = glm::translate(entity.Location());
            float4x4 mat = P * V * T;
            entity.UniformBuffer()->CopyFromSource(sizeof(mat), &mat);
        }
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
        grfx::RenderPassPtr renderPass = swapchain->GetRenderPass(imageIndex);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

        grfx::RenderPassBeginInfo beginInfo = {};
        beginInfo.pRenderPass               = renderPass;
        beginInfo.renderArea                = renderPass->GetRenderArea();
        beginInfo.RTVClearCount             = 1;
        beginInfo.RTVClearValues[0]         = {{0, 0, 0, 0}};
        beginInfo.DSVClearValue             = {1.0f, 0xFF};

        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_RENDER_TARGET);
        frame.cmd->BeginRenderPass(&beginInfo);
        {
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());

            for (auto& entity : mEntities) {
                frame.cmd->BindGraphicsPipeline(entity.Pipeline());
                frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, entity.DescriptorSetPtr());
                frame.cmd->BindIndexBuffer(entity.Mesh());
                frame.cmd->BindVertexBuffers(entity.Mesh());
                frame.cmd->DrawIndexed(entity.Mesh()->GetIndexCount());
            }

            // Draw ImGui
            DrawDebugInfo();
            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();
        frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_RENDER_TARGET, grfx::RESOURCE_STATE_PRESENT);
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

void ProjApp::DrawGui()
{
    DrawCameraInfo();
    DrawInstructions();
}

SETUP_APPLICATION(ProjApp)
