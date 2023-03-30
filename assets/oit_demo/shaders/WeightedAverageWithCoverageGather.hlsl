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

#define IS_SHADER
#include "Common.hlsli"
#include "TransparencyVS.hlsli"

struct PSOutput
{
    float4 color    : SV_TARGET0;
    float  coverage : SV_TARGET1;
};

PSOutput psmain(VSOutput input)
{
    PSOutput output = (PSOutput)0;
    output.color    = float4(input.color * g_Globals.meshOpacity, g_Globals.meshOpacity);
    output.coverage = (1.0f - g_Globals.meshOpacity);
    return output;
}
