// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord  coord = BlurVS(tid, Params.normalizationScale, Params.texelSize);
    float4 sum   = UTexture.SampleLevel(ClampSampler, coord.vUv, 0) * 0.29411764;
    sum += UTexture.SampleLevel(ClampSampler, coord.vL, 0) * 0.35294117;
    sum += UTexture.SampleLevel(ClampSampler, coord.vR, 0) * 0.35294117;
    Output[coord.xy] = sum;
}
