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

#include "ppx/ppx.h"
#include "ppx/knob.h"
#include "ppx/timer.h"
#include "ppx/camera.h"
#include "ppx/graphics_util.h"
#include "ppx/grfx/grfx_scope.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "glm/gtc/type_ptr.hpp"

using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

static constexpr std::array<const char*, 2> kAvailableVsShaders = {
    "Benchmark_VsSimple",
    "Benchmark_VsAluBound"};

static constexpr std::array<const char*, 3> kAvailablePsShaders = {
    "Benchmark_PsSimple",
    "Benchmark_PsAluBound",
    "Benchmark_PsMemBound"};

static constexpr std::array<const char*, 7> kAvailableScenes = {
    "altimeter",
    "sphere_0",
    "sphere_1",
    "sphere_2",
    "sphere_3",
    "sphere_4",
    "sphere_5"};

static constexpr std::array<const char*, kAvailableScenes.size()> kAvailableScenesFilePath = {
    "basic/models/altimeter/altimeter.gltf",
    "basic/models/spheres/sphere_0.gltf",
    "basic/models/spheres/sphere_1.gltf",
    "basic/models/spheres/sphere_2.gltf",
    "basic/models/spheres/sphere_3.gltf",
    "basic/models/spheres/sphere_4.gltf",
    "basic/models/spheres/sphere_5.gltf"};

static constexpr uint32_t kPipelineCount = kAvailablePsShaders.size() * kAvailableVsShaders.size();

class ProjApp
    : public ppx::Application
{
public:
    virtual void InitKnobs() override;
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

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
        grfx::PipelineInterfacePtr                            pInterface;
        std::array<grfx::GraphicsPipelinePtr, kPipelineCount> mPipelines;
        grfx::DescriptorSetPtr                                pDescriptorSet;
        std::vector<Texture>                                  textures;
    };

    struct Primitive
    {
        grfx::Mesh* mesh;
    };

    struct Renderable
    {
        Material*            pMaterial;
        Primitive*           pPrimitive;
        grfx::DescriptorSet* pDescriptorSet;

        Renderable(Material* m, Primitive* p, grfx::DescriptorSet* set)
            : pMaterial(m), pPrimitive(p), pDescriptorSet(set) {}
    };

    struct Object
    {
        float4x4                modelMatrix;
        float4x4                ITModelMatrix;
        grfx::BufferPtr         pUniformBuffer;
        std::vector<Renderable> renderables;
    };

    struct Scene
    {
        std::vector<Object>    objects;
        std::vector<Material>  materials;
        std::vector<Primitive> primitives;
    };

    using RenderList   = std::unordered_map<Material*, std::vector<Object*>>;
    using TextureCache = std::unordered_map<std::string, grfx::ImagePtr>;

    std::vector<PerFrame>                                         mPerFrame;
    grfx::DescriptorPoolPtr                                       mDescriptorPool;
    grfx::DescriptorSetLayoutPtr                                  mSetLayout;
    std::array<grfx::ShaderModulePtr, kAvailableVsShaders.size()> mVsShaders;
    std::array<grfx::ShaderModulePtr, kAvailablePsShaders.size()> mPsShaders;
    PerspCamera                                                   mCamera;
    float3                                                        mLightPosition = float3(10, 100, 10);
    std::array<Scene, kAvailableScenes.size()>                    mScenes;
    size_t                                                        mCurrentSceneIndex;

    TextureCache mTextureCache;

private:
    std::shared_ptr<KnobDropdown<std::string>> pKnobVs;
    std::shared_ptr<KnobDropdown<std::string>> pKnobPs;
    std::shared_ptr<KnobDropdown<std::string>> pCurrentScene;
    std::shared_ptr<KnobCheckbox>              pKnobPlaceholder1;
    std::shared_ptr<KnobSlider<int>>           pKnobPlaceholder2;
    std::shared_ptr<KnobDropdown<std::string>> pKnobPlaceholder3;
    std::vector<std::string>                   placeholder3Choices = {"one", "two", "three"};

