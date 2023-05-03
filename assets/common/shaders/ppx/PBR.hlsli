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

#ifndef PBR_HLSLI
#define PBR_HLSLI

#include "BRDF.hlsli"
#include "Math.hlsli"

float CalculateDielectricness(float metalness)
{
    float dielectricness = (1.0 - metalness);
    return dielectricness;
}

float3 CalculateDiffuseColor(float3 baseColor, float metalness)
{
    float  dielectricness = CalculateDielectricness(metalness);
    float3 diffuseColor = dielectricness * baseColor;
    return diffuseColor;
}

float CalculateRoughness(float perceptualRoughness)
{
    float roughness = perceptualRoughness * perceptualRoughness;
    return roughness;
}

float3 CalculateF0(float3 baseColor, float metalness, float specularReflectance)
{
    float  dielectricness = CalculateDielectricness(metalness);
    float3 F0 = (0.16 * specularReflectance * specularReflectance * dielectricness) + (baseColor * metalness);
    return F0;
}

float3 SampleIBLTexture(
    Texture2D    iblTexture,
    SamplerState iblSampler,
    float3       dir,
    float        lod)
{
    float2 uv = CartesianToSpherical(normalize(dir));
    uv.x = saturate(uv.x / (2.0 * PI));
    uv.y = saturate(uv.y / PI);
    float3 color = iblTexture.SampleLevel(iblSampler, uv, lod).rgb;
    return color;
}

float3 ApproxIndirectDFGPolynomial(float3 specularColor, float gloss, float NoV)
{
    float x = gloss;
    float y = NoV;

    float b1 = -0.1688;
    float b2 = 1.895;
    float b3 = 0.9903;
    float b4 = -4.853;
    float b5 = 8.404;
    float b6 = -5.069;
    float bias = saturate( min( b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y ) );

    float d0 = 0.6045;
    float d1 = 1.699;
    float d2 = -0.5228;
    float d3 = -3.603;
    float d4 = 1.404;
    float d5 = 0.1939;
    float d6 = 2.661;
    float delta = saturate( d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x );
    float scale = delta - bias;

    bias *= saturate( 50.0 * specularColor.y );
    return specularColor * scale + bias;
}

//
// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
float3 ApproxIndirectDFGKaris(float3 specularColor, float roughness, float NoV)
{
    const float4 c0 = {-1, -0.0275, -0.572, 0.022};
    const float4 c1 = {1, 0.0425, 1.04, -0.04};
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
    return specularColor * AB.x + AB.y;
}

float3 Light_GGX_Direct(
    float3 P,                   // Surface position
    float3 N,                   // Surface normal from vertex attribute or normal map
    float3 V,                   // Normalized view vector
    float3 R,                   // Reflection vector: R = reflect(-V, N)
    float3 L,                   // Normalized light direction
    float3 Lc,                  // Light color, hard code float3(1.0, 1.0, 1.0) if in doubt
    float  Ls,                  // Light intensity, hard code 1.0 if in doubt
    float3 F0,                  // F0 (aka Fresnel reflectance at 0 degrees)
    float3 diffuseColor,        // Material diffsue color (renamed from base color / albedo)
    float  roughness,           // Material roughness
    float  metalness,           // Material metalness
    float  dielectricness,      // Dielectricness remapped from metal: dielectricness = (1.0 - metalness)
    float  specularReflectance, // Specular reflectance of dielectric materials
    float  NoV)                 // Clamped dot product of N and V: saturate(dot(N, V))
{
    float NoL = saturate(dot(N, L));

    // Calculate radiance
    float3 radiance = Lc * Ls;

    // Diffuse BRDF
    float3 diffuseBRDF = BRDF_GGX_Diffuse(N, V, L, roughness, metalness, F0);

    // Specular BRDF
    float3 specular = BRDF_GGX_Specular(N, V, L, roughness, metalness, F0);
    float3 dielectricSpecular = dielectricness * specularReflectance * specular; // non-metal specular
    float3 metalSpecular = metalness * specular;
    float3 specularBRDF = dielectricSpecular + metalSpecular;

    // Lit color
    float3 color = ((diffuseColor * diffuseBRDF) + specularBRDF) * radiance * NoL;

    return color;
}

float3 Light_GGX_Indirect(
    float        envLevelCount,        // Number of mips in prefiltered environment texture
    Texture2D    envTexture,           // Prefiltered environment texture (mipmap)
    Texture2D    irrTexture,           // Preconvolved irradiance texture
    SamplerState samplerUWrapVClamped, // Sampler wrap on U and clamped on V
    Texture2D    brdfLutTexture,       // BRDF integration LUT texture
    SamplerState samplerClamped,       // Sampler clamped on both U and V
    float3       N,                    // Surface normal from vertex attribute or normal map
    float3       V,                    // Normalized light direction
    float3       R,                    // Reflection vector: R = reflect(-V, N)
    float3       F0,                   // F0 (aka Fresnel reflectance at 0 degrees)
    float3       diffuseColor,         // Material diffsue color (renamed from base color / albedo)
    float        roughness,            // Material roughness
    float        metalness,            // Material metalness
    float        dielectricness,       // Dielectricness remapped from metal: dielectricness = (1.0 - metalness)
    float        specularReflectance,  // Specular reflectance of dielectric materials
    float        NoV)                  // Clamped dot product of N and V: saturate(dot(N, V))
{
    // Fresnel
    float3 F = Fresnel_SchlickRoughness(NoV, F0, roughness);

    // Diffuse BRDF
    float3 diffuseBRDF = 0;
    {
        float3 kD         = (1.0 - F) * (1.0 - metalness);
        float3 irradiance = SampleIBLTexture(irrTexture, samplerUWrapVClamped, N, 0);
        float3 diffuse     = irradiance * diffuseColor * Lambert();
        diffuseBRDF = kD * diffuse;
    }

    // Specular BRDF
    float3 specularBRDF = 0;
    {
        float  lod              = envLevelCount * roughness;
        float3 prefilteredColor = SampleIBLTexture(envTexture, samplerUWrapVClamped, R, lod);
        prefilteredColor        = min(prefilteredColor, float3(10.0, 10.0, 10.0));

        float2 uv       = float2(saturate(roughness), saturate(NoV));
        float2 envBRDF  = brdfLutTexture.Sample(samplerClamped, uv).rg;
        float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

        float3 dielectricSpecular = dielectricness * specularReflectance * specular;
        float3 metalSpecular = metalness * specular;
        specularBRDF = dielectricSpecular + metalSpecular;
    }

    // Lit color
    float3 color = diffuseBRDF + specularBRDF;

    return color;
}

