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

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

VSOutput vsmain(uint VertexID : SV_VertexID)
{
    const float4 vertices[] =
    {
        float4(-1.0f, -1.0f, +0.0f, +1.0f),
        float4(+3.0f, -1.0f, +2.0f, +1.0f),
        float4(-1.0f, +3.0f, +0.0f, -1.0f),
    };

    VSOutput result;
    result.position = float4(vertices[VertexID].xy, 0.0f, 1.0f);
    result.uv       = float2(vertices[VertexID].zw);
    return result;
}
