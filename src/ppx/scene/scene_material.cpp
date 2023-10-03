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
        mImageView.Reset();
    }

    if (mImage) {
        auto pDevice = mImage->GetDevice();
        pDevice->DestroyImage(mImage);
        mImage.Reset();
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
        mSampler.Reset();
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

Texture::~Texture()
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
}

TextureView::~TextureView()
{
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
// UnlitMaterial
// -------------------------------------------------------------------------------------------------
scene::VertexAttributeFlags UnlitMaterial::GetRequiredVertexAttributes() const
{
    scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
    attrFlags.bits.texCoords              = true;
    return attrFlags;
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

scene::MaterialRef MaterialFactory::CreateMaterial(
    const std::string& materialIdent) const
{
    scene::MaterialRef material;

    if (materialIdent == PPX_MATERIAL_IDENT_UNLIT) {
        material = scene::MakeRef(new scene::UnlitMaterial());
    }
    else if (materialIdent == PPX_MATERIAL_IDENT_STANDARD) {
        material = scene::MakeRef(new scene::StandardMaterial());
    }
    else {
        if (!mErrorMaterial) {
            mErrorMaterial = scene::MakeRef(new scene::ErrorMaterial());
        }
        material = mErrorMaterial;
    }

    return material;
}

} // namespace scene
} // namespace ppx
