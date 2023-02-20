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

#include "ppx/ppx.h"
#include "ppx/camera.h"
#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_scope.h"
#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf/cgltf.h"
#include "glm/gtc/type_ptr.hpp"

using namespace ppx;

#if defined(USE_DX11)
const grfx::Api kApi = grfx::API_DX_11_1;
#elif defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    void DrawGui();

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
    };

    struct Entity
    {
        float3                 translate = float3(0, 0, 0);
        float3                 rotate    = float3(0, 0, 0);
        float3                 scale     = float3(1, 1, 1);
        grfx::MeshPtr          mesh;
        grfx::DescriptorSetPtr drawDescriptorSet;
        grfx::BufferPtr        drawUniformBuffer;
        grfx::DescriptorSetPtr shadowDescriptorSet;
    };

    struct Object
    {
      glm::mat4        model;
      const Entity*      entity;
      std::vector<const Object*> children;
    };

    std::vector<PerFrame>        mPerFrame;
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDrawObjectSetLayout;
    grfx::PipelineInterfacePtr   mDrawObjectPipelineInterface;
    grfx::GraphicsPipelinePtr    mDrawObjectPipeline;
    Entity                       mGroundPlane;
    //Entity                       mKnob;
    //std::vector<Entity*>         mEntities2;
    PerspCamera                  mCamera;
    float3                       mLightPosition = float3(0, 5, 5);

    std::vector<Entity>          mEntities;
    std::vector<Object>          mObjects;
    const Object*                root;

private:
    void SetupEntity(
        const TriMesh&                   mesh,
        grfx::DescriptorPool*            pDescriptorPool,
        const grfx::DescriptorSetLayout* pDrawSetLayout,
        Entity*                          pEntity);

    static void LoadScene(
          const std::filesystem::path& filename,
          grfx::Device *pDevice,
          grfx::Queue *pQueue,
          grfx::DescriptorPool* pDescriptorPool,
          const grfx::DescriptorSetLayout* pDrawSetLayout,
          std::vector<Object> *objects,
          std::vector<Entity> *entities);

    static void LoadNodes(
          const cgltf_data *data,
          grfx::Device *pDevice,
          grfx::Queue *pQueue,
          grfx::DescriptorPool* pDescriptorPool,
          const grfx::DescriptorSetLayout* pDrawSetLayout,
          std::vector<Object> *objects,
          std::vector<Entity> *entities);

    static void LoadGlb(
          cgltf_mesh *src_mesh,
          cgltf_buffer *src_buffer,
          grfx::Device *pDevice,
          grfx::Queue *pQueue,
          grfx::DescriptorPool* pDescriptorPool,
          const grfx::DescriptorSetLayout* pDrawSetLayout,
          Entity* pEntity);
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "gltf";
    settings.enableImGui                = true;
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

void ProjApp::SetupEntity(
    const TriMesh&                   mesh,
    grfx::DescriptorPool*            pDescriptorPool,
    const grfx::DescriptorSetLayout* pDrawSetLayout,
    Entity*                          pEntity)
{
    Geometry geo;
    PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
    PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &pEntity->mesh));

    // Draw uniform buffer
    grfx::BufferCreateInfo bufferCreateInfo        = {};
    bufferCreateInfo.size                          = RoundUp(512, PPX_CONSTANT_BUFFER_ALIGNMENT);
    bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
    bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &pEntity->drawUniformBuffer));

    // Draw descriptor set
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

    // Update draw descriptor set
    grfx::WriteDescriptor write = {};
    write.binding               = 0;
    write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.bufferOffset          = 0;
    write.bufferRange           = PPX_WHOLE_SIZE;
    write.pBuffer               = pEntity->drawUniformBuffer;
    PPX_CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(1, &write));
}

void ProjApp::LoadScene(
    const std::filesystem::path& filename,
    grfx::Device *pDevice,
    grfx::Queue *pQueue,
    grfx::DescriptorPool* pDescriptorPool,
    const grfx::DescriptorSetLayout* pDrawSetLayout,
    std::vector<Object> *objects,
    std::vector<Entity> *entities) {

  objects->clear();
  entities->clear();

  cgltf_options options = { };
  cgltf_data *data = nullptr;
  cgltf_result result = cgltf_parse_file(&options, filename.c_str(), &data);
  PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading GLB file.");
  result = cgltf_validate(data);
  PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while validating GLB file.");
  result = cgltf_load_buffers(&options, data, filename.c_str());
  PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading buffers.");

  // FIXME: add constraints here for now.
  PPX_ASSERT_MSG(data->buffers_count == 1, "Only supports one buffer for now.");
  PPX_ASSERT_MSG(data->meshes_count == 1, "Only supports one mesh for now.");
  PPX_ASSERT_MSG(data->buffers[0].data != nullptr, "Data not loaded. Was cgltf_load_buffer called?");

  LoadNodes(data, pDevice, pQueue, pDescriptorPool, pDrawSetLayout, objects, entities);
}

