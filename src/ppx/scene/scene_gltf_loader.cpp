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

#include "ppx/scene/scene_gltf_loader.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_scope.h"
#include "ppx/graphics_util.h"
#include "cgltf.h"
#include "xxhash.h"

#if defined(WIN32) && defined(LoadImage)
#undef LoadImage
#endif

namespace ppx {
namespace scene {

namespace {

#define GLTF_LOD_CLAMP_NONE 1000.0f

enum GltfTextureFilter
{
    GLTF_TEXTURE_FILTER_NEAREST = 9728,
    GLTF_TEXTURE_FILTER_LINEAR  = 9729,
};

enum GltfTextureWrap
{
    GLTF_TEXTURE_WRAP_REPEAT          = 10497,
    GLTF_TEXTURE_WRAP_CLAMP_TO_EDGE   = 33071,
    GLTF_TEXTURE_WRAP_MIRRORED_REPEAT = 33648,
};

struct VertexAccessors
{
    const cgltf_accessor* pPositions;
    const cgltf_accessor* pNormals;
    const cgltf_accessor* pTangents;
    const cgltf_accessor* pColors;
    const cgltf_accessor* pTexCoords;
};

static std::string ToStringSafe(const char* cStr)
{
    return IsNull(cStr) ? "" : std::string(cStr);
}

template <typename GltfObjectT>
static std::string GetName(const GltfObjectT* pGltfObject)
{
    PPX_ASSERT_MSG(!IsNull(pGltfObject), "Cannot get name of a NULL GLTF object, pGltfObject is NULL");
    return IsNull(pGltfObject->name) ? "" : std::string(pGltfObject->name);
}

static grfx::Format GetFormat(const cgltf_accessor* pGltfAccessor)
{
    if (IsNull(pGltfAccessor)) {
        return grfx::FORMAT_UNDEFINED;
    }

    // clang-format off
    switch (pGltfAccessor->type) {
        default: break;

        case cgltf_type_scalar: {
            switch (pGltfAccessor->component_type) {
                default: return grfx::FORMAT_UNDEFINED;
                case cgltf_component_type_r_8   : return grfx::FORMAT_R8_SINT;
                case cgltf_component_type_r_8u  : return grfx::FORMAT_R8_UINT;
                case cgltf_component_type_r_16  : return grfx::FORMAT_R16_SINT;
                case cgltf_component_type_r_16u : return grfx::FORMAT_R16_UINT;
                case cgltf_component_type_r_32u : return grfx::FORMAT_R32_UINT;
                case cgltf_component_type_r_32f : return grfx::FORMAT_R32_FLOAT;
            }
        } break;

        case cgltf_type_vec2: {
            switch (pGltfAccessor->component_type) {
                default: return grfx::FORMAT_UNDEFINED;
                case cgltf_component_type_r_8   : return grfx::FORMAT_R8G8_SINT;
                case cgltf_component_type_r_8u  : return grfx::FORMAT_R8G8_UINT;
                case cgltf_component_type_r_16  : return grfx::FORMAT_R16G16_SINT;
                case cgltf_component_type_r_16u : return grfx::FORMAT_R16G16_UINT;
                case cgltf_component_type_r_32u : return grfx::FORMAT_R32G32_UINT;
                case cgltf_component_type_r_32f : return grfx::FORMAT_R32G32_FLOAT;
            }
        } break;

        case cgltf_type_vec3: {
            switch (pGltfAccessor->component_type) {
                default: return grfx::FORMAT_UNDEFINED;
                case cgltf_component_type_r_8   : return grfx::FORMAT_R8G8B8_SINT;
                case cgltf_component_type_r_8u  : return grfx::FORMAT_R8G8B8_UINT;
                case cgltf_component_type_r_16  : return grfx::FORMAT_R16G16B16_SINT;
                case cgltf_component_type_r_16u : return grfx::FORMAT_R16G16B16_UINT;
                case cgltf_component_type_r_32u : return grfx::FORMAT_R32G32B32_UINT;
                case cgltf_component_type_r_32f : return grfx::FORMAT_R32G32B32_FLOAT;
            }
        } break;

        case cgltf_type_vec4: {
            switch (pGltfAccessor->component_type) {
                default: return grfx::FORMAT_UNDEFINED;
                case cgltf_component_type_r_8   : return grfx::FORMAT_R8G8B8A8_SINT;
                case cgltf_component_type_r_8u  : return grfx::FORMAT_R8G8B8A8_UINT;
                case cgltf_component_type_r_16  : return grfx::FORMAT_R16G16B16A16_SINT;
                case cgltf_component_type_r_16u : return grfx::FORMAT_R16G16B16A16_UINT;
                case cgltf_component_type_r_32u : return grfx::FORMAT_R32G32B32A32_UINT;
                case cgltf_component_type_r_32f : return grfx::FORMAT_R32G32B32A32_FLOAT;
            }
        } break;
    }
    // clang-format on

    return grfx::FORMAT_UNDEFINED;
}

static scene::NodeType GetNodeType(const cgltf_node* pGltfNode)
{
    if (IsNull(pGltfNode)) {
        return scene::NODE_TYPE_UNSUPPORTED;
    }

    if (!IsNull(pGltfNode->mesh)) {
        return scene::NODE_TYPE_MESH;
    }
    else if (!IsNull(pGltfNode->camera)) {
        return scene::NODE_TYPE_CAMERA;
    }
    else if (!IsNull(pGltfNode->light)) {
        return scene::NODE_TYPE_LIGHT;
    }
    else if (!IsNull(pGltfNode->skin) || !IsNull(pGltfNode->weights)) {
        return scene::NODE_TYPE_UNSUPPORTED;
    }

    return scene::NODE_TYPE_TRANSFORM;
}

static grfx::SamplerAddressMode ToSamplerAddressMode(cgltf_int mode)
{
    switch (mode) {
        default: break;
        case GLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case GLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return grfx::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    return grfx::SAMPLER_ADDRESS_MODE_REPEAT;
}

// Calcualte a unique hash based a meshes primitive accessors
static uint64_t GetMeshAccessorsHash(
    const cgltf_data* pGltfData,
    const cgltf_mesh* pGltfMesh)
{
    const auto kInvalidAccessorIndex = InvalidValue<cgltf_size>();

    std::set<cgltf_size> uniqueAccessorIndices;
    for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
        const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];

        // Indices
        if (!IsNull(pGltfPrimitive->indices)) {
            cgltf_size accessorIndex = cgltf_accessor_index(pGltfData, pGltfPrimitive->indices);
            uniqueAccessorIndices.insert(accessorIndex);
        }

        for (cgltf_size attrIdx = 0; attrIdx < pGltfPrimitive->attributes_count; ++attrIdx) {
            const cgltf_attribute* pGltfAttr     = &pGltfPrimitive->attributes[attrIdx];
            cgltf_size             accessorIndex = cgltf_accessor_index(pGltfData, pGltfAttr->data);

            switch (pGltfAttr->type) {
                default: break;
                case cgltf_attribute_type_position:
                case cgltf_attribute_type_normal:
                case cgltf_attribute_type_tangent:
                case cgltf_attribute_type_color:
                case cgltf_attribute_type_texcoord: {
                    uniqueAccessorIndices.insert(accessorIndex);
                } break;
            }
        }
    }

    // Copy to vector
    std::vector<cgltf_size> orderedAccessorIndices(uniqueAccessorIndices.begin(), uniqueAccessorIndices.end());

    // Sort accessor to indices
    std::sort(orderedAccessorIndices.begin(), orderedAccessorIndices.end());

    uint64_t hash = 0;
    if (!orderedAccessorIndices.empty()) {
        const XXH64_hash_t kSeed = 0x5874bc9de50a7627;

        hash = XXH64(
            DataPtr(orderedAccessorIndices),
            static_cast<size_t>(SizeInBytesU32(orderedAccessorIndices)),
            kSeed);
    }

