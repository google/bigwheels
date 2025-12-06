// Copyright 2024 Google LLC
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

#ifndef ppx_scene_gltf_loader_h
#define ppx_scene_gltf_loader_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_loader.h"
#include "cgltf.h"

#include <set>

#if defined(WIN32) && defined(LoadImage)
#define _SAVE_MACRO_LoadImage LoadImage
#undef LoadImage
#endif

namespace ppx {
namespace scene {

class GltfMaterialSelector
{
public:
    GltfMaterialSelector();
    virtual ~GltfMaterialSelector();

    virtual std::string DetermineMaterial(const cgltf_material* pGltfMaterial) const;
};

// -------------------------------------------------------------------------------------------------

class GltfLoader
{
private:
    // Make non-copyable
    GltfLoader(const GltfLoader&)            = delete;
    GltfLoader& operator=(const GltfLoader&) = delete;

protected:
    GltfLoader(
        const std::filesystem::path& filePath,
        const std::filesystem::path& textureDirPath,
        cgltf_data*                  pGltfData,
        bool                         ownsGtlfData,
        scene::GltfMaterialSelector* pMaterialSelector,
        bool                         ownsMaterialSelector);

public:
    ~GltfLoader();

    // Calling code is responsbile for lifetime of pMaterialSeelctor
    static ppx::Result Create(
        const std::filesystem::path& filePath,
        const std::filesystem::path& textureDirPath,
        scene::GltfMaterialSelector* pMaterialSelector, // Use null for default selector
        GltfLoader**                 ppLoader);

    // Calling code is responsbile for lifetime of pMaterialSeelctor
    static ppx::Result Create(
        const std::filesystem::path& path,
        scene::GltfMaterialSelector* pMaterialSelector, // Use null for default selector
        GltfLoader**                 ppLoader);

    cgltf_data* GetGltfData() const { return mGltfData; }
    bool        HasGltfData() const { return (mGltfData != nullptr); }

    uint32_t GetSamplerCount() const;
    uint32_t GetImageCount() const;
    uint32_t GetTextureCount() const;
    uint32_t GetMaterialCount() const;
    uint32_t GetMeshCount() const;
    uint32_t GetNodeCount() const;
    uint32_t GetSceneCount() const;

    // These functions return a -1 if the object cannot be located by name.
    int32_t GetSamplerIndex(const std::string& name) const;
    int32_t GetImageIndex(const std::string& name) const;
    int32_t GetTextureIndex(const std::string& name) const;
    int32_t GetMaterialIndex(const std::string& name) const;
    int32_t GetMeshIndex(const std::string& name) const;
    int32_t GetNodeIndex(const std::string& name) const;
    int32_t GetSceneIndex(const std::string& name) const;

    // ---------------------------------------------------------------------------------------------
    // Loads a GLTF sampler, image, texture, or material
    //
    // These load functions create standalone objects that can be used
    // outside of a scene. Caching is not used.
    //
    // ---------------------------------------------------------------------------------------------
    ppx::Result LoadSampler(
        grfx::Device*    pDevice,
        uint32_t         samplerIndex,
        scene::Sampler** ppTargetSampler);

    ppx::Result LoadSampler(
        grfx::Device*      pDevice,
        const std::string& samplerName,
        scene::Sampler**   ppTargetSampler);

    ppx::Result LoadImage(
        grfx::Device*  pDevice,
        uint32_t       imageIndex,
        scene::Image** ppTargetImage);

    ppx::Result LoadImage(
        grfx::Device*      pDevice,
        const std::string& imageName,
        scene::Image**     ppTargetImage);

    ppx::Result LoadTexture(
        grfx::Device*    pDevice,
        uint32_t         textureIndex,
        scene::Texture** ppTargetTexture);

    ppx::Result LoadTexture(
        grfx::Device*      pDevice,
        const std::string& textureName,
        scene::Texture**   ppTargetTexture);

