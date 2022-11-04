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

#ifndef ppx_random_h
#define ppx_random_h

#include "ppx/math_config.h"
#include "pcg32.h"

namespace ppx {

class Random
{
public:
    Random()
    {
        Seed(0xDEAD, 0xBEEF);
    }

    Random(uint64_t initialState, uint64_t initialSequence)
    {
        Seed(initialSequence, initialSequence);
    }

    ~Random() {}

    void Seed(uint64_t initialState, uint64_t initialSequence)
    {
        mRng.seed(initialSequence, initialSequence);
    }

    uint32_t UInt32()
    {
        uint32_t value = mRng.nextUInt();
        return value;
    }

    float Float()
    {
        float value = mRng.nextFloat();
        return value;
    }

    float Float(float a, float b)
    {
        float value = glm::lerp(a, b, Float());
        return value;
    }

    float2 Float2()
    {
        float2 value(
            Float(),
            Float());
        return value;
    }

    float2 Float2(const float2& a, const float2& b)
    {
        float2 value(
            Float(a.x, b.x),
            Float(a.y, b.y));
        return value;
    }

    float3 Float3()
    {
        float3 value(
            Float(),
            Float(),
            Float());
        return value;
    }

    float3 Float3(const float3& a, const float3& b)
    {
        float3 value(
            Float(a.x, b.x),
            Float(a.y, b.y),
            Float(a.z, b.z));
        return value;
    }

    float4 Float4()
    {
        float4 value(
            Float(),
            Float(),
            Float(),
            Float());
        return value;
    }

    float4 Float4(const float4& a, const float4& b)
    {
        float4 value(
            Float(a.x, b.x),
            Float(a.y, b.y),
            Float(a.z, b.z),
            Float(a.w, b.w));
        return value;
    }

private:
    pcg32 mRng;
};

} // namespace ppx

#endif // ppx_random_h
