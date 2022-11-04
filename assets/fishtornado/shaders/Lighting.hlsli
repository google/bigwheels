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

#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#define CAUSTICS_IMAGE_COUNT 32

float Lambert(float3 N, float3 L)
{
    float diffuse = saturate(dot(N, L));
    return diffuse;
}

float Phong(float3 N, float3 L, float3 V, float hardness)
{
    float3 R        = reflect(-L, N);
    float  theta    = max(dot(R, V), 0);
    float  specular = pow(theta, hardness);
    return specular;
}

float BlinnPhong(float3 N, float3 L, float3 V, float hardness)
{
    float3 H        = normalize(L + V);
    float  theta    = saturate(dot(N, H));
    float  specular = pow(theta, hardness);
    return specular;
}

float3 CalculateCaustics(float time, float2 texCoord, Texture2DArray tex, SamplerState sam)
{
    float2 tc        = texCoord;
    float  tcz       = (float)((uint)time % CAUSTICS_IMAGE_COUNT);
    float  t         = 0.5 * time;
    float3 caustics1 = tex.Sample(sam, float3(tc * 0.035 + t *  0.005, tcz)).rgb;
    float3 caustics2 = tex.Sample(sam, float3(tc * 0.025 + t * -0.010, tcz)).rgb;
    float3 caustics3 = tex.Sample(sam, float3(tc * 0.010 + t * -0.002, tcz)).rgb;
    float3 caustics  = (caustics1 * caustics2 * 1.0 + caustics3 * 0.8);
    return caustics;
}

#define PCF_SIZE 4

float ShadowPCF(float2 uv, float lightDepth, float2 texDim, Texture2D tex, SamplerComparisonState sam)
{
    float2 invDim = 1.0 / texDim;
    
    float sum = 0.0;
    for (uint y = 0; y < PCF_SIZE; ++y) {
        for (uint x = 0; x < PCF_SIZE; ++x) {
            float2 offset = (float2(x, y) - (float2(PCF_SIZE, PCF_SIZE) / 2.0f)) * invDim;
            sum += tex.SampleCmpLevelZero(sam, uv + offset, lightDepth).r;  
        }    
    }
      
    sum = sum / (PCF_SIZE * PCF_SIZE);
    return sum;
}


float CalculateShadow(float4 positionLS, float2 texDim, Texture2D tex, SamplerComparisonState sam, bool usePCF)
{
    // Lower values may introduce artifacts
    const float bias = 0.0015;

    // Complete projection into NDC
    positionLS.xyz = positionLS.xyz / positionLS.w;
    
    // Readjust to [0, 1] for texture sampling
    positionLS.x =  positionLS.x / 2.0 + 0.5;
    positionLS.y = -positionLS.y / 2.0 + 0.5;
    
    // Calculate depth in light space
    float depth = positionLS.z - bias;

    // Assume 
    float shadowFactor = 1.0;

    bool isInBoundsX = (positionLS.x >= 0) && (positionLS.x < 1);
    bool isInBoundsY = (positionLS.y >= 0) && (positionLS.y < 1);
    bool isInBoundsZ = (positionLS.z >= 0) && (positionLS.z < 1);
    if (isInBoundsX && isInBoundsY && isInBoundsZ) {
        shadowFactor = tex.SampleCmpLevelZero(sam, positionLS.xy, depth);
        if (usePCF) {
            shadowFactor = ShadowPCF(positionLS.xy, depth, texDim, tex, sam);
        }
    }
    
    return shadowFactor;
}

#endif // LIGHTING_HLSLI