    ppx::Result LoadMaterial(
        grfx::Device*             pDevice,
        uint32_t                  materialIndex,
        scene::Material**         ppTargetMaterial,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    ppx::Result LoadMaterial(
        grfx::Device*             pDevice,
        const std::string&        materialName,
        scene::Material**         ppTargetMaterial,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    // ---------------------------------------------------------------------------------------------
    // Loads a GLTF mesh
    //
    // This function creates a standalone mesh that usable outside of a scene.
    //
    // Standalone meshes uses an internal scene::ResourceManager to
    // manage its required objects. All required objects created as apart
    // of the mesh loading will be managed by the mesh. The reasoning
    // for this is because images, textures, and materials can be shared
    // between primitive batches.
    //
    // Object sharing requires that the lifetime of an object to be managed
    // by an external mechanism, scene::ResourceManager. When a mesh is
    // destroyed, its destructor calls scene::ResourceManager::DestroyAll
    // to destroy its reference to the shared objects. If afterwards a shared
    // object has an external reference, the code holding the reference
    // is responsible for the shared objct.
    //
    // activeVertexAttributes - specifies the attributes that the a mesh
    // should be created with. If a GLTF file doesn't provide data for
    // an attribute, an sensible default value is used - usually zeroes.
    //
    // ---------------------------------------------------------------------------------------------
    ppx::Result LoadMesh(
        grfx::Device*             pDevice,
        uint32_t                  meshIndex,
        scene::Mesh**             ppTargetMesh,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    ppx::Result LoadMesh(
        grfx::Device*             pDevice,
        const std::string&        meshName,
        scene::Mesh**             ppTargetMesh,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    // ---------------------------------------------------------------------------------------------
    // Loads a GLTF node
    // ---------------------------------------------------------------------------------------------
    ppx::Result LoadNode(
        grfx::Device*             pDevice,
        uint32_t                  nodeIndex,
        scene::Node**             ppTargetNode,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    ppx::Result LoadNode(
        grfx::Device*             pDevice,
        const std::string&        nodeName,
        scene::Node**             ppTargetNode,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    ppx::Result LoadNodeTransformOnly(
        uint32_t      nodeIndex,
        scene::Node** ppTargetNode);

    ppx::Result LoadNodeTransformOnly(
        const std::string& nodeName,
        scene::Node**      ppTargetNode);

    // ---------------------------------------------------------------------------------------------
    // Loads a GLTF scene
    //
    // @TODO: Figure out a way to load more than one GLTF scene into a
    //        a target scene object without cache stomping.
    //
    // ---------------------------------------------------------------------------------------------
    ppx::Result LoadScene(
        grfx::Device*             pDevice,
        uint32_t                  sceneIndex, // Use 0 if unsure
        scene::Scene**            ppTargetScene,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

    ppx::Result LoadScene(
        grfx::Device*             pDevice,
        const std::string&        sceneName,
        scene::Scene**            ppTargetScene,
        const scene::LoadOptions& loadOptions = scene::LoadOptions());

private:
    // Stores a look up of a vertex attribute mask that is comprised of all
    // the required vertex attributes in a meshes materials in a given
    // material factory.
    //
    // For example:
    //   If a mesh has 2 primitives that uses 2 different materials, A and B:
    //     - material A requires only tex-coords
    //     - material B requires normals and tex-coords
    //   Then the mask for the mesh would be an OR of material A's material flags
    //   and material B's required flags, resulting in:
    //     mask.texCoords = true;
    //     mask.normals   = true;
    //     mask.tangetns  = false;
    //     mask.colors    = false;
    //
    // This masks enables the loader to select the different vertex attributes
    // required by a meshes mesh data so that it doesn't have to generically
    // load all attributes if the mesh data is shared between multiple meshes.
    //
    using MeshMaterialVertexAttributeMasks = std::unordered_map<const cgltf_mesh*, scene::VertexAttributeFlags>;

    void CalculateMeshMaterialVertexAttributeMasks(
        const scene::MaterialFactory*                 pMaterialFactory,
        GltfLoader::MeshMaterialVertexAttributeMasks* pOutMasks) const;

    struct InternalLoadParams
    {
        grfx::Device*                     pDevice                           = nullptr;
        scene::MaterialFactory*           pMaterialFactory                  = nullptr;
        scene::VertexAttributeFlags       requiredVertexAttributes          = scene::VertexAttributeFlags::None();
        scene::ResourceManager*           pResourceManager                  = nullptr;
        MeshMaterialVertexAttributeMasks* pMeshMaterialVertexAttributeMasks = nullptr;
        bool                              transformOnly                     = false;
        scene::Scene*                     pTargetScene                      = nullptr;

        struct
        {
            uint64_t image    = 0;
            uint64_t sampler  = 0;
            uint64_t texture  = 0;
            uint64_t material = 0;
            uint64_t mesh     = 0;
        } baseObjectIds;
    };

    // To avoid potential cache collision when loading multiple GLTF files into the
    // same scene we need to apply an offset (base object id) to the object index
    // so that the final object id is unique.
    //
    // These functions calculate the object id used when caching the resource.
    //
    uint64_t CalculateImageObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
    uint64_t CalculateSamplerObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
    uint64_t CalculateTextureObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
    uint64_t CalculateMaterialObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
    uint64_t CalculateMeshObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndexk);

    ppx::Result LoadSamplerInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_sampler*                  pGltfSampler,
        scene::Sampler**                      ppTargetSampler);

    ppx::Result FetchSamplerInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_sampler*                  pGltfSampler,
        scene::SamplerRef&                    outSampler);

    ppx::Result LoadImageInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_image*                    pGltfImage,
        scene::Image**                        ppTargetImage);

    ppx::Result FetchImageInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_image*                    pGltfImage,
        scene::ImageRef&                      outImage);

    ppx::Result LoadTextureInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_texture*                  pGltfTexture,
        scene::Texture**                      pptTexture);

    ppx::Result FetchTextureInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_texture*                  pGltfTexture,
        scene::TextureRef&                    outTexture);

