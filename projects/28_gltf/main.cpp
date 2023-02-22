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

    struct Texture
    {
        grfx::ImagePtr            pImage;
        grfx::SampledImageViewPtr pTexture;
        grfx::SamplerPtr          pSampler;
    };

    struct Material
    {
        grfx::PipelineInterfacePtr   pInterface;
        grfx::DescriptorSetLayoutPtr pSetLayout;
        grfx::GraphicsPipelinePtr    pPipeline;
        grfx::DescriptorSetPtr       pDescriptorSet;
        std::vector<Texture>         textures;
    };

    struct Primitive
    {
        grfx::Mesh* mesh;
    };

    using RenderableMap = std::unordered_map<Material*, Primitive*>;

    struct Object
    {
        float4x4        model;
        grfx::BufferPtr pUniformBuffer;
        RenderableMap   renderables;
    };

    using RenderList = std::unordered_map<Material*, std::vector<Object*>>;

    std::vector<PerFrame>   mPerFrame;
    grfx::DescriptorPoolPtr mDescriptorPool;
    PerspCamera             mCamera;
    float3                  mLightPosition = float3(0, 5, 5);

    RenderList             mRenderList;
    std::vector<Material>  mMaterials;
    std::vector<Primitive> mPrimitives;
    std::vector<Object>    mObjects;

private:
    void LoadScene(
        const std::filesystem::path& filename,
        grfx::Device*                pDevice,
        grfx::Swapchain*             pSwapchain,
        grfx::Queue*                 pQueue,
        grfx::DescriptorPool*        pDescriptorPool,
        std::vector<Object>*         objects,
        std::vector<Primitive>*      pPrimitives,
        std::vector<Material>*       pMaterials) const;

    void LoadMaterial(
        const cgltf_material& material,
        grfx::Swapchain*      pSwapchain,
        grfx::Queue*          pQueue,
        grfx::DescriptorPool* pDescriptorPool,
        Material*             pOutput) const;

    void LoadTexture(const cgltf_texture_view& texture_view, grfx::Queue* pQueue, Texture* pOutput) const;

    void LoadPrimitive(
        const cgltf_primitive& primitive,
        grfx::BufferPtr        pStagingBuffer,
        grfx::Queue*           pQueue,
        Primitive*             pOutput) const;

    static void LoadNodes(
        const cgltf_data*                                         data,
        grfx::BufferPtr                                           pStagingBuffer,
        grfx::Queue*                                              pQueue,
        std::vector<Object>*                                      objects,
        const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
        std::vector<Primitive>*                                   pPrimitives,
        std::vector<Material>*                                    pMaterials);
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

void ProjApp::LoadTexture(const cgltf_texture_view& texture_view, grfx::Queue* pQueue, Texture* pOutput) const
{
    PPX_ASSERT_MSG(texture_view.texture != nullptr, "Texture with no image are not supported.");
    PPX_ASSERT_MSG(texture_view.has_transform == false, "Texture transforms are not supported yet.");
    const auto& texture = *texture_view.texture;
    PPX_ASSERT_MSG(texture.image != nullptr, "Texture with no image are not supported.");
    PPX_ASSERT_MSG(texture.image->uri != nullptr, "Texture with embedded data is not supported yet.");
    PPX_ASSERT_MSG(strcmp(texture.image->mime_type, "image/vnd-ms.dds") == 0, "Texture format others than DDS are not supported.");

    std::filesystem::path   gltf_path = "basic/models/altimeter/";
    grfx_util::ImageOptions options   = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
    PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(pQueue, GetAssetPath(gltf_path / texture.image->uri), &pOutput->pImage, options, false));

    grfx::SampledImageViewCreateInfo sivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(pOutput->pImage);
    PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sivCreateInfo, &pOutput->pTexture));

    grfx::SamplerCreateInfo samplerCreateInfo = {};
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &pOutput->pSampler));
}

