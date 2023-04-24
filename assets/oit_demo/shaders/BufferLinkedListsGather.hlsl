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

Texture2D                 OpaqueDepthTexture    : register(CUSTOM_TEXTURE_0_REGISTER);
RWTexture2D<uint>         LinkedListHeadTexture : register(CUSTOM_UAV_0_REGISTER);
RWStructuredBuffer<uint4> FragmentBuffer        : register(CUSTOM_UAV_1_REGISTER);
RWStructuredBuffer<uint>  AtomicCounter         : register(CUSTOM_UAV_2_REGISTER);

void psmain(VSOutput input)
{
    // Test fragment against opaque depth
    {
        const float opaqueDepth = OpaqueDepthTexture.Load(int3(input.position.xy, 0)).r;
        clip(input.position.z < opaqueDepth ? 1.0f : -1.0f);
    }

    // Find the next fragment index
    uint nextFragmentIndex = 0;
    InterlockedAdd(AtomicCounter[0], 1U, nextFragmentIndex);

    // Ignore the fragment if the fragment buffer is full
    uint fragmentBufferMaxElementCount = 0;
    uint fragmentBufferStride          = 0;
    FragmentBuffer.GetDimensions(fragmentBufferMaxElementCount, fragmentBufferStride);
    const uint fragmentBufferElementCount = min((fragmentBufferMaxElementCount / BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE) * g_Globals.bufferListsFragmentBufferScale, fragmentBufferMaxElementCount);
    if(nextFragmentIndex >= fragmentBufferElementCount)
    {
        clip(-1.0f);
    }

    // Update the linked list head
    uint previousFragmentIndex = 0;
    InterlockedExchange(LinkedListHeadTexture[input.position.xy], nextFragmentIndex, previousFragmentIndex);

    // Add the fragment to the buffer
    FragmentBuffer[nextFragmentIndex] = uint4(PackColor(float4(input.color, g_Globals.meshOpacity)), asuint(input.position.z), previousFragmentIndex, 0U);
}