private:
    void LoadScene(
        const std::filesystem::path& filename,
        grfx::Device*                pDevice,
        grfx::Swapchain*             pSwapchain,
        grfx::Queue*                 pQueue,
        grfx::DescriptorPool*        pDescriptorPool,
        TextureCache*                pTextureCache,
        std::vector<Object>*         pObjects,
        std::vector<Primitive>*      pPrimitives,
        std::vector<Material>*       pMaterials) const;

    void LoadMaterial(
        const std::filesystem::path& gltfFolder,
        const cgltf_material&        material,
        grfx::Swapchain*             pSwapchain,
        grfx::Queue*                 pQueue,
        grfx::DescriptorPool*        pDescriptorPool,
        TextureCache*                pTextureCache,
        Material*                    pOutput) const;

    void LoadTexture(
        const std::filesystem::path& gltfFolder,
        const cgltf_texture_view&    textureView,
        grfx::Queue*                 pQueue,
        TextureCache*                pTextureCache,
        Texture*                     pOutput) const;

    void LoadTexture(
        const Bitmap& bitmap,
        grfx::Queue*  pQueue,
        Texture*      pOutput) const;

    // Load the given primitive to the GPU.
    // `pStagingBuffer` must already contain all data referenced by `primitive`.
    void LoadPrimitive(
        const cgltf_primitive& primitive,
        grfx::BufferPtr        pStagingBuffer,
        grfx::Queue*           pQueue,
        Primitive*             pOutput) const;

    void LoadNodes(
        const cgltf_data*                                         data,
        grfx::Queue*                                              pQueue,
        grfx::DescriptorPool*                                     pDescriptorPool,
        std::vector<Object>*                                      objects,
        const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
        std::vector<Primitive>*                                   pPrimitives,
        std::vector<Material>*                                    pMaterials) const;

    void UpdateGUI();
};

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

void ProjApp::InitKnobs()
{
    const auto& cl_options = GetExtraOptions();
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("vs-shader-index"), "--vs-shader-index flag has been replaced, instead use --vs and specify the name of the vertex shader");
    PPX_ASSERT_MSG(!cl_options.HasExtraOption("ps-shader-index"), "--ps-shader-index flag has been replaced, instead use --ps and specify the name of the pixel shader");

    pKnobVs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("vs", 0, kAvailableVsShaders);
    pKnobVs->SetDisplayName("Vertex Shader");

    pKnobPs = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("ps", 0, kAvailablePsShaders);
    pKnobPs->SetDisplayName("Pixel Shader");

    pCurrentScene = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("scene", 0, kAvailableScenes);
    pCurrentScene->SetDisplayName("Scene");

    pKnobPlaceholder1 = GetKnobManager().CreateKnob<ppx::KnobCheckbox>("placeholder1", false);
    pKnobPlaceholder2 = GetKnobManager().CreateKnob<ppx::KnobSlider<int>>("placeholder2", 5, 0, 10);
    pKnobPlaceholder3 = GetKnobManager().CreateKnob<ppx::KnobDropdown<std::string>>("placeholder3", 1, placeholder3Choices);
}

void ProjApp::LoadTexture(
    const std::filesystem::path& gltfFolder,
    const cgltf_texture_view&    textureView,
    grfx::Queue*                 pQueue,
    TextureCache*                pTextureCache,
    Texture*                     pOutput) const
{
    const auto& texture = *textureView.texture;
    PPX_ASSERT_MSG(textureView.texture != nullptr, "Texture with no image are not supported.");
    PPX_ASSERT_MSG(textureView.has_transform == false, "Texture transforms are not supported yet.");
    PPX_ASSERT_MSG(texture.image != nullptr, "image pointer is null.");
    PPX_ASSERT_MSG(texture.image->uri != nullptr, "image uri is null.");

    auto it = pTextureCache->find(texture.image->uri);
    if (it == pTextureCache->end()) {
        grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(PPX_REMAINING_MIP_LEVELS);
        PPX_CHECKED_CALL(grfx_util::CreateImageFromFile(pQueue, GetAssetPath(gltfFolder / texture.image->uri), &pOutput->pImage, options, false));
        pTextureCache->emplace(texture.image->uri, pOutput->pImage);
    }
    else {
        pOutput->pImage = it->second;
    }

    grfx::SampledImageViewCreateInfo sivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(pOutput->pImage);
    PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sivCreateInfo, &pOutput->pTexture));

    // FIXME: read sampler info from GLTF.
    grfx::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.anisotropyEnable        = true;
    samplerCreateInfo.maxAnisotropy           = 16;
    samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.minLod                  = 0.f;
    samplerCreateInfo.maxLod                  = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &pOutput->pSampler));
}