void ProjApp::LoadMaterial(
    const cgltf_material& material,
    grfx::Swapchain*      pSwapchain,
    grfx::Queue*          pQueue,
    grfx::DescriptorPool* pDescriptorPool,
    Material*             pOutput) const
{
    grfx::Device* pDevice = pQueue->GetDevice();
    if (material.extensions_count != 0) {
        printf("Material %s has extensions, but they are ignored. Rendered aspect may vary.\n", material.name);
    }

    // This is to simplify the pipeline creation for now. Need to revisit later.
    PPX_ASSERT_MSG(material.has_pbr_metallic_roughness, "Only PBR metallic roughness supported for now.");
    PPX_ASSERT_MSG(material.normal_texture.texture != nullptr, "Missing normal texture not supported yet.");
    PPX_ASSERT_MSG(material.pbr_metallic_roughness.base_color_texture.texture != nullptr, "Missing albedo.");
    PPX_ASSERT_MSG(material.pbr_metallic_roughness.metallic_roughness_texture.texture != nullptr, "Missing metallic+roughness.");

    {
        grfx::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 0,
            /* type= */ grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_ALL_GRAPHICS});

        // Albedo
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 1,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 2,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLER,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});

        // Normal
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 3,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 4,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLER,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});

        // Metallic/Roughness
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 5,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});
        layoutCreateInfo.bindings.push_back(grfx::DescriptorBinding{
            /* binding= */ 6,
            /* type= */ grfx::DESCRIPTOR_TYPE_SAMPLER,
            /* array_count= */ 1,
            /* shader_visibility= */ grfx::SHADER_STAGE_PS});

        PPX_CHECKED_CALL(pDevice->CreateDescriptorSetLayout(&layoutCreateInfo, &pOutput->pSetLayout));
    }
    PPX_CHECKED_CALL(pQueue->GetDevice()->AllocateDescriptorSet(pDescriptorPool, pOutput->pSetLayout, &pOutput->pDescriptorSet));

    grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
    piCreateInfo.setCount                          = 1;
    piCreateInfo.sets[0].set                       = 0;
    piCreateInfo.sets[0].pLayout                   = pOutput->pSetLayout;
    PPX_CHECKED_CALL(pDevice->CreatePipelineInterface(&piCreateInfo, &pOutput->pInterface));

    grfx::ShaderModulePtr VS;
    std::vector<char>     bytecode = LoadShader("basic/shaders", "Lambert.vs");
    PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(pDevice->CreateShaderModule(&shaderCreateInfo, &VS));

    grfx::ShaderModulePtr PS;
    bytecode = LoadShader("basic/shaders", "Lambert.ps");
    PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
    shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    PPX_CHECKED_CALL(pDevice->CreateShaderModule(&shaderCreateInfo, &PS));

    grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
    gpCreateInfo.VS                                = {VS.Get(), "vsmain"};
    gpCreateInfo.PS                                = {PS.Get(), "psmain"};
    // FIXME: assuming all primitives provides POSITION, UV, and NORMAL. Might not be the case.
    gpCreateInfo.vertexInputState.bindingCount      = 3;
    gpCreateInfo.vertexInputState.bindings[0]       = mPrimitives[0].mesh->GetDerivedVertexBindings()[0];
    gpCreateInfo.vertexInputState.bindings[1]       = mPrimitives[0].mesh->GetDerivedVertexBindings()[1];
    gpCreateInfo.vertexInputState.bindings[2]       = mPrimitives[0].mesh->GetDerivedVertexBindings()[2];
    gpCreateInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gpCreateInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
    gpCreateInfo.cullMode                           = grfx::CULL_MODE_BACK;
    gpCreateInfo.frontFace                          = grfx::FRONT_FACE_CCW;
    gpCreateInfo.depthReadEnable                    = true;
    gpCreateInfo.depthWriteEnable                   = true;
    gpCreateInfo.blendModes[0]                      = grfx::BLEND_MODE_NONE;
    gpCreateInfo.outputState.renderTargetCount      = 1;
    gpCreateInfo.outputState.renderTargetFormats[0] = pSwapchain->GetColorFormat();
    gpCreateInfo.outputState.depthStencilFormat     = pSwapchain->GetDepthFormat();
    gpCreateInfo.pPipelineInterface                 = pOutput->pInterface;

    PPX_CHECKED_CALL(pDevice->CreateGraphicsPipeline(&gpCreateInfo, &pOutput->pPipeline));
    pDevice->DestroyShaderModule(VS);
    pDevice->DestroyShaderModule(PS);

    pOutput->textures.resize(3);
    LoadTexture(material.pbr_metallic_roughness.base_color_texture, pQueue, &pOutput->textures[0]);
    LoadTexture(material.normal_texture, pQueue, &pOutput->textures[1]);
    LoadTexture(material.pbr_metallic_roughness.metallic_roughness_texture, pQueue, &pOutput->textures[2]);
}

static inline size_t count_primitives(const cgltf_mesh* array, size_t count)
{
    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        total += array[i].primitives_count;
    }
    return total;
}

