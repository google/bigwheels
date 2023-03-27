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

#include "ppx/ppx.h"
#include "ppx/camera.h"
#include "ppx/graphics_util.h"

#include <filesystem>

#define ENABLE_GPU_QUERIES

using namespace ppx;

#if defined(USE_DX11)
const grfx::Api kApi = grfx::API_DX_11_1;
#elif defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

// Skybox registers
#define SKYBOX_CONSTANTS_REGISTER 0
#define SKYBOX_TEXTURE_REGISTER   1
#define SKYBOX_SAMPLER_REGISTER   2

// Material registers
// b#
#define SCENE_CONSTANTS_REGISTER    0
#define MATERIAL_CONSTANTS_REGISTER 1
#define MODEL_CONSTANTS_REGISTER    2
// s#
#define CLAMPED_SAMPLER_REGISTER 3
// t#
#define LIGHT_DATA_REGISTER         4
#define ALBEDO_TEXTURE_REGISTER     5
#define ROUGHNESS_TEXTURE_REGISTER  6
#define METALNESS_TEXTURE_REGISTER  7
#define NORMAL_MAP_TEXTURE_REGISTER 8
#define AMB_OCC_TEXTURE_REGISTER    9
#define HEIGHT_MAP_TEXTURE_REGISTER 10
#define IBL_MAP_TEXTURE_REGISTER    11
#define ENV_MAP_TEXTURE_REGISTER    12

const float3 F0_MetalTitanium   = float3(0.542f, 0.497f, 0.449f);
const float3 F0_MetalChromium   = float3(0.549f, 0.556f, 0.554f);
const float3 F0_MetalIron       = float3(0.562f, 0.565f, 0.578f);
const float3 F0_MetalNickel     = float3(0.660f, 0.609f, 0.526f);
const float3 F0_MetalPlatinum   = float3(0.673f, 0.637f, 0.585f);
const float3 F0_MetalCopper     = float3(0.955f, 0.638f, 0.538f);
const float3 F0_MetalPalladium  = float3(0.733f, 0.697f, 0.652f);
const float3 F0_MetalZinc       = float3(0.664f, 0.824f, 0.850f);
const float3 F0_MetalGold       = float3(1.022f, 0.782f, 0.344f);
const float3 F0_MetalAluminum   = float3(0.913f, 0.922f, 0.924f);
const float3 F0_MetalSilver     = float3(0.972f, 0.960f, 0.915f);
const float3 F0_DiletricWater   = float3(0.020f);
const float3 F0_DiletricPlastic = float3(0.040f);
const float3 F0_DiletricGlass   = float3(0.045f);
const float3 F0_DiletricCrystal = float3(0.050f);
const float3 F0_DiletricGem     = float3(0.080f);
const float3 F0_DiletricDiamond = float3(0.150f);

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) override;
    virtual void Shutdown() override;
    virtual void Render() override;

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
#ifdef ENABLE_GPU_QUERIES
        ppx::grfx::QueryPtr timestampQuery;
        ppx::grfx::QueryPtr pipelineStatsQuery;
