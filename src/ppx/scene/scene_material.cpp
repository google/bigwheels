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

#include "ppx/scene/scene_material.h"
#include "ppx/grfx/grfx_device.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// Image
// -------------------------------------------------------------------------------------------------
Image::Image(
    grfx::Image*            pImage,
    grfx::SampledImageView* pImageView)
    : mImage(pImage),
      mImageView(pImageView)
{
}

Image::~Image()
{
    if (mImageView) {
        auto pDevice = mImageView->GetDevice();
        pDevice->DestroySampledImageView(mImageView);
    }

    if (mImage) {
        auto pDevice = mImage->GetDevice();
        pDevice->DestroyImage(mImage);
    }
}

// -------------------------------------------------------------------------------------------------
// Sampler
// -------------------------------------------------------------------------------------------------
Sampler::Sampler(
    grfx::Sampler* pSampler)
    : mSampler(pSampler)
{
}

Sampler::~Sampler()
{
    if (mSampler) {
        auto pDevice = mSampler->GetDevice();
        pDevice->DestroySampler(mSampler);
    }
}

// -------------------------------------------------------------------------------------------------
// Texture
// -------------------------------------------------------------------------------------------------
Texture::Texture(
    const scene::ImageRef   image,
    const scene::SamplerRef sampler)
    : mImage(image),
      mSampler(sampler)
{
}

// -------------------------------------------------------------------------------------------------
// TextureView
// -------------------------------------------------------------------------------------------------
TextureView::TextureView()
{
}

TextureView::TextureView(
    const scene::TextureRef& texture,
    float2                   texCoordTranslate,
    float                    texCoordRotate,
    float2                   texCoordScale)
    : mTexture(texture),
      mTexCoordTranslate(texCoordTranslate),
      mTexCoordRotate(texCoordRotate),
      mTexCoordScale(texCoordScale)
{
    float2x2 T         = glm::translate(float3(mTexCoordTranslate, 0));
    float2x2 R         = glm::rotate(mTexCoordRotate, float3(0, 0, 1));
    float2x2 S         = glm::scale(float3(mTexCoordScale, 0));
    mTexCoordTransform = T * R * S;
}

// -------------------------------------------------------------------------------------------------
// ErrorMaterial
// -------------------------------------------------------------------------------------------------
scene::VertexAttributeFlags ErrorMaterial::GetRequiredVertexAttributes() const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
    return attrFlags;
}

// -------------------------------------------------------------------------------------------------
// DebugMaterial
// -------------------------------------------------------------------------------------------------
scene::VertexAttributeFlags DebugMaterial::GetRequiredVertexAttributes() const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
    attrFlags.bits.texCoords              = true;
    attrFlags.bits.normals                = true;
    attrFlags.bits.tangents               = true;
    attrFlags.bits.colors                 = true;
    return attrFlags;
}

// -------------------------------------------------------------------------------------------------
// UnlitMaterial
// -------------------------------------------------------------------------------------------------
scene::VertexAttributeFlags UnlitMaterial::GetRequiredVertexAttributes() const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
    attrFlags.bits.texCoords              = true;
    return attrFlags;
}

bool UnlitMaterial::HasTextures() const
{
    bool hasBaseColorTex = HasBaseColorTexture();
    return hasBaseColorTex;
}

void UnlitMaterial::SetBaseColorFactor(const float4& value)
{
    mBaseColorFactor = value;
}

// -------------------------------------------------------------------------------------------------
// StandardMaterial
// -------------------------------------------------------------------------------------------------
scene::VertexAttributeFlags StandardMaterial::GetRequiredVertexAttributes() const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
    attrFlags.bits.texCoords              = true;
    attrFlags.bits.normals                = true;
    attrFlags.bits.tangents               = true;
    attrFlags.bits.colors                 = true;
    return attrFlags;
}

bool StandardMaterial::HasTextures() const
{
    bool hasBaseColorTex         = HasBaseColorTexture();
    bool hasMetallicRoughnessTex = HasMetallicRoughnessTexture();
    bool hasNormalTex            = HasNormalTexture();
    bool hasOcclusionTex         = HasOcclusionTexture();
    bool hasEmissiveTex          = HasEmissiveTexture();
    bool hasTextures             = hasBaseColorTex || hasMetallicRoughnessTex || hasNormalTex || hasOcclusionTex || hasNormalTex;
    return hasTextures;
}

void StandardMaterial::SetBaseColorFactor(const float4& value)
{
    mBaseColorFactor = value;
}

void StandardMaterial::SetMetallicFactor(float value)
{
    mMetallicFactor = value;
}

void StandardMaterial::SetRoughnessFactor(float value)
{
    mRoughnessFactor = value;
}

void StandardMaterial::SetOcclusionStrength(float value)
{
    mOcclusionStrength = value;
}

void StandardMaterial::SetEmissiveFactor(const float3& value)
{
    mEmissiveFactor = value;
}

void StandardMaterial::SetEmissiveStrength(float value)
{
    mEmissiveStrength = value;
}

// -------------------------------------------------------------------------------------------------
// MaterialFactory
// -------------------------------------------------------------------------------------------------
MaterialFactory::MaterialFactory()
{
}

MaterialFactory::~MaterialFactory()
{
}

scene::VertexAttributeFlags MaterialFactory::GetRequiredVertexAttributes(const std::string& materialIdent) const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();

    // Error material only uses position and no other vertex attribues.
    // No need to put in a specific if statement for it.
    if (materialIdent == PPX_MATERIAL_IDENT_UNLIT) {
        attrFlags = UnlitMaterial().GetRequiredVertexAttributes();
    }
    else if (materialIdent == PPX_MATERIAL_IDENT_STANDARD) {
        attrFlags = StandardMaterial().GetRequiredVertexAttributes();
    }

    return attrFlags;
}

scene::Material* MaterialFactory::CreateMaterial(
    const std::string& materialIdent) const
{
    scene::Material* pMaterial = nullptr;

    if (materialIdent == PPX_MATERIAL_IDENT_UNLIT) {
        pMaterial = new scene::UnlitMaterial();
    }
    else if (materialIdent == PPX_MATERIAL_IDENT_STANDARD) {
        pMaterial = new scene::StandardMaterial();
    }
    else {
        pMaterial = new scene::ErrorMaterial();
    }

    return pMaterial;
}

} // namespace scene
} // namespace ppx