static void GetAccessorsForPrimitive(const cgltf_primitive& primitive, const cgltf_accessor** position, const cgltf_accessor** uv, const cgltf_accessor** normal)
{
    PPX_ASSERT_NULL_ARG(position);
    PPX_ASSERT_NULL_ARG(uv);
    PPX_ASSERT_NULL_ARG(normal);

    for (size_t i = 0; i < primitive.attributes_count; i++) {
        const auto& type = primitive.attributes[i].type;
        const auto* data = primitive.attributes[i].data;
        if (type == cgltf_attribute_type_position) {
            *position = data;
        }
        else if (type == cgltf_attribute_type_normal) {
            *normal = data;
        }
        else if (type == cgltf_attribute_type_texcoord) {
            *uv = data;
        }
    }

    PPX_ASSERT_MSG(*position != nullptr && *uv != nullptr && *normal != nullptr, "For now, only supports model with position, normal and UV attributes");
}

void ProjApp::LoadPrimitive(const cgltf_primitive& primitive, grfx::BufferPtr pStagingBuffer, grfx::Queue* pQueue, Primitive* pOutput) const
{
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
    PPX_ASSERT_MSG(primitive.type == cgltf_primitive_type_triangles, "only supporting tri primitives for now.");
    PPX_ASSERT_MSG(!primitive.has_draco_mesh_compression, "draco compression not supported yet.");
    PPX_ASSERT_MSG(primitive.indices != nullptr, "only primitives with indices are supported for now.");

    // Attribute accessors.
    constexpr size_t                     POSITION_INDEX = 0;
    constexpr size_t                     UV_INDEX       = 1;
    constexpr size_t                     NORMAL_INDEX   = 2;
    std::array<const cgltf_accessor*, 3> accessors;
    GetAccessorsForPrimitive(primitive, &accessors[POSITION_INDEX], &accessors[UV_INDEX], &accessors[NORMAL_INDEX]);

    grfx::MeshPtr targetMesh;
    {
        // Indices.
        const cgltf_accessor&      indices_accessor = *primitive.indices;
        const cgltf_component_type indices_type     = indices_accessor.component_type;
        PPX_ASSERT_MSG(indices_type == cgltf_component_type_r_16u || indices_type == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");

        // Create mesh.
        grfx::MeshCreateInfo ci;
        std::memset(&ci.vertexBuffers, 0, PPX_MAX_VERTEX_BINDINGS * sizeof(*ci.vertexBuffers));

        ci.indexType         = indices_type == cgltf_component_type_r_16u
                                   ? grfx::IndexType::INDEX_TYPE_UINT16
                                   : grfx::IndexType::INDEX_TYPE_UINT32;
        ci.indexCount        = indices_accessor.count;
        ci.vertexCount       = accessors[POSITION_INDEX]->count;
        ci.memoryUsage       = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.vertexBufferCount = 3;

        for (size_t i = 0; i < accessors.size(); i++) {
            const auto& a  = *accessors[i];
            const auto& bv = *a.buffer_view;
            // If the buffer_view doesn't declare a stride, the accessor must define it.
            PPX_ASSERT_MSG(bv.stride == 0, "Stride declared in buffer-view not supported.");
            PPX_ASSERT_MSG(a.offset == 0, "Non-0 offset in accessor are not supported.");
            PPX_ASSERT_MSG(a.type == cgltf_type_vec2 || a.type == cgltf_type_vec3, "Non supported accessor type.");
            PPX_ASSERT_MSG(a.component_type == cgltf_component_type_r_32f, "only float for POS, NORM, TEX are supported.");

            ci.vertexBuffers[i].attributeCount               = 1;
            ci.vertexBuffers[i].vertexInputRate              = grfx::VERTEX_INPUT_RATE_VERTEX;
            ci.vertexBuffers[i].attributes[0].format         = a.type == cgltf_type_vec2 ? grfx::FORMAT_R32G32_FLOAT : grfx::FORMAT_R32G32B32_FLOAT;
            ci.vertexBuffers[i].attributes[0].stride         = a.stride;
            ci.vertexBuffers[i].attributes[0].vertexSemantic = i == POSITION_INDEX ? grfx::VERTEX_SEMANTIC_POSITION
                                                                                   : (i == UV_INDEX ? grfx::VERTEX_SEMANTIC_TEXCOORD
                                                                                                    : grfx::VERTEX_SEMANTIC_NORMAL);
        }
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateMesh(&ci, &targetMesh));
        SCOPED_DESTROYER.AddObject(targetMesh);
    }

    // Copy geometry data to mesh.
    {
        const cgltf_accessor&      indices      = *primitive.indices;
        const cgltf_component_type indices_type = indices.component_type;
        const auto&                buffer_view  = *indices.buffer_view;
        PPX_ASSERT_MSG(indices_type == cgltf_component_type_r_16u || indices_type == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");
        PPX_ASSERT_MSG(buffer_view.data == nullptr, "Doesn't support extra data");

        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.size                         = buffer_view.size;
        copyInfo.srcBuffer.offset             = buffer_view.offset;
        copyInfo.dstBuffer.offset             = 0;
        PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, targetMesh->GetIndexBuffer(), grfx::RESOURCE_STATE_INDEX_BUFFER, grfx::RESOURCE_STATE_INDEX_BUFFER));
        for (size_t i = 0; i < accessors.size(); i++) {
            const auto& buffer_view = *accessors[i]->buffer_view;

            grfx::BufferToBufferCopyInfo copyInfo = {};
            copyInfo.size                         = buffer_view.size;
            copyInfo.srcBuffer.offset             = buffer_view.offset;
            copyInfo.dstBuffer.offset             = 0;
            PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, targetMesh->GetVertexBuffer(i), grfx::RESOURCE_STATE_VERTEX_BUFFER, grfx::RESOURCE_STATE_VERTEX_BUFFER));
        }
    }

    targetMesh->SetOwnership(grfx::OWNERSHIP_REFERENCE);
    pOutput->mesh = targetMesh;
}