#endif
    };

    grfx::PipelineStatistics mPipelineStatistics = {};
    uint64_t                 mTotalGpuFrameTime  = 0;

    grfx::TexturePtr m1x1BlackTexture;
    grfx::TexturePtr m1x1WhiteTexture;

    std::vector<PerFrame>      mPerFrame;
    PerspCamera                mCamera;
    grfx::DescriptorPoolPtr    mDescriptorPool;
    grfx::MeshPtr              mKnob;
    grfx::MeshPtr              mSphere;
    grfx::MeshPtr              mCube;
    grfx::MeshPtr              mMonkey;
    grfx::MeshPtr              mAltimeterModel;
    std::vector<grfx::MeshPtr> mMeshes;

    // Descriptor Set 0 - Scene Data
    grfx::DescriptorSetLayoutPtr mSceneDataLayout;
    grfx::DescriptorSetPtr       mSceneDataSet;
    grfx::BufferPtr              mCpuSceneConstants;
    grfx::BufferPtr              mGpuSceneConstants;
    grfx::BufferPtr              mCpuLightConstants;
    grfx::BufferPtr              mGpuLightConstants;

    // Descriptor Set 1 - MaterialData Resources
    grfx::DescriptorSetLayoutPtr mMaterialResourcesLayout;

    struct MaterialResources
    {
        grfx::DescriptorSetPtr set;
        grfx::TexturePtr       albedoTexture;
        grfx::TexturePtr       roughnessTexture;
        grfx::TexturePtr       metalnessTexture;
        grfx::TexturePtr       normalMapTexture;
    };

    grfx::SamplerPtr                  mSampler;
    MaterialResources                 mWoodMaterial;
    MaterialResources                 mTilesMaterial;
    MaterialResources                 mAltimeterMaterial;
    std::vector<grfx::DescriptorSet*> mMaterialResourcesSets;

    // Descriptor Set 2 - MaterialData Data
    grfx::DescriptorSetLayoutPtr mMaterialDataLayout;
    grfx::DescriptorSetPtr       mMaterialDataSet;
    grfx::BufferPtr              mCpuMaterialConstants;
    grfx::BufferPtr              mGpuMaterialConstants;

    // Descriptor Set 3 - Model Data
    grfx::DescriptorSetLayoutPtr mModelDataLayout;
    grfx::DescriptorSetPtr       mModelDataSet;
    grfx::BufferPtr              mCpuModelConstants;
    grfx::BufferPtr              mGpuModelConstants;

    grfx::PipelineInterfacePtr           mPipelineInterface;
    grfx::GraphicsPipelinePtr            mGouraudPipeline;
    grfx::GraphicsPipelinePtr            mPhongPipeline;
    grfx::GraphicsPipelinePtr            mBlinnPhongPipeline;
    grfx::GraphicsPipelinePtr            mPBRPipeline;
    std::vector<grfx::GraphicsPipeline*> mShaderPipelines;

    struct MaterialData
    {
        float3 albedo          = float3(0.4f, 0.4f, 0.7f);
        float  roughness       = 0.5f; // 0 = smooth, 1 = rough
        float  metalness       = 0.5f; // 0 = dielectric, 1 = metal
        float  iblStrength     = 0.4f; // 0 = nocontrib, 10 = max
        float  envStrength     = 0.3f; // 0 = nocontrib, 1 = max
        bool   albedoSelect    = 1;    // 0 = value, 1 = texture
        bool   roughnessSelect = 1;    // 0 = value, 1 = texture
        bool   metalnessSelect = 1;    // 0 = value, 1 = texture
        bool   normalSelect    = 0;    // 0 = attrb, 1 = texture
        bool   iblSelect       = 0;    // 0 = white, 1 = texture
        bool   envSelect       = 1;    // 0 = none,  1 = texture
    };

    float        mRotY         = 0;
    float        mTargetRotY   = 0;
    float        mAmbient      = 0;
    MaterialData mMaterialData = {};
    float3       mAlbedoColor  = float3(1);

    std::vector<float3> mF0 = {
        F0_MetalTitanium,
        F0_MetalChromium,
        F0_MetalIron,
        F0_MetalNickel,
        F0_MetalPlatinum,
        F0_MetalCopper,
        F0_MetalPalladium,
        F0_MetalZinc,
        F0_MetalGold,
        F0_MetalAluminum,
        F0_MetalSilver,
        F0_DiletricWater,
        F0_DiletricPlastic,
        F0_DiletricGlass,
        F0_DiletricCrystal,
        F0_DiletricGem,
        F0_DiletricDiamond,
        float3(0.04f),
    };

    uint32_t                 mMeshIndex = 0;
    std::vector<const char*> mMeshNames = {
        "Knob",
        "Sphere",
        "Cube",
        "Monkey",
        "Altimeter",
    };

    uint32_t                 mF0Index = 0;
    std::vector<const char*> mF0Names = {
        "MetalTitanium",
        "MetalChromium",
        "MetalIron",
        "MetalNickel",
        "MetalPlatinum",
        "MetalCopper",
        "MetalPalladium",
        "MetalZinc",
        "MetalGold",
        "MetalAluminum",
        "MetalSilver",
        "DiletricWater",
        "DiletricPlastic",
        "DiletricGlass",
        "DiletricCrystal",
        "DiletricGem",
        "DiletricDiamond",
        "Use Albedo Color",
    };

    uint32_t                 mMaterialIndex = 0;
    std::vector<const char*> mMaterialNames = {
        "Wood",
        "Tiles",
        "Altimeter",
    };

    uint32_t                 mShaderIndex = 3;
    std::vector<const char*> mShaderNames = {
        "Gouraud",
        "Phong",
        "Blinn",
        "PBR",
    };

private:
    void SetupSamplers();
    void SetupMaterialResources(
        const std::filesystem::path& albedoPath,
        const std::filesystem::path& roughnessPath,
        const std::filesystem::path& metalnessPath,
        const std::filesystem::path& normalMapPath,
        MaterialResources&           materialResources);
    void SetupMaterials();
    void DrawGui();
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName                    = "basic_material";
    settings.grfx.api                   = kApi;
    settings.grfx.swapchain.depthFormat = grfx::FORMAT_D32_FLOAT;
    settings.grfx.enableDebug           = false;
    settings.enableImGui                = true;
    settings.grfx.numFramesInFlight     = 1;
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