    ppx::Result LoadTextureViewInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_texture_view*             pGltfTextureView,
        scene::TextureView*                   pTargetTextureView);

    ppx::Result LoadUnlitMaterialInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_material*                 pGltfMaterial,
        scene::UnlitMaterial*                 pTargetMaterial);

    ppx::Result LoadPbrMetallicRoughnessMaterialInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_material*                 pGltfMaterial,
        scene::StandardMaterial*              pTargetMaterial);

    ppx::Result LoadMaterialInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_material*                 pGltfMaterial,
        scene::Material**                     ppTargetMaterial);

    ppx::Result FetchMaterialInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_material*                 pGltfMaterial,
        scene::MaterialRef&                   outMaterial);

    ppx::Result LoadMeshData(
        const GltfLoader::InternalLoadParams& extgernalLoadParams,
        const cgltf_mesh*                     pGltfMesh,
        scene::MeshDataRef&                   outMeshData,
        std::vector<scene::PrimitiveBatch>&   outBatches);

    ppx::Result LoadMeshInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_mesh*                     pGltfMesh,
        scene::Mesh**                         ppTargetMesh);

    ppx::Result FetchMeshInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_mesh*                     pGltfMesh,
        scene::MeshRef&                       outMesh);

    ppx::Result LoadNodeInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_node*                     pGltfNode,
        scene::Node**                         ppTargetNode);

    ppx::Result FetchNodeInternal(
        const GltfLoader::InternalLoadParams& loadParams,
        const cgltf_node*                     pGltfNode,
        scene::NodeRef&                       outNode);

    ppx::Result LoadSceneInternal(
        const GltfLoader::InternalLoadParams& externalLoadParams,
        const cgltf_scene*                    pGltfScene,
        scene::Scene*                         pTargetScene);

private:
    // Builds a set of node indices that include pGltfNode and all its children.
    void GetUniqueGltfNodeIndices(
        const cgltf_node*     pGltfNode,
        std::set<cgltf_size>& uniqueGltfNodeIndices) const;

private:
    std::filesystem::path        mGltfFilePath           = "";
    std::filesystem::path        mGltfTextureDir         = ""; // This might be different than the parent dir of mGltfFilePath
    cgltf_data*                  mGltfData               = nullptr;
    bool                         mOwnsGltfData           = false;
    scene::GltfMaterialSelector* mMaterialSelector       = nullptr;
    bool                         mOwnsMaterialSelector   = false;
    scene::MaterialFactory       mDefaultMaterialFactory = {};
};

} // namespace scene
} // namespace ppx

#if defined(WIN32) && defined(LoadImage)
#define LoadImage _SAVE_MACRO_LoadImage
#endif

#endif // ppx_scene_gltf_loader_h
