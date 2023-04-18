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
#include "FullscreenVS.hlsli"

RWTexture2D<uint>         LinkedListHeadTexture : register(CUSTOM_UAV_0_REGISTER);
RWStructuredBuffer<uint4> FragmentBuffer        : register(CUSTOM_UAV_1_REGISTER);
RWStructuredBuffer<uint>  AtomicCounter         : register(CUSTOM_UAV_2_REGISTER);

float4 psmain(VSOutput input) : SV_TARGET
{
    // Reset the atomic counter for the next frame
    AtomicCounter[0] = 0;

    // Get the first fragment index in the linked list
    uint fragmentIndex = LinkedListHeadTexture[input.position.xy];
    LinkedListHeadTexture[input.position.xy] = BUFFER_LINKED_LIST_INVALID_INDEX; // Reset the list head for the next frame

    // Copy the fragments into local memory for sorting
    uint2 sortedFragments[BUFFER_LINKED_LIST_MAX_SIZE];
    uint fragmentCount = 0U;
    while(fragmentIndex != BUFFER_LINKED_LIST_INVALID_INDEX && fragmentCount < BUFFER_LINKED_LIST_MAX_SIZE)
    {
        const uint4 fragment = FragmentBuffer[fragmentIndex];
        fragmentIndex = fragment.z;
        sortedFragments[fragmentCount] = fragment.xy;
        ++fragmentCount;
    }

    if(fragmentCount <= 0)
    {
        return (float4)0.0f;
    }

    // Sort the fragments by depth (back to front)
    {
        for(int i = 0; i < fragmentCount - 1; ++i)
        {
            for(int j = i + 1; j < fragmentCount; ++j)
            {
                if(sortedFragments[j].y > sortedFragments[i].y)
                {
                    const uint2 tmp  = sortedFragments[i];
                    sortedFragments[i] = sortedFragments[j];
                    sortedFragments[j] = tmp;
                }
            }
        }
    }

    // Merge the fragments to get the final color
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for(int i = 0; i < fragmentCount; ++i)
    {
        MergeColor(color, UnpackColor(sortedFragments[i].x));
    }
    color.a = 1.0f - color.a;
    return color;
}