void ProjApp::LoadNodes(
    const cgltf_data *data,
    grfx::Device *pDevice,
    grfx::Queue *pQueue,
    grfx::DescriptorPool* pDescriptorPool,
    const grfx::DescriptorSetLayout* pDrawSetLayout,
    std::vector<Object> *objects,
    std::vector<Entity> *entities) {
  const size_t node_count = data->nodes_count;
  const size_t mesh_count = data->meshes_count;

  objects->resize(node_count);
  entities->resize(mesh_count);

  for (size_t i = 0; i < node_count; i++) {
    const auto& node = data->nodes[i];
    auto& item = (*objects)[i];

    // Compute model mat for child.
    glm::mat4 matrix(1.f);
    cgltf_node* it = &data->nodes[i];
    while (it != nullptr) {
      matrix = glm::make_mat4(it->matrix) * matrix;
      it = it->parent;
    }

    item.model = matrix;
    //glm::make_mat4(node.matrix);
    item.children.resize(node.children_count);
    for (size_t j = 0; j < node.children_count; j++) {
      const size_t child_index = std::distance(data->nodes, node.children[j]);
      item.children[j] = &(objects->data())[child_index];
    }

    if (node.mesh == nullptr) {
      item.entity = nullptr;
    } else {
      const size_t mesh_index = std::distance(data->meshes, node.mesh);
      item.entity = &(entities->data())[mesh_index];
      LoadGlb(node.mesh, &data->buffers[0], pDevice, pQueue, pDescriptorPool, pDrawSetLayout, entities->data() + mesh_index);
    }
  }
}

