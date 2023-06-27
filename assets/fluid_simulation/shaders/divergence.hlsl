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

    float L = UVelocity.SampleLevel(ClampSampler, coord.vL, 0).x;
    float R = UVelocity.SampleLevel(ClampSampler, coord.vR, 0).x;
    float T = UVelocity.SampleLevel(ClampSampler, coord.vT, 0).y;
    float B = UVelocity.SampleLevel(ClampSampler, coord.vB, 0).y;

    float2 C = UVelocity.SampleLevel(ClampSampler, coord.vUv, 0).xy;
    if (coord.vL.x < 0.0) {
        L = -C.x;
    }
    if (coord.vR.x > 1.0) {
        R = -C.x;
    }
    if (coord.vT.y > 1.0) {
        T = -C.y;
    }
    if (coord.vB.y < 0.0) {
        B = -C.y;
    }

    float div        = 0.5 * (R - L + T - B);
    Output[coord.xy] = float4(div, 0.0, 0.0, 1.0);
}
