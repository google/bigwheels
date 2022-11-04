// Copyright 2022 Google LLC
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

Texture2D    Tex0 : register(t0);
SamplerState Sampler0  : register(s1);

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
    VSOutput result;
    result.Position = Position;
    result.TexCoord = TexCoord;
    return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    return Tex0.Sample(Sampler0, input.TexCoord);
}
