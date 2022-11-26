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

    float L = UCurl.SampleLevel(Sampler, coord.vL, 0).x;
    float R = UCurl.SampleLevel(Sampler, coord.vR, 0).x;
    float T = UCurl.SampleLevel(Sampler, coord.vT, 0).x;
    float B = UCurl.SampleLevel(Sampler, coord.vB, 0).x;
    float C = UCurl.SampleLevel(Sampler, coord.vUv, 0).x;

    float2 force = 0.5 * float2(abs(T) - abs(B), abs(R) - abs(L));
    force /= length(force) + 0.0001;
    force *= Params.curl * C;
    force.y *= -1.0;

    float2 velocity = UVelocity.SampleLevel(Sampler, coord.vUv, 0).xy;
    velocity += force * Params.dt;
    velocity = min(max(velocity, -1000.0), 1000.0);

    Output[coord.xy] = float4(velocity, 0.0, 1.0);
}
