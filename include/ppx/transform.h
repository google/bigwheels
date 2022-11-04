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

#ifndef ppx_transform_h
#define ppx_transform_h

#include "ppx/math_config.h"

namespace ppx {

class Transform
{
public:
    enum class RotationOrder
    {
        XYZ,
        XZY,
        YZX,
        YXZ,
        ZXY,
        ZYX,
    };

    Transform();
    Transform(const float3& translation);
    virtual ~Transform();

    const float3& GetTranslation() const { return mTranslation; }
    const float3& GetRotation() const { return mRotation; }
    const float3& GetScale() const { return mScale; }
    RotationOrder GetRotationOrder() const { return mRotationOrder; }

    void SetTranslation(const float3& value);
    void SetTranslation(float x, float y, float z) { SetTranslation(float3(x, y, z)); }
    void SetRotation(const float3& value);
    void SetRotation(float x, float y, float z) { SetRotation(float3(x, y, z)); }
    void SetScale(const float3& value);
    void SetScale(float x, float y, float z) { SetScale(float3(x, y, z)); }
    void SetRotationOrder(RotationOrder value);

    const float4x4& GetTranslationMatrix() const;
    const float4x4& GetRotationMatrix() const;
    const float4x4& GetScaleMatrix() const;
    const float4x4& GetConcatenatedMatrix() const;

protected:
    mutable struct
    {
        union
        {
            struct
            {
                bool translation  : 1;
                bool rotation     : 1;
                bool scale        : 1;
                bool concatenated : 1;
            };
            uint32_t mask = 0xF;
        };
    } mDirty;

    float3           mTranslation   = float3(0, 0, 0);
    float3           mRotation      = float3(0, 0, 0);
    float3           mScale         = float3(1, 1, 1);
    RotationOrder    mRotationOrder = RotationOrder::XYZ;
    mutable float4x4 mTranslationMatrix;
    mutable float4x4 mRotationMatrix;
    mutable float4x4 mScaleMatrix;
    mutable float4x4 mConcatenatedMatrix;
};

} // namespace ppx

#endif // ppx_transform_h
