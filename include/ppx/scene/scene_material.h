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

#ifndef ppx_scene_material_h
#define ppx_scene_material_h

#include "ppx/scene/scene_config.h"
#include "ppx/grfx/grfx_image.h"

#define PPX_MATERIAL_IDENT_ERROR    "ppx_material_ident:error"
#define PPX_MATERIAL_IDENT_UNLIT    "ppx_material_ident:unlit"
#define PPX_MATERIAL_IDENT_STANDARD "ppx_material_ident:standard"

namespace ppx {
namespace scene {

// Image
//
// This class wraps a grfx::Image and a grfx::SampledImage objects to make
// pathing access to the GPU pipelines easier.
//
// This class owns mImage and mImageView and destroys them in the destructor.
//
// scene::Image objects can be shared between different scene::Texture objects.
//
// Corresponds to GLTF's image object.
//
class Image
    : public grfx::NamedObjectTrait
{
public:
    Image(
        grfx::Image*            pImage,
        grfx::SampledImageView* pImageView);
    virtual ~Image();

    grfx::Image*            GetImage() const { return mImage.Get(); }
    grfx::SampledImageView* GetImageView() const { return mImageView.Get(); }

private:
    grfx::ImagePtr            mImage     = nullptr;
    grfx::SampledImageViewPtr mImageView = nullptr;
};

// -------------------------------------------------------------------------------------------------

// Sampler
//
// This class wraps a grfx::Sampler object to make sharability on the ppx::scene
// level easier to understand.
//
// This class owns mSampler and destroys it in the destructor.
//
// scene::Sampler objects can be shared between different scene::Texture objects.
//
// Corresponds to GLTF's sampler object.
//
class Sampler
    : public grfx::NamedObjectTrait
{
public:
    Sampler(
        grfx::Sampler* pSampler);
    virtual ~Sampler();

    grfx::Sampler* GetSampler() const { return mSampler.Get(); }

private:
    grfx::SamplerPtr mSampler;
};

// -------------------------------------------------------------------------------------------------

// Texture
//
// This class is container for references to a scene::Image and a scene::Sampler
// objects.
//
// scene::Texture objects can be shared between different scene::Material objects
// via the scene::TextureView struct.
//
// Corresponds to GLTF's texture objects
//
class Texture
    : public grfx::NamedObjectTrait
{
public:
    Texture(
        const scene::ImageRef   image,
        const scene::SamplerRef sampler);
    virtual ~Texture();

    const scene::Image*   GetImage() const { return mImage.get(); }
    const scene::Sampler* GetSampler() const { return mSampler.get(); }

private:
    scene::ImageRef   mImage   = nullptr;
    scene::SamplerRef mSampler = nullptr;
};

// -------------------------------------------------------------------------------------------------

// Texture View
//
// This class contains a reference to a texture object and the transform
// data that must be applied by the shader before sampling a pixel.
//
// scene::Texture view objects are used directly by scene::Matreial objects.
//
// Corresponds to cgltf's texture view object.
//
class TextureView
{
public:
    TextureView();

    TextureView(
        const scene::TextureRef& texture,
        float2                   texCoordTranslate,
        float                    texCoordRotate,
        float2                   texCoordScale);

    ~TextureView();

    const scene::Texture* GetTexture() const { return mTexture.get(); }
    const float2&         GetTexCoordTranslate() const { return mTexCoordTranslate; }
    float                 GetTexCoordRotate() const { return mTexCoordRotate; }
    const float2&         GetTexCoordScale() const { return mTexCoordScale; }

