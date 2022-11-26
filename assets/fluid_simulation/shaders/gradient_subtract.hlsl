// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord coord = BaseVS(tid, Params.normalizationScale, Params.texelSize);

    float L = UPressure.SampleLevel(Sampler, coord.vL, 0).x;
    float R = UPressure.SampleLevel(Sampler, coord.vR, 0).x;
    float T = UPressure.SampleLevel(Sampler, coord.vT, 0).x;
    float B = UPressure.SampleLevel(Sampler, coord.vB, 0).x;

    float2 velocity = UVelocity.SampleLevel(Sampler, coord.vUv, 0).xy;
    velocity.xy -= float2(R - L, T - B);

    Output[coord.xy] = float4(velocity, 0.0, 1.0);
}