float3 Light_GGX_Indirect_ApproxPolynomial(
    float        envLevelCount,        // Number of mips in prefiltered environment texture
    Texture2D    envTexture,           // Prefiltered environment texture (mipmap)
    Texture2D    irrTexture,           // Preconvolved irradiance texture
    SamplerState samplerUWrapVClamped, // Sampler wrap on U and clamped on V
    float3       N,                    // Surface normal from vertex attribute or normal map
    float3       V,                    // Normalized light direction
    float3       R,                    // Reflection vector: R = reflect(-V, N)
    float3       F0,                   // F0 (aka Fresnel reflectance at 0 degrees)
    float3       diffuseColor,         // Material diffsue color (renamed from base color / albedo)
    float        roughness,            // Material roughness
    float        metalness,            // Material metalness
    float        dielectricness,       // Dielectricness remapped from metal: dielectricness = (1.0 - metalness)
    float        specularReflectance,  // Specular reflectance of dielectric materials
    float        NoV)                  // Clamped dot product of N and V: saturate(dot(N, V))
{
    // Fresnel
    float3 F = Fresnel_SchlickRoughness(NoV, F0, roughness);

    // Diffuse BRDF
    float3 diffuseBRDF = 0;
    {
        float3 kD         = (1.0 - F) * (1.0 - metalness);
        float3 irradiance = SampleIBLTexture(irrTexture, samplerUWrapVClamped, N, 0);
        float3 diffuse     = irradiance * diffuseColor * Lambert();
        diffuseBRDF = kD * diffuse;
    }

    // Specular BRDF
    float3 specularBRDF = 0;
    {
        float  lod              = envLevelCount * roughness;
        float3 prefilteredColor = SampleIBLTexture(envTexture, samplerUWrapVClamped, R, lod);
        prefilteredColor        = min(prefilteredColor, float3(10.0, 10.0, 10.0));

        float3 specular = F * ApproxIndirectDFGPolynomial(prefilteredColor, roughness, NoV);

        float3 dielectricSpecular = dielectricness * specularReflectance * specular;
        float3 metalSpecular = metalness * specular;
        specularBRDF = dielectricSpecular + metalSpecular;
    }

    // Lit color
    float3 color = diffuseBRDF + specularBRDF;

    return color;
}

float3 Light_GGX_Indirect_ApproxKaris(
    float        envLevelCount,        // Number of mips in prefiltered environment texture
    Texture2D    envTexture,           // Prefiltered environment texture (mipmap)
    Texture2D    irrTexture,           // Preconvolved irradiance texture
    SamplerState samplerUWrapVClamped, // Sampler wrap on U and clamped on V
    float3       N,                    // Surface normal from vertex attribute or normal map
    float3       V,                    // Normalized light direction
    float3       R,                    // Reflection vector: R = reflect(-V, N)
    float3       F0,                   // F0 (aka Fresnel reflectance at 0 degrees)
    float3       diffuseColor,         // Material diffsue color (renamed from base color / albedo)
    float        roughness,            // Material roughness
    float        metalness,            // Material metalness
    float        dielectricness,       // Dielectricness remapped from metal: dielectricness = (1.0 - metalness)
    float        specularReflectance,  // Specular reflectance of dielectric materials
    float        NoV)                  // Clamped dot product of N and V: saturate(dot(N, V))
{
    // Fresnel
    float3 F = Fresnel_SchlickRoughness(NoV, F0, roughness);

    // Diffuse BRDF
    float3 diffuseBRDF = 0;
    {
        float3 kD         = (1.0 - F) * (1.0 - metalness);
        float3 irradiance = SampleIBLTexture(irrTexture, samplerUWrapVClamped, N, 0);
        float3 diffuse     = irradiance * diffuseColor * Lambert();
        diffuseBRDF = kD * diffuse;
    }

    // Specular BRDF
    float3 specularBRDF = 0;
    {
        float  lod              = envLevelCount * roughness;
        float3 prefilteredColor = SampleIBLTexture(envTexture, samplerUWrapVClamped, R, lod);
        prefilteredColor        = min(prefilteredColor, float3(10.0, 10.0, 10.0));

        float3 specular = F * ApproxIndirectDFGKaris(prefilteredColor, roughness, NoV);

        float3 dielectricSpecular = dielectricness * specularReflectance * specular;
        float3 metalSpecular = metalness * specular;
        specularBRDF = dielectricSpecular + metalSpecular;
    }

    // Lit color
    float3 color = diffuseBRDF + specularBRDF;

    return color;
}

#endif // PBR_HLSLI
