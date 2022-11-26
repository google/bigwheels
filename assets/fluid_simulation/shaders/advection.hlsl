// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

// Undefine to disable manual filtering.
#define MANUAL_FILTERING

float4 bilerp(Texture2D sam, float2 vUv, float2 tsize)
{
    float2 st = vUv / tsize - 0.5;

    float2 iuv = floor(st);
    float2 fuv = frac(st);

    float4 a = sam.SampleLevel(Sampler, (iuv + float2(0.5, 0.5)) * tsize, 0);
    float4 b = sam.SampleLevel(Sampler, (iuv + float2(1.5, 0.5)) * tsize, 0);
    float4 c = sam.SampleLevel(Sampler, (iuv + float2(0.5, 1.5)) * tsize, 0);
    float4 d = sam.SampleLevel(Sampler, (iuv + float2(1.5, 1.5)) * tsize, 0);

    return lerp(lerp(a, b, fuv.x), lerp(c, d, fuv.x), fuv.y);
}

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord inputCoord = BaseVS(tid, Params.normalizationScale, Params.texelSize);

#ifdef MANUAL_FILTERING
    float2 coord  = inputCoord.vUv - Params.dt * bilerp(UVelocity, inputCoord.vUv, Params.texelSize).xy * Params.texelSize;
    float4 result = bilerp(USource, coord, Params.dyeTexelSize);
#else
    float2 coord  = inputCoord.vUv - Params.dt * UVelocity.SampleLevel(Sampler, inputCoord.vUv, 0).xy * Params.texelSize;
    float4 result = USource.SampleLevel(Sampler, coord, 0);
#endif
    float decay = 1.0 + Params.dissipation * Params.dt;

    Output[inputCoord.xy] = result / decay;
}
