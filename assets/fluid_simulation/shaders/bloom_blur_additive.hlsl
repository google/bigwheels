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
    float4 sum   = 0.0;
    sum += UTexture.SampleLevel(Sampler, coord.vL, 0);
    sum += UTexture.SampleLevel(Sampler, coord.vR, 0);
    sum += UTexture.SampleLevel(Sampler, coord.vT, 0);
    sum += UTexture.SampleLevel(Sampler, coord.vB, 0);
    sum *= 0.25;
    Output[coord.xy] += sum;
}
