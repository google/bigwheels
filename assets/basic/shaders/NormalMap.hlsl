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
    
    float4   LightPosition; // Light's position

    float4   EyePosition; // Eye (camera) position
};

ConstantBuffer<SceneData> Scene : register(b0);

Texture2D                 AlbedoTexture : register(t1);
Texture2D                 NormalMap     : register(t2);
SamplerState              Sampler       : register(s3);

struct VSOutput {
    float4 PositionWS : POSITION;
	float4 Position   : SV_POSITION;
	float3 Color      : COLOR;
    float3 Normal     : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float3 Tangent    : TANGENT;
    float3 Bitangent  : BITANGENT;
};

VSOutput vsmain(
    float4 Position  : POSITION, 
    float3 Color     : COLOR, 
    float3 Normal    : NORMAL,  
    float2 TexCoord  : TEXCOORD,
    float3 Tangent   : TANGENT, 
    float3 Bitangent : BITANGENT)
{
	VSOutput result;
    
    // Tranform input position into world space
    result.PositionWS = mul(Scene.ModelMatrix, Position);
    
    // Transform world space position into camera's view 
	result.Position = mul(Scene.CameraViewProjectionMatrix, result.PositionWS);
    
    // Color and normal
	result.Color  = Color;
    result.Normal = mul(Scene.ModelMatrix, float4(Normal, 0)).xyz;
    
    // Texture coordinates
    result.TexCoord = TexCoord;
    
    // Tangent and bitangent
    result.Tangent   = mul(Scene.ModelMatrix, float4(Tangent, 0)).xyz;
    result.Bitangent = mul(Scene.ModelMatrix, float4(Bitangent, 0)).xyz;
    
	return result;
}


float4 psmain(VSOutput input) : SV_TARGET
{
    float3 V = normalize(Scene.EyePosition.xyz - input.PositionWS.xyz);

    // Construct TBN matrix to transform from tangent space into world space
    float3   N = normalize(input.Normal);
    float3   T = normalize(input.Tangent);
    float3   B = normalize(input.Bitangent);
    float3x3 TBN = float3x3(T.x, B.x, N.x,
                            T.y, B.y, N.y,
                            T.z, B.z, N.z);    
    
    // Read albedo texture value
    float3 albedo = AlbedoTexture.Sample(Sampler, input.TexCoord).rgb;
    // Read normal map value in tangent space
    float3 normal = (NormalMap.Sample(Sampler, input.TexCoord).rgb * 2.0f) - 1.0;

    // Transform from tangent space to world space
    N = mul(TBN, normal);
    // Normalize the transformed normal
    N = normalize(N);
    
    // Light
    float3 L = normalize(Scene.LightPosition.xyz - input.PositionWS.xyz);
    float3 R = reflect(-L, N);

    float diffuse  = saturate(dot(N, L));           // Lambert
    float specular = pow(saturate(dot(R, V)), 8.0); // Phong
    float ambient  = Scene.Ambient.x;
    
    float3 Co = (diffuse + specular + ambient) * albedo;
    return float4(Co, 1);
}