void ProjApp::LoadTexture(const Bitmap& bitmap, grfx::Queue* pQueue, Texture* pOutput) const
{
    grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(1);
    PPX_CHECKED_CALL(grfx_util::CreateImageFromBitmap(pQueue, &bitmap, &pOutput->pImage, options));

    grfx::SampledImageViewCreateInfo sivCreateInfo = grfx::SampledImageViewCreateInfo::GuessFromImage(pOutput->pImage);
    PPX_CHECKED_CALL(GetDevice()->CreateSampledImageView(&sivCreateInfo, &pOutput->pTexture));
    grfx::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.anisotropyEnable        = true;
    samplerCreateInfo.maxAnisotropy           = 1.f;
    samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &pOutput->pSampler));
}

Bitmap ColorToBitmap(const float3& color)
{
    Bitmap bitmap;
    PPX_CHECKED_CALL(Bitmap::Create(1, 1, Bitmap::Format::FORMAT_RGBA_FLOAT, &bitmap));
    float* ptr = bitmap.GetPixel32f(0, 0);
    ptr[0]     = color.r;
    ptr[1]     = color.g;
    ptr[2]     = color.b;
    ptr[3]     = 1.f;
    return bitmap;
}

void ProjApp::LoadMaterial(
    const std::filesystem::path& gltfFolder,
    const cgltf_material&        material,
    grfx::Swapchain*             pSwapchain,
    grfx::Queue*                 pQueue,
    grfx::DescriptorPool*        pDescriptorPool,
    TextureCache*                pTextureCache,
    Material*                    pOutput) const
{
    grfx::Device* pDevice = pQueue->GetDevice();
    if (material.extensions_count != 0) {
        printf("Material %s has extensions, but they are ignored. Rendered result may vary.\n", material.name);
    }

    // This is to simplify the pipeline creation for now. Need to revisit later.
    PPX_ASSERT_MSG(material.has_pbr_metallic_roughness, "Only PBR metallic roughness supported for now.");

    uint32_t pipeline_index = 0;
    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
            grfx::PipelineInterfaceCreateInfo piCreateInfo = {};
            piCreateInfo.setCount                          = 1;
            piCreateInfo.sets[0].set                       = 0;
            piCreateInfo.sets[0].pLayout                   = mSetLayout;
            PPX_CHECKED_CALL(pDevice->CreatePipelineInterface(&piCreateInfo, &pOutput->pInterface));

            grfx::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
            gpCreateInfo.VS                                = {mVsShaders[i].Get(), "vsmain"};
            gpCreateInfo.PS                                = {mPsShaders[j].Get(), "psmain"};

            // FIXME: assuming all primitives provides POSITION, UV, NORMAL and TANGENT. Might not be the case.
            gpCreateInfo.vertexInputState.bindingCount      = 4;
            gpCreateInfo.vertexInputState.bindings[0]       = mScenes[0].primitives[0].mesh->GetDerivedVertexBindings()[0];
            gpCreateInfo.vertexInputState.bindings[1]       = mScenes[0].primitives[0].mesh->GetDerivedVertexBindings()[1];
            gpCreateInfo.vertexInputState.bindings[2]       = mScenes[0].primitives[0].mesh->GetDerivedVertexBindings()[2];
            gpCreateInfo.vertexInputState.bindings[3]       = mScenes[0].primitives[0].mesh->GetDerivedVertexBindings()[3];
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

            PPX_CHECKED_CALL(pDevice->CreateGraphicsPipeline(&gpCreateInfo, &pOutput->mPipelines[pipeline_index++]));
        }
    }

    pOutput->textures.resize(3);
    if (material.pbr_metallic_roughness.base_color_texture.texture == nullptr) {
        float3 color = glm::make_vec3(material.pbr_metallic_roughness.base_color_factor);
        LoadTexture(ColorToBitmap(color), pQueue, &pOutput->textures[0]);
    }
    else {
        const auto& texture_path = material.pbr_metallic_roughness.base_color_texture;
        LoadTexture(gltfFolder, texture_path, pQueue, pTextureCache, &pOutput->textures[0]);
    }

    if (material.normal_texture.texture == nullptr) {
        LoadTexture(ColorToBitmap(float3(0.f, 0.f, 1.f)), pQueue, &pOutput->textures[1]);
    }
    else {
        LoadTexture(gltfFolder, material.normal_texture, pQueue, pTextureCache, &pOutput->textures[1]);
    }

    if (material.pbr_metallic_roughness.metallic_roughness_texture.texture == nullptr) {
        const auto& mtl   = material.pbr_metallic_roughness;
        float3      color = float3(mtl.metallic_factor, mtl.roughness_factor, 0.f);
        LoadTexture(ColorToBitmap(color), pQueue, &pOutput->textures[2]);
    }
    else {
        const auto& texture_path = material.pbr_metallic_roughness.metallic_roughness_texture;
        LoadTexture(gltfFolder, texture_path, pQueue, pTextureCache, &pOutput->textures[2]);
    }
}

