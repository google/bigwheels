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

#define VTX_POSITION 0
#define VTX_TEXCOORD 1
#define VTX_NORMAL   2
#define VTX_TANGENT  3

float4 psmain(StandardVertexOutput input) : SV_TARGET
{
    // Default output is purple
    float3 color = float3(1, 0, 1);

    if (Draw.dbgVtxAttrIndex == VTX_POSITION) {
        color = input.PositionWS.xyz;
    }
    else if(Draw.dbgVtxAttrIndex == VTX_TEXCOORD) {
        color = float3(input.TexCoord, 0);
    }
    else if(Draw.dbgVtxAttrIndex == VTX_NORMAL) {
        color = mul(Instances[Draw.instanceIndex].modelMatrix, float4(input.Normal, 0)).xyz;
    }
    else if(Draw.dbgVtxAttrIndex == VTX_TANGENT) {
        color = mul(Instances[Draw.instanceIndex].modelMatrix, float4(input.Tangent.xyz, 0)).xyz;
    }

    return float4(color, 1);
}