void ProjApp::LoadGlb(
    cgltf_mesh *src_mesh,
    cgltf_buffer *src_buffer,
    grfx::Device *pDevice,
    grfx::Queue *pQueue,
    grfx::DescriptorPool* pDescriptorPool,
    const grfx::DescriptorSetLayout* pDrawSetLayout,
    Entity* pEntity) {

  grfx::Mesh **ppMesh = &pEntity->mesh;
  PPX_ASSERT_NULL_ARG(pQueue);
  PPX_ASSERT_NULL_ARG(ppMesh);

  grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
  const size_t primitive_count = src_mesh->primitives_count;
  PPX_ASSERT_MSG(primitive_count == 1, "Only supports one primitive for now.");
  const cgltf_primitive& primitive = src_mesh->primitives[0];
  PPX_ASSERT_MSG(primitive.type == cgltf_primitive_type_triangles,
                 "only supporting tri primitives for now.");
  PPX_ASSERT_MSG(!primitive.has_draco_mesh_compression,
                 "draco compression not supported yet.");
  PPX_ASSERT_MSG(primitive.indices != nullptr,
                 "only primitives with indices are supported for now.");


  grfx::MeshPtr targetMesh;
  {
    // Indices.
    const cgltf_accessor& indices_accessor = *primitive.indices;
    const cgltf_component_type indices_type = indices_accessor.component_type;
    PPX_ASSERT_MSG(indices_type == cgltf_component_type_r_16u
                || indices_type == cgltf_component_type_r_32u,
                   "only 32u or 16u are supported for indices.");

    // Attribute accessors.
    const cgltf_accessor *position_accessor = nullptr;
    const cgltf_accessor *normal_accessor = nullptr;
    const cgltf_accessor *uv_accessor = nullptr;
    for (size_t i = 0; i < primitive.attributes_count; i++) {
      const auto& type = primitive.attributes[i].type;
      const auto *data = primitive.attributes[i].data;
      if (type == cgltf_attribute_type_position) {
        position_accessor = data;
      } else if (type == cgltf_attribute_type_normal) {
        normal_accessor = data;
      } else if (type == cgltf_attribute_type_texcoord) {
        uv_accessor = data;
      }
    }
    PPX_ASSERT_MSG(position_accessor != nullptr && normal_accessor != nullptr && uv_accessor != nullptr,
                   "For now, only supports model with position, normal and UV attributes");

    // Create mesh.
    grfx::MeshCreateInfo ci;
    std::memset(&ci.vertexBuffers, 0, PPX_MAX_VERTEX_BINDINGS * sizeof(*ci.vertexBuffers));

    ci.indexType = indices_type == cgltf_component_type_r_16u
                 ? grfx::IndexType::INDEX_TYPE_UINT16
                 : grfx::IndexType::INDEX_TYPE_UINT32;
    ci.indexCount = indices_accessor.count;
    ci.vertexCount = position_accessor->count;
    ci.memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY;
    ci.vertexBufferCount = 3;

    std::array<const cgltf_accessor*, 3> accessors = { position_accessor, uv_accessor, normal_accessor};
    for (size_t i = 0; i < accessors.size(); i++) {
      const auto& a = *accessors[i];
      const auto& bv = *a.buffer_view;
      // If the buffer_view doesn't declare a stride, the accessor must define it.
      PPX_ASSERT_MSG(bv.stride == 0, "Stride declared in buffer-view not supported.");
      PPX_ASSERT_MSG(a.offset == 0, "Non-0 offset in accessor are not supported.");
      PPX_ASSERT_MSG(a.type == cgltf_type_vec2 || a.type == cgltf_type_vec3, "Non supported accessor type.");
      PPX_ASSERT_MSG(a.component_type == cgltf_component_type_r_32f, "only float for POS, NORM, TEX are supported.");

      const cgltf_type type = a.type;
      const size_t stride = a.stride;
      const size_t count = a.count;
      const size_t offset = bv.offset;
      const size_t byte_size = bv.size;

      ci.vertexBuffers[i].attributeCount = 1;
      ci.vertexBuffers[i].vertexInputRate = grfx::VERTEX_INPUT_RATE_VERTEX;
      ci.vertexBuffers[i].attributes[0].format = a.type == cgltf_type_vec2 ? grfx::FORMAT_R32G32_FLOAT
                                                                           : grfx::FORMAT_R32G32B32_FLOAT;
      ci.vertexBuffers[i].attributes[0].stride = a.stride;
      ci.vertexBuffers[i].attributes[0].vertexSemantic = i == 0 ? grfx::VERTEX_SEMANTIC_POSITION
                                                                : (i == 1 ? grfx::VERTEX_SEMANTIC_TEXCOORD
                                                                          : grfx::VERTEX_SEMANTIC_NORMAL);
    }
    PPX_CHECKED_CALL(pQueue->GetDevice()->CreateMesh(&ci, &targetMesh));
    SCOPED_DESTROYER.AddObject(targetMesh);
  }

  // Copy data to GPU.
  grfx::BufferPtr stagingBuffer;
  {
    const auto& buffer = *src_buffer;
    printf("buffer: size=%zu\n", buffer.size);

    grfx::BufferCreateInfo ci = { };
    ci.size = buffer.size;
    ci.usageFlags.bits.transferSrc = true;
    ci.memoryUsage = grfx::MEMORY_USAGE_CPU_TO_GPU;

    PPX_CHECKED_CALL(pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer));
    SCOPED_DESTROYER.AddObject(stagingBuffer);

    PPX_CHECKED_CALL(stagingBuffer->CopyFromSource(buffer.size, buffer.data));
  }

  // Copy geometry data to mesh.
  {
    const cgltf_accessor& indices = *primitive.indices;
    const cgltf_component_type indices_type = indices.component_type;
    PPX_ASSERT_MSG(indices_type == cgltf_component_type_r_16u
                || indices_type == cgltf_component_type_r_32u,
                   "only 32u or 16u are supported for indices.");
    const auto& buffer_view = *indices.buffer_view;
    PPX_ASSERT_MSG(buffer_view.data == nullptr, "Doesn't support extra data");

    const size_t stride = indices.stride;
    const size_t offset = buffer_view.offset;
    const size_t size = buffer_view.offset;
    const size_t max_index = std::min(10ul, indices.count);

    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size = buffer_view.size;
    copyInfo.srcBuffer.offset = buffer_view.offset;
    copyInfo.dstBuffer.offset = 0;
    PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetMesh->GetIndexBuffer(),
                                                grfx::RESOURCE_STATE_INDEX_BUFFER,
                                                grfx::RESOURCE_STATE_INDEX_BUFFER));

    // PRINT
    printf("indices type: %s\n", indices_type == cgltf_component_type_r_16u ? "16u" : "32u");
    printf("accessor: offset=%zu, stride=%zu, count=%zu\n",
           indices.offset, indices.stride, indices.count);
    printf("buffer view: offset=%zu, stride=%zu, count=%zu\n",
           buffer_view.offset, buffer_view.stride, buffer_view.size);
    for (size_t i = 0; i < max_index; ++i) {
      uint8_t *data = reinterpret_cast<uint8_t*>(buffer_view.buffer->data);
      data += buffer_view.offset;
      uint16_t *ptr = reinterpret_cast<uint16_t*>(data);
      printf("index: %u\n", ptr[i]);
    }
    // PRINT

  }

  {
    const cgltf_accessor *position_accessor = nullptr;
    const cgltf_accessor *normal_accessor = nullptr;
    const cgltf_accessor *uv_accessor = nullptr;
    for (size_t i = 0; i < primitive.attributes_count; i++) {
      const auto& type = primitive.attributes[i].type;
      const auto *data = primitive.attributes[i].data;
      if (type == cgltf_attribute_type_position) {
        printf("%zu - POSITION\n", i);
        position_accessor = data;
      } else if (type == cgltf_attribute_type_normal) {
        printf("%zu - NORMAL\n", i);
        normal_accessor = data;
      } else if (type == cgltf_attribute_type_texcoord) {
        printf("%zu - TEXCOORD\n", i);
        uv_accessor = data;
      }
    }
    PPX_ASSERT_MSG(position_accessor != nullptr && normal_accessor != nullptr && uv_accessor != nullptr,
                   "For now, only supports model with position, normal and UV attributes");

    std::array<const cgltf_accessor*, 3> accessors = { position_accessor, uv_accessor, normal_accessor };
    for (size_t i = 0; i < accessors.size(); i++) {
      const auto& buffer_view = *accessors[i]->buffer_view;

      grfx::BufferToBufferCopyInfo copyInfo = {};
      copyInfo.size = buffer_view.size;
      copyInfo.srcBuffer.offset = buffer_view.offset;
      copyInfo.dstBuffer.offset = 0;
      PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetMesh->GetVertexBuffer(i),
                                                  grfx::RESOURCE_STATE_VERTEX_BUFFER,
                                                  grfx::RESOURCE_STATE_VERTEX_BUFFER));
    }

    // PRINT
    const char* names[3] = { "POSITION", "TEXCOORD", "NORMAL" };
    for (size_t i = 0; i < accessors.size(); i++) {
      const auto& a = *accessors[i];
      const char * name = names[i];
      size_t offset = a.buffer_view->offset + a.offset;
      size_t stride = a.stride;
      size_t count = a.count;
      printf("%s - offset=%zu, stride=%zu, count=%zu\n", name, offset, stride, count);
    }
  }

  targetMesh->SetOwnership(grfx::OWNERSHIP_REFERENCE);
  *ppMesh = targetMesh;

  // Draw uniform buffer
  grfx::BufferCreateInfo bufferCreateInfo        = {};
  bufferCreateInfo.size                          = RoundUp(512, PPX_CONSTANT_BUFFER_ALIGNMENT);
  bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
  bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
  PPX_CHECKED_CALL(pDevice->CreateBuffer(&bufferCreateInfo, &pEntity->drawUniformBuffer));

  // Draw descriptor set
  PPX_CHECKED_CALL(pDevice->AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

  // Update draw descriptor set
  grfx::WriteDescriptor write = {};
  write.binding               = 0;
  write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.bufferOffset          = 0;
  write.bufferRange           = PPX_WHOLE_SIZE;
  write.pBuffer               = pEntity->drawUniformBuffer;
  PPX_CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(1, &write));
}

