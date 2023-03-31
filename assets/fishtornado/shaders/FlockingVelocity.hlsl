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

#include "Config.hlsli"

ConstantBuffer<FlockingData> Flocking           : register(RENDER_FLOCKING_DATA_REGISTER, space0);              // Flocking params
Texture2D<float4>            InPositionTexture  : register(RENDER_PREVIOUS_POSITION_TEXTURE_REGISTER, space0);  // Previous position
Texture2D<float4>            InVelocityTexture  : register(RENDER_PREVIOUS_VELOCITY_TEXTURE_REGISTER, space0);  // Previous velocity
RWTexture2D<float4>          OutVelocityTexture : register(RENDER_OUTPUT_VELOCITY_TEXTURE_REGISTER, space0);    // Out position

// -------------------------------------------------------------------------------------------------

[numthreads(8, 8, 1)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
#if 0    
    OutVelocityTexture[tid.xy] = InVelocityTexture[tid.xy];;
#else     
    float4 vPos       = InPositionTexture[tid.xy];
    float3 myPos      = vPos.xyz;
    float  leadership = vPos.a;
    float  accMulti   = leadership * Flocking.timeDelta * 1.5; // Cinder version used 0.5 multiplier

    float4 vVel    = InVelocityTexture[tid.xy];
    float3 myVel   = vVel.xyz;
    float  myCrowd = vVel.a;

    float3 acc = (float3)0;

    float zoneRadSqrd = (Flocking.zoneRadius * Flocking.zoneRadius);

    int   myX     = (int)tid.x;
    int   myY     = (int)tid.y;
    float crowded = 5.0;

    float threshDelta0 = Flocking.maxThresh - Flocking.minThresh;
    float threshDelta1 = 1.0 - Flocking.maxThresh;

    // Apply the attraction, alignment, and repulsion forces
    // 
    // Tempted to use Texture2D::GetDimensions()?
    //   - Some micro benchmarking shows it's slow: 
    //       https://www.gamedev.net/forums/topic/605580-performance-comparison-hlsl-texturegetdimension/
    // 
    for (int y = 0; y < Flocking.resY; ++y) {
        for (int x = 0; x < Flocking.resX; ++x) {
            float  s        = abs(x - myX) + abs(y - myY);
            uint2  xy       = uint2(x, y);
            float3 pos      = InPositionTexture[xy].xyz;
            float3 vel      = InVelocityTexture[xy].xyz;
            float3 dir      = myPos - pos;
            float  distSqrd = dot(dir, dir);

            if ((x != myX) && (y != myY) && (distSqrd < zoneRadSqrd)) {
                float3 dirNorm  = normalize(dir);
                float  percent  = distSqrd / zoneRadSqrd;
                float  crowdPer = 1.0 - percent;

                if (percent < Flocking.minThresh) {
                    float F = (Flocking.minThresh / percent - 1.0) * accMulti;
                    acc += dirNorm * F;
                    crowded += crowdPer; //  * 2.0
                }
                else if (percent < Flocking.maxThresh) {
                    float threshDelta     = Flocking.maxThresh - Flocking.minThresh;
                    float adjustedPercent = (percent - Flocking.minThresh) / threshDelta;
                    float F               = (1.0 - (cos(adjustedPercent * 6.28318) * -0.5 + 0.5)) * accMulti;
                    acc += normalize(vel) * F;
                    crowded += crowdPer * 0.5;
                }
                else {
                    float threshDelta     = 1.0 - Flocking.maxThresh;
                    float adjustedPercent = (percent - Flocking.maxThresh) / threshDelta;
                    float F               = (1.0 - (cos(adjustedPercent * 6.28318) * -0.5 + 0.5)) * accMulti;
                    acc += -dirNorm * F;
                    crowded += crowdPer * 0.25;
                }
            }
        }
    }

    acc.y *= 0.960;
    acc.y -= 0.005;

    // Avoid predator
    float3 dirToPred   = myPos - Flocking.predPos;
    float  distToPred  = length(dirToPred);
    float  distPredPer = max(1.0 - (distToPred / 200.0), 0.0);
    crowded += distPredPer * 10.0;
    acc += (normalize(dirToPred) * float3(1.0, 0.25, 1.0)) * distPredPer * Flocking.timeDelta * 25.0;

    // Avoid camera
    float3 dirToCam   = myPos - Flocking.camPos;
    float  distToCam  = length(dirToCam);
    float  distCamPer = max(1.0 - (distToCam / 60.0), 0.0);
    crowded += distCamPer * 50.0;
    acc += (normalize(dirToCam) * float3(1.0, 0.25, 1.0)) * distCamPer * Flocking.timeDelta * 5.0;

    // Pull to center line
    float2 centerLine = float2(sin(Flocking.time * 0.0002 + myPos.y * 0.01), sin(Flocking.time * 0.0002 - 1.5 + myPos.y * 0.01)) * 80.0;
    myVel.xz -= (myPos.xz - centerLine) * Flocking.timeDelta * 0.0025; //0.005;

    // Pull to center point
    myVel -= normalize(myPos - float3(0.0, 150.0, 0.0)) * Flocking.timeDelta * 0.1; //0.2;

    myVel += acc * Flocking.timeDelta;
    myCrowd -= (myCrowd - crowded) * (Flocking.timeDelta);

    float3 tempNewPos  = myPos + myVel * Flocking.timeDelta;
    float  tempNewDist = length(tempNewPos);
    float3 tempNewDir  = normalize(tempNewPos);

    // Set speed limit
    float  newMaxSpeed = clamp(Flocking.maxSpeed + myCrowd * 0.001, Flocking.minSpeed, 5.0);
    float  velLength   = length(myVel);
    float3 velNorm     = normalize(myVel);

    if (velLength < Flocking.minSpeed) {
        myVel = velNorm * Flocking.minSpeed;
    }
    else if (velLength > newMaxSpeed) {
        myVel = velNorm * newMaxSpeed;
    }

    float3 nextPos = myPos + myVel * Flocking.timeDelta;

    // Avoid floor and sky planes
    if (nextPos.y > 470.0) {
        myVel.y -= (myPos.y - 470.0) * 0.01;
    }

    if (nextPos.y < 50.0) {
        myVel.y -= (myPos.y - 50.0) * 0.01;
    }

    myVel.y *= 0.999;

    OutVelocityTexture[tid.xy] = float4(myVel, myCrowd);
#endif
}