    return hash;
}

static VertexAccessors GetVertexAccessors(const cgltf_primitive* pGltfPrimitive)
{
    VertexAccessors accessors{nullptr, nullptr, nullptr, nullptr, nullptr};
    if (IsNull(pGltfPrimitive)) {
        return accessors;
    }

    for (cgltf_size i = 0; i < pGltfPrimitive->attributes_count; ++i) {
        const cgltf_attribute* pGltfAttr     = &pGltfPrimitive->attributes[i];
        const cgltf_accessor*  pGltfAccessor = pGltfAttr->data;

        // clang-format off
        switch (pGltfAttr->type) {
            default: break;
            case cgltf_attribute_type_position : accessors.pPositions = pGltfAccessor; break;
            case cgltf_attribute_type_normal   : accessors.pNormals   = pGltfAccessor; break;
            case cgltf_attribute_type_tangent  : accessors.pTangents  = pGltfAccessor; break;
            case cgltf_attribute_type_color    : accessors.pColors    = pGltfAccessor; break;
            case cgltf_attribute_type_texcoord : accessors.pTexCoords = pGltfAccessor; break;
        };
        // clang-format on
    }
    return accessors;
}

// Get a buffer view's start address
static const void* GetStartAddress(
    const cgltf_buffer_view* pGltfBufferView)
{
    //
    // NOTE: Don't assert in this function since any of the fields can be NULL for different reasons.
    //

    if (IsNull(pGltfBufferView) || IsNull(pGltfBufferView->buffer) || IsNull(pGltfBufferView->buffer->data)) {
        return nullptr;
    }

    const size_t bufferViewOffset = static_cast<size_t>(pGltfBufferView->offset);
    const char*  pBufferAddress   = static_cast<const char*>(pGltfBufferView->buffer->data);
    const char*  pDataStart       = pBufferAddress + bufferViewOffset;

    return static_cast<const void*>(pDataStart);
}

// Get an accessor's starting address
static const void* GetStartAddress(
    const cgltf_accessor* pGltfAccessor)
{
    //
    // NOTE: Don't assert in this function since any of the fields can be NULL for different reasons.
    //

    if (IsNull(pGltfAccessor)) {
        return nullptr;
    }

    // Get buffer view's start address
    const char* pBufferViewDataStart = static_cast<const char*>(GetStartAddress(pGltfAccessor->buffer_view));
    if (IsNull(pBufferViewDataStart)) {
        return nullptr;
    }

    // Calculate accesor's start address
    const size_t accessorOffset     = static_cast<size_t>(pGltfAccessor->offset);
    const char*  pAccessorDataStart = pBufferViewDataStart + accessorOffset;

    return static_cast<const void*>(pAccessorDataStart);
}

const char* ToString(cgltf_component_type componentType)
{
    switch (componentType) {
        case cgltf_component_type_r_8:
            return "BYTE";
        case cgltf_component_type_r_8u:
            return "UNSIGNED_BYTE";
        case cgltf_component_type_r_16:
            return "SHORT";
        case cgltf_component_type_r_16u:
            return "UNSIGNED_SHORT";
        case cgltf_component_type_r_32u:
            return "UNSIGNED_INT";
        case cgltf_component_type_r_32f:
            return "FLOAT";
        default:
            break;
    }

    return "<unknown cgltf_component_type value>";
}

const char* ToString(cgltf_type type)
{
    switch (type) {
        case cgltf_type_scalar:
            return "SCALAR";
        case cgltf_type_vec2:
            return "VEC2";
        case cgltf_type_vec3:
            return "VEC3";
        case cgltf_type_vec4:
            return "VEC4";
        case cgltf_type_mat2:
            return "MAT2";
        case cgltf_type_mat3:
            return "MAT3";
        case cgltf_type_mat4:
            return "MAT4";
        default:
            break;
    }

    return "<unknown cgltf_type value>";
}

// Tries to derive an IndexType from the accessor. Fails for formats that don't comply to the GLTF spec.
// The GLTF 2.0 spec 5.24.2 says "When [format] is undefined, the primitive defines non-indexed geometry. When defined, the accessor MUST have SCALAR type and an unsigned integer component type".
ppx::Result ValidateAccessorIndexType(const cgltf_accessor* pGltfAccessor, grfx::IndexType& outIndexType)
{
    if (IsNull(pGltfAccessor)) {
        outIndexType = grfx::INDEX_TYPE_UNDEFINED;
        return ppx::SUCCESS;
    }

    if (pGltfAccessor->type != cgltf_type_scalar) {
        PPX_ASSERT_MSG(false, "Index accessor type must be SCALAR, got: " << ToString(pGltfAccessor->type));
        return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE;
    }

    switch (pGltfAccessor->component_type) {
        case cgltf_component_type_r_8u:
            outIndexType = grfx::INDEX_TYPE_UINT8;
            return ppx::SUCCESS;
        case cgltf_component_type_r_16u:
            outIndexType = grfx::INDEX_TYPE_UINT16;
            return ppx::SUCCESS;
        case cgltf_component_type_r_32u:
            outIndexType = grfx::INDEX_TYPE_UINT32;
            return ppx::SUCCESS;
        default:
            break;
    }

    PPX_ASSERT_MSG(false, "Index accessor component ype must be an unsigned integer, got: " << ToString(pGltfAccessor->component_type));
    return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE;
}

} // namespace

// -------------------------------------------------------------------------------------------------
// GltfMaterialSelector
// -------------------------------------------------------------------------------------------------
GltfMaterialSelector::GltfMaterialSelector()
{
}

GltfMaterialSelector::~GltfMaterialSelector()
{
}

std::string GltfMaterialSelector::DetermineMaterial(const cgltf_material* pGltfMaterial) const
{
    std::string ident = PPX_MATERIAL_IDENT_ERROR;

    // Determine material type
    if (pGltfMaterial->unlit) {
        ident = PPX_MATERIAL_IDENT_UNLIT;
    }
    else if (pGltfMaterial->has_pbr_metallic_roughness) {
        ident = PPX_MATERIAL_IDENT_STANDARD;
    }

    return ident;
}

// -------------------------------------------------------------------------------------------------
// GltfLoader
// -------------------------------------------------------------------------------------------------
GltfLoader::GltfLoader(
    const std::filesystem::path& filePath,
    const std::filesystem::path& textureDirPath,
    cgltf_data*                  pGltfData,
    bool                         ownsGtlfData,
    scene::GltfMaterialSelector* pMaterialSelector,
    bool                         ownsMaterialSelector)
    : mGltfFilePath(filePath),
      mGltfTextureDir(textureDirPath),
      mGltfData(pGltfData),
      mOwnsGltfData(ownsGtlfData),
      mMaterialSelector(pMaterialSelector),
      mOwnsMaterialSelector(ownsMaterialSelector)
{
}

GltfLoader::~GltfLoader()
{
    if (HasGltfData() && mOwnsGltfData) {
        cgltf_free(mGltfData);
        mGltfData = nullptr;

        PPX_LOG_INFO("Closed GLTF file: " << mGltfFilePath);
    }

    if (!IsNull(mMaterialSelector) && mOwnsMaterialSelector) {
        delete mMaterialSelector;
        mMaterialSelector = nullptr;
    }
}

ppx::Result GltfLoader::Create(
    const std::filesystem::path& filePath,
    const std::filesystem::path& textureDirPath,
    scene::GltfMaterialSelector* pMaterialSelector,
    GltfLoader**                 ppLoader)
{
    if (IsNull(ppLoader)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!(std::filesystem::exists(filePath) && std::filesystem::exists(textureDirPath))) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    // Parse gltf data
    cgltf_options cgltfOptions = {};
    cgltf_data*   pGltfData    = nullptr;

    cgltf_result cgres = cgltf_parse_file(
        &cgltfOptions,
        filePath.string().c_str(),
        &pGltfData);
    if (cgres != cgltf_result_success) {
        return ppx::ERROR_SCENE_SOURCE_FILE_LOAD_FAILED;
    }

    // Load GLTF buffers
    {
        cgltf_result res = cgltf_load_buffers(
            &cgltfOptions,
            pGltfData,
            filePath.string().c_str());
        if (res != cgltf_result_success) {
            cgltf_free(pGltfData);
            PPX_ASSERT_MSG(false, "GLTF: cgltf_load_buffers failed (res=" << res << ")");
            return ppx::ERROR_SCENE_SOURCE_FILE_LOAD_FAILED;
        }
    }

    // Loading from file means we own the GLTF data
    const bool ownsGltfData = true;

    // Create material selector
    const bool ownsMaterialSelector = IsNull(pMaterialSelector) ? true : false;
    if (IsNull(pMaterialSelector)) {
        pMaterialSelector = new GltfMaterialSelector();
        if (IsNull(pMaterialSelector)) {
            cgltf_free(pGltfData);
            return ppx::ERROR_ALLOCATION_FAILED;
        }
    }

    // Create loader object
    auto pLoader = new GltfLoader(
        filePath,
        textureDirPath,
        pGltfData,
        ownsGltfData,
        pMaterialSelector,
        ownsMaterialSelector);
    if (IsNull(pLoader)) {
        cgltf_free(pGltfData);
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    *ppLoader = pLoader;

    PPX_LOG_INFO("Successfully opened GLTF file: " << filePath);

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::Create(
    const std::filesystem::path& path,
    scene::GltfMaterialSelector* pMaterialSelector,
    GltfLoader**                 ppLoader)
{
    return Create(
        path,
        path.parent_path(),
        pMaterialSelector,
        ppLoader);
}

void GltfLoader::CalculateMeshMaterialVertexAttributeMasks(
    const scene::MaterialFactory*                        pMaterialFactory,
    scene::GltfLoader::MeshMaterialVertexAttributeMasks* pOutMasks) const
{
    if (IsNull(mGltfData) || IsNull(pMaterialFactory) || IsNull(pOutMasks)) {
        return;
    }

    auto& outMasks = *pOutMasks;

    for (cgltf_size meshIdx = 0; meshIdx < mGltfData->meshes_count; ++meshIdx) {
        const cgltf_mesh* pGltfMesh = &mGltfData->meshes[meshIdx];

        // Initial value
        outMasks[pGltfMesh] = scene::VertexAttributeFlags::None();

        for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
            const cgltf_material* pGltfMaterial = pGltfMesh->primitives[primIdx].material;

            // Skip if no material
            if (IsNull(pGltfMaterial)) {
                continue;
            }

            // Get material ident
            auto materialIdent = mMaterialSelector->DetermineMaterial(pGltfMaterial);

            // Get required vertex attributes
            auto requiredVertexAttributes = pMaterialFactory->GetRequiredVertexAttributes(materialIdent);

            // OR the masks
            outMasks[pGltfMesh] |= requiredVertexAttributes;
        }
    }
}

uint64_t GltfLoader::CalculateImageObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
{
    uint64_t objectId = objectIndex + loadParams.baseObjectIds.image;
    return objectId;
}

uint64_t GltfLoader::CalculateSamplerObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
{
    uint64_t objectId = objectIndex + loadParams.baseObjectIds.sampler;
    return objectId;
}

uint64_t GltfLoader::CalculateTextureObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
{
    uint64_t objectId = objectIndex + loadParams.baseObjectIds.texture;
    return objectId;
}

uint64_t GltfLoader::CalculateMaterialObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
{
    uint64_t objectId = objectIndex + loadParams.baseObjectIds.material;
    return objectId;
}

uint64_t GltfLoader::CalculateMeshObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
{
    uint64_t objectId = objectIndex + loadParams.baseObjectIds.mesh;
    return objectId;
}

ppx::Result GltfLoader::LoadSamplerInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_sampler*                  pGltfSampler,
    scene::Sampler**                      ppTargetSampler)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfSampler) || IsNull(ppTargetSampler)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfSampler);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_sampler_index(mGltfData, pGltfSampler));
    PPX_LOG_INFO("Loading GLTF sampler[" << gltfObjectIndex << "]: " << gltfObjectName);

    // Load sampler
    grfx::Sampler* pGrfxSampler = nullptr;
    //
    {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = (pGltfSampler->mag_filter == GLTF_TEXTURE_FILTER_LINEAR) ? grfx::FILTER_LINEAR : grfx::FILTER_NEAREST;
        createInfo.minFilter               = (pGltfSampler->mag_filter == GLTF_TEXTURE_FILTER_LINEAR) ? grfx::FILTER_LINEAR : grfx::FILTER_NEAREST;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR; // @TODO: add option to control this
        createInfo.addressModeU            = ToSamplerAddressMode(pGltfSampler->wrap_s);
        createInfo.addressModeV            = ToSamplerAddressMode(pGltfSampler->wrap_t);
        createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.mipLodBias              = 0.0f;
        createInfo.anisotropyEnable        = false;
        createInfo.maxAnisotropy           = 0.0f;
        createInfo.compareEnable           = false;
        createInfo.compareOp               = grfx::COMPARE_OP_NEVER;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = GLTF_LOD_CLAMP_NONE;
        createInfo.borderColor             = grfx::BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

        auto ppxres = loadParams.pDevice->CreateSampler(&createInfo, &pGrfxSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Create target object
    auto pSampler = new scene::Sampler(pGrfxSampler);
    if (IsNull(pSampler)) {
        loadParams.pDevice->DestroySampler(pGrfxSampler);
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Set name
    pSampler->SetName(gltfObjectName);

    // Assign output
    *ppTargetSampler = pSampler;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchSamplerInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_sampler*                  pGltfSampler,
    scene::SamplerRef&                    outSampler)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfSampler)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfSampler);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_sampler_index(mGltfData, pGltfSampler));

    // Cached load if object was previously cached
    const uint64_t objectId = CalculateSamplerObjectId(loadParams, gltfObjectIndex);
    if (loadParams.pResourceManager->Find(objectId, outSampler)) {
        PPX_LOG_INFO("Fetched cached sampler[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
        return ppx::SUCCESS;
    }

    // Cached failed, so load object
    scene::Sampler* pSampler = nullptr;
    //
    auto ppxres = LoadSamplerInternal(loadParams, pGltfSampler, &pSampler);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pSampler);

    // Create object ref
    outSampler = scene::MakeRef(pSampler);
    if (!outSampler) {
        delete pSampler;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Cache object
    {
        loadParams.pResourceManager->Cache(objectId, outSampler);
        PPX_LOG_INFO("   ...cached sampler[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadImageInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_image*                    pGltfImage,
    scene::Image**                        ppTargetImage)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfImage) || IsNull(ppTargetImage)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfImage);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_image_index(mGltfData, pGltfImage));
    PPX_LOG_INFO("Loading GLTF image[" << gltfObjectIndex << "]: " << gltfObjectName);

    // Load image
    grfx::Image* pGrfxImage = nullptr;
    //
    if (!IsNull(pGltfImage->uri)) {
        std::filesystem::path filePath = mGltfTextureDir / ToStringSafe(pGltfImage->uri);
        if (!std::filesystem::exists(filePath)) {
            PPX_LOG_ERROR("GLTF file references an image file that doesn't exist (image=" << ToStringSafe(pGltfImage->name) << ", uri=" << ToStringSafe(pGltfImage->uri) << ", file=" << filePath);
            return ppx::ERROR_PATH_DOES_NOT_EXIST;
        }

        auto ppxres = grfx_util::CreateImageFromFile(
            loadParams.pDevice->GetGraphicsQueue(),
            filePath,
            &pGrfxImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    else if (!IsNull(pGltfImage->buffer_view)) {
        const size_t dataSize = static_cast<size_t>(pGltfImage->buffer_view->size);
        const void*  pData    = GetStartAddress(pGltfImage->buffer_view);
        if (IsNull(pData)) {
            return ppx::ERROR_BAD_DATA_SOURCE;
        }

        ppx::Bitmap bitmap;
        auto        ppxres = ppx::Bitmap::LoadFromMemory(dataSize, pData, &bitmap);
        if (Failed(ppxres)) {
            return ppxres;
        }

        ppxres = grfx_util::CreateImageFromBitmap(
            loadParams.pDevice->GetGraphicsQueue(),
            &bitmap,
            &pGrfxImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    else {
        return ppx::ERROR_SCENE_INVALID_SOURCE_IMAGE;
    }

    // Create image view
    grfx::SampledImageView* pGrfxImageView = nullptr;
    //
    {
        grfx::SampledImageViewCreateInfo createInfo = {};
        createInfo.pImage                           = pGrfxImage;
        createInfo.imageViewType                    = grfx::IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = pGrfxImage->GetFormat();
        createInfo.sampleCount                      = grfx::SAMPLE_COUNT_1;
        createInfo.mipLevel                         = 0;
        createInfo.mipLevelCount                    = pGrfxImage->GetMipLevelCount();
        createInfo.arrayLayer                       = 0;
        createInfo.arrayLayerCount                  = pGrfxImage->GetArrayLayerCount();
        createInfo.components                       = {};

        auto ppxres = loadParams.pDevice->CreateSampledImageView(
            &createInfo,
            &pGrfxImageView);
        if (ppxres) {
            loadParams.pDevice->DestroyImage(pGrfxImage);
            return ppxres;
        }
    }

    // Creat target object
    auto pImage = new scene::Image(pGrfxImage, pGrfxImageView);
    if (IsNull(pImage)) {
        loadParams.pDevice->DestroySampledImageView(pGrfxImageView);
        loadParams.pDevice->DestroyImage(pGrfxImage);
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Set name
    pImage->SetName(gltfObjectName);

    // Assign output
    *ppTargetImage = pImage;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchImageInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_image*                    pGltfImage,
    scene::ImageRef&                      outImage)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfImage)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfImage);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_image_index(mGltfData, pGltfImage));

    // Cached load if object was previously cached
    const uint64_t objectId = CalculateImageObjectId(loadParams, gltfObjectIndex);
    if (loadParams.pResourceManager->Find(objectId, outImage)) {
        PPX_LOG_INFO("Fetched cached image[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
        return ppx::SUCCESS;
    }

    // Cached failed, so load object
    scene::Image* pImage = nullptr;
    //
    auto ppxres = LoadImageInternal(loadParams, pGltfImage, &pImage);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pImage);

    // Create object ref
    outImage = scene::MakeRef(pImage);
    if (!outImage) {
        delete pImage;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Cache object
    {
        loadParams.pResourceManager->Cache(objectId, outImage);
        PPX_LOG_INFO("   ...cached image[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadTextureInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_texture*                  pGltfTexture,
    scene::Texture**                      ppTexture)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfTexture) || IsNull(ppTexture)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfTextureObjectName = GetName(pGltfTexture);
    const std::string gltfImageObjectName   = IsNull(pGltfTexture->image) ? "<NULL>" : GetName(pGltfTexture->image);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_texture_index(mGltfData, pGltfTexture));
    // Textures are often unnamed, so include image name to make log smg more meaningful.
    PPX_LOG_INFO("Loading GLTF texture[" << gltfObjectIndex << "]: " << gltfTextureObjectName << " (image=" << gltfImageObjectName << ")");

    // Required objects
    scene::SamplerRef targetSampler = nullptr;
    scene::ImageRef   targetImage   = nullptr;

    // Fetch if there's a resource manager...
    if (!IsNull(loadParams.pResourceManager)) {
        auto ppxres = FetchSamplerInternal(loadParams, pGltfTexture->sampler, targetSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }

        ppxres = FetchImageInternal(loadParams, pGltfTexture->image, targetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    // ...otherwise load!
    else {
        // Load sampler
        scene::Sampler* pTargetSampler = nullptr;
        auto            ppxres         = LoadSamplerInternal(loadParams, pGltfTexture->sampler, &pTargetSampler);
        if (Failed(ppxres)) {
            return ppxres;
        }
        PPX_ASSERT_NULL_ARG(pTargetSampler);

        targetSampler = scene::MakeRef(pTargetSampler);
        if (!targetSampler) {
            delete pTargetSampler;
            return ppx::ERROR_ALLOCATION_FAILED;
        }

        // Load image
        scene::Image* pTargetImage = nullptr;
        ppxres                     = LoadImageInternal(loadParams, pGltfTexture->image, &pTargetImage);
        if (Failed(ppxres)) {
            return ppxres;
        }
        PPX_ASSERT_NULL_ARG(pTargetImage);

        targetImage = scene::MakeRef(pTargetImage);
        if (!targetImage) {
            delete pTargetImage;
            return ppx::ERROR_ALLOCATION_FAILED;
        }
    }

    // Create target object
    auto pTexture = new scene::Texture(targetImage, targetSampler);
    if (IsNull(pTexture)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Set name
    pTexture->SetName(gltfTextureObjectName);

    // Assign output
    *ppTexture = pTexture;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchTextureInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_texture*                  pGltfTexture,
    scene::TextureRef&                    outTexture)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfTexture)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfTexture);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_texture_index(mGltfData, pGltfTexture));

    // Cached load if object was previously cached
    const uint64_t objectId = CalculateTextureObjectId(loadParams, gltfObjectIndex);
    if (loadParams.pResourceManager->Find(objectId, outTexture)) {
        PPX_LOG_INFO("Fetched cached texture[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
        return ppx::SUCCESS;
    }

    // Cached failed, so load object
    scene::Texture* pTexture = nullptr;
    //
    auto ppxres = LoadTextureInternal(loadParams, pGltfTexture, &pTexture);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pTexture);

    // Create object ref
    outTexture = scene::MakeRef(pTexture);
    if (!outTexture) {
        delete pTexture;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Cache object
    {
        loadParams.pResourceManager->Cache(objectId, outTexture);
        PPX_LOG_INFO("   ...cached texture[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadTextureViewInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_texture_view*             pGltfTextureView,
    scene::TextureView*                   pTargetTextureView)
{
    PPX_ASSERT_NULL_ARG(loadParams.pDevice);
    PPX_ASSERT_NULL_ARG(pGltfTextureView);
    PPX_ASSERT_NULL_ARG(pTargetTextureView);

    // Required object
    scene::TextureRef targetTexture = nullptr;

    // Fetch if there's a resource manager...
    if (!IsNull(loadParams.pResourceManager)) {
        auto ppxres = FetchTextureInternal(loadParams, pGltfTextureView->texture, targetTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    // ...otherwise load!
    else {
        scene::Texture* pTargetTexture = nullptr;
        auto            ppxres         = LoadTextureInternal(loadParams, pGltfTextureView->texture, &pTargetTexture);
        if (Failed(ppxres)) {
            return ppxres;
        }

        targetTexture = scene::MakeRef(pTargetTexture);
        if (!targetTexture) {
            delete pTargetTexture;
            return ppx::ERROR_ALLOCATION_FAILED;
        }
    }

    // Set texture transform if needed
    float2 texCoordTranslate = float2(0, 0);
    float  texCoordRotate    = 0;
    float2 texCoordScale     = float2(1, 1);
    if (pGltfTextureView->has_transform) {
        texCoordTranslate = float2(pGltfTextureView->transform.offset[0], pGltfTextureView->transform.offset[1]);
        texCoordRotate    = pGltfTextureView->transform.rotation;
        texCoordScale     = float2(pGltfTextureView->transform.scale[0], pGltfTextureView->transform.scale[1]);
    }

    *pTargetTextureView = scene::TextureView(
        targetTexture,
        texCoordTranslate,
        texCoordRotate,
        texCoordScale);

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadUnlitMaterialInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_material*                 pGltfMaterial,
    scene::UnlitMaterial*                 pTargetMaterial)
{
    PPX_ASSERT_NULL_ARG(loadParams.pDevice);
    PPX_ASSERT_NULL_ARG(pGltfMaterial);
    PPX_ASSERT_NULL_ARG(pTargetMaterial);

    float4 baseColorFactor = float4(0.5f, 0.5f, 0.5f, 1.0f);

    // KHR_materials_unlit uses attributes from pbrMetallicRoughness
    if (pGltfMaterial->has_pbr_metallic_roughness) {
        if (!IsNull(pGltfMaterial->pbr_metallic_roughness.base_color_texture.texture)) {
            auto ppxres = LoadTextureViewInternal(
                loadParams,
                &pGltfMaterial->pbr_metallic_roughness.base_color_texture,
                pTargetMaterial->GetBaseColorTextureViewPtr());
            if (Failed(ppxres)) {
                return ppxres;
            }
        }

        baseColorFactor = *(reinterpret_cast<const float4*>(pGltfMaterial->pbr_metallic_roughness.base_color_factor));
    }

    // Set base color factor
    pTargetMaterial->SetBaseColorFactor(baseColorFactor);

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadPbrMetallicRoughnessMaterialInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_material*                 pGltfMaterial,
    scene::StandardMaterial*              pTargetMaterial)
{
    PPX_ASSERT_NULL_ARG(loadParams.pDevice);
    PPX_ASSERT_NULL_ARG(pGltfMaterial);
    PPX_ASSERT_NULL_ARG(pTargetMaterial);

    // pbrMetallicRoughness textures
    if (!IsNull(pGltfMaterial->pbr_metallic_roughness.base_color_texture.texture)) {
        auto ppxres = LoadTextureViewInternal(
            loadParams,
            &pGltfMaterial->pbr_metallic_roughness.base_color_texture,
            pTargetMaterial->GetBaseColorTextureViewPtr());
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    if (!IsNull(pGltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture)) {
        auto ppxres = LoadTextureViewInternal(
            loadParams,
            &pGltfMaterial->pbr_metallic_roughness.metallic_roughness_texture,
            pTargetMaterial->GetMetallicRoughnessTextureViewPtr());
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Normal texture
    if (!IsNull(pGltfMaterial->normal_texture.texture)) {
        auto ppxres = LoadTextureViewInternal(
            loadParams,
            &pGltfMaterial->normal_texture,
            pTargetMaterial->GetNormalTextureViewPtr());
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Occlusion texture
    if (!IsNull(pGltfMaterial->occlusion_texture.texture)) {
        auto ppxres = LoadTextureViewInternal(
            loadParams,
            &pGltfMaterial->occlusion_texture,
            pTargetMaterial->GetOcclusionTextureViewPtr());
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Emissive texture
    if (!IsNull(pGltfMaterial->emissive_texture.texture)) {
        auto ppxres = LoadTextureViewInternal(
            loadParams,
            &pGltfMaterial->emissive_texture,
            pTargetMaterial->GetEmissiveTextureViewPtr());
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    pTargetMaterial->SetBaseColorFactor(*(reinterpret_cast<const float4*>(pGltfMaterial->pbr_metallic_roughness.base_color_factor)));
    pTargetMaterial->SetMetallicFactor(pGltfMaterial->pbr_metallic_roughness.metallic_factor);
    pTargetMaterial->SetRoughnessFactor(pGltfMaterial->pbr_metallic_roughness.roughness_factor);
    pTargetMaterial->SetEmissiveFactor(*(reinterpret_cast<const float3*>(pGltfMaterial->emissive_factor)));

    if (pGltfMaterial->has_emissive_strength) {
        pTargetMaterial->SetEmissiveStrength(pGltfMaterial->emissive_strength.emissive_strength);
    }

    if (!IsNull(pGltfMaterial->occlusion_texture.texture)) {
        pTargetMaterial->SetOcclusionStrength(pGltfMaterial->occlusion_texture.scale);
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadMaterialInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_material*                 pGltfMaterial,
    scene::Material**                     ppTargetMaterial)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfMaterial) || IsNull(ppTargetMaterial)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfMaterial);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_material_index(mGltfData, pGltfMaterial));
    PPX_LOG_INFO("Loading GLTF material[" << gltfObjectIndex << "]: " << gltfObjectName);

    // Get material ident
    const std::string materialIdent = mMaterialSelector->DetermineMaterial(pGltfMaterial);

    // Create material - this should never return a NULL object
    auto pMaterial = loadParams.pMaterialFactory->CreateMaterial(materialIdent);
    if (IsNull(pMaterial)) {
        PPX_ASSERT_MSG(false, "Material factory returned a NULL material");
    }

    // Load Unlit
    if (pMaterial->GetIdentString() == PPX_MATERIAL_IDENT_UNLIT) {
        auto ppxres = LoadUnlitMaterialInternal(
            loadParams,
            pGltfMaterial,
            static_cast<scene::UnlitMaterial*>(pMaterial));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }
    // Load MetallicRoughness
    else if (pMaterial->GetIdentString() == PPX_MATERIAL_IDENT_STANDARD) {
        auto ppxres = LoadPbrMetallicRoughnessMaterialInternal(
            loadParams,
            pGltfMaterial,
            static_cast<scene::StandardMaterial*>(pMaterial));
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Set name
    pMaterial->SetName(gltfObjectName);

    // Assign output
    *ppTargetMaterial = pMaterial;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchMaterialInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_material*                 pGltfMaterial,
    scene::MaterialRef&                   outMaterial)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfMaterial)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfMaterial);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_material_index(mGltfData, pGltfMaterial));

    // Cached load if object was previously cached
    const uint64_t objectId = CalculateMaterialObjectId(loadParams, gltfObjectIndex);
    if (loadParams.pResourceManager->Find(objectId, outMaterial)) {
        PPX_LOG_INFO("Fetched cached material[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
        return ppx::SUCCESS;
    }

    // Cached failed, so load object
    scene::Material* pMaterial = nullptr;
    //
    auto ppxres = LoadMaterialInternal(loadParams, pGltfMaterial, &pMaterial);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pMaterial);

    // Create object ref
    outMaterial = scene::MakeRef(pMaterial);
    if (!outMaterial) {
        delete pMaterial;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Cache object
    {
        loadParams.pResourceManager->Cache(objectId, outMaterial);
        PPX_LOG_INFO("   ...cached material[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadMeshData(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_mesh*                     pGltfMesh,
    scene::MeshDataRef&                   outMeshData,
    std::vector<scene::PrimitiveBatch>&   outBatches)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfMesh)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfMesh);

    // Get GLTF mesh index
    const uint64_t gltfMeshIndex = static_cast<uint64_t>(cgltf_mesh_index(mGltfData, pGltfMesh));

    // Calculate id using geometry related accessor hash
    const uint64_t objectId = GetMeshAccessorsHash(mGltfData, pGltfMesh);
    PPX_LOG_INFO("Loading mesh data (id=" << objectId << ") for GLTF mesh[" << gltfMeshIndex << "]: " << gltfObjectName);

    // Use cached object if possible
    bool hasCachedGeometry = false;
    if (!IsNull(loadParams.pResourceManager)) {
        if (loadParams.pResourceManager->Find(objectId, outMeshData)) {
            PPX_LOG_INFO("   ...cache load mesh data (objectId=" << objectId << ") for GLTF mesh[" << gltfMeshIndex << "]: " << gltfObjectName);

            // We don't return here like the other functions because we still need
            // to process the primitives, instead we just set the flag to prevent
            // geometry creation.
            //
            hasCachedGeometry = true;
        }
    }

    // ---------------------------------------------------------------------------------------------

    // Target vertex formats
    auto targetPositionFormat = scene::kVertexPositionFormat;
    auto targetTexCoordFormat = loadParams.requiredVertexAttributes.bits.texCoords ? scene::kVertexAttributeTexCoordFormat : grfx::FORMAT_UNDEFINED;
    auto targetNormalFormat   = loadParams.requiredVertexAttributes.bits.normals ? scene::kVertexAttributeNormalFormat : grfx::FORMAT_UNDEFINED;
    auto targetTangentFormat  = loadParams.requiredVertexAttributes.bits.tangents ? scene::kVertexAttributeTagentFormat : grfx::FORMAT_UNDEFINED;
    auto targetColorFormat    = loadParams.requiredVertexAttributes.bits.colors ? scene::kVertexAttributeColorFormat : grfx::FORMAT_UNDEFINED;

    const uint32_t targetTexCoordElementSize = (targetTexCoordFormat != grfx::FORMAT_UNDEFINED) ? grfx::GetFormatDescription(targetTexCoordFormat)->bytesPerTexel : 0;
    const uint32_t targetNormalElementSize   = (targetNormalFormat != grfx::FORMAT_UNDEFINED) ? grfx::GetFormatDescription(targetNormalFormat)->bytesPerTexel : 0;
    const uint32_t targetTangentElementSize  = (targetTangentFormat != grfx::FORMAT_UNDEFINED) ? grfx::GetFormatDescription(targetTangentFormat)->bytesPerTexel : 0;
    const uint32_t targetColorElementSize    = (targetColorFormat != grfx::FORMAT_UNDEFINED) ? grfx::GetFormatDescription(targetColorFormat)->bytesPerTexel : 0;

    const uint32_t targetPositionElementSize   = grfx::GetFormatDescription(targetPositionFormat)->bytesPerTexel;
    const uint32_t targetAttributesElementSize = targetTexCoordElementSize + targetNormalElementSize + targetTangentElementSize + targetColorElementSize;

    struct BatchInfo
    {
        scene::MaterialRef material = nullptr;
        // Start of the index plane in the final repacked GPU buffer.
        uint32_t indexDataOffset = 0; // Must have 4 byte alignment
        // Total size of the index plane in the final repacked GPU buffer.
        uint32_t indexDataSize       = 0;
        uint32_t positionDataOffset  = 0; // Must have 4 byte alignment
        uint32_t positionDataSize    = 0;
        uint32_t attributeDataOffset = 0; // Must have 4 byte alignment
        uint32_t attributeDataSize   = 0;
        // Format of the input index buffer.
        grfx::IndexType indexType = grfx::INDEX_TYPE_UNDEFINED;
        // Format of the index plane in the final repacked GPU buffer.
        grfx::IndexType repackedIndexType = grfx::INDEX_TYPE_UNDEFINED;
        // How many indices are in the input index buffer.
        uint32_t  indexCount  = 0;
        uint32_t  vertexCount = 0;
        ppx::AABB boundingBox = {};
    };

    // Build out batch infos
    std::vector<BatchInfo> batchInfos;
    // Size of the final GPU buffer to allocate. Must account for growth during repacking.
    uint32_t totalDataSize = 0;
    //
    for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
        const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];

        // Only triangle geometry right now
        if (pGltfPrimitive->type != cgltf_primitive_type_triangles) {
            PPX_ASSERT_MSG(false, "GLTF: only triangle primitives are supported");
            return ppx::ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE;
        }

        // Get index format
        grfx::IndexType indexType = grfx::INDEX_TYPE_UNDEFINED;
        if (ppx::Result ppxres = ValidateAccessorIndexType(pGltfPrimitive->indices, indexType); Failed(ppxres)) {
            return ppxres;
        }

        // We require index data so bail if there isn't index data. See #474
        if (indexType == grfx::INDEX_TYPE_UNDEFINED) {
            PPX_ASSERT_MSG(false, "GLTF mesh primitive does not have index data");
            return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
        }

        // UINT8 index buffer availability varies: Vulkan requires an extension, whereas DX12 lacks support entirely.
        // If it's not supported then repack as UINT16 (the smallest mandated size for both).
        grfx::IndexType repackedIndexType = indexType;
        if (repackedIndexType == grfx::INDEX_TYPE_UINT8 && !loadParams.pDevice->IndexTypeUint8Supported()) {
            PPX_LOG_INFO("Device doesn't support UINT8 index buffers! Repacking data as UINT16.");
            repackedIndexType = grfx::INDEX_TYPE_UINT16;
        }

        // Index data size of input
        const uint32_t indexCount       = !IsNull(pGltfPrimitive->indices) ? static_cast<uint32_t>(pGltfPrimitive->indices->count) : 0;
        const uint32_t indexElementSize = grfx::IndexTypeSize(indexType);
        // If we repack indices into a buffer of a different format then we need to account for disparity between input and output sizes.
        const uint32_t repackedSizeRatio = grfx::IndexTypeSize(repackedIndexType) / indexElementSize;
        const uint32_t indexDataSize     = indexCount * indexElementSize * repackedSizeRatio;

        // Get position accessor
        const VertexAccessors gltflAccessors = GetVertexAccessors(pGltfPrimitive);
        if (IsNull(gltflAccessors.pPositions)) {
            PPX_ASSERT_MSG(false, "GLTF mesh primitive position accessor is NULL");
            return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA;
        }

        // Vertex data sizes
        const uint32_t vertexCount       = static_cast<uint32_t>(gltflAccessors.pPositions->count);
        const uint32_t positionDataSize  = vertexCount * targetPositionElementSize;
        const uint32_t attributeDataSize = vertexCount * targetAttributesElementSize;

        // Index data offset
        const uint32_t indexDataOffset = totalDataSize;
        totalDataSize += RoundUp<uint32_t>(indexDataSize, 4);
        // Position data offset
        const uint32_t positionDataOffset = totalDataSize;
        totalDataSize += RoundUp<uint32_t>(positionDataSize, 4);
        // Attribute data offset;
        const uint32_t attributeDataOffset = totalDataSize;
        totalDataSize += RoundUp<uint32_t>(attributeDataSize, 4);

        // Build out batch info with data we'll need later
        BatchInfo batchInfo           = {};
        batchInfo.indexDataOffset     = indexDataOffset;
        batchInfo.indexDataSize       = indexDataSize;
        batchInfo.positionDataOffset  = positionDataOffset;
        batchInfo.positionDataSize    = positionDataSize;
        batchInfo.attributeDataOffset = attributeDataOffset;
        batchInfo.attributeDataSize   = attributeDataSize;
        batchInfo.indexType           = indexType;
        batchInfo.repackedIndexType   = repackedIndexType;
        batchInfo.indexCount          = indexCount;

        // Material
        {
            // Yes, it's completely possible for GLTF primitives to have no material.
            // For example, if you create a cube in Blender and export it without
            // assigning a material to it. Obviously, this results in material being
            // NULL. Use error material if GLTF material is NULL.
            //
            if (!IsNull(pGltfPrimitive->material)) {
                const uint64_t materialId = cgltf_material_index(mGltfData, pGltfPrimitive->material);
                loadParams.pResourceManager->Find(materialId, batchInfo.material);
            }
            else {
                auto pMaterial = loadParams.pMaterialFactory->CreateMaterial(PPX_MATERIAL_IDENT_ERROR);
                if (IsNull(pMaterial)) {
                    PPX_ASSERT_MSG(false, "could not create ErrorMaterial for GLTF mesh primitive");
                    return ppx::ERROR_SCENE_INVALID_SOURCE_MATERIAL;
                }

                batchInfo.material = scene::MakeRef(pMaterial);
                if (!batchInfo.material) {
                    delete pMaterial;
                    return ppx::ERROR_ALLOCATION_FAILED;
                }
            }
            PPX_ASSERT_MSG(batchInfo.material != nullptr, "GLTF mesh primitive material is NULL");
        }

        // Add
        batchInfos.push_back(batchInfo);
    }

    // Create GPU buffer and copy geometry data to it
    grfx::BufferPtr targetGpuBuffer = outMeshData ? outMeshData->GetGpuBuffer() : nullptr;
    //
    if (!targetGpuBuffer) {
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = totalDataSize;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        bufferCreateInfo.initialState                = grfx::RESOURCE_STATE_COPY_SRC;

        // Create staging buffer
        //
        grfx::BufferPtr stagingBuffer;
        auto            ppxres = loadParams.pDevice->CreateBuffer(&bufferCreateInfo, &stagingBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "staging buffer creation failed");
            return ppxres;
        }

        // Scoped destory buffers if there's an early exit
        grfx::ScopeDestroyer SCOPED_DESTROYER = grfx::ScopeDestroyer(loadParams.pDevice);
        SCOPED_DESTROYER.AddObject(stagingBuffer);

        // Create GPU buffer
        bufferCreateInfo.usageFlags.bits.indexBuffer  = true;
        bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
        bufferCreateInfo.usageFlags.bits.transferDst  = true;
        bufferCreateInfo.memoryUsage                  = grfx::MEMORY_USAGE_GPU_ONLY;
        bufferCreateInfo.initialState                 = grfx::RESOURCE_STATE_GENERAL;
        //
        ppxres = loadParams.pDevice->CreateBuffer(&bufferCreateInfo, &targetGpuBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "GPU buffer creation failed");
            return ppxres;
        }
        SCOPED_DESTROYER.AddObject(targetGpuBuffer);

        // Map staging buffer
        char* pStagingBaseAddr = nullptr;
        ppxres                 = stagingBuffer->MapMemory(0, reinterpret_cast<void**>(&pStagingBaseAddr));
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "staging buffer mapping failed");
            return ppxres;
        }

        // Stage data for copy
        for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
            const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];
            BatchInfo&             batch          = batchInfos[primIdx];

            // Create targetGeometry so we can repack gemetry data into position planar + packed vertex attributes.
            Geometry   targetGeometry = {};
            const bool hasAttributes  = (loadParams.requiredVertexAttributes.mask != 0);
            //
            {
                GeometryCreateInfo createInfo = (hasAttributes ? GeometryCreateInfo::PositionPlanar() : GeometryCreateInfo::Planar()).IndexType(batch.repackedIndexType);

                // clang-format off
                if (loadParams.requiredVertexAttributes.bits.texCoords) createInfo.AddTexCoord(targetTexCoordFormat);
                if (loadParams.requiredVertexAttributes.bits.normals) createInfo.AddNormal(targetNormalFormat);
                if (loadParams.requiredVertexAttributes.bits.tangents) createInfo.AddTangent(targetTangentFormat);
                if (loadParams.requiredVertexAttributes.bits.colors) createInfo.AddColor(targetColorFormat);
                // clang-format on

                auto ppxres = ppx::Geometry::Create(createInfo, &targetGeometry);
                if (Failed(ppxres)) {
                    return ppxres;
                }
            }

            // Repack geometry data for batch
            {
                // Process indices
                //
                // REMINDER: It's possible for a primitive to not have index data
                //
                switch (batch.indexType) {
                    case grfx::INDEX_TYPE_UNDEFINED:
                        PPX_ASSERT_MSG(false, "Non-indexed geoemetry is not supported. See #474");
                        break;
                    case grfx::INDEX_TYPE_UINT16: {
                        auto pGltfIndices = GetStartAddress(pGltfPrimitive->indices);
                        PPX_ASSERT_MSG(!IsNull(pGltfIndices), "GLTF: indices data start is NULL");
                        const uint16_t* pGltfIndex = static_cast<const uint16_t*>(pGltfIndices);
                        for (cgltf_size i = 0; i < pGltfPrimitive->indices->count; ++i, ++pGltfIndex) {
                            targetGeometry.AppendIndex(*pGltfIndex);
                        }
                        break;
                    }
                    case grfx::INDEX_TYPE_UINT32: {
                        auto pGltfIndices = GetStartAddress(pGltfPrimitive->indices);
                        PPX_ASSERT_MSG(!IsNull(pGltfIndices), "GLTF: indices data start is NULL");
                        const uint32_t* pGltfIndex = static_cast<const uint32_t*>(pGltfIndices);
                        for (cgltf_size i = 0; i < pGltfPrimitive->indices->count; ++i, ++pGltfIndex) {
                            targetGeometry.AppendIndex(*pGltfIndex);
                        }
                        break;
                    }
                    case grfx::INDEX_TYPE_UINT8: {
                        auto pGltfIndices = GetStartAddress(pGltfPrimitive->indices);
                        PPX_ASSERT_MSG(!IsNull(pGltfIndices), "GLTF: indices data start is NULL");
                        const uint8_t* pGltfIndex = static_cast<const uint8_t*>(pGltfIndices);
                        for (cgltf_size i = 0; i < pGltfPrimitive->indices->count; ++i, ++pGltfIndex) {
                            targetGeometry.AppendIndex(*pGltfIndex);
                        }
                        break;
                    }
                }
            }

            // Vertices
            {
                VertexAccessors gltflAccessors = GetVertexAccessors(pGltfPrimitive);
                //
                // Bail if position accessor is NULL: no vertex positions, no geometry data
                //
                if (IsNull(gltflAccessors.pPositions)) {
                    PPX_ASSERT_MSG(false, "GLTF mesh primitive is missing position data");
                    return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA;
                }

                // Bounding box
                bool hasBoundingBox = (gltflAccessors.pPositions->has_min && gltflAccessors.pPositions->has_max);
                if (hasBoundingBox) {
                    batch.boundingBox = ppx::AABB(
                        *reinterpret_cast<const float3*>(gltflAccessors.pPositions->min),
                        *reinterpret_cast<const float3*>(gltflAccessors.pPositions->max));
                }

                // Determine if we need to process vertices
                //
                // Assume we have to procss the vertices
                bool processVertices = true;
                if (hasCachedGeometry) {
                    // If we have cached geometry, we only process the vertices if
                    // we need the bounding box.
                    //
                    processVertices = !hasBoundingBox;
                }

                // Check vertex data formats
                auto positionFormat = GetFormat(gltflAccessors.pPositions);
                auto texCoordFormat = GetFormat(gltflAccessors.pTexCoords);
                auto normalFormat   = GetFormat(gltflAccessors.pNormals);
                auto tangentFormat  = GetFormat(gltflAccessors.pTangents);
                auto colorFormat    = GetFormat(gltflAccessors.pColors);
                //
                PPX_ASSERT_MSG((positionFormat == targetPositionFormat), "GLTF: vertex positions format is not supported");
                //
                if (loadParams.requiredVertexAttributes.bits.texCoords && !IsNull(gltflAccessors.pTexCoords)) {
                    PPX_ASSERT_MSG((texCoordFormat == targetTexCoordFormat), "GLTF: vertex tex coords sourceIndexTypeFormat is not supported");
                }
                if (loadParams.requiredVertexAttributes.bits.normals && !IsNull(gltflAccessors.pNormals)) {
                    PPX_ASSERT_MSG((normalFormat == targetNormalFormat), "GLTF: vertex normals format is not supported");
                }
                if (loadParams.requiredVertexAttributes.bits.tangents && !IsNull(gltflAccessors.pTangents)) {
                    PPX_ASSERT_MSG((tangentFormat == targetTangentFormat), "GLTF: vertex tangents format is not supported");
                }
                if (loadParams.requiredVertexAttributes.bits.colors && !IsNull(gltflAccessors.pColors)) {
                    PPX_ASSERT_MSG((colorFormat == targetColorFormat), "GLTF: vertex colors format is not supported");
                }

                // Data starts
                const float3* pGltflPositions = static_cast<const float3*>(GetStartAddress(gltflAccessors.pPositions));
                const float3* pGltflNormals   = static_cast<const float3*>(GetStartAddress(gltflAccessors.pNormals));
                const float4* pGltflTangents  = static_cast<const float4*>(GetStartAddress(gltflAccessors.pTangents));
                const float3* pGltflColors    = static_cast<const float3*>(GetStartAddress(gltflAccessors.pColors));
                const float2* pGltflTexCoords = static_cast<const float2*>(GetStartAddress(gltflAccessors.pTexCoords));

                // Process vertex data
                for (cgltf_size i = 0; i < gltflAccessors.pPositions->count; ++i) {
                    TriMeshVertexData vertexData = {};

                    // Position
                    vertexData.position = *pGltflPositions;
                    ++pGltflPositions;
                    // Normals
                    if (loadParams.requiredVertexAttributes.bits.normals && !IsNull(pGltflNormals)) {
                        vertexData.normal = *pGltflNormals;
                        ++pGltflNormals;
                    }
                    // Tangents
                    if (loadParams.requiredVertexAttributes.bits.tangents && !IsNull(pGltflTangents)) {
                        vertexData.tangent = *pGltflTangents;
                        ++pGltflTangents;
                    }
                    // Colors
                    if (loadParams.requiredVertexAttributes.bits.colors && !IsNull(pGltflColors)) {
                        vertexData.color = *pGltflColors;
                        ++pGltflColors;
                    }
                    // Tex cooord
                    if (loadParams.requiredVertexAttributes.bits.texCoords && !IsNull(pGltflTexCoords)) {
                        vertexData.texCoord = *pGltflTexCoords;
                        ++pGltflTexCoords;
                    }

                    // Append vertex data
                    targetGeometry.AppendVertexData(vertexData);

                    if (!hasBoundingBox) {
                        if (i > 0) {
                            batch.boundingBox.Expand(vertexData.position);
                        }
                        else {
                            batch.boundingBox = ppx::AABB(vertexData.position, vertexData.position);
                        }
                    }
                }
            }

            // Geometry data must match what's in the batch
            const uint32_t repackedIndexBufferSize     = targetGeometry.GetIndexBuffer()->GetSize();
            const uint32_t repackedPositionBufferSize  = targetGeometry.GetVertexBuffer(0)->GetSize();
            const uint32_t repackedAttributeBufferSize = hasAttributes ? targetGeometry.GetVertexBuffer(1)->GetSize() : 0;
            if (repackedIndexBufferSize != batch.indexDataSize) {
                PPX_ASSERT_MSG(false, "repacked index buffer size (" << repackedIndexBufferSize << ") does not match batch's index data size (" << batch.indexDataSize << ")");
                return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
            }
            if (repackedPositionBufferSize != batch.positionDataSize) {
                PPX_ASSERT_MSG(false, "repacked position buffer size does not match batch's position data size");
                return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
            }
            if (repackedAttributeBufferSize != batch.attributeDataSize) {
                PPX_ASSERT_MSG(false, "repacked attribute buffer size does not match batch's attribute data size");
                return ppx::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
            }

            // We're good - copy data to the staging buffer
            {
                // Indices
                const void* pSrcData = targetGeometry.GetIndexBuffer()->GetData();
                char*       pDstData = pStagingBaseAddr + batch.indexDataOffset;
                PPX_ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedIndexBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "index data exceeds buffer range");
                memcpy(pDstData, pSrcData, repackedIndexBufferSize);

                // Positions
                pSrcData = targetGeometry.GetVertexBuffer(0)->GetData();
                pDstData = pStagingBaseAddr + batch.positionDataOffset;
                PPX_ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedPositionBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "position data exceeds buffer range");
                memcpy(pDstData, pSrcData, repackedPositionBufferSize);

                // Attributes
                if (hasAttributes) {
                    pSrcData = targetGeometry.GetVertexBuffer(1)->GetData();
                    pDstData = pStagingBaseAddr + batch.attributeDataOffset;
                    PPX_ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedAttributeBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "attribute data exceeds buffer range");
                    memcpy(pDstData, pSrcData, repackedAttributeBufferSize);
                }
            }
        }

        // Copy staging buffer to GPU buffer
        grfx::BufferToBufferCopyInfo copyInfo = {};
        copyInfo.srcBuffer.offset             = 0;
        copyInfo.dstBuffer.offset             = 0;
        copyInfo.size                         = stagingBuffer->GetSize();
        //
        ppxres = loadParams.pDevice->GetGraphicsQueue()->CopyBufferToBuffer(
            &copyInfo,
            stagingBuffer,
            targetGpuBuffer,
            grfx::RESOURCE_STATE_GENERAL,
            grfx::RESOURCE_STATE_GENERAL);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "staging buffer to GPU buffer copy failed");
            return ppxres;
        }

        // We're good if we got here, release objects from scoped destroy
        SCOPED_DESTROYER.ReleaseAll();
        // Destroy staging buffer since we're done with it
        stagingBuffer->UnmapMemory();
        loadParams.pDevice->DestroyBuffer(stagingBuffer);
    }

    // Build batches
    for (uint32_t batchIdx = 0; batchIdx < CountU32(batchInfos); ++batchIdx) {
        const auto& batch = batchInfos[batchIdx];

        grfx::IndexBufferView indexBufferView = grfx::IndexBufferView(targetGpuBuffer, batch.repackedIndexType, batch.indexDataOffset, batch.indexDataSize);

        grfx::VertexBufferView positionBufferView  = grfx::VertexBufferView(targetGpuBuffer, targetPositionElementSize, batch.positionDataOffset, batch.positionDataSize);
        grfx::VertexBufferView attributeBufferView = grfx::VertexBufferView((batch.attributeDataSize != 0) ? targetGpuBuffer : nullptr, targetAttributesElementSize, batch.attributeDataOffset, batch.attributeDataSize);

        scene::PrimitiveBatch targetBatch = scene::PrimitiveBatch(
            batch.material,
            indexBufferView,
            positionBufferView,
            attributeBufferView,
            batch.indexCount,
            batch.vertexCount,
            batch.boundingBox);

        outBatches.push_back(targetBatch);
    }

    // ---------------------------------------------------------------------------------------------

    // Create GPU mesh from geometry if we don't have cached geometry
    if (!hasCachedGeometry) {
        // Allocate mesh data
        auto pTargetMeshData = new scene::MeshData(loadParams.requiredVertexAttributes, targetGpuBuffer);
        if (IsNull(pTargetMeshData)) {
            loadParams.pDevice->DestroyBuffer(targetGpuBuffer);
            return ppx::ERROR_ALLOCATION_FAILED;
        }

        // Create ref
        outMeshData = scene::MakeRef(pTargetMeshData);

        // Set name
        outMeshData->SetName(gltfObjectName);

        // Cache object if caching
        if (!IsNull(loadParams.pResourceManager)) {
            PPX_LOG_INFO("   ...caching mesh data (objectId=" << objectId << ") for GLTF mesh[" << gltfMeshIndex << "]: " << gltfObjectName);
            loadParams.pResourceManager->Cache(objectId, outMeshData);
        }
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadMeshInternal(
    const GltfLoader::InternalLoadParams& externalLoadParams,
    const cgltf_mesh*                     pGltfMesh,
    scene::Mesh**                         ppTargetMesh)
{
    if (IsNull(externalLoadParams.pDevice) || IsNull(pGltfMesh) || IsNull(ppTargetMesh)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfMesh);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_mesh_index(mGltfData, pGltfMesh));
    PPX_LOG_INFO("Loading GLTF mesh[" << gltfObjectIndex << "]: " << gltfObjectName);

    // Create target mesh - scoped to prevent localLoadParams from leaking
    scene::Mesh* pTargetMesh = nullptr;
    //
    {
        // Copy the external load params so we can control the resource manager and vertex attributes.
        //
        auto localLoadParams                     = externalLoadParams;
        localLoadParams.requiredVertexAttributes = scene::VertexAttributeFlags::None();

        // If a resource manager wasn't passed in, this means we're dealing with
        // a standalone mesh which needs a local resource manager. So, we'll
        // create one if that's the case.
        //
        std::unique_ptr<scene::ResourceManager> localResourceManager;
        if (IsNull(localLoadParams.pResourceManager)) {
            localResourceManager = std::make_unique<scene::ResourceManager>();
            if (!localResourceManager) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }

            // Override resource manager
            localLoadParams.pResourceManager = localResourceManager.get();
        }

        // Load materials for primitives and get required vertex attributes
        for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
            const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];
            const cgltf_material*  pGltfMaterial  = pGltfPrimitive->material;

            // Yes, it's completely possible for GLTF primitives to have no material.
            // For example, if you create a cube in Blender and export it without
            // assigning a material to it. Obviously, this results in material being
            // NULL. No need to load anything if it's NULL.
            //
            if (IsNull(pGltfMaterial)) {
                continue;
            }

            // Fetch material since we'll always have a resource manager
            scene::MaterialRef loadedMaterial;
            //
            auto ppxres = FetchMaterialInternal(
                localLoadParams,
                pGltfMaterial,
                loadedMaterial);
            if (ppxres) {
                return ppxres;
            }

            // Get material ident
            const std::string materialIdent = loadedMaterial->GetIdentString();

            // Get material's required vertex attributes
            auto materialRequiredVertexAttributes = localLoadParams.pMaterialFactory->GetRequiredVertexAttributes(materialIdent);
            localLoadParams.requiredVertexAttributes |= materialRequiredVertexAttributes;
        }

        // If we don't have a local resource manager, then it means we're loading in through a scene.
        // If we're loading in through a scene, then we need to user the mesh data vertex attributes
        // supplied to this function...if they were supplied.
        //
        if (!localResourceManager && !IsNull(localLoadParams.pMeshMaterialVertexAttributeMasks)) {
            auto it = localLoadParams.pMeshMaterialVertexAttributeMasks->find(pGltfMesh);

            // Keeps the local mesh's vertex attribute if search failed.
            //
            if (it != localLoadParams.pMeshMaterialVertexAttributeMasks->end()) {
                localLoadParams.requiredVertexAttributes = (*it).second;
            }
        }

        // Override the local vertex attributes with loadParams has vertex attributes
        if (externalLoadParams.requiredVertexAttributes.mask != 0) {
            localLoadParams.requiredVertexAttributes = externalLoadParams.requiredVertexAttributes;
        }

        //
        // Disable vertex colors for now: some work is needed to handle format conversion.
        //
        localLoadParams.requiredVertexAttributes.bits.colors = false;

        // Load mesh data and batches
        scene::MeshDataRef                 meshData = nullptr;
        std::vector<scene::PrimitiveBatch> batches  = {};
        //
        {
            auto ppxres = LoadMeshData(
                localLoadParams,
                pGltfMesh,
                meshData,
                batches);
            if (Failed(ppxres)) {
                return ppxres;
            }
        }

        // Create target mesh
        if (localResourceManager) {
            // Allocate mesh with local resource manager
            pTargetMesh = new scene::Mesh(
                std::move(localResourceManager),
                meshData,
                std::move(batches));
            if (IsNull(pTargetMesh)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }
        }
        else {
            // Allocate mesh
            pTargetMesh = new scene::Mesh(meshData, std::move(batches));
            if (IsNull(pTargetMesh)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }
        }
    }

    // Set name
    pTargetMesh->SetName(gltfObjectName);

    // Assign output
    *ppTargetMesh = pTargetMesh;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchMeshInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_mesh*                     pGltfMesh,
    scene::MeshRef&                       outMesh)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfMesh)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfMesh);

    // Get GLTF object index to use as object id
    const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_mesh_index(mGltfData, pGltfMesh));

    // Cached load if object was previously cached
    const uint64_t objectId = CalculateMeshObjectId(loadParams, gltfObjectIndex);
    if (loadParams.pResourceManager->Find(objectId, outMesh)) {
        PPX_LOG_INFO("Fetched cached mesh[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
        return ppx::SUCCESS;
    }

    // Cached failed, so load object
    scene::Mesh* pMesh = nullptr;
    //
    auto ppxres = LoadMeshInternal(loadParams, pGltfMesh, &pMesh);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pMesh);

    // Create object ref
    outMesh = scene::MakeRef(pMesh);
    if (!outMesh) {
        delete pMesh;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Cache object
    {
        loadParams.pResourceManager->Cache(objectId, outMesh);
        PPX_LOG_INFO("   ...cached mesh[" << gltfObjectIndex << "]: " << gltfObjectName << " (objectId=" << objectId << ")");
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadNodeInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_node*                     pGltfNode,
    scene::Node**                         ppTargetNode)
{
    if ((!loadParams.transformOnly && IsNull(loadParams.pDevice)) || IsNull(pGltfNode) || IsNull(ppTargetNode)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfNode);

    // Get GLTF object index to use as object id
    const uint32_t index = static_cast<uint32_t>(cgltf_node_index(mGltfData, pGltfNode));
    PPX_LOG_INFO("Loading GLTF node[" << index << "]: " << gltfObjectName);

    // Get node type
    const scene::NodeType nodeType = loadParams.transformOnly ? scene::NODE_TYPE_TRANSFORM : GetNodeType(pGltfNode);

    // Load based on node type
    scene::Node* pTargetNode = nullptr;
    //
    switch (nodeType) {
        default: {
            return ppx::ERROR_SCENE_UNSUPPORTED_NODE_TYPE;
        } break;

        // Transform node
        case scene::NODE_TYPE_TRANSFORM: {
            pTargetNode = new scene::Node(loadParams.pTargetScene);
        } break;

        // Mesh node
        case scene::NODE_TYPE_MESH: {
            const cgltf_mesh* pGltfMesh = pGltfNode->mesh;
            if (IsNull(pGltfMesh)) {
                return ppx::ERROR_SCENE_INVALID_SOURCE_MESH;
            }

            // Required object
            scene::MeshRef targetMesh = nullptr;

            // Fetch if there's a resource manager...
            if (!IsNull(loadParams.pResourceManager)) {
                auto ppxres = FetchMeshInternal(loadParams, pGltfMesh, targetMesh);
                if (Failed(ppxres)) {
                    return ppxres;
                }
            }
            // ...otherwise load!
            else {
                scene::Mesh* pTargetMesh = nullptr;
                auto         ppxres      = LoadMeshInternal(loadParams, pGltfMesh, &pTargetMesh);
                if (Failed(ppxres)) {
                    return ppxres;
                }

                targetMesh = scene::MakeRef(pTargetMesh);
                if (!targetMesh) {
                    delete pTargetMesh;
                    return ppx::ERROR_ALLOCATION_FAILED;
                }
            }

            // Allocate node
            pTargetNode = new scene::MeshNode(targetMesh, loadParams.pTargetScene);
        } break;

        // Camera node
        case scene::NODE_TYPE_CAMERA: {
            const cgltf_camera* pGltfCamera = pGltfNode->camera;
            if (IsNull(pGltfCamera)) {
                return ppx::ERROR_SCENE_INVALID_SOURCE_CAMERA;
            }

            // Create camera
            std::unique_ptr<ppx::Camera> camera;
            if (pGltfCamera->type == cgltf_camera_type_perspective) {
                float fov = pGltfCamera->data.perspective.yfov;

                float aspect = 1.0f;
                if (pGltfCamera->data.perspective.has_aspect_ratio) {
                    aspect = pGltfCamera->data.perspective.aspect_ratio;
                    // BigWheels using horizontal FoV
                    fov = aspect * pGltfCamera->data.perspective.yfov;
                }

                float nearClip = pGltfCamera->data.perspective.znear;
                float farClip  = pGltfCamera->data.perspective.has_zfar ? pGltfCamera->data.perspective.zfar : (nearClip + 1000.0f);

                camera = std::unique_ptr<ppx::Camera>(new ppx::PerspCamera(glm::degrees(fov), aspect, nearClip, farClip));
            }
            else if (pGltfCamera->type == cgltf_camera_type_orthographic) {
                float left     = -pGltfCamera->data.orthographic.xmag;
                float right    = pGltfCamera->data.orthographic.xmag;
                float top      = -pGltfCamera->data.orthographic.ymag;
                float bottom   = pGltfCamera->data.orthographic.ymag;
                float nearClip = pGltfCamera->data.orthographic.znear;
                float farClip  = pGltfCamera->data.orthographic.zfar;

                camera = std::unique_ptr<ppx::Camera>(new ppx::OrthoCamera(left, right, bottom, top, nearClip, farClip));
            }
            //
            if (!camera) {
                return ppx::ERROR_SCENE_INVALID_SOURCE_CAMERA;
            }

            // Allocate node
            pTargetNode = new scene::CameraNode(std::move(camera), loadParams.pTargetScene);
        } break;

        // Light node
        case scene::NODE_TYPE_LIGHT: {
            const cgltf_light* pGltfLight = pGltfNode->light;
            if (IsNull(pGltfLight)) {
                return ppx::ERROR_SCENE_INVALID_SOURCE_LIGHT;
            }

            scene::LightType lightType = scene::LIGHT_TYPE_UNDEFINED;
            //
            switch (pGltfLight->type) {
                default: break;
                case cgltf_light_type_directional: lightType = scene::LIGHT_TYPE_DIRECTIONAL; break;
                case cgltf_light_type_point: lightType = scene::LIGHT_TYPE_POINT; break;
                case cgltf_light_type_spot: lightType = scene::LIGHT_TYPE_SPOT; break;
            }
            //
            if (lightType == scene::LIGHT_TYPE_UNDEFINED) {
                return ppx::ERROR_SCENE_INVALID_SOURCE_LIGHT;
            }

            // Allocate node
            auto pLightNode = new scene::LightNode(loadParams.pTargetScene);
            if (IsNull(pLightNode)) {
                return ppx::ERROR_ALLOCATION_FAILED;
            }

            pLightNode->SetType(lightType);
            pLightNode->SetColor(float3(pGltfLight->color[0], pGltfLight->color[1], pGltfLight->color[2]));
            pLightNode->SetIntensity(pGltfLight->intensity);
            pLightNode->SetDistance(pGltfLight->range);
            pLightNode->SetSpotInnerConeAngle(pGltfLight->spot_inner_cone_angle);
            pLightNode->SetSpotOuterConeAngle(pGltfLight->spot_outer_cone_angle);

            // Assign node
            pTargetNode = pLightNode;
        } break;
    }
    //
    if (IsNull(pTargetNode)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Set transform
    {
        if (pGltfNode->has_translation) {
            pTargetNode->SetTranslation(float3(
                pGltfNode->translation[0],
                pGltfNode->translation[1],
                pGltfNode->translation[2]));
        }

        if (pGltfNode->has_rotation) {
            float x = pGltfNode->rotation[0];
            float y = pGltfNode->rotation[1];
            float z = pGltfNode->rotation[2];
            float w = pGltfNode->rotation[3];

            // Extract euler angles using a matrix
            //
            // The values returned by glm::eulerAngles(quat) expects a
            // certain rotation order. It wasn't clear at the time of
            // this writing what that should be exactly. So, for the time
            // being, we'll use the matrix route and stick with XYZ.
            //
            auto q = glm::quat(w, x, y, z);
            auto R = glm::toMat4(q);

            float3 euler = float3(0);
            glm::extractEulerAngleXYZ(R, euler.x, euler.y, euler.z);

            pTargetNode->SetRotation(euler);
            pTargetNode->SetRotationOrder(ppx::Transform::RotationOrder::XYZ);
        }

        if (pGltfNode->has_scale) {
            pTargetNode->SetScale(float3(
                pGltfNode->scale[0],
                pGltfNode->scale[1],
                pGltfNode->scale[2]));
        }
    }

    // Set name
    pTargetNode->SetName(gltfObjectName);

    // Assign output
    *ppTargetNode = pTargetNode;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::FetchNodeInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_node*                     pGltfNode,
    scene::NodeRef&                       outNode)
{
    if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfNode)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Load object
    scene::Node* pNode = nullptr;
    //
    auto ppxres = LoadNodeInternal(loadParams, pGltfNode, &pNode);
    if (Failed(ppxres)) {
        return ppxres;
    }
    PPX_ASSERT_NULL_ARG(pNode);

    // Create object ref
    outNode = scene::MakeRef(pNode);
    if (!outNode) {
        delete pNode;
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    return ppx::SUCCESS;
}

void GltfLoader::GetUniqueGltfNodeIndices(
    const cgltf_node*     pGltfNode,
    std::set<cgltf_size>& uniqueGltfNodeIndices) const
{
    if (IsNull(pGltfNode)) {
        return;
    }

    cgltf_size nodeIndex = cgltf_node_index(mGltfData, pGltfNode);
    uniqueGltfNodeIndices.insert(nodeIndex);

    // Process children
    for (cgltf_size i = 0; i < pGltfNode->children_count; ++i) {
        const cgltf_node* pGltfChildNode = pGltfNode->children[i];

        // Recurse!
        GetUniqueGltfNodeIndices(pGltfChildNode, uniqueGltfNodeIndices);
    }
}

ppx::Result GltfLoader::LoadSceneInternal(
    const GltfLoader::InternalLoadParams& loadParams,
    const cgltf_scene*                    pGltfScene,
    scene::Scene*                         pTargetScene)
{
    if (IsNull(loadParams.pDevice) || IsNull(pGltfScene)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Get GLTF object name
    const std::string gltfObjectName = GetName(pGltfScene);

    // Get GLTF object index to use as object id
    const uint32_t index = static_cast<uint32_t>(cgltf_scene_index(mGltfData, pGltfScene));
    PPX_LOG_INFO("Loading GLTF scene[" << index << "]: " << gltfObjectName);

    // GLTF scenes contain only the root nodes. We need to walk scene's
    // root nodes and collect the file level indices for all the nodes.
    //
    std::set<cgltf_size> uniqueGltfNodeIndices;
    {
        for (cgltf_size i = 0; i < pGltfScene->nodes_count; ++i) {
            const cgltf_node* pGltfNode = pGltfScene->nodes[i];
            GetUniqueGltfNodeIndices(pGltfNode, uniqueGltfNodeIndices);
        }
    }

    // Load scene
    //
    // Keeps some maps so we can process the children
    std::unordered_map<cgltf_size, scene::Node*> indexToNodeMap;
    //
    {
        // Load nodes
        for (cgltf_size gltfNodeIndex : uniqueGltfNodeIndices) {
            const cgltf_node* pGltfNode = &mGltfData->nodes[gltfNodeIndex];

            scene::NodeRef node;
            //
            auto ppxres = FetchNodeInternal(
                loadParams,
                pGltfNode,
                node);
            if (Failed(ppxres)) {
                return ppxres;
            }

            // Save pointer to update map
            auto pNode = node.get();

            // Add node to scene
            ppxres = pTargetScene->AddNode(std::move(node));
            if (Failed(ppxres)) {
                return ppxres;
            }

            // Update map
            indexToNodeMap[gltfNodeIndex] = pNode;
        }
    }

    // Build children nodes
    {
        // Since all the nodes were flattened out, we don't need to recurse.
        for (cgltf_size gltfNodeIndex : uniqueGltfNodeIndices) {
            // Get target node
            auto it = indexToNodeMap.find(gltfNodeIndex);
            PPX_ASSERT_MSG((it != indexToNodeMap.end()), "GLTF node gltfObjectIndex has no mappping to a target node");
            scene::Node* pTargetNode = (*it).second;

            // GLTF node
            const cgltf_node* pGltfNode = &mGltfData->nodes[gltfNodeIndex];

            // Iterate node's children
            for (cgltf_size i = 0; i < pGltfNode->children_count; ++i) {
                // Get GLTF child node
                const cgltf_node* pGltfChildNode = pGltfNode->children[i];

                // Get GLTF child node index
                cgltf_size gltfChildNodeIndex = cgltf_node_index(mGltfData, pGltfChildNode);

                // Get target child node
                auto itChild = indexToNodeMap.find(gltfChildNodeIndex);
                PPX_ASSERT_MSG((it != indexToNodeMap.end()), "GLTF child node gltfObjectIndex has no mappping to a target child node");
                scene::Node* pTargetChildNode = (*itChild).second;

                // Add target child node to target node
                pTargetNode->AddChild(pTargetChildNode);
            }
        }
    }

    // Set name
    pTargetScene->SetName(gltfObjectName);

    return ppx::SUCCESS;
}

uint32_t GltfLoader::GetSamplerCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->samplers_count);
}

uint32_t GltfLoader::GetImageCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->images_count);
}

