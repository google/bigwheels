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

#include "ppx/bounding_volume.h"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// AABB
// -------------------------------------------------------------------------------------------------
AABB::AABB(const OBB& obb)
{
    Set(obb);
}

AABB& AABB::operator=(const OBB& rhs)
{
    Set(rhs);
    return *this;
}

void AABB::Set(const OBB& obb)
{
    float3 obbVertices[8];
    obb.GetPoints(obbVertices);

    Set(obbVertices[0]);
    for (size_t i = 1; i < 8; ++i) {
        Expand(obbVertices[i]);
    }
}

void AABB::Transform(const float4x4& matrix, float3 obbVertices[8]) const
{
    obbVertices[0] = matrix * float4(mMin.x, mMax.y, mMin.z, 1.0f);
    obbVertices[1] = matrix * float4(mMin.x, mMin.y, mMin.z, 1.0f);
    obbVertices[2] = matrix * float4(mMax.x, mMin.y, mMin.z, 1.0f);
    obbVertices[3] = matrix * float4(mMax.x, mMax.y, mMin.z, 1.0f);
    obbVertices[4] = matrix * float4(mMin.x, mMax.y, mMax.z, 1.0f);
    obbVertices[5] = matrix * float4(mMin.x, mMin.y, mMax.z, 1.0f);
    obbVertices[6] = matrix * float4(mMax.x, mMin.y, mMax.z, 1.0f);
    obbVertices[7] = matrix * float4(mMax.x, mMax.y, mMax.z, 1.0f);
}

// -------------------------------------------------------------------------------------------------
// OBB
// -------------------------------------------------------------------------------------------------
OBB::OBB(const AABB& aabb)
{
    Set(aabb);
}

void OBB::Set(const AABB& aabb)
{
    mCenter = aabb.GetCenter();
    mSize   = aabb.GetSize();
    mU      = aabb.GetU();
    mV      = aabb.GetV();
    mW      = aabb.GetW();
}

void OBB::GetPoints(float3 obbVertices[8]) const
{
    float3 s       = mSize / 2.0f;
    float3 u       = s.x * mU;
    float3 v       = s.y * mV;
    float3 w       = s.z * mW;
    obbVertices[0] = mCenter - u + v - w;
    obbVertices[1] = mCenter - u - v - w;
    obbVertices[2] = mCenter + u - v - w;
    obbVertices[3] = mCenter + w + v - w;
    obbVertices[4] = mCenter - u + v + w;
    obbVertices[5] = mCenter - u - v + w;
    obbVertices[6] = mCenter + u - v + w;
    obbVertices[7] = mCenter + w + v + w;
}

} // namespace ppx
