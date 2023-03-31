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

//
// Keep things easy for now and use 16-byte aligned types
//
struct SceneData
{
    float4x4 ModelMatrix;  // Transforms object space to world space
    float4x4 NormalMatrix; // Transforms object space to normal space
    float4   Ambient;      // Object's ambient intensity
    
    float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
    
    float4   LightPosition;             // Light's position
    float4x4 LightViewProjectionMatrix; // Light's view projection matrix
    
    uint4    UsePCF; // Enable/disable PCF
};

ConstantBuffer<SceneData> Scene : register(b0);

Texture2D                 ShadowDepthTexture : register(t1);
SamplerComparisonState    ShadowDepthSampler : register(s2);

struct VSOutput {
    float4 PositionWS : POSITION;
	float4 Position   : SV_POSITION;
	float3 Color      : COLOR;
    float3 Normal     : NORMAL;
    float4 PositionLS : POSITIONLS;
};

VSOutput vsmain(
    float4 Position : POSITION, 
    float3 Color    : COLOR, 
    float3 Normal   : NORMAL)
{
	VSOutput result;
    
    // Tranform input position into world space
    result.PositionWS = mul(Scene.ModelMatrix, Position);
    
    // Transform world space position into camera's view 
	result.Position = mul(Scene.CameraViewProjectionMatrix, result.PositionWS);
    
    // Color and normal
	result.Color  = Color;
    result.Normal = mul(Scene.NormalMatrix, float4(Normal, 0)).xyz;
    
    // Transform world space psoition into light's view
    result.PositionLS = mul(Scene.LightViewProjectionMatrix, result.PositionWS);
    
	return result;
}

#define PCF_SIZE 16

float ShadowPCF(float2 uv, float lightDepth)
{
    float2 dim = (float2)0;
    ShadowDepthTexture.GetDimensions(dim.x, dim.y);
    float2 invDim = 1.0 / dim;
    
    float sum = 0.0;
    for (uint y = 0; y < PCF_SIZE; ++y) {
        for (uint x = 0; x < PCF_SIZE; ++x) {
            float2 offset = (float2(x, y) - (float2(PCF_SIZE, PCF_SIZE) / 2.0f)) * invDim;
            sum += ShadowDepthTexture.SampleCmpLevelZero(ShadowDepthSampler, uv + offset, lightDepth).r;  
        }    
    }
      
    sum = sum / (PCF_SIZE * PCF_SIZE);
    return sum;
}

float4 psmain(VSOutput input) : SV_TARGET
{
    // Lower values may introduce artifacts
    const float bias = 0.0015;

    // Position in light space
    float4 positionLS = input.PositionLS;

    // Complete projection into NDC
    positionLS.xyz = positionLS.xyz / positionLS.w;
    
    // Readjust to [0, 1] for texture sampling
    positionLS.x =  positionLS.x / 2.0 + 0.5;
    positionLS.y = -positionLS.y / 2.0 + 0.5;
    
    // Calculate depth in light space
    float depth = positionLS.z - bias;

    // Assume 
    float shadowFactor = 1;

    bool isInBoundsX = (positionLS.x >= 0) && (positionLS.x < 1);
    bool isInBoundsY = (positionLS.y >= 0) && (positionLS.y < 1);
    bool isInBoundsZ = (positionLS.z >= 0) && (positionLS.z < 1);
    if (isInBoundsX && isInBoundsY && isInBoundsZ) {
        shadowFactor = ShadowDepthTexture.SampleCmpLevelZero(ShadowDepthSampler, positionLS.xy, depth);
        if (Scene.UsePCF.x) {
            shadowFactor = ShadowPCF(positionLS.xy, depth);
        }
    }

    // Calculate diffuse lighting
    float3 L       = normalize(Scene.LightPosition.xyz - input.PositionWS.xyz);
    float3 N       = input.Normal;    
    float  diffuse = saturate(dot(N, L));
    
    // Final output color
    float  ambient = Scene.Ambient.x;   
    float3 Co       = (diffuse * shadowFactor + ambient)  * input.Color;
	return float4(Co, 1);
}