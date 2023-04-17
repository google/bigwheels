// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord  coord = BaseVS(tid, Params.normalizationScale, Params.texelSize);
    float3 c     = UTexture.SampleLevel(ClampSampler, coord.vUv, 0).rgb;
    float  br    = max(c.r, max(c.g, c.b));
    float  rq    = clamp(br - Params.curve.x, 0.0, Params.curve.y);
    rq           = Params.curve.z * rq * rq;
    c *= max(rq, br - Params.threshold) / max(br, 0.0001);
    Output[coord.xy] = float4(c, 0.0);
}
