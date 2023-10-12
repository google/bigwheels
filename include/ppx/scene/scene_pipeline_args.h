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

#ifndef ppx_scene_pipeline_args_h
#define ppx_scene_pipeline_args_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_material.h"
#include "ppx/grfx/grfx_descriptor.h"
#include "ppx/camera.h"

#include <unordered_map>

namespace ppx {
namespace scene {

class MaterialPipelineArgs;

// -------------------------------------------------------------------------------------------------

// size = 8
struct FrameParams
{
    uint32_t frameIndex; // offset = 0
    float    time;       // offset = 8
};

// size = 96
struct CameraParams
{
    float4x4 viewProjectionMatrix; // offset = 0
    float3   eyePosition;          // offset = 64
    float    nearDepth;            // offset = 76
    float3   viewDirection;        // offset = 80
    float    farDepth;             // offset = 92
};

// size = 128
struct InstanceParams
{
    float4x4 modelMatrix;        // offset = 0
    float4x4 inverseModelMatrix; // offset = 64
};

// size = 24
struct MaterialTextureParams
{
    uint     textureIndex;      // offset = 0
    uint     samplerIndex;      // offset = 4
    float2x2 texCoordTransform; // offset = 8
};

// size = 184
struct MaterialParams
{
    float4                       baseColorFactor;      // offset = 0
    float                        metallicFactor;       // offset = 16
    float                        roughnessFactor;      // offset = 20
    float                        occlusionStrength;    // offset = 24
    float3                       emissiveFactor;       // offset = 28
    float                        emissiveStrength;     // offset = 40
    scene::MaterialTextureParams baseColorTex;         // offset = 44
    scene::MaterialTextureParams metallicRoughnessTex; // offset = 68
    scene::MaterialTextureParams normalTex;            // offset = 92
    scene::MaterialTextureParams occlusionTex;         // offset = 116
    scene::MaterialTextureParams emssiveTex;           // offset = 140
};

// -------------------------------------------------------------------------------------------------

// Material Pipeline Arguments
//
//
class MaterialPipelineArgs
{
public:
    //
    // These constants correspond to values in:
    //   - scene_renderer/shaders/Config.hlsli
    //   - scene_renderer/shaders/MaterialInterface.hlsli
    //
    // @TODO: Find a more appropriate location for these
    //
    static const uint32_t MAX_UNIQUE_MATERIALS      = 32;
    static const uint32_t MAX_TEXTURES_PER_MATERIAL = 6;
    static const uint32_t MAX_MATERIAL_TEXTURES     = MAX_UNIQUE_MATERIALS * MAX_TEXTURES_PER_MATERIAL;

    static const uint32_t MAX_IBL_MAPS          = 8;
    static const uint32_t MAX_MATERIAL_SAMPLERS = 8;

    static const uint32_t FRAME_PARAMS_REGISTER            = 1;
    static const uint32_t CAMERA_PARAMS_REGISTER           = 2;
    static const uint32_t INSTANCE_PARAMS_REGISTER         = 3;
    static const uint32_t MATERIAL_PARAMS_REGISTER         = 4;
    static const uint32_t BRDF_LUT_SAMPLER_REGISTER        = 124;
    static const uint32_t BRDF_LUT_TEXTURE_REGISTER        = 125;
    static const uint32_t IBL_IRRADIANCE_SAMPLER_REGISTER  = 126;
    static const uint32_t IBL_ENVIRONMENT_SAMPLER_REGISTER = 127;
    static const uint32_t IBL_IRRADIANCE_MAP_REGISTER      = 128;
    static const uint32_t IBL_ENVIRONMENT_MAP_REGISTER     = 144;
    static const uint32_t MATERIAL_SAMPLERS_REGISTER       = 512;
    static const uint32_t MATERIAL_TEXTURES_REGISTER       = 1024;

    // ---------------------------------------------------------------------------------------------
    static const uint32_t INSTANCE_INDEX_CONSTANT_OFFSET     = 0;
    static const uint32_t MATERIAL_INDEX_CONSTANT_OFFSET     = 1;
    static const uint32_t IBL_INDEX_CONSTANT_OFFSET          = 2;
    static const uint32_t IBL_LEVEL_COUNT_CONSTANT_OFFSET    = 3;
    static const uint32_t DBG_VTX_ATTR_INDEX_CONSTANT_OFFSET = 4;

    // ---------------------------------------------------------------------------------------------

    // Maximum number of drawable instances
    static const uint32_t MAX_DRAWABLE_INSTANCES = 65536;