void ProjApp::SetupSamplers()
{
    // Sampler
    grfx::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.magFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.minFilter               = grfx::FILTER_LINEAR;
    samplerCreateInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.minLod                  = 0.0f;
    samplerCreateInfo.maxLod                  = FLT_MAX;
    PPX_CHECKED_CALL(GetDevice()->CreateSampler(&samplerCreateInfo, &mSampler));
}

void ProjApp::SetupMaterialResources(
    const std::filesystem::path& albedoPath,
    const std::filesystem::path& roughnessPath,
    const std::filesystem::path& metalnessPath,
    const std::filesystem::path& normalMapPath,
    MaterialResources&           materialResources)
{
    PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mMaterialResourcesLayout, &materialResources.set));

    // Albedo
    {
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(albedoPath), &materialResources.albedoTexture));

        grfx::WriteDescriptor write = {};
        write.binding               = ALBEDO_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = materialResources.albedoTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // Roughness
    {
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(roughnessPath), &materialResources.roughnessTexture));

        grfx::WriteDescriptor write = {};
        write.binding               = ROUGHNESS_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = materialResources.roughnessTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // Metalness
    {
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(metalnessPath), &materialResources.metalnessTexture));

        grfx::WriteDescriptor write = {};
        write.binding               = METALNESS_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = materialResources.metalnessTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // Normal map
    {
        PPX_CHECKED_CALL(grfx_util::CreateTextureFromFile(GetDevice()->GetGraphicsQueue(), GetAssetPath(normalMapPath), &materialResources.normalMapTexture));

        grfx::WriteDescriptor write = {};
        write.binding               = NORMAL_MAP_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = materialResources.normalMapTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // IBL map (not used)
    {
        grfx::WriteDescriptor write = {};
        write.binding               = IBL_MAP_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = m1x1WhiteTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // Environment reflection map (not used)
    {
        grfx::WriteDescriptor write = {};
        write.binding               = ENV_MAP_TEXTURE_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageView            = m1x1WhiteTexture->GetSampledImageView();
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }

    // Sampler
    {
        grfx::WriteDescriptor write = {};
        write.binding               = CLAMPED_SAMPLER_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_SAMPLER;
        write.pSampler              = mSampler;
        PPX_CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
    }
}

void ProjApp::SetupMaterials()
{
    // Layout
    grfx::DescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.bindings.push_back({grfx::DescriptorBinding{ALBEDO_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{ROUGHNESS_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{METALNESS_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{NORMAL_MAP_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{IBL_MAP_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{ENV_MAP_TEXTURE_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    createInfo.bindings.push_back({grfx::DescriptorBinding{CLAMPED_SAMPLER_REGISTER, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mMaterialResourcesLayout));

    // Wood
    {
        SetupMaterialResources(
            "materials/textures/wood/albedo.png",
            "materials/textures/wood/roughness.png",
            "materials/textures/wood/metalness.png",
            "materials/textures/wood/normal.png",
            mWoodMaterial);
        mMaterialResourcesSets.push_back(mWoodMaterial.set);
    };

    // Tiles
    {
        SetupMaterialResources(
            "materials/textures/tiles/albedo.png",
            "materials/textures/tiles/roughness.png",
            "materials/textures/tiles/metalness.png",
            "materials/textures/tiles/normal.png",
            mTilesMaterial);
        mMaterialResourcesSets.push_back(mTilesMaterial.set);
    };

    // Altimeter
    {
        SetupMaterialResources(
            "materials/textures/altimeter/albedo.jpg",
            "materials/textures/altimeter/roughness.jpg",
            "materials/textures/altimeter/metalness.jpg",
            "materials/textures/altimeter/normal.jpg",
            mAltimeterMaterial);
        mMaterialResourcesSets.push_back(mAltimeterMaterial.set);
    }
}

void ProjApp::Setup()
{
    PPX_CHECKED_CALL(grfx_util::CreateTexture1x1<uint8_t>(GetDevice()->GetGraphicsQueue(), {0, 0, 0, 0}, &m1x1BlackTexture));
    PPX_CHECKED_CALL(grfx_util::CreateTexture1x1<uint8_t>(GetDevice()->GetGraphicsQueue(), {255, 255, 255, 255}, &m1x1WhiteTexture));
    mF0Index = static_cast<uint32_t>(mF0Names.size() - 1);

    // Cameras
    {
        mCamera = PerspCamera(60.0f, GetWindowAspect());
    }

    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 1000;
        createInfo.sampledImage                   = 1000;
        createInfo.uniformBuffer                  = 1000;
        createInfo.structuredBuffer               = 1000;

        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool));
    }

    // Meshes
    {
        TriMeshOptions options = TriMeshOptions().Indices().VertexColors().Normals().TexCoords().Tangents();

        {
            Geometry geo;
            TriMesh  mesh = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/material_sphere.obj"), options);
            PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
            PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mKnob));
            mMeshes.push_back(mKnob);
        }

        {
            Geometry geo;
            TriMesh  mesh = TriMesh::CreateSphere(0.75f, 128, 64, TriMeshOptions(options).TexCoordScale(float2(2)));
            PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
            PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mSphere));
            mMeshes.push_back(mSphere);
        }

        {
            Geometry geo;
            TriMesh  mesh = TriMesh::CreateCube(float3(1.0f), options);
            PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
            PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mCube));
            mMeshes.push_back(mCube);
        }

        {
            Geometry geo;
            TriMesh  mesh = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/monkey.obj"), TriMeshOptions(options).Scale(float3(0.75f)));
            PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
            PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mMonkey));
            mMeshes.push_back(mMonkey);
        }

        {
            Geometry geo;
            TriMesh  mesh = TriMesh::CreateFromOBJ(GetAssetPath("basic/models/altimeter/altimeter.obj"), TriMeshOptions(options).Scale(float3(0.75f)));
            PPX_CHECKED_CALL(Geometry::Create(mesh, &geo));
            PPX_CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetGraphicsQueue(), &geo, &mAltimeterModel));
            mMeshes.push_back(mAltimeterModel);
        }
    }

    // Scene data
    {
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings.push_back({grfx::DescriptorBinding{SCENE_CONSTANTS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        createInfo.bindings.push_back({grfx::DescriptorBinding{LIGHT_DATA_REGISTER, grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mSceneDataLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mSceneDataLayout, &mSceneDataSet));

        // Scene constants
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_CONSTANT_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mCpuSceneConstants));

        bufferCreateInfo.usageFlags.bits.transferDst   = true;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mGpuSceneConstants));

        // HlslLight constants
        bufferCreateInfo                             = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_STRUCTURED_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mCpuLightConstants));

        bufferCreateInfo.structuredElementStride          = 32;
        bufferCreateInfo.usageFlags.bits.transferDst      = true;
        bufferCreateInfo.usageFlags.bits.structuredBuffer = true;
        bufferCreateInfo.memoryUsage                      = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mGpuLightConstants));

        grfx::WriteDescriptor write = {};
        write.binding               = SCENE_CONSTANTS_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mGpuSceneConstants;
        PPX_CHECKED_CALL(mSceneDataSet->UpdateDescriptors(1, &write));

        write                        = {};
        write.binding                = LIGHT_DATA_REGISTER;
        write.arrayIndex             = 0;
        write.type                   = grfx::DESCRIPTOR_TYPE_STRUCTURED_BUFFER;
        write.bufferOffset           = 0;
        write.bufferRange            = PPX_WHOLE_SIZE;
        write.structuredElementCount = 1;
        write.pBuffer                = mGpuLightConstants;
        PPX_CHECKED_CALL(mSceneDataSet->UpdateDescriptors(1, &write));
    }

    // Samplers
    SetupSamplers();

    // Material data resources
    SetupMaterials();

    // MaterialData data
    {
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings.push_back({grfx::DescriptorBinding{MATERIAL_CONSTANTS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mMaterialDataLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mMaterialDataLayout, &mMaterialDataSet));

        // MaterialData constants
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_CONSTANT_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mCpuMaterialConstants));

        bufferCreateInfo.usageFlags.bits.transferDst   = true;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mGpuMaterialConstants));

        grfx::WriteDescriptor write = {};
        write.binding               = MATERIAL_CONSTANTS_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mGpuMaterialConstants;
        PPX_CHECKED_CALL(mMaterialDataSet->UpdateDescriptors(1, &write));
    }

    // Model data
    {
        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings.push_back({grfx::DescriptorBinding{MODEL_CONSTANTS_REGISTER, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS}});
        PPX_CHECKED_CALL(GetDevice()->CreateDescriptorSetLayout(&createInfo, &mModelDataLayout));

        PPX_CHECKED_CALL(GetDevice()->AllocateDescriptorSet(mDescriptorPool, mModelDataLayout, &mModelDataSet));

        // Model constants
        grfx::BufferCreateInfo bufferCreateInfo      = {};
        bufferCreateInfo.size                        = PPX_MINIMUM_CONSTANT_BUFFER_SIZE;
        bufferCreateInfo.usageFlags.bits.transferSrc = true;
        bufferCreateInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mCpuModelConstants));

        bufferCreateInfo.usageFlags.bits.transferDst   = true;
        bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
        bufferCreateInfo.memoryUsage                   = grfx::MEMORY_USAGE_GPU_ONLY;
        PPX_CHECKED_CALL(GetDevice()->CreateBuffer(&bufferCreateInfo, &mGpuModelConstants));

        grfx::WriteDescriptor write = {};
        write.binding               = MODEL_CONSTANTS_REGISTER;
        write.arrayIndex            = 0;
        write.type                  = grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.bufferOffset          = 0;
        write.bufferRange           = PPX_WHOLE_SIZE;
        write.pBuffer               = mGpuModelConstants;
        PPX_CHECKED_CALL(mModelDataSet->UpdateDescriptors(1, &write));
    }

    // Pipeline Interface
    {
        grfx::PipelineInterfaceCreateInfo createInfo = {};
        createInfo.setCount                          = 4;
        createInfo.sets[0].set                       = 0;
        createInfo.sets[0].pLayout                   = mSceneDataLayout;
        createInfo.sets[1].set                       = 1;
        createInfo.sets[1].pLayout                   = mMaterialResourcesLayout;
        createInfo.sets[2].set                       = 2;
        createInfo.sets[2].pLayout                   = mMaterialDataLayout;
        createInfo.sets[3].set                       = 3;
        createInfo.sets[3].pLayout                   = mModelDataLayout;

        PPX_CHECKED_CALL(GetDevice()->CreatePipelineInterface(&createInfo, &mPipelineInterface));
    }

    // Pipeline
    {
        grfx::GraphicsPipelineCreateInfo2 gpCreateInfo  = {};
        gpCreateInfo.vertexInputState.bindingCount      = CountU32(mKnob->GetDerivedVertexBindings());
        gpCreateInfo.vertexInputState.bindings[0]       = mKnob->GetDerivedVertexBindings()[0];
        gpCreateInfo.vertexInputState.bindings[1]       = mKnob->GetDerivedVertexBindings()[1];
        gpCreateInfo.vertexInputState.bindings[2]       = mKnob->GetDerivedVertexBindings()[2];
        gpCreateInfo.vertexInputState.bindings[3]       = mKnob->GetDerivedVertexBindings()[3];
        gpCreateInfo.vertexInputState.bindings[4]       = mKnob->GetDerivedVertexBindings()[4];
        gpCreateInfo.vertexInputState.bindings[5]       = mKnob->GetDerivedVertexBindings()[5];
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
        gpCreateInfo.pPipelineInterface                 = mPipelineInterface;

        grfx::ShaderModulePtr VS;

        std::vector<char> bytecode = LoadShader("materials/shaders", "VertexShader.vs");
        PPX_ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
        grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
        PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &VS));

        // Gouraud
        {
            grfx::ShaderModulePtr PS;

            bytecode = LoadShader("materials/shaders", "Gouraud.ps");
            PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
            shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

            gpCreateInfo.VS = {VS.Get(), "vsmain"};
            gpCreateInfo.PS = {PS.Get(), "psmain"};

            PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mGouraudPipeline));
            GetDevice()->DestroyShaderModule(PS);

            mShaderPipelines.push_back(mGouraudPipeline);
        }

        // Phong
        {
            grfx::ShaderModulePtr PS;

            bytecode = LoadShader("materials/shaders", "Phong.ps");
            PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
            shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

            gpCreateInfo.VS = {VS.Get(), "vsmain"};
            gpCreateInfo.PS = {PS.Get(), "psmain"};

            PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPhongPipeline));
            GetDevice()->DestroyShaderModule(PS);

            mShaderPipelines.push_back(mPhongPipeline);
        }

        // BlinnPhong
        {
            grfx::ShaderModulePtr PS;

            bytecode = LoadShader("materials/shaders", "BlinnPhong.ps");
            PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
            shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

            gpCreateInfo.VS = {VS.Get(), "vsmain"};
            gpCreateInfo.PS = {PS.Get(), "psmain"};

            PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mBlinnPhongPipeline));
            GetDevice()->DestroyShaderModule(PS);

            mShaderPipelines.push_back(mBlinnPhongPipeline);
        }

        // PBR
        {
            grfx::ShaderModulePtr PS;

            bytecode = LoadShader("materials/shaders", "PBR.ps");
            PPX_ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
            shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
            PPX_CHECKED_CALL(GetDevice()->CreateShaderModule(&shaderCreateInfo, &PS));

            gpCreateInfo.VS = {VS.Get(), "vsmain"};
            gpCreateInfo.PS = {PS.Get(), "psmain"};

            PPX_CHECKED_CALL(GetDevice()->CreateGraphicsPipeline(&gpCreateInfo, &mPBRPipeline));
            GetDevice()->DestroyShaderModule(PS);

            mShaderPipelines.push_back(mPBRPipeline);
        }
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

