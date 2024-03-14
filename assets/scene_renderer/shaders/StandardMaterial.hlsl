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

#define ENABLE_VTX_ATTR_TEXCOORD
#define ENABLE_VTX_ATTR_NORMAL
#define ENABLE_VTX_ATTR_TANGENT
#include "MaterialInterface.hlsli"

#include "ppx/PBR.hlsli"

// -------------------------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------------------------
float4 psmain(StandardVertexOutput input) : SV_TARGET
{
    InstanceParams instance = Instances[Draw.instanceIndex];

    MaterialParams material = Materials[Draw.materialIndex];

    MaterialTextureParams baseColorTex   = material.baseColorTex;
    MaterialTextureParams metalRoughTex  = material.metallicRoughnessTex;
    MaterialTextureParams normalTex      = material.normalTex;
    MaterialTextureParams occlusionTex   = material.occlusionTex;
    MaterialTextureParams emssiveTex     = material.emssiveTex;

    // Apply texture coordinate transform
    float2 uv = mul(material.baseColorTex.texCoordTransform, input.TexCoord);

    // Base color
    float4 baseColor = material.baseColorFactor;
    if (baseColorTex.textureIndex != INVALID_INDEX) {
        Texture2D    tex = MaterialTextures[baseColorTex.textureIndex];
        SamplerState samp = MaterialSamplers[baseColorTex.samplerIndex];
        float4       color = tex.Sample(samp, uv);

        baseColor =  baseColor * float4(RemoveGamma(color.rgb, 2.2), color.a);
    }

    // Metal/roughness
    float metallic  = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (metalRoughTex.textureIndex != INVALID_INDEX) {
        Texture2D    tex = MaterialTextures[metalRoughTex.textureIndex];
        SamplerState samp = MaterialSamplers[metalRoughTex.samplerIndex];                
        float4       value = tex.Sample(samp, uv);

        metallic  = metallic * value.b;
        roughness = roughness * value.g;
    }

    // Normal (N)
    float3 N = normalize(mul(instance.modelMatrix, float4(input.Normal, 0)).xyz);
    if (normalTex.textureIndex != INVALID_INDEX) {
        Texture2D    tex = MaterialTextures[normalTex.textureIndex];
        SamplerState samp = MaterialSamplers[normalTex.samplerIndex];
    
        // Read normal map value in tangent space
        float3 vNt = normalize((tex.Sample(samp, uv).rgb * 2.0) - 1.0);
    
        // Apply TBN transform
        float3 vN = N;
        float3 vT = mul(instance.modelMatrix, float4(input.Tangent.xyz, 0)).xyz;
        float3 vB = cross(vN, vT) * input.Tangent.w;
        N = normalize(vNt.x * vT + vNt.y * vB + vNt.z * vN);
    }

    // Remap
    float3 diffuseColor = CalculateDiffuseColor(baseColor.rgb, metallic);
    float dielectricness = CalculateDielectricness(metallic);    
    roughness = CalculateRoughness(roughness);

    // Calculate F0
    float specularReflectance = 0.5;
    float3 F0 = CalculateF0(baseColor.rgb, metallic, specularReflectance);

    // Position, View
    float3 P = input.PositionWS.xyz;
    float3 V = normalize(Camera.eyePosition - P);

    // Calculate reflection vector and cosine angle bewteen N and V
    float3 R = reflect(-V, N);
    float  NoV = saturate(dot(N, V));    

    // Calculate indirect lighting
    float3 indirectLighting = Light_GGX_Indirect(
        Draw.iblLevelCount,
        IBLEnvironmentMaps[0],
        IBLIrradianceMaps[0],
        IBLEnvironmentSampler,
        BRDFLUTTexture,
        BRDFLUTSampler,
        N,
        V,
        R,
        F0,
        diffuseColor,
        roughness,
        metallic,
        dielectricness,
        specularReflectance,
        NoV);

    float3 color = indirectLighting;
    return float4(ApplyGamma(color, 2.2), 1.0);
}