void ProjApp::Setup()
{
    // Cameras
    {
        mCamera      = PerspCamera(60.0f, GetWindowAspect());
    }

    // Create descriptor pool large enough for this project
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 512;
        poolCreateInfo.sampledImage                   = 512;
        poolCreateInfo.sampler                        = 512;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    // Descriptor set layouts
    {
        // Draw objects
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{2, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_PS});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mDrawObjectSetLayout));
    }

    // Setup entities
    {
        TriMeshOptions options = TriMeshOptions().Indices().VertexColors().Normals();
        TriMesh        mesh    = TriMesh::CreatePlane(TRI_MESH_PLANE_POSITIVE_Y, float2(50, 50), 1, 1, TriMeshOptions(options).ObjectColor(float3(0.7f)));
        SetupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, &mGroundPlane);

        LoadScene(GetAssetPath("basic/models/monkey.glb"), GetDevice(), GetGraphicsQueue(), mDescriptorPool, mDrawObjectSetLayout, &mObjects, &mEntities);
        //assert(mEntities.size() == 1);
        //mKnob = mEntities[0];
        //mKnob.translate = float3(2, 1, 0);
        //mKnob.rotate    = float3(0, glm::radians(180.0f), 0);
        //mKnob.scale     = float3(2, 2, 2);
        //mEntities2.push_back(&mKnob);
    }

    // Draw object pipeline interface and pipeline
    {
        // Pipeline interface
        grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
        piCreateInfo.setCount                          = 1;
        piCreateInfo.sets[0].set                       = 0;
        piCreateInfo.sets[0].pLayout                   = mDrawObjectSetLayout;
        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&piCreateInfo, &mDrawObjectPipelineInterface));

        // Pipeline
        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("basic/shaders", "Lambert.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        grfx::ShaderModulePtr PS;

        bytecode = LoadShader("basic/shaders", "Lambert.ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.VS                                 = {VS.Get(), "vsmain"};
        gpCreateInfo.PS                                 = {PS.Get(), "psmain"};
        gpCreateInfo.vertexInputState.bindingCount      = 3;
        gpCreateInfo.vertexInputState.bindings[0]       = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mGroundPlane.mesh->GetDerivedVertexBindings()[1];
        gpCreateInfo.vertexInputState.bindings[2]       = mGroundPlane.mesh->GetDerivedVertexBindings()[2];
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
        gpCreateInfo.pPipelineInterface                 = mDrawObjectPipelineInterface;

        PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mDrawObjectPipeline));
        GetDevice()->DestroyShaderModule(VS);
        GetDevice()->DestroyShaderModule(PS);
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

        mPerFrame.push_back(frame);
    }
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

    // Update light position
    const float t        = GetElapsedSeconds() / 2.0f;
    const float r        = 7.0f;
    mLightPosition = float3(r * cos(t), 5.0f, r * sin(t));
    // Update camera(s)
    mCamera.LookAt(float3(5, 7, 7), float3(0, 1, 0));


    // Update uniform buffers
    {
        Entity* pEntity = &mGroundPlane;

        const float4x4 T = glm::translate(pEntity->translate);
        const float4x4 R = glm::rotate(pEntity->rotate.z, float3(0, 0, 1)) *
                     glm::rotate(pEntity->rotate.y, float3(0, 1, 0)) *
                     glm::rotate(pEntity->rotate.x, float3(1, 0, 0));
        const float4x4 S = glm::scale(pEntity->scale);
        const float4x4 M = T * R * S;

        // Draw uniform buffers
        struct Scene
        {
            float4x4 ModelMatrix;                // Transforms object space to world space
            float4   Ambient;                    // Object's ambient intensity
            float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
            float4   LightPosition;              // Light's position
            float4   EyePosition;
        };

        Scene scene                      = {};
        scene.ModelMatrix                = M;
        scene.Ambient                    = float4(0.3f);
        scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        scene.LightPosition              = float4(mLightPosition, 0);
        scene.EyePosition                = glm::float4(mCamera.GetEyePosition(), 0.f);

        pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);
    }
    for (auto& object : mObjects) {
        if (object.entity == nullptr) {
          continue;
        }
        // Draw uniform buffers
        struct Scene
        {
            float4x4 ModelMatrix;                // Transforms object space to world space
            float4   Ambient;                    // Object's ambient intensity
            float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
            float4   LightPosition;              // Light's position
            float4   EyePosition;
        };

        Scene scene                      = {};
        scene.ModelMatrix                = object.model;
        scene.Ambient                    = float4(0.3f);
        scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        scene.LightPosition              = float4(mLightPosition, 0);
        scene.EyePosition                = glm::float4(mCamera.GetEyePosition(), 0.f);

        object.entity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);
    }

    // Build command buffer
    PPX_CHECKED_CALL(frame.cmd->Begin());
    {
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

            // Draw entities
            frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
            {
              frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &mGroundPlane.drawDescriptorSet);
              frame.cmd->BindIndexBuffer(mGroundPlane.mesh);
              frame.cmd->BindVertexBuffers(mGroundPlane.mesh);
              frame.cmd->DrawIndexed(mGroundPlane.mesh->GetIndexCount());
            }
            for (auto& entity : mEntities) {
              frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &entity.drawDescriptorSet);
              frame.cmd->BindIndexBuffer(entity.mesh);
              frame.cmd->BindVertexBuffers(entity.mesh);
              frame.cmd->DrawIndexed(entity.mesh->GetIndexCount());
            }

            // Draw ImGui
            DrawDebugInfo([this]() { this->DrawGui(); });
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
{ }

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);

    return res;
}