static inline size_t CountPrimitives(const cgltf_mesh* array, size_t count)
{
    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        total += array[i].primitives_count;
    }
    return total;
}

static void GetAccessorsForPrimitive(const cgltf_primitive& ppPrimitive, const cgltf_accessor** ppPosition, const cgltf_accessor** ppUv, const cgltf_accessor** ppNormal, const cgltf_accessor** ppTangent)
{
    PPX_ASSERT_NULL_ARG(ppPosition);
    PPX_ASSERT_NULL_ARG(ppUv);
    PPX_ASSERT_NULL_ARG(ppNormal);
    PPX_ASSERT_NULL_ARG(ppTangent);

    *ppPosition = nullptr;
    *ppUv       = nullptr;
    *ppNormal   = nullptr;
    *ppTangent  = nullptr;

    for (size_t i = 0; i < ppPrimitive.attributes_count; i++) {
        const auto& type = ppPrimitive.attributes[i].type;
        const auto* data = ppPrimitive.attributes[i].data;
        if (type == cgltf_attribute_type_position) {
            *ppPosition = data;
        }
        else if (type == cgltf_attribute_type_normal) {
            *ppNormal = data;
        }
        else if (type == cgltf_attribute_type_tangent) {
            *ppTangent = data;
        }
        else if (type == cgltf_attribute_type_texcoord && *ppUv == nullptr) {
            // For UV we only load the first TEXCOORDs (FIXME: support multiple tex coordinates).
            *ppUv = data;
        }
    }

    PPX_ASSERT_MSG(*ppPosition != nullptr && *ppUv != nullptr && *ppNormal != nullptr && *ppTangent != nullptr, "For now, only supports model with position, normal, tangent and UV attributes");
}