#ifdef ENABLE_GPU_QUERIES
        grfx::QueryCreateInfo queryCreateInfo = {};
        queryCreateInfo.type                  = grfx::QUERY_TYPE_TIMESTAMP;
        queryCreateInfo.count                 = 2;
        PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.timestampQuery));

        // Pipeline statistics query pool
        if (GetDevice()->PipelineStatsAvailable()) {
            queryCreateInfo       = {};
            queryCreateInfo.type  = grfx::QUERY_TYPE_PIPELINE_STATISTICS;
            queryCreateInfo.count = 1;
            PPX_CHECKED_CALL(GetDevice()->CreateQuery(&queryCreateInfo, &frame.pipelineStatsQuery));
        }
#endif

        mPerFrame.push_back(frame);
    }
}

void ProjApp::Shutdown()
{
}

void ProjApp::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    if (buttons & ppx::MOUSE_BUTTON_LEFT) {
        mTargetRotY += 0.25f * dx;
    }
}

void ProjApp::Render()
{
    uint32_t  iffIndex = GetInFlightFrameIndex();
    PerFrame& frame    = mPerFrame[0];

    grfx::SwapchainPtr swapchain = GetSwapchain();

    uint32_t imageIndex = UINT32_MAX;
    PPX_CHECKED_CALL(swapchain->AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

    // Wait for and reset image acquired fence
    PPX_CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

    // Wait for and reset render complete fence
    PPX_CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

    // ---------------------------------------------------------------------------------------------

    // Smooth out the rotation on Y
    mRotY += (mTargetRotY - mRotY) * 0.1f;

    // ---------------------------------------------------------------------------------------------

    // Update camera(s)
    mCamera.LookAt(float3(0, 0, 8), float3(0, 0, 0));

    // Update scene constants
    {
        PPX_HLSL_PACK_BEGIN();
        struct HlslSceneData
        {
            hlsl_uint<4>      frameNumber;
            hlsl_float<12>    time;
            hlsl_float4x4<64> viewProjectionMatrix;
            hlsl_float3<12>   eyePosition;
            hlsl_uint<4>      lightCount;
            hlsl_float<4>     ambient;
            hlsl_float<4>     iblLevelCount;
            hlsl_float<4>     envLevelCount;
        };
        PPX_HLSL_PACK_END();

        void* pMappedAddress = nullptr;
        PPX_CHECKED_CALL(mCpuSceneConstants->MapMemory(0, &pMappedAddress));

        HlslSceneData* pSceneData        = static_cast<HlslSceneData*>(pMappedAddress);
        pSceneData->viewProjectionMatrix = mCamera.GetViewProjectionMatrix();
        pSceneData->eyePosition          = mCamera.GetEyePosition();
        pSceneData->lightCount           = 4;
        pSceneData->ambient              = mAmbient;
        pSceneData->iblLevelCount        = 0;
        pSceneData->envLevelCount        = 0;

        mCpuSceneConstants->UnmapMemory();

        grfx::BufferToBufferCopyInfo copyInfo = {mCpuSceneConstants->GetSize()};
        GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuSceneConstants, mGpuSceneConstants, grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
    }

    // Lights
    {
        PPX_HLSL_PACK_BEGIN();
        struct HlslLight
        {
            hlsl_uint<4>    type;
            hlsl_float3<12> position;
            hlsl_float3<12> color;
            hlsl_float<4>   intensity;
        };
        PPX_HLSL_PACK_END();

        void* pMappedAddress = nullptr;
        PPX_CHECKED_CALL(mCpuLightConstants->MapMemory(0, &pMappedAddress));

        HlslLight* pLight  = static_cast<HlslLight*>(pMappedAddress);
        pLight[0].position = float3(10, 5, 10);
        pLight[1].position = float3(-10, 0, 5);
        pLight[2].position = float3(1, 10, 3);
        pLight[3].position = float3(-1, 0, 15);

        pLight[0].intensity = 0.07f;
        pLight[1].intensity = 0.10f;
        pLight[2].intensity = 0.15f;
        pLight[3].intensity = 0.17f;

        mCpuLightConstants->UnmapMemory();

        grfx::BufferToBufferCopyInfo copyInfo = {mCpuLightConstants->GetSize()};
        GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuLightConstants, mGpuLightConstants, grfx::RESOURCE_STATE_SHADER_RESOURCE, grfx::RESOURCE_STATE_SHADER_RESOURCE);
    }

    // MaterialData constatns
    {
        PPX_HLSL_PACK_BEGIN();
        struct HlslMaterial
        {
            hlsl_float3<16> F0;
            hlsl_float3<12> albedo;
            hlsl_float<4>   roughness;
            hlsl_float<4>   metalness;
            hlsl_float<4>   iblStrength;
            hlsl_float<4>   envStrength;
            hlsl_uint<4>    albedoSelect;
            hlsl_uint<4>    roughnessSelect;
            hlsl_uint<4>    metalnessSelect;
            hlsl_uint<4>    normalSelect;
            hlsl_uint<4>    iblSelect;
            hlsl_uint<4>    envSelect;
        };
        PPX_HLSL_PACK_END();

        void* pMappedAddress = nullptr;
        PPX_CHECKED_CALL(mCpuMaterialConstants->MapMemory(0, &pMappedAddress));

        HlslMaterial* pMaterial    = static_cast<HlslMaterial*>(pMappedAddress);
        pMaterial->F0              = mF0[mF0Index];
        pMaterial->albedo          = (mF0Index <= 10) ? mF0[mF0Index] : mAlbedoColor;
        pMaterial->roughness       = mMaterialData.roughness;
        pMaterial->metalness       = mMaterialData.metalness;
        pMaterial->iblStrength     = mMaterialData.iblStrength;
        pMaterial->envStrength     = mMaterialData.envStrength;
        pMaterial->albedoSelect    = mMaterialData.albedoSelect;
        pMaterial->roughnessSelect = mMaterialData.roughnessSelect;
        pMaterial->metalnessSelect = mMaterialData.metalnessSelect;
        pMaterial->normalSelect    = mMaterialData.normalSelect;
        pMaterial->iblSelect       = mMaterialData.iblSelect;
        pMaterial->envSelect       = mMaterialData.envSelect;

        mCpuMaterialConstants->UnmapMemory();

        grfx::BufferToBufferCopyInfo copyInfo = {mCpuMaterialConstants->GetSize()};
        GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuMaterialConstants, mGpuMaterialConstants, grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
    }

    // Update model constants
    {
        float4x4 R = glm::rotate(glm::radians(mRotY + 180.0f), float3(0, 1, 0));
        float4x4 S = glm::scale(float3(3.0f));
        float4x4 M = R * S;

        PPX_HLSL_PACK_BEGIN();
        struct HlslModelData
        {
            hlsl_float4x4<64> modelMatrix;
            hlsl_float4x4<64> normalMatrix;
            hlsl_float3<12>   debugColor;
        };
        PPX_HLSL_PACK_END();

        void* pMappedAddress = nullptr;
        PPX_CHECKED_CALL(mCpuModelConstants->MapMemory(0, &pMappedAddress));

        HlslModelData* pModelData = static_cast<HlslModelData*>(pMappedAddress);
        pModelData->modelMatrix   = M;
        pModelData->normalMatrix  = glm::inverseTranspose(M);

        mCpuModelConstants->UnmapMemory();

        grfx::BufferToBufferCopyInfo copyInfo = {mCpuModelConstants->GetSize()};
        GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuModelConstants, mGpuModelConstants, grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
    }

#ifdef ENABLE_GPU_QUERIES
    // Read query results
    if (GetFrameCount() > 0) {
        uint64_t data[2] = {0};
        PPX_CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
        mTotalGpuFrameTime = data[1] - data[0];
        if (GetDevice()->PipelineStatsAvailable()) {
            PPX_CHECKED_CALL(frame.pipelineStatsQuery->GetData(&mPipelineStatistics, sizeof(grfx::PipelineStatistics)));
        }
    }

    // Reset query
    frame.timestampQuery->Reset(0, 2);
    if (GetDevice()->PipelineStatsAvailable()) {
        frame.pipelineStatsQuery->Reset(0, 1);
    }
#endif

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
#ifdef ENABLE_GPU_QUERIES
            frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif
            frame.cmd->SetScissors(GetScissor());
            frame.cmd->SetViewports(GetViewport());
            // Draw model
            grfx::DescriptorSet* sets[4] = {nullptr};
            sets[0]                      = mSceneDataSet;
            sets[1]                      = mMaterialResourcesSets[mMaterialIndex];
            sets[2]                      = mMaterialDataSet;
            sets[3]                      = mModelDataSet;
            frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 4, sets);

            frame.cmd->BindGraphicsPipeline(mShaderPipelines[mShaderIndex]);

            frame.cmd->BindIndexBuffer(mMeshes[mMeshIndex]);
            frame.cmd->BindVertexBuffers(mMeshes[mMeshIndex]);

