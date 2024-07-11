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

//
// https://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
//

[[vk::combinedImageSampler]]
Texture2D    Tex0     : register(t0);
[[vk::combinedImageSampler]]
SamplerState Sampler0 : register(s0);

struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(uint id : SV_VertexID)
{
	VSOutput result;

    // Clip space position
	result.Position.x = (float)(id / 2) * 4.0 - 1.0;
    result.Position.y = (float)(id % 2) * 4.0 - 1.0;
    result.Position.z = 0.0;
    result.Position.w = 1.0;

    // Texture coordinates
	result.TexCoord.x = (float)(id / 2) * 2.0;
    result.TexCoord.y = 1.0 - (float)(id % 2) * 2.0;

	return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
	return Tex0.Sample(Sampler0, input.TexCoord);
}