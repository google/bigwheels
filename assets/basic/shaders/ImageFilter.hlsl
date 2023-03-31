// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

struct ParamsData
{
    float2 texel_size;
    int    filter;
};

ConstantBuffer<ParamsData> Param : register(b1);
RWTexture2D<float4> Output : register(u0);
SamplerState        nearestSampler : register(s2);
Texture2D<float4>   srcTex : register(t3);

float3 blurFilter(float2 coordCenter, float2 d);
float3 sharpenFilter(float2 coordCenter, float2 d);
float3 sobelFilter(float2 coordCenter, float2 d);
float3 noFilter(float2 coordCenter);
float3 desaturate(float2 coordCenter);

[numthreads(32, 32, 1)] void csmain(uint3 tid
                                  : SV_DispatchThreadID) {
    const float2 coordCenter = Param.texel_size * (tid.xy + 0.5);
    float2       d           = Param.texel_size;

    float3 pixel;
    switch (Param.filter) {
        case 1:
            pixel = blurFilter(coordCenter, d);
            break;

        case 2:
            pixel = sharpenFilter(coordCenter, d);
            break;

        case 3:
            pixel = desaturate(coordCenter);
            break;

        case 4:
            pixel = sobelFilter(coordCenter, d);
            break;

        default:
            pixel = noFilter(coordCenter);
            break;
    }

    Output[tid.xy] = float4(pixel, 1);
}

float3 blurFilter(float2 coordCenter, float2 d)
{
    // clang-format off
    const float2 samplingDeltas[3][3] = {
        {{-d.x, -d.y}, {0.0, -d.y}, {d.x, -d.y}},
        {{-d.x, 0.0}, {0.0, 0.0}, {d.x, 0.0}},
        {{-d.x, d.y}, {0.0, d.y}, {d.x, d.y}}};
    
    const float k = 1.0 / 16.0;

    const float filter[3][3] = {
        {1 * k, 2 * k, 1 * k},
        {2 * k, 4 * k, 2 * k},
        {1 * k, 2 * k, 1 * k}
    };
    // clang-format on

    float3 color = float3(0.0, 0.0, 0.0);
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            float2 sampleCoords = coordCenter + samplingDeltas[j][i];
            color += filter[j][i] * srcTex.SampleLevel(nearestSampler, sampleCoords, 0).rgb;
        }
    }

    return color;
}

float3 sharpenFilter(float2 coordCenter, float2 d)
{
    // clang-format off
    const float2 samplingDeltas[3][3] = {
        {{-d.x, -d.y}, {0.0, -d.y}, {d.x, -d.y}},
        {{-d.x, 0.0}, {0.0, 0.0}, {d.x, 0.0}},
        {{-d.x, d.y}, {0.0, d.y}, {d.x, d.y}}};
    
    const float k = 2.0;

    const float filter[3][3] = {
		{ -0.0625 * k, -0.0625 * k, -0.0625 * k},
		{ -0.0625 * k,     1.0 * k, -0.0625 * k},
		{ -0.0625 * k, -0.0625 * k, -0.0625 * k}
    };
    // clang-format on

    float3 color = float3(0.0, 0.0, 0.0);
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            float2 sampleCoords = coordCenter + samplingDeltas[j][i];
            color += filter[j][i] * srcTex.SampleLevel(nearestSampler, sampleCoords, 0).rgb;
        }
    }

    return color;
}

float3 sobelFilter(float2 coordCenter, float2 d)
{
    // clang-format off
    const float2 samplingDeltas[3][3] = {
        {{-d.x, -d.y}, {0.0, -d.y}, {d.x, -d.y}},
        {{-d.x, 0.0}, {0.0, 0.0}, {d.x, 0.0}},
        {{-d.x, d.y}, {0.0, d.y}, {d.x, d.y}}};
    
    const float horizontalFilter[3][3] = {
        {1, 0, -1},
        {2, 0, -2},
        {1, 0, -1}
    };

    const float verticalFilter[3][3] = {
        { 1,  2,  1},
        { 0,  0,  0},
        {-1, -2, -1}
    };
    // clang-format on

    float horizontal = 0.0;
    float vertical   = 0.0;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            float2 sampleCoords = coordCenter + samplingDeltas[j][i];
            float3 sourceColor  = srcTex.SampleLevel(nearestSampler, sampleCoords, 0).rgb;

            // Apply filter to luminance component.
            float lum = 0.3 * sourceColor.r + 0.59 * sourceColor.g + 0.11 * sourceColor.b;
            horizontal += horizontalFilter[j][i] * lum;
            vertical += verticalFilter[j][i] * lum;
        }
    }

    // Compute edge weight.
    float weight = sqrt(horizontal * horizontal + vertical * vertical);

    // Draw edges in red.
    float3 sourceColor = srcTex.SampleLevel(nearestSampler, coordCenter, 0).rgb;
    if (weight > 0.5)
        return lerp(sourceColor, float3(1, 0, 0), weight);
    else
        return sourceColor;
}

float3 desaturate(float2 coordCenter)
{
    float3 pixel     = srcTex.SampleLevel(nearestSampler, coordCenter, 0).rgb;
    float  grayLevel = pixel.r * 0.3 + pixel.g * 0.59 + pixel.b * 0.11;
    return float3(grayLevel, grayLevel, grayLevel);
}

float3 noFilter(float2 coordCenter)
{
    return srcTex.SampleLevel(nearestSampler, coordCenter, 0).rgb;
}