#ifdef ENABLE_GPU_QUERIES
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
            }
#endif
            frame.cmd->DrawIndexed(mMeshes[mMeshIndex]->GetIndexCount());
#ifdef ENABLE_GPU_QUERIES
            if (GetDevice()->PipelineStatsAvailable()) {
                frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
            }
#endif

            // Draw ImGui
            DrawDebugInfo([this]() { this->DrawGui(); });
            DrawImGui(frame.cmd);
        }
        frame.cmd->EndRenderPass();
#ifdef ENABLE_GPU_QUERIES
        frame.cmd->WriteTimestamp(frame.timestampQuery, grfx::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);

        // Resolve queries
        frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
        if (GetDevice()->PipelineStatsAvailable()) {
            frame.cmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
        }
#endif
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
    ImGui::Separator();

    // ImGui::SliderFloat("Rot Y", &mRotY, -180.0f, 180.0f, "%.03f degrees");
    //
    // ImGui::Separator();

    ImGui::SliderFloat("Ambient", &mAmbient, 0.0f, 1.0f, "%.03f");

    ImGui::Separator();

    static const char* currentModelName = mMeshNames[0];
    if (ImGui::BeginCombo("Geometry", currentModelName)) {
        for (size_t i = 0; i < mMeshNames.size(); ++i) {
            bool isSelected = (currentModelName == mMeshNames[i]);
            if (ImGui::Selectable(mMeshNames[i], isSelected)) {
                currentModelName = mMeshNames[i];
                mMeshIndex       = static_cast<uint32_t>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    static const char* currentShaderName = "PBR";
    if (ImGui::BeginCombo("Shader Pipeline", currentShaderName)) {
        for (size_t i = 0; i < mShaderNames.size(); ++i) {
            bool isSelected = (currentShaderName == mShaderNames[i]);
            if (ImGui::Selectable(mShaderNames[i], isSelected)) {
                currentShaderName = mShaderNames[i];
                mShaderIndex      = static_cast<uint32_t>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    static const char* currentF0Name = "Albedo Color";
    if (ImGui::BeginCombo("F0", currentF0Name)) {
        for (size_t i = 0; i < mF0Names.size(); ++i) {
            bool isSelected = (currentF0Name == mF0Names[i]);
            if (ImGui::Selectable(mF0Names[i], isSelected)) {
                currentF0Name = mF0Names[i];
                mF0Index      = static_cast<uint32_t>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    ImGui::ColorPicker4("Albedo Color", (float*)&mAlbedoColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);

    ImGui::Separator();

    static const char* currentMaterialName = mMaterialNames[0];
    if (ImGui::BeginCombo("Material Textures", currentMaterialName)) {
        for (size_t i = 0; i < mMaterialNames.size(); ++i) {
            bool isSelected = (currentMaterialName == mMaterialNames[i]);
            if (ImGui::Selectable(mMaterialNames[i], isSelected)) {
                currentMaterialName = mMaterialNames[i];
                mMaterialIndex      = static_cast<uint32_t>(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    ImGui::SliderFloat("Roughness", &mMaterialData.roughness, 0.0f, 1.0f, "%.03f degrees");
    ImGui::SliderFloat("Metalness", &mMaterialData.metalness, 0.0f, 1.0f, "%.03f degrees");
    ImGui::Checkbox("PBR Use Albedo Texture", &mMaterialData.albedoSelect);
    ImGui::Checkbox("PBR Use Roughness Texture", &mMaterialData.roughnessSelect);
    ImGui::Checkbox("PBR Use Metalness Texture", &mMaterialData.metalnessSelect);
    ImGui::Checkbox("PBR Use Normal Map", &mMaterialData.normalSelect);
    ImGui::Checkbox("PBR Use Reflection Map", &mMaterialData.envSelect);

    ImGui::Separator();

    ImGui::Columns(2);

    uint64_t frequency = 0;
    GetGraphicsQueue()->GetTimestampFrequency(&frequency);
    ImGui::Text("Previous GPU Frame Time");
    ImGui::NextColumn();
    ImGui::Text("%f ms ", static_cast<float>(mTotalGpuFrameTime / static_cast<double>(frequency)) * 1000.0f);
    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::Text("IAVertices");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.IAVertices);
    ImGui::NextColumn();

    ImGui::Text("IAPrimitives");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.IAPrimitives);
    ImGui::NextColumn();

    ImGui::Text("VSInvocations");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.VSInvocations);
    ImGui::NextColumn();

    ImGui::Text("CInvocations");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.CInvocations);
    ImGui::NextColumn();

    ImGui::Text("CPrimitives");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.CPrimitives);
    ImGui::NextColumn();

    ImGui::Text("PSInvocations");
    ImGui::NextColumn();
    ImGui::Text("%lu", mPipelineStatistics.PSInvocations);
    ImGui::NextColumn();

    ImGui::Columns(1);
}

int main(int argc, char** argv)
{
    ProjApp app;

    int res = app.Run(argc, argv);

    return res;
}
