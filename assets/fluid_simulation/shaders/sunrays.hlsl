// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

#define ITERATIONS 16

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord inputCoord = BaseVS(tid, Params.normalizationScale, Params.texelSize);

    float Density  = 0.3;
    float Decay    = 0.95;
    float Exposure = 0.7;

    float2 coord = inputCoord.vUv;
    float2 dir   = inputCoord.vUv - 0.5;

    dir *= 1.0 / float(ITERATIONS) * Density;
    float illuminationDecay = 1.0;

    float color = UTexture.SampleLevel(Sampler, inputCoord.vUv, 0).a;

    for (int i = 0; i < ITERATIONS; i++) {
        coord -= dir;
        float col = UTexture.SampleLevel(Sampler, coord, 0).a;
        color += col * illuminationDecay * Params.weight;
        illuminationDecay *= Decay;
    }

    Output[inputCoord.xy] = float4(color * Exposure, 0.0, 0.0, 1.0);
}
