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

// -------------------------------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------------------------------
float4 psmain(StandardVertexOutput input) : SV_TARGET
{
    MaterialParams material = Materials[Draw.materialIndex];
    
    MaterialTextureParams baseColorTex = material.baseColorTex;

    // Apply texture coordinate transforms
    float2 uv = mul(material.baseColorTex.texCoordTransform, input.TexCoord);

    // Calculate output color
    float4 color = material.baseColorFactor;
    if (baseColorTex.textureIndex != INVALID_INDEX) {
        Texture2D    tex = MaterialTextures[baseColorTex.textureIndex];
        SamplerState sam = MaterialSamplers[baseColorTex.samplerIndex];

        float4 value = tex.Sample(sam, uv);
        color = color * value;
    }

    return color;
}