    bool HasTexture() const { return mTexture ? true : false; }

private:
    scene::TextureRef mTexture           = nullptr;
    float2            mTexCoordTranslate = float2(0, 0);
    float             mTexCoordRotate    = 0;
    float2            mTexCoordScale     = float2(1, 1);
};

// -------------------------------------------------------------------------------------------------

// Base Material Class
//
// All materials deriving from this class must have a uniquely identifiable
// Material Ident String that's returned by GetIdentString().
//
// Materials must also provide a mask of all the vertex attributes it requires
// for rendering.
//
// scene::Material derivative objects can be shared beween different scene::Mesh
// objects via scene::PrimitiveBatch.
//
class Material
    : public grfx::NamedObjectTrait
{
public:
    Material() {}
    virtual ~Material() {}

    virtual std::string GetIdentString() const = 0;

    virtual scene::VertexAttributeFlags GetRequiredVertexAttributes() const = 0;
};

// -------------------------------------------------------------------------------------------------

// Error Material for when a primitive batch is missing a material.
//
// Implementations should render something recognizable. Default version
// renders solid purple: float3(1, 0, 1).
//
class ErrorMaterial
    : public scene::Material
{
public:
    ErrorMaterial() {}
    virtual ~ErrorMaterial() {}

    virtual std::string GetIdentString() const override { return PPX_MATERIAL_IDENT_ERROR; }

    virtual scene::VertexAttributeFlags GetRequiredVertexAttributes() const override;
};

// -------------------------------------------------------------------------------------------------

// Unlit Material
//
// Implementations should render a texture without any lighting. Base color factor
// can act as a multiplier for the values from base color texture.
//
// Corresponds to GLTF's KHR_materials_unlit:
//   https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_unlit/README.md
//
class UnlitMaterial
    : public scene::Material
{
public:
    UnlitMaterial() {}
    virtual ~UnlitMaterial() {}

    virtual std::string GetIdentString() const override { return PPX_MATERIAL_IDENT_UNLIT; }

    virtual scene::VertexAttributeFlags GetRequiredVertexAttributes() const override;

    const float4&             GetBaseColorFactor() const { return mBaseColorFactor; }
    const scene::TextureView& GetBaseColorTexture() const { return mBaseColorTextureView; }
    scene::TextureView*       GetBaseColorTextureViewPtr() { return &mBaseColorTextureView; }

    bool HasBaseColorTexture() const { return mBaseColorTextureView.HasTexture(); }

    void SetBaseColorFactor(const float4& value);

private:
    float4             mBaseColorFactor      = float4(1, 1, 1, 1);
    scene::TextureView mBaseColorTextureView = {};
};

// -------------------------------------------------------------------------------------------------

// Standard (PBR) Material
//
// Implementations should render a lit pixel using a PBR method that makes
// use of the provided fields and textures of this class.
//
// Corresponds to GLTF's metallic roughness material:
//   https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-material-pbrmetallicroughness
//
class StandardMaterial
    : public scene::Material
{
public:
    StandardMaterial() {}
    virtual ~StandardMaterial() {}

    virtual std::string GetIdentString() const override { return PPX_MATERIAL_IDENT_STANDARD; }

    virtual scene::VertexAttributeFlags GetRequiredVertexAttributes() const override;

    const float4& GetBaseColorFactor() const { return mBaseColorFactor; }
    float         GetMetallicFactor() const { return mMetallicFactor; }
    float         GetRoughnessFactor() const { return mRoughnessFactor; }
    float         GetOcclusionStrength() const { return mOcclusionStrength; }
    const float3& GetEmissiveFactor() const { return mEmissiveFactor; }
    float         GetEmissiveStrength() const { return mEmissiveStrength; }

    const scene::TextureView& GetBaseColorTextureView() const { return mBaseColorTextureView; }
    const scene::TextureView& GetMetallicRoughnessTextureView() const { return mMetallicRoughnessTextureView; }
    const scene::TextureView& GetNormalTextureView() const { return mNormalTextureView; }
    const scene::TextureView& GetOcclusionTextureView() const { return mOcclusionTextureView; }
    const scene::TextureView& GetEmissiveTextureView() const { return mEmissiveTextureView; }

    scene::TextureView* GetBaseColorTextureViewPtr() { return &mBaseColorTextureView; }
    scene::TextureView* GetMetallicRoughnessTextureViewPtr() { return &mMetallicRoughnessTextureView; }
    scene::TextureView* GetNormalTextureViewPtr() { return &mNormalTextureView; }
    scene::TextureView* GetOcclusionTextureViewPtr() { return &mOcclusionTextureView; }
    scene::TextureView* GetEmissiveTextureViewPtr() { return &mEmissiveTextureView; }

    bool HasBaseColorTexture() const { return mBaseColorTextureView.HasTexture(); }
    bool HasMetallicRoughnessTexture() const { return mMetallicRoughnessTextureView.HasTexture(); }
    bool HasNormalTexture() const { return mNormalTextureView.HasTexture(); }
    bool HasOcclusionTexture() const { return mOcclusionTextureView.HasTexture(); }
    bool HasEmissiveTexture() const { return mEmissiveTextureView.HasTexture(); }

    void SetBaseColorFactor(const float4& value);
    void SetMetallicFactor(float value);
    void SetRoughnessFactor(float value);
    void SetOcclusionStrength(float value);
    void SetEmissiveFactor(const float3& value);
    void SetEmissiveStrength(float value);

private:
    float4             mBaseColorFactor              = float4(1, 1, 1, 1);
    float              mMetallicFactor               = 1;
    float              mRoughnessFactor              = 1;
    float              mOcclusionStrength            = 1;
    float3             mEmissiveFactor               = float3(0, 0, 0);
    float              mEmissiveStrength             = 0;
    scene::TextureView mBaseColorTextureView         = {};
    scene::TextureView mMetallicRoughnessTextureView = {};
    scene::TextureView mNormalTextureView            = {};
    scene::TextureView mOcclusionTextureView         = {};
    scene::TextureView mEmissiveTextureView          = {};
};

// -------------------------------------------------------------------------------------------------

// Material Factory
//
// Customizable factory that provides implementations of materials. An application
// can inherit this class to provide implementations of materials as it sees fit.
//
// Materials must be uniquely identifiable by their Material Ident string.
//
// Materials that do not take any parameters, such as the default ErrorMaterial
// class, can the same shared copy across all of its instances.
//
class MaterialFactory
{
public:
    MaterialFactory();
    virtual ~MaterialFactory();

    virtual scene::VertexAttributeFlags GetRequiredVertexAttributes(const std::string& materialIdent) const;

    virtual scene::MaterialRef CreateMaterial(
        const std::string& materialIdent) const;

private:
    mutable scene::MaterialRef mErrorMaterial;
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_material_h