uint32_t GltfLoader::GetTextureCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->textures_count);
}

uint32_t GltfLoader::GetMaterialCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->materials_count);
}

uint32_t GltfLoader::GetMeshCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->meshes_count);
}

uint32_t GltfLoader::GetNodeCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->nodes_count);
}

uint32_t GltfLoader::GetSceneCount() const
{
    return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->scenes_count);
}

int32_t GltfLoader::GetSamplerIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->samplers;
    auto arrayEnd   = mGltfData->samplers + mGltfData->samplers_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_sampler& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetImageIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->images;
    auto arrayEnd   = mGltfData->images + mGltfData->images_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_image& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetTextureIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->textures;
    auto arrayEnd   = mGltfData->textures + mGltfData->textures_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_texture& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetMaterialIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->materials;
    auto arrayEnd   = mGltfData->materials + mGltfData->materials_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_material& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetMeshIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->meshes;
    auto arrayEnd   = mGltfData->meshes + mGltfData->meshes_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_mesh& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetNodeIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->nodes;
    auto arrayEnd   = mGltfData->nodes + mGltfData->nodes_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_node& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

int32_t GltfLoader::GetSceneIndex(const std::string& name) const
{
    auto arrayBegin = mGltfData->scenes;
    auto arrayEnd   = mGltfData->scenes + mGltfData->scenes_count;

    auto pGltfObject = std::find_if(
        arrayBegin,
        arrayEnd,
        [&name](const cgltf_scene& elem) -> bool {
            bool match = !IsNull(elem.name) && (name == elem.name);
            return match;
        });

    int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
    return index;
}

