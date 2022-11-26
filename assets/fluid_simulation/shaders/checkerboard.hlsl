// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

#define SCALE 25.0

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord  coord = BaseVS(tid, Params.normalizationScale, Params.texelSize);
    float2 vUv    = floor(coord.vUv * SCALE * float2(Params.aspectRatio, 1.0));
    float  v     = fmod(vUv.x + vUv.y, 2.0);
    v            = v * 0.1 + 0.8;

    Output[coord.xy] = float4(float3(v, v, v), 1.0);
}
