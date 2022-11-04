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

#ifndef ppx_bounding_volume_h
#define ppx_bounding_volume_h

#include "ppx/math_config.h"

namespace ppx {

class AABB;
class OBB;

//! @class AABB
//!
//! @brief Axis-aligned bounding box
//!
class AABB
{
public:
    AABB() {}

    AABB(const float3& pos)
    {
        Set(pos);
    }

    AABB(const float3& minPos, const float3& maxPos)
    {
        Set(minPos, maxPos);
    }

    AABB(const OBB& obb);

    ~AABB() {}

    AABB& operator=(const OBB& rhs);

    void Set(const float3& pos)
    {
        mMin = mMax = pos;
    }

    void Set(const float3& minPos, const float3& maxPos)
    {
        mMin = glm::min(minPos, maxPos);
        mMax = glm::max(minPos, maxPos);
    }

    void Set(const OBB& obb);

    void Expand(const float3& pos)
    {
        mMin = glm::min(pos, mMin);
        mMax = glm::max(pos, mMax);
    }

    const float3& GetMin() const
    {
        return mMin;
    }

    const float3& GetMax() const
    {
        return mMax;
    }

    float3 GetCenter() const
    {
        float3 center = (mMin + mMax) / 2.0f;
        return center;
    }

    float3 GetSize() const
    {
        float3 size = (mMax - mMin);
        return size;
    }

    float GetWidth() const
    {
        return (mMax.x - mMin.x);
    }

    float GetHeight() const
    {
        return (mMax.y - mMin.y);
    }

    float GetDepth() const
    {
        return (mMax.y - mMin.y);
    }

    float3 GetU() const
    {
        float3 P = float3(mMax.x, mMin.y, mMin.z);
        float3 U = glm::normalize(P - mMin);
        return U;
    }

    float3 GetV() const
    {
        float3 P = float3(mMin.x, mMax.y, mMin.z);
        float3 V = glm::normalize(P - mMin);
        return V;
    }

    float3 GetW() const
    {
        float3 P = float3(mMin.x, mMin.y, mMax.z);
        float3 W = glm::normalize(P - mMin);
        return W;
    }

    void Transform(const float4x4& matrix, float3 obbVertices[8]) const;

private:
    float3 mMin = float3(0, 0, 0);
    float3 mMax = float3(0, 0, 0);
};

//! @class OBB
//!
//! @brief Oriented bounding box
//!
class OBB
{
public:
    OBB() {}

    OBB(const float3& center, const float3& size, const float3& U, const float3& V, const float3& W)
        : mCenter(center),
          mSize(size),
          mU(glm::normalize(U)),
          mV(glm::normalize(V)),
          mW(glm::normalize(W)) {}

    OBB(const AABB& aabb);

    ~OBB() {}

    void Set(const AABB& aabb);

    const float3& GetPos() const
    {
        return mCenter;
    }

    const float3& GetSize() const
    {
        return mSize;
    }

    const float3& GetU() const
    {
        return mU;
    }

    const float3& GetV() const
    {
        return mV;
    }

    const float3& GetW() const
    {
        return mW;
    }

    void GetPoints(float3 obbVertices[8]) const;

private:
    float3 mCenter = float3(0, 0, 0);
    float3 mSize   = float3(0, 0, 0);
    float3 mU      = float3(1, 0, 0);
    float3 mV      = float3(0, 1, 0);
    float3 mW      = float3(0, 0, 1);
};

} // namespace ppx

#endif // ppx_bounding_volume_h