    // Required size of structs that map to HLSL
    static const uint32_t FRAME_PARAMS_STRUCT_SIZE            = 8;
    static const uint32_t CAMERA_PARAMS_STRUCT_SIZE           = 96;
    static const uint32_t INSTANCE_PARAMS_STRUCT_SIZE         = 128;
    static const uint32_t MATERIAL_TEXTURE_PARAMS_STRUCT_SIZE = 28;
    static const uint32_t MATERIAL_PARAMS_STRUCT_SIZE         = 164;

    // ---------------------------------------------------------------------------------------------

    MaterialPipelineArgs();
    virtual ~MaterialPipelineArgs();

    static ppx::Result Create(grfx::Device* pDevice, scene::MaterialPipelineArgs** ppTargetPipelineArgs);

    grfx::DescriptorPool*      GetDescriptorPool() const { return mDescriptorPool.Get(); }
    grfx::DescriptorSetLayout* GetDescriptorSetLayout() const { return mDescriptorSetLayout.Get(); }
    grfx::DescriptorSet*       GetDescriptorSet() const { return mDescriptorSet.Get(); }

    scene::FrameParams*  GetFrameParams() { return mFrameParamsAddress; }
    scene::CameraParams* GetCameraParams() { return mCameraParamsAddress; }
    void                 SetCameraParams(const ppx::Camera* pCamera);

    scene::InstanceParams* GetInstanceParams(uint32_t index);
    scene::MaterialParams* GetMaterialParams(uint32_t index);

    void SetIBLTextures(
        uint32_t                index,
        grfx::SampledImageView* pIrradiance,
        grfx::SampledImageView* pEnvironment);

    void SetMaterialSampler(uint32_t index, const scene::Sampler* pSampler);
    void SetMaterialTexture(uint32_t index, const scene::Image* pImage);

    void CopyBuffers(grfx::CommandBuffer* pCmd);

private:
    ppx::Result InitializeDefaultObjects(grfx::Device* pDevice);
    ppx::Result InitializeDescriptorSet(grfx::Device* pDevice);
    ppx::Result InitializeBuffers(grfx::Device* pDevice);
    ppx::Result SetDescriptors(grfx::Device* pDevice);
    ppx::Result InitializeResource(grfx::Device* pDevice);

protected:
    grfx::DescriptorPoolPtr      mDescriptorPool;
    grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    grfx::DescriptorSetPtr       mDescriptorSet;

    grfx::BufferPtr mCpuConstantParamsBuffer;
    grfx::BufferPtr mGpuConstantParamsBuffer;
    grfx::BufferPtr mCpuInstanceParamsBuffer;
    grfx::BufferPtr mGpuInstanceParamsBuffer;
    grfx::BufferPtr mCpuMateriaParamsBuffer;
    grfx::BufferPtr mGpuMateriaParamsBuffer;

    uint32_t mFrameParamsPaddedSize  = 0;
    uint32_t mCameraParamsPaddedSize = 0;
    uint32_t mFrameParamsOffset      = UINT32_MAX;
    uint32_t mCameraParamsOffset     = UINT32_MAX;

    uint32_t mTotalConstantParamsPaddedSize = 0;
    uint32_t mTotalInstanceParamsPaddedSize = 0;
    uint32_t mTotalMaterialParamsPaddedSize = 0;

    char*                mConstantParamsMappedAddress = nullptr;
    scene::FrameParams*  mFrameParamsAddress          = nullptr;
    scene::CameraParams* mCameraParamsAddress         = nullptr;

    char* mInstanceParamsMappedAddress = nullptr;
    char* mMaterialParamsMappedAddress = nullptr;

    ppx::grfx::SamplerPtr mDefaultSampler; // Nearest, repeats
    ppx::grfx::TexturePtr mDefaultTexture; // Purple texture

    ppx::grfx::SamplerPtr mDefaultBRDFLUTSampler; // Linear, clamps to edge
    ppx::grfx::TexturePtr mDefaultBRDFLUTTexture;

    // Make samplers useable for most cases
    ppx::grfx::SamplerPtr mDefaultIBLIrradianceSampler;  // Linear, U repeats, V clamps to edge, mip 0 only
    ppx::grfx::SamplerPtr mDefaultIBLEnvironmentSampler; // Linear, U repeats, V clamps to edge
    ppx::grfx::TexturePtr mDefaultIBLTexture;            // White texture
};

void CopyMaterialTextureParams(
    const std::unordered_map<const scene::Sampler*, uint32_t>& samplersIndexMap,
    const std::unordered_map<const scene::Image*, uint32_t>&   imagesIndexMap,
    const scene::TextureView&                                  srcTextureView,
    scene::MaterialTextureParams&                              dstTextureParams);

} // namespace scene
} // namespace ppx

#endif // ppx_scene_pipeline_args_h
