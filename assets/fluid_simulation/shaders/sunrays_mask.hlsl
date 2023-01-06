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
    float4 c     = UTexture.SampleLevel(Sampler, coord.vUv, 0);
    float  br    = max(c.r, max(c.g, c.b));
    c.a          = 1.0 - min(max(br * 20.0, 0.0), 0.8);
    Output[coord.xy] = c;
}
