// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

struct VSInput
{
    float2 aPosition : POSITION;
};

struct PSInput
{
    float2 vUv : VUV;
    float2 vL : VL;
    float2 vR : VR;
    float2 vT : VT;
    float2 vB : VB;
    float4 pos : POSITION;
};

cbuffer texelSize : register(b0)
{
    float2 texelSize;
};

PSInput vsmain(VSInput input)
{
    PSInput output;
    output.vUv = input.aPosition * 0.5 + 0.5;
    output.vL  = output.vUv - float2(texelSize.x, 0.0);
    output.vR  = output.vUv + float2(texelSize.x, 0.0);
    output.vT  = output.vUv + float2(0.0, texelSize.y);
    output.vB  = output.vUv - float2(0.0, texelSize.y);
    output.pos = float4(input.aPosition, 0.0, 1.0);
    return output;
}
