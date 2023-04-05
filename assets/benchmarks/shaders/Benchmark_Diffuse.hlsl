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

#include "Benchmark.hlsli"

float4 psmain(VSOutput input) : SV_TARGET
{
    const float3   nTS = normalize(input.normalTS);
    const float3   tTS = normalize(input.tangentTS);
    const float3   bTS = normalize(input.bitangentTS);
    const float3x3 TBN = float3x3(tTS.x, bTS.x, nTS.x,
                            tTS.y, bTS.y, nTS.y,
                            tTS.z, bTS.z, nTS.z);

    const float3 normal = NormalMap.Sample(NormalMapSampler, input.uv).rgb;
    const float3 N = normalize(mul(TBN, normal * 2.0 - 1.0));

    const float4 albedo = AlbedoTexture.Sample(AlbedoSampler, input.uv).rgba;
    if (albedo.a < 0.8f) {
      discard;
    }

    const float3 Li    = normalize(Scene.LightPosition.xyz - input.world_position.xyz);
    const float  cosLi = saturate(dot(N, Li));

    const float3 Co = cosLi * albedo.rgb;
    return float4(Co, 1);
}
