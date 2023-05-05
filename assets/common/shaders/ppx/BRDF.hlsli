// Copyright 2023 Google LLC
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

#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include "Math.hlsli"

//
// The functions in this file assume that lighting is done in world space.
// While some of these functions may work in object or view space, it is
// not a gurantee.
//
// Common Lighting Variables
//   float3 P        - The position (or point) on the surface that is currently being shaded.
//   float3 N        - Normalized normal vector at position P. Points away from the surface.
//   float3 V        - Normalized view vector at position P. Points from surface position P
//                     towards the eye position: V = normalize(EyePosition - P).
//   float3 L        - Normalized light direction vector at position P. Points from the surface
//                     position P towards the light position: L = normalize(LightPosition - P).
//   float roughness - The roughness of the material at position P. While roughness ranges from
//                     [0, 1], the lower bound may be clamped to prevent divide by zero or
//                     other asymptotic behavior.
//   float metalness - The metalness (or metallic) of the material at position P. Metalness
//                     ranges from [0, 1].
//

// -------------------------------------------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------------------------------------------

float Distribution_GGX(float3 N, float3 H, float roughness)
{
    float NoH    = saturate(dot(N, H));
    float NoH2   = NoH * NoH;
    float alpha2 = max(roughness * roughness, EPSILON);
    float A      = NoH2 * (alpha2 - 1) + 1;
    return alpha2 / (PI * A * A);
}

float Geometry_SchlickBeckman(float NoV, float k)
{
    return NoV / (NoV * (1 - k) + k);
}

float Geometry_Smiths(float3 N, float3 V, float3 L,  float roughness)
{
    float k   = pow(roughness + 1, 2) / 8.0;
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float G1  = Geometry_SchlickBeckman(NoV, k);
    float G2  = Geometry_SchlickBeckman(NoL, k);
    return G1 * G2;
}

float3 Fresnel_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 r = (float3)(1 - roughness);
    return F0 + (max(r, F0) - F0) * pow(1 - cosTheta, 5);
}

float Lambert()
{
    return 1.0 / PI;
}

// -------------------------------------------------------------------------------------------------
// Direct Lighting BRDF Functions
// -------------------------------------------------------------------------------------------------

float3 BRDF_BlinnPhong(float3 N, float3 V, float3 L, float hardness)
{
    float3 H        = normalize(L + V);
    float  NoL      = saturate(dot(N, H));
    float  specular = pow(NoL, hardness);
    return (float3)specular;
}

float3 BRDF_Phong(float3 N, float3 V, float3 L, float hardness)
{
    float3 R        = reflect(-L, N);
    float  RoV      = saturate(dot(R, V));
    float  specular = pow(RoV, hardness);
    return (float3)specular;
}

float3 BRDF_Gouraud(float3 N, float3 L)
{
    float diffuse = saturate(dot(N, L));
    return (float3)diffuse;
}

float3 BRDF_GGX_Diffuse(float3 N, float3 V, float3 L, float roughness, float metalness, float3 F0)
{
	float3 H   = normalize(L + V);
	float  VoH = saturate(dot(V, H));
	float3 F   = Fresnel_SchlickRoughness(VoH, F0, roughness);
	float3 kD  = (1.0 - F) * (1.0 - metalness); // Fresnel refelectance for diffuse
	float3 diffuse = kD * Lambert();
	return diffuse;
}

float3 BRDF_GGX_Specular(float3 N, float3 V, float3 L, float roughness, float metalness, float3 F0)
{
	float  NoV      = saturate(dot(N, V));
	float  NoL      = saturate(dot(N, L));
	float3 H        = normalize(L + V);
	float  VoH      = saturate(dot(V, H));
	float  D        = Distribution_GGX(N, H, roughness);
	float3 F        = Fresnel_SchlickRoughness(VoH, F0, roughness);
	float  G        = Geometry_Smiths(N, V, L, roughness);
	float  Vis      = G / max(0.0001, (4.0 * NoV * NoL));
	float3 specular = (D * F * Vis);
	return specular;
}

#endif // BRDF_HLSLI