void ProjApp::LoadScene(
    const std::filesystem::path& filename,
    grfx::Device*                pDevice,
    grfx::Swapchain*             pSwapchain,
    grfx::Queue*                 pQueue,
    grfx::DescriptorPool*        pDescriptorPool,
    std::vector<Object>*         objects,
    std::vector<Primitive>*      pPrimitives,
    std::vector<Material>*       pMaterials) const
{
    cgltf_options options = {};
    cgltf_data*   data    = nullptr;
    cgltf_result  result  = cgltf_parse_file(&options, filename.c_str(), &data);
    PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading GLB file.");
    result = cgltf_validate(data);
    PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while validating GLB file.");
    result = cgltf_load_buffers(&options, data, filename.c_str());
    PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading buffers.");

    PPX_ASSERT_MSG(data->buffers_count == 1, "Only supports one buffer for now.");
    PPX_ASSERT_MSG(data->buffers[0].data != nullptr, "Data not loaded. Was cgltf_load_buffer called?");

    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
    // Copy main buffer data to stating buffer.
    grfx::BufferPtr stagingBuffer;
    {
        grfx::BufferCreateInfo ci      = {};
        ci.size                        = data->buffers[0].size;
        ci.usageFlags.bits.transferSrc = true;
        ci.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;

        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateBuffer(&ci, &stagingBuffer));
        SCOPED_DESTROYER.AddObject(stagingBuffer);
        PPX_CHECKED_CALL(stagingBuffer->CopyFromSource(data->buffers[0].size, data->buffers[0].data));
    }

    std::unordered_map<const cgltf_primitive*, size_t> primitiveToIndex;
    pPrimitives->resize(count_primitives(data->meshes, data->meshes_count));
    {
        size_t next_slot = 0;
        for (size_t i = 0; i < data->meshes_count; i++) {
            const auto& mesh = data->meshes[i];
            for (size_t j = 0; j < mesh.primitives_count; j++) {
                LoadPrimitive(mesh.primitives[i], stagingBuffer, pQueue, &(*pPrimitives)[next_slot]);
                primitiveToIndex.insert({&mesh.primitives[i], next_slot});
                next_slot++;
            }
        }
    }

    pMaterials->resize(data->materials_count);
    for (size_t i = 0; i < data->materials_count; i++) {
        LoadMaterial(data->materials[i], pSwapchain, pQueue, pDescriptorPool, &(*pMaterials)[i]);
    }

    LoadNodes(data, stagingBuffer, pQueue, objects, primitiveToIndex, pPrimitives, pMaterials);
}

float4x4 compute_object_matrix(const cgltf_node* node)
{
    float4x4 output(1.f);
    while (node != nullptr) {
        if (node->has_matrix) {
            output = glm::make_mat4(node->matrix) * output;
        }
        else {
            const float4x4 T = node->has_translation ? glm::translate(glm::make_vec3(node->translation)) : glm::mat4(1.f);
            const float4x4 R = node->has_rotation
                                   ? glm::mat4_cast(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]))
                                   : glm::mat4(1.f);
            const float4x4 S = node->has_scale ? glm::scale(glm::make_vec3(node->scale)) : glm::mat4(1.f);
            const float4x4 M = T * R * S;
            output           = M * output;
        }
        node = node->parent;
    }
    return output;
}

