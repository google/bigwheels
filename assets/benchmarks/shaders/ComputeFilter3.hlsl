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

struct ParamsData
{
    float2 texel_size;
};

ConstantBuffer<ParamsData> Param : register(b1);
RWTexture2D<float4>        Output : register(u0);
SamplerState               nearestSampler : register(s2);
Texture2D<float4>          srcTex : register(t3);

[numthreads(1, 1, 1)] void csmain(uint3 tid
                                  : SV_DispatchThreadID) {

    const float2 coordCenter = Param.texel_size * (tid.xy + 0.5);
    
    // clang-format off
    float2 d = Param.texel_size;
    const float2 samplingDeltas[3][3] = {
        {{-d.x, -d.y}, {0.0, -d.y}, {d.x, -d.y}},
        {{-d.x,  0.0}, {0.0,  0.0}, {d.x,  0.0}},
        {{-d.x,  d.y}, {0.0,  d.y}, {d.x,  d.y}}
    };
    // clang-format on

    const float k = 1.0 / 16.0;

    // clang-format off
    const float filter[3][3] = {
        {1 * k, 2 * k, 1 * k},
        {2 * k, 4 * k, 2 * k},
        {1 * k, 2 * k, 1 * k}
    };
    // clang-format on

    float3 color = float3(0.0, 0.0, 0.0);
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            float2 sampleCoords = coordCenter + samplingDeltas[j][i];
            color += filter[j][i] * srcTex.SampleLevel(nearestSampler, sampleCoords, 0).rgb;
        }
    }

    Output[tid.xy] = float4(color, 1);
}
