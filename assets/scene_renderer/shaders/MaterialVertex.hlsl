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

StandardVertexOutput vsmain(StandardVertexInput input)
{
    InstanceParams instance = Instances[Draw.instanceIndex];

    float4 PositionWS4 = mul(instance.modelMatrix, float4(input.PositionOS, 1));

    StandardVertexOutput output = (StandardVertexOutput)0;
    output.PositionWS = PositionWS4;
    output.PositionCS = mul(Camera.viewProjectionMatrix, PositionWS4);
    output.TexCoord   = input.TexCoord;
    output.Normal     = input.Normal;
    output.Tangent    = input.Tangent;
    return output;
}