void ProjApp::LoadPrimitive(const cgltf_primitive& primitive, grfx::BufferPtr pStagingBuffer, grfx::Queue* pQueue, Primitive* pOutput) const
{
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
    PPX_ASSERT_MSG(primitive.type == cgltf_primitive_type_triangles, "only supporting tri primitives for now.");
    PPX_ASSERT_MSG(!primitive.has_draco_mesh_compression, "draco compression not supported yet.");
    PPX_ASSERT_MSG(primitive.indices != nullptr, "only primitives with indices are supported for now.");

    // Attribute accessors.
    constexpr size_t                                   POSITION_INDEX  = 0;
    constexpr size_t                                   UV_INDEX        = 1;
    constexpr size_t                                   NORMAL_INDEX    = 2;
    constexpr size_t                                   TANGENT_INDEX   = 3;
    constexpr size_t                                   ATTRIBUTE_COUNT = 4;
    std::array<const cgltf_accessor*, ATTRIBUTE_COUNT> accessors;
    GetAccessorsForPrimitive(primitive, &accessors[POSITION_INDEX], &accessors[UV_INDEX], &accessors[NORMAL_INDEX], &accessors[TANGENT_INDEX]);

    const cgltf_accessor&      indices      = *primitive.indices;
    const cgltf_component_type indicesTypes = indices.component_type;

    grfx::MeshPtr targetMesh;
    {
        // Indices.
        PPX_ASSERT_MSG(indicesTypes == cgltf_component_type_r_16u || indicesTypes == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");

        // Create mesh.
        grfx::MeshCreateInfo ci;

        ci.indexType         = indicesTypes == cgltf_component_type_r_16u
                                   ? grfx::IndexType::INDEX_TYPE_UINT16
                                   : grfx::IndexType::INDEX_TYPE_UINT32;
        ci.indexCount        = static_cast<uint32_t>(indices.count);
        ci.vertexCount       = static_cast<uint32_t>(accessors[POSITION_INDEX]->count);
        ci.memoryUsage       = grfx::MEMORY_USAGE_GPU_ONLY;
        ci.vertexBufferCount = 4;

        for (size_t i = 0; i < accessors.size(); i++) {
            const auto& a  = *accessors[i];
            const auto& bv = *a.buffer_view;
            PPX_ASSERT_MSG(a.type == cgltf_type_vec2 || a.type == cgltf_type_vec3 || a.type == cgltf_type_vec4, "Non supported accessor type.");
            PPX_ASSERT_MSG(a.component_type == cgltf_component_type_r_32f, "only float for POS, NORM, TEX are supported.");

            ci.vertexBuffers[i].attributeCount       = 1;
            ci.vertexBuffers[i].vertexInputRate      = grfx::VERTEX_INPUT_RATE_VERTEX;
            ci.vertexBuffers[i].attributes[0].format = a.type == cgltf_type_vec2   ? grfx::FORMAT_R32G32_FLOAT
                                                       : a.type == cgltf_type_vec3 ? grfx::FORMAT_R32G32B32_FLOAT
                                                                                   : grfx::FORMAT_R32G32B32A32_FLOAT;
            ci.vertexBuffers[i].attributes[0].stride = static_cast<uint32_t>(bv.stride == 0 ? a.stride : bv.stride);

            constexpr std::array<grfx::VertexSemantic, ATTRIBUTE_COUNT> semantics = {
                grfx::VERTEX_SEMANTIC_POSITION,
                grfx::VERTEX_SEMANTIC_TEXCOORD,
                grfx::VERTEX_SEMANTIC_NORMAL,
                grfx::VERTEX_SEMANTIC_TANGENT};
            ci.vertexBuffers[i].attributes[0].vertexSemantic = semantics[i];
        }
        PPX_CHECKED_CALL(pQueue->GetDevice()->CreateMesh(&ci, &targetMesh));
        SCOPED_DESTROYER.AddObject(targetMesh);
    }

    // Copy geometry data to mesh.
    {
        const auto& bufferView = *indices.buffer_view;
        PPX_ASSERT_MSG(indicesTypes == cgltf_component_type_r_16u || indicesTypes == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");
        PPX_ASSERT_MSG(bufferView.data == nullptr, "Doesn't support extra data");

        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.size                         = targetMesh->GetIndexBuffer()->GetSize();
        copyInfo.srcBuffer.offset             = indices.offset + bufferView.offset;
        copyInfo.dstBuffer.offset             = 0;
        PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, targetMesh->GetIndexBuffer(), grfx::RESOURCE_STATE_INDEX_BUFFER, grfx::RESOURCE_STATE_INDEX_BUFFER));
        for (size_t i = 0; i < accessors.size(); i++) {
            const auto& bufferView = *accessors[i]->buffer_view;

            const auto&                  vertexBuffer = targetMesh->GetVertexBuffer(i);
            grfx::BufferToBufferCopyInfo copyInfo     = {};
            copyInfo.size                             = vertexBuffer->GetSize();
            copyInfo.srcBuffer.offset                 = accessors[i]->offset + bufferView.offset;
            copyInfo.dstBuffer.offset                 = 0;
            PPX_CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, vertexBuffer, grfx::RESOURCE_STATE_VERTEX_BUFFER, grfx::RESOURCE_STATE_VERTEX_BUFFER));
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
    TextureCache*                pTextureCache,
    std::vector<Object>*         pObjects,
    std::vector<Primitive>*      pPrimitives,
    std::vector<Material>*       pMaterials) const
{
    Timer timerGlobal;
    timerGlobal.Start();

    Timer timerModelLoading;
    timerModelLoading.Start();
    const std::filesystem::path gltfFolder = std::filesystem::path(filename).remove_filename();
    cgltf_data*                 data       = nullptr;
    {
        const std::filesystem::path gltfFilePath = GetAssetPath(filename);
        PPX_ASSERT_MSG(gltfFilePath != "", "Cannot resolve asset path.");
        cgltf_options options = {};
        cgltf_result  result  = cgltf_parse_file(&options, gltfFilePath.string().c_str(), &data);
        PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading GLB file.");
        result = cgltf_validate(data);
        PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while validating GLB file.");
        result = cgltf_load_buffers(&options, data, gltfFilePath.string().c_str());
        PPX_ASSERT_MSG(result == cgltf_result_success, "Failure while loading buffers.");

        PPX_ASSERT_MSG(data->buffers_count == 1, "Only supports one buffer for now.");
        PPX_ASSERT_MSG(data->buffers[0].data != nullptr, "Data not loaded. Was cgltf_load_buffer called?");
    }
    const double timerModelLoadingElapsed = timerModelLoading.SecondsSinceStart();

    Timer timerStagingBufferLoading;
    timerStagingBufferLoading.Start();
    grfx::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
    // Copy main buffer data to staging buffer.
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
    const double timerStagingBufferLoadingElapsed = timerStagingBufferLoading.SecondsSinceStart();

    Timer timerPrimitiveLoading;
    timerPrimitiveLoading.Start();
    std::unordered_map<const cgltf_primitive*, size_t> primitiveToIndex;
    pPrimitives->resize(CountPrimitives(data->meshes, data->meshes_count));
    {
        size_t nextSlot = 0;
        for (size_t i = 0; i < data->meshes_count; i++) {
            const auto& mesh = data->meshes[i];
            for (size_t j = 0; j < mesh.primitives_count; j++) {
                LoadPrimitive(mesh.primitives[j], stagingBuffer, pQueue, &(*pPrimitives)[nextSlot]);
                primitiveToIndex.insert({&mesh.primitives[j], nextSlot});
                nextSlot++;
            }
        }
    }
    const double timerPrimitiveLoadingElapsed = timerPrimitiveLoading.SecondsSinceStart();

    Timer timerMaterialLoading;
    timerMaterialLoading.Start();
    pMaterials->resize(data->materials_count);
    for (size_t i = 0; i < data->materials_count; i++) {
        LoadMaterial(gltfFolder, data->materials[i], pSwapchain, pQueue, pDescriptorPool, pTextureCache, &(*pMaterials)[i]);
    }
    const double timerMaterialLoadingElapsed = timerMaterialLoading.SecondsSinceStart();

    Timer timerNodeLoading;
    timerNodeLoading.Start();
    LoadNodes(data, pQueue, pDescriptorPool, pObjects, primitiveToIndex, pPrimitives, pMaterials);
    const double timerNodeLoadingElapsed = timerNodeLoading.SecondsSinceStart();

    printf("Scene loading time breakdown for '%s':\n", filename.c_str());
    printf("\t             total: %lfs\n", timerGlobal.SecondsSinceStart());
    printf("\t      GLtf parsing: %lfs\n", timerModelLoadingElapsed);
    printf("\t    staging buffer: %lfs\n", timerStagingBufferLoadingElapsed);
    printf("\tprimitives loading: %lfs\n", timerPrimitiveLoadingElapsed);
    printf("\t materials loading: %lfs\n", timerMaterialLoadingElapsed);
    printf("\t     nodes loading: %lfs\n", timerNodeLoadingElapsed);
}