void ProjApp::LoadNodes(
    const cgltf_data*                                         data,
    grfx::BufferPtr                                           pStagingBuffer,
    grfx::Queue*                                              pQueue,
    std::vector<Object>*                                      objects,
    const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
    std::vector<Primitive>*                                   pPrimitives,
    std::vector<Material>*                                    pMaterials)
{
    const size_t                                node_count = data->nodes_count;
    const size_t                                mesh_count = data->meshes_count;
    std::unordered_map<cgltf_mesh*, size_t>     mesh_to_index;
    std::unordered_map<cgltf_material*, size_t> material_to_index;

    for (size_t i = 0; i < node_count; i++) {
        const auto& node = data->nodes[i];
        if (node.mesh == nullptr) {
            continue;
        }

        Object item;
        item.model = compute_object_matrix(&node);

        for (size_t j = 0; j < node.mesh->primitives_count; j++) {
            const size_t primitive_index = primitiveToIndex.at(&node.mesh->primitives[j]);
            const size_t material_index  = std::distance(data->materials, node.mesh->primitives[j].material);
            PPX_ASSERT_MSG(primitive_index < pPrimitives->size(), "Invalid GLB file. Primitive index out of range.");
            PPX_ASSERT_MSG(material_index < pMaterials->size(), "Invalid GLB file. Material index out of range.");
            Primitive* pPrimitive = &(*pPrimitives)[primitive_index];
            Material*  pMaterial  = &(*pMaterials)[material_index];
            item.renderables.emplace(pMaterial, pPrimitive);

            // FIXME: multiple primitives per mesh.
            // break;
        }

        // Create uniform buffer.
        grfx::BufferCreateInfo bufferCreateInfo        = {};
        bufferCreateInfo.size                          = RoundUp(512, PPX_CONSTANT_BUFFER_ALIGNMENT);
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateBuffer(&bufferCreateInfo, &item.pUniformBuffer));

        objects->emplace_back(std::move(item));
    }
}

void ProjApp::Setup()
{
    // Cameras
    {
        mCamera = PerspCamera(60.0f, GetWindowAspect());
    }

    // Create descriptor pool large enough for this project
    {
        grfx::DescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.uniformBuffer                  = 512;
        poolCreateInfo.sampledImage                   = 512;
        poolCreateInfo.sampler                        = 512;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    LoadScene(GetAssetPath("basic/models/altimeter/altimeter.gltf"), GetDevice(), GetSwapchain(), GetGraphicsQueue(), mDescriptorPool, &mObjects, &mPrimitives, &mMaterials);

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
    PerFrame&          frame      = mPerFrame[0];
    grfx::SwapchainPtr swapchain  = GetSwapchain();
    uint32_t           imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update light position
    const float t  = GetElapsedSeconds() / 2.0f;
    const float r  = 7.0f;
    mLightPosition = float3(r * cos(t), 5.0f, r * sin(t));
    // Update camera(s)
    mCamera.LookAt(float3(5, 7, 7), float3(0, 1, 0));

    // Update uniform buffers
    for (auto& object : mObjects) {
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

        object.pUniformBuffer->CopyFromSource(sizeof(scene), &scene);
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
            for (auto& object : mObjects) {
                for (auto& [pMaterial, pPrimitive] : object.renderables) {
                    std::array<grfx::WriteDescriptor, 1 + 3 * 2> write;
                    std::memset(write.data(), 0, sizeof(write[0]) * write.size());

                    write[0].binding      = 0;
                    write[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    write[0].bufferOffset = 0;
                    write[0].bufferRange  = PPX_WHOLE_SIZE;
                    write[0].pBuffer      = object.pUniformBuffer;

                    for (size_t i = 0; i < 3; i++) {
                        write[1 + i * 2 + 0].binding    = 1 + i * 2 + 0;
                        write[1 + i * 2 + 0].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        write[1 + i * 2 + 0].pImageView = pMaterial->textures[i].pTexture;
                        write[1 + i * 2 + 1].binding    = 1 + i * 2 + 1;
                        write[1 + i * 2 + 1].type       = grfx::DESCRIPTOR_TYPE_SAMPLER;
                        write[1 + i * 2 + 1].pSampler   = pMaterial->textures[i].pSampler;
                    }

                    PPX_CHECKED_CALL(pMaterial->pDescriptorSet->UpdateDescriptors(write.size(), write.data()));

                    frame.cmd->BindGraphicsPipeline(pMaterial->pPipeline);
                    frame.cmd->BindGraphicsDescriptorSets(pMaterial->pInterface, 1, &pMaterial->pDescriptorSet);

                    frame.cmd->BindIndexBuffer(pPrimitive->mesh);
                    frame.cmd->BindVertexBuffers(pPrimitive->mesh);
                    frame.cmd->DrawIndexed(pPrimitive->mesh->GetIndexCount());
                }
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
{
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);

    return res;
}
