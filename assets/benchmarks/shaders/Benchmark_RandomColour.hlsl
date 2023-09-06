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

struct RandomParams
{
    uint32_t Seed;
};

#if defined(__spirv__)
[[vk::push_constant]]
#endif
ConstantBuffer<RandomParams> Random : register(b0);

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput vsmain(float4 Position : POSITION)
{
	VSOutput result;
	result.position = Position;
	return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    float rnd = abs(cos(float(Random.Seed) / 10.0f));
    return float4(0.0f, 0.0f, rnd, 1.0f);
}