ppx::Result GltfLoader::LoadSampler(
    grfx::Device*    pDevice,
    uint32_t         samplerIndex,
    scene::Sampler** ppTargetSampler)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (samplerIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfSampler = &mGltfData->samplers[samplerIndex];
    if (IsNull(pGltfSampler)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;

    auto ppxres = LoadSamplerInternal(
        loadParams,
        pGltfSampler,
        ppTargetSampler);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadSampler(
    grfx::Device*      pDevice,
    const std::string& samplerName,
    scene::Sampler**   ppTargetSampler)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    int32_t meshIndex = this->GetSamplerIndex(samplerName);
    if (meshIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadSampler(
        pDevice,
        static_cast<uint32_t>(meshIndex),
        ppTargetSampler);
}

ppx::Result GltfLoader::LoadImage(
    grfx::Device*  pDevice,
    uint32_t       imageIndex,
    scene::Image** ppTargetImage)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (imageIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfImage = &mGltfData->images[imageIndex];
    if (IsNull(pGltfImage)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;

    auto ppxres = LoadImageInternal(
        loadParams,
        pGltfImage,
        ppTargetImage);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadImage(
    grfx::Device*      pDevice,
    const std::string& imageName,
    scene::Image**     ppTargetImage)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    int32_t imageIndex = this->GetImageIndex(imageName);
    if (imageIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadImage(
        pDevice,
        static_cast<uint32_t>(imageIndex),
        ppTargetImage);
}

ppx::Result GltfLoader::LoadTexture(
    grfx::Device*    pDevice,
    uint32_t         textureIndex,
    scene::Texture** ppTargetTexture)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (textureIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfTexture = &mGltfData->textures[textureIndex];
    if (IsNull(pGltfTexture)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;

    auto ppxres = LoadTextureInternal(
        loadParams,
        pGltfTexture,
        ppTargetTexture);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadTexture(
    grfx::Device*      pDevice,
    const std::string& textureName,
    scene::Texture**   ppTargetTexture)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    int32_t textureIndex = this->GetTextureIndex(textureName);
    if (textureIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadTexture(
        pDevice,
        static_cast<uint32_t>(textureIndex),
        ppTargetTexture);
}

ppx::Result GltfLoader::LoadMaterial(
    grfx::Device*             pDevice,
    uint32_t                  materialIndex,
    scene::Material**         ppTargetMaterial,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (materialIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfMaterial = &mGltfData->materials[materialIndex];
    if (IsNull(pGltfMaterial)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;

    auto ppxres = LoadMaterialInternal(
        loadParams,
        pGltfMaterial,
        ppTargetMaterial);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadMaterial(
    grfx::Device*             pDevice,
    const std::string&        materialName,
    scene::Material**         ppTargetMaterial,
    const scene::LoadOptions& loadOptions)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    int32_t materialIndex = this->GetMaterialIndex(materialName);
    if (materialIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadMaterial(
        pDevice,
        static_cast<uint32_t>(materialIndex),
        ppTargetMaterial,
        loadOptions);
}

ppx::Result GltfLoader::LoadMesh(
    grfx::Device*             pDevice,
    uint32_t                  meshIndex,
    scene::Mesh**             ppTargetMesh,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (meshIndex >= static_cast<uint32_t>(mGltfData->meshes_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfMesh = &mGltfData->meshes[meshIndex];
    if (IsNull(pGltfMesh)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;
    loadParams.pMaterialFactory               = loadOptions.GetMaterialFactory();
    loadParams.requiredVertexAttributes       = loadOptions.GetRequiredAttributes();

    // Use default material factory if one wasn't supplied
    if (IsNull(loadParams.pMaterialFactory)) {
        loadParams.pMaterialFactory = &mDefaultMaterialFactory;
    }

    auto ppxres = LoadMeshInternal(
        loadParams,
        pGltfMesh,
        ppTargetMesh);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadMesh(
    grfx::Device*             pDevice,
    const std::string&        meshName,
    scene::Mesh**             ppTargetMesh,
    const scene::LoadOptions& loadOptions)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    int32_t meshIndex = this->GetMeshIndex(meshName);
    if (meshIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadMesh(
        pDevice,
        static_cast<uint32_t>(meshIndex),
        ppTargetMesh,
        loadOptions);
}

ppx::Result GltfLoader::LoadNode(
    grfx::Device*             pDevice,
    uint32_t                  nodeIndex,
    scene::Node**             ppTargetNode,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (nodeIndex >= static_cast<uint32_t>(mGltfData->nodes_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltNode = &mGltfData->nodes[nodeIndex];
    if (IsNull(pGltNode)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;
    loadParams.pMaterialFactory               = loadOptions.GetMaterialFactory();
    loadParams.requiredVertexAttributes       = loadOptions.GetRequiredAttributes();

    // Use default material factory if one wasn't supplied
    if (IsNull(loadParams.pMaterialFactory)) {
        loadParams.pMaterialFactory = &mDefaultMaterialFactory;
    }

    auto ppxres = LoadNodeInternal(
        loadParams,
        pGltNode,
        ppTargetNode);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return SUCCESS;
}

ppx::Result GltfLoader::LoadNode(
    grfx::Device*             pDevice,
    const std::string&        nodeName,
    scene::Node**             ppTargetNode,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    int32_t nodeIndex = this->GetNodeIndex(nodeName);
    if (nodeIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadNode(
        pDevice,
        static_cast<uint32_t>(nodeIndex),
        ppTargetNode,
        loadOptions);
}

ppx::Result GltfLoader::LoadNodeTransformOnly(
    uint32_t      nodeIndex,
    scene::Node** ppTargetNode)
{
    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (nodeIndex >= static_cast<uint32_t>(mGltfData->nodes_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltNode = &mGltfData->nodes[nodeIndex];
    if (IsNull(pGltNode)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.transformOnly                  = true;

    auto ppxres = LoadNodeInternal(
        loadParams,
        pGltNode,
        ppTargetNode);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return SUCCESS;
}

ppx::Result GltfLoader::LoadNodeTransformOnly(
    const std::string& nodeName,
    scene::Node**      ppTargetNode)
{
    int32_t nodeIndex = this->GetNodeIndex(nodeName);
    if (nodeIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadNodeTransformOnly(
        static_cast<uint32_t>(nodeIndex),
        ppTargetNode);
}

ppx::Result GltfLoader::LoadScene(
    grfx::Device*             pDevice,
    uint32_t                  sceneIndex,
    scene::Scene**            ppTargetScene,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice) || IsNull(ppTargetScene)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!HasGltfData()) {
        return ppx::ERROR_SCENE_NO_SOURCE_DATA;
    }

    if (sceneIndex >= static_cast<uint32_t>(mGltfData->scenes_count)) {
        return ppx::ERROR_OUT_OF_RANGE;
    }

    auto pGltfScene = &mGltfData->scenes[sceneIndex];
    if (IsNull(pGltfScene)) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    GltfLoader::InternalLoadParams loadParams = {};
    loadParams.pDevice                        = pDevice;
    loadParams.pMaterialFactory               = loadOptions.GetMaterialFactory();
    loadParams.requiredVertexAttributes       = loadOptions.GetRequiredAttributes();

    // Use default material factory if one wasn't supplied
    if (IsNull(loadParams.pMaterialFactory)) {
        loadParams.pMaterialFactory = &mDefaultMaterialFactory;
    }

    // Build mesh material -> vertex attribute masks mappings
    GltfLoader::MeshMaterialVertexAttributeMasks meshDataVertexAttributes;
    CalculateMeshMaterialVertexAttributeMasks(
        loadParams.pMaterialFactory,
        &meshDataVertexAttributes);

    loadParams.pMeshMaterialVertexAttributeMasks = &meshDataVertexAttributes;

    // Allocate resource manager
    auto resourceManager = std::make_unique<scene::ResourceManager>();

    // Set laod params resource manager
    loadParams.pResourceManager = resourceManager.get();

    // Allocate the node so we can set the resource manager and target scene
    scene::Scene* pTargetScene = new scene::Scene(std::move(resourceManager));
    if (IsNull(pTargetScene)) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    // Set load params target scene
    loadParams.pTargetScene = pTargetScene;

    // Load scene
    auto ppxres = LoadSceneInternal(
        loadParams,
        pGltfScene,
        pTargetScene);
    if (Failed(ppxres)) {
        return ppxres;
    }

    PPX_LOG_INFO("Scene load complete: " << GetName(pGltfScene));
    PPX_LOG_INFO("   Num samplers : " << pTargetScene->GetSamplerCount());
    PPX_LOG_INFO("   Num images   : " << pTargetScene->GetImageCount());
    PPX_LOG_INFO("   Num textures : " << pTargetScene->GetTextureCount());
    PPX_LOG_INFO("   Num materials: " << pTargetScene->GetMaterialCount());
    PPX_LOG_INFO("   Num mesh data: " << pTargetScene->GetMeshDataCount());
    PPX_LOG_INFO("   Num meshes   : " << pTargetScene->GetMeshCount());

    *ppTargetScene = pTargetScene;

    return ppx::SUCCESS;
}

ppx::Result GltfLoader::LoadScene(
    grfx::Device*             pDevice,
    const std::string&        sceneName,
    scene::Scene**            ppTargetScene,
    const scene::LoadOptions& loadOptions)
{
    if (IsNull(pDevice)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    int32_t sceneIndex = this->GetSceneIndex(sceneName);
    if (sceneIndex < 0) {
        return ppx::ERROR_ELEMENT_NOT_FOUND;
    }

    return LoadScene(
        pDevice,
        static_cast<uint32_t>(sceneIndex),
        ppTargetScene,
        loadOptions);
}

} // namespace scene
} // namespace ppx