float4x4 ComputeObjectMatrix(const cgltf_node* node)
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
    grfx::Queue*                                              pQueue,
    grfx::DescriptorPool*                                     pDescriptorPool,
    std::vector<Object>*                                      objects,
    const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
    std::vector<Primitive>*                                   pPrimitives,
    std::vector<Material>*                                    pMaterials) const
{
    const size_t nodeCount = data->nodes_count;
    for (size_t i = 0; i < nodeCount; i++) {
        const auto& node = data->nodes[i];
        if (node.mesh == nullptr) {
            continue;
        }

        Object item;
        item.modelMatrix   = ComputeObjectMatrix(&node);
        item.ITModelMatrix = glm::inverse(glm::transpose(item.modelMatrix));

        for (size_t j = 0; j < node.mesh->primitives_count; j++) {
            const size_t primitive_index = primitiveToIndex.at(&node.mesh->primitives[j]);
            // FIXME: support meshes with no material.
            // For now, assign the first available material.
            PPX_ASSERT_MSG(pMaterials->size() != 0, "Doesn't support GLTF files with no materials.");
            cgltf_material* mtl            = node.mesh->primitives[j].material;
            const size_t    material_index = mtl == nullptr ? 0 : std::distance(data->materials, mtl);

            PPX_ASSERT_MSG(primitive_index < pPrimitives->size(), "Invalid GLB file. Primitive index out of range.");
            PPX_ASSERT_MSG(material_index < pMaterials->size(), "Invalid GLB file. Material index out of range.");
            Primitive* pPrimitive = &(*pPrimitives)[primitive_index];
            Material*  pMaterial  = &(*pMaterials)[material_index];

            grfx::DescriptorSet* pDescriptorSet = nullptr;
            PPX_CHECKED_CALL(pQueue->GetDevice()->AllocateDescriptorSet(pDescriptorPool, mSetLayout, &pDescriptorSet));
            item.renderables.emplace_back(pMaterial, pPrimitive, pDescriptorSet);
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
        poolCreateInfo.uniformBuffer                  = 1024;
        poolCreateInfo.sampledImage                   = 1024;
        poolCreateInfo.sampler                        = 1024;
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&poolCreateInfo, &mDescriptorPool));
    }

    for (size_t i = 0; i < kAvailableVsShaders.size(); i++) {
        const std::string vsShaderBaseName = kAvailableVsShaders[i];
        std::vector<char> bytecode         = LoadShader("benchmarks/shaders", vsShaderBaseName + ".vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mVsShaders[i]));
    }

    for (size_t j = 0; j < kAvailablePsShaders.size(); j++) {
        const std::string psShaderBaseName = kAvailablePsShaders[j];
        std::vector<char> bytecode         = LoadShader("benchmarks/shaders", psShaderBaseName + ".ps");
        PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &mPsShaders[j]));
    }

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

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&layoutCreateInfo, &mSetLayout));
    }

    for (size_t i = 0; i < kAvailableScenesFilePath.size(); i++) {
        LoadScene(
            kAvailableScenesFilePath[i],
            GetDevice(),
            GetSwapchain(),
            GetGraphicsQueue(),
            mDescriptorPool,
            &mTextureCache,
            &mScenes[i].objects,
            &mScenes[i].primitives,
            &mScenes[i].materials);
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

    for (const auto& shader : mVsShaders) {
        GetDevice()->DestroyShaderModule(shader);
    }
    for (const auto& shader : mPsShaders) {
        GetDevice()->DestroyShaderModule(shader);
    }
}

