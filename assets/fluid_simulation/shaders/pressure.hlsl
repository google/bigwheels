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

    float L = UPressure.SampleLevel(ClampSampler, coord.vL, 0).x;
    float R = UPressure.SampleLevel(ClampSampler, coord.vR, 0).x;
    float T = UPressure.SampleLevel(ClampSampler, coord.vT, 0).x;
    float B = UPressure.SampleLevel(ClampSampler, coord.vB, 0).x;
    float C = UPressure.SampleLevel(ClampSampler, coord.vUv, 0).x;

    float divergence = UDivergence.SampleLevel(ClampSampler, coord.vUv, 0).x;
    float pressure   = (L + R + B + T - divergence) * 0.25;

    Output[coord.xy] = float4(pressure, 0.0, 0.0, 1.0);
}
