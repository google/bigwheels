// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "config.hlsli"

#define SHADING 1
#define BLOOM   1
#define SUNRAYS 1

float3 linearToGamma(float3 color)
{
    color = max(color, float3(0, 0, 0));
    return max(1.055 * pow(color, float3(0.416666667, 0.416666667, 0.416666667)) - 0.055, float3(0, 0, 0));
}

[numthreads(1, 1, 1)] void csmain(uint2 tid
                                  : SV_DispatchThreadID) {
    Coord  coord = BaseVS(tid, Params.normalizationScale, Params.texelSize);
    float3 c     = UTexture.SampleLevel(Sampler, coord.vUv, 0).rgb;

#ifdef SHADING
    float3 lc = UTexture.SampleLevel(Sampler, coord.vL, 0).rgb;
    float3 rc = UTexture.SampleLevel(Sampler, coord.vR, 0).rgb;
    float3 tc = UTexture.SampleLevel(Sampler, coord.vT, 0).rgb;
    float3 bc = UTexture.SampleLevel(Sampler, coord.vB, 0).rgb;

    float dx = length(rc) - length(lc);
    float dy = length(tc) - length(bc);

    float3 n = normalize(float3(dx, dy, length(Params.texelSize)));
    float3 l = float3(0.0, 0.0, 1.0);

    float diffuse = clamp(dot(n, l) + 0.7, 0.7, 1.0);
    c *= diffuse;
#endif

#ifdef BLOOM
    float3 bloom = UBloom.SampleLevel(Sampler, coord.vUv, 0).rgb;
#endif

#ifdef SUNRAYS
    float sunrays = USunrays.SampleLevel(Sampler, coord.vUv, 0).r;
    c *= sunrays;
#ifdef BLOOM
    bloom *= sunrays;
#endif
#endif

#ifdef BLOOM
    float noise = UDithering.SampleLevel(Sampler, coord.vUv * Params.ditherScale, 0).r;
    noise       = noise * 2.0 - 1.0;
    bloom += noise / 255.0;
    bloom = linearToGamma(bloom);
    c += bloom;
#endif

    float a          = max(c.r, max(c.g, c.b));
    Output[coord.xy] = float4(c, a);
}