void ProjApp::Render()
{
    // This is important: If we directly passed mCurrentSceneIndex to ImGUI, the value would change during
    // the drawing pass, meaning we would change descriptors while drawing.
    // That's why we delay the change to the next frame (now).
    mCurrentSceneIndex = pCurrentScene->GetIndex();

    // Example where changing either the slider or the dropdown will uncheck the box.
    if (pKnobPlaceholder2->DigestUpdate()) {
        std::cout << "placeholder2 knob new value: " << pKnobPlaceholder2->GetValue() << std::endl;
        pKnobPlaceholder1->SetValue(false);
    }
    if (pKnobPlaceholder3->DigestUpdate()) {
        std::cout << "placeholder3 knob new value: " << pKnobPlaceholder3->GetValue() << std::endl;
        pKnobPlaceholder1->SetValue(false);
    }
    if (pKnobPlaceholder1->DigestUpdate()) {
        std::cout << "placeholder1 knob new value: " << pKnobPlaceholder1->GetValue() << std::endl;
    }

    PerFrame&          frame      = mPerFrame[0];
    grfx::SwapchainPtr swapchain  = GetSwapchain();
    uint32_t           imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // Update camera(s)
    mCamera.LookAt(float3(2, 2, 2), float3(0, 0, 0));

    // Update uniform buffers
    for (auto& object : mScenes[mCurrentSceneIndex].objects) {
        struct FrameData
        {
            float4x4 modelMatrix;                // Transforms object space to world space
            float4x4 ITModelMatrix;              // Inverse-transpose of the model matrix.
            float4   ambient;                    // Object's ambient intensity
            float4x4 cameraViewProjectionMatrix; // Camera's view projection matrix
            float4   lightPosition;              // Light's position
            float4   eyePosition;
        };

        FrameData data                  = {};
        data.modelMatrix                = object.modelMatrix;
        data.ITModelMatrix              = object.ITModelMatrix;
        data.ambient                    = float4(0.3f);
        data.cameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        data.lightPosition              = float4(mLightPosition, 0);
        data.eyePosition                = float4(mCamera.GetEyePosition(), 0.f);

        object.pUniformBuffer->CopyFromSource(sizeof(data), &data);
    }

    {
        // FIXME: this assumes we have only PBR, and with 3 textures per materials. Needs to be revisited.
        constexpr size_t                                    TEXTURE_COUNT    = 3;
        constexpr size_t                                    DESCRIPTOR_COUNT = 1 + TEXTURE_COUNT * 2 /* uniform + 3 * (sampler + texture) */;
        std::array<grfx::WriteDescriptor, DESCRIPTOR_COUNT> write;
        for (auto& object : mScenes[mCurrentSceneIndex].objects) {
            for (auto& renderable : object.renderables) {
                auto* pPrimitive     = renderable.pPrimitive;
                auto* pMaterial      = renderable.pMaterial;
                auto* pDescriptorSet = renderable.pDescriptorSet;

                write[0].binding      = 0;
                write[0].type         = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write[0].bufferOffset = 0;
                write[0].bufferRange  = PPX_WHOLE_SIZE;
                write[0].pBuffer      = object.pUniformBuffer;

                for (size_t i = 0; i < TEXTURE_COUNT; i++) {
                    write[1 + i * 2 + 0].binding    = 1 + i * 2 + 0;
                    write[1 + i * 2 + 0].type       = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    write[1 + i * 2 + 0].pImageView = pMaterial->textures[i].pTexture;
                    write[1 + i * 2 + 1].binding    = 1 + i * 2 + 1;
                    write[1 + i * 2 + 1].type       = grfx::DESCRIPTOR_TYPE_SAMPLER;
                    write[1 + i * 2 + 1].pSampler   = pMaterial->textures[i].pSampler;
                }
                PPX_CHECKED_CALL(pDescriptorSet->UpdateDescriptors(write.size(), write.data()));
            }
        }
    }

    UpdateGUI();

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

            size_t pipeline_index = pKnobVs->GetIndex() * kAvailablePsShaders.size() + pKnobPs->GetIndex();
            // Draw entities
            for (auto& object : mScenes[mCurrentSceneIndex].objects) {
                for (auto& renderable : object.renderables) {
                    frame.cmd->BindGraphicsPipeline(renderable.pMaterial->mPipelines[pipeline_index]);
                    frame.cmd->BindGraphicsDescriptorSets(renderable.pMaterial->pInterface, 1, &renderable.pDescriptorSet);

                    frame.cmd->BindIndexBuffer(renderable.pPrimitive->mesh);
                    frame.cmd->BindVertexBuffers(renderable.pPrimitive->mesh);
                    frame.cmd->DrawIndexed(renderable.pPrimitive->mesh->GetIndexCount());
                }
            }

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

void ProjApp::UpdateGUI()
{
    if (!GetSettings()->enableImGui) {
        return;
    }

    // GUI
    GetKnobManager().DrawAllKnobs();
}

SETUP_APPLICATION(ProjApp)
