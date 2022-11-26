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

#ifndef ppx_math_config_h
#define ppx_math_config_h

#include <ostream>

// clang-format off
#if defined(__cplusplus)
#   define GLM_FORCE_RADIANS 
#   define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#   define GLM_ENABLE_EXPERIMENTAL
#   include <glm/glm.hpp>
#   include <glm/gtc/matrix_access.hpp>
#   include <glm/gtc/matrix_inverse.hpp>
#   include <glm/gtc/matrix_transform.hpp>
#   include <glm/gtx/euler_angles.hpp>
#   include <glm/gtx/matrix_decompose.hpp>
#   include <glm/gtx/quaternion.hpp>
#   include <glm/gtx/transform.hpp>
#endif
// clang-format on

#include "pcg32.h"

namespace ppx {

// clang-format off
// Import GLM types as HLSL friendly names
// bool
using bool2     = glm::bool2;
using bool3     = glm::bool3;
using bool4     = glm::bool4;

// 32-bit signed integer
using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;
// 32-bit unsigned integer
using uint      = glm::uint;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;

// 32-bit float
using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
// 32-bit float2 matrices
using float2x2  = glm::mat2x2;
using float2x3  = glm::mat2x3;
using float2x4  = glm::mat2x4;
// 32-bit float3 matrices
using float3x2  = glm::mat3x2;
using float3x3  = glm::mat3x3;
using float3x4  = glm::mat3x4;
// 32-bit float4 matrices
using float4x2  = glm::mat4x2;
using float4x3  = glm::mat4x3;
using float4x4  = glm::mat4x4;
// 32-bit float quaternion
using quat      = glm::quat;

// 64-bit float
using double2   = glm::dvec2;
using double3   = glm::dvec3;
using double4   = glm::dvec4;
// 64-bit float2 matrices
using double2x2 = glm::dmat2x2;
using double2x3 = glm::dmat2x3;
using double2x4 = glm::dmat2x4;
// 64-bit float3 matrices
using double3x2 = glm::dmat3x2;
using double3x3 = glm::dmat3x3;
using double3x4 = glm::dmat3x4;
// 64-bit float4 matrices
using double4x2 = glm::dmat4x2;
using double4x3 = glm::dmat4x3;
using double4x4 = glm::dmat4x4;
// clang-format on

#if defined(_MSC_VER)
#define PRAGMA(X) __pragma(X)
#else
#define PRAGMA(X) _Pragma(#X)
#endif
#define PPX_HLSL_PACK_BEGIN() PRAGMA(pack(push, 1))
#define PPX_HLSL_PACK_END()   PRAGMA(pack(pop))

PPX_HLSL_PACK_BEGIN();

struct float2x2_aligned
{
    float4 v0;
    float2 v1;

    float2x2_aligned() {}

    float2x2_aligned(const float2x2& m)
        : v0(m[0], 0, 0), v1(m[1]) {}

    float2x2_aligned& operator=(const float2x2& rhs)
    {
        v0 = float4(rhs[0], 0, 0);
        v1 = rhs[1];
        return *this;
    }

    operator float2x2() const
    {
        float2x2 m;
        m[0] = float2(v0);
        m[1] = float2(v1);
        return m;
    }
};

struct float3x3_aligned
{
    float4 v0;
    float4 v1;
    float3 v2;

    float3x3_aligned() {}

    float3x3_aligned(const float3x3& m)
        : v0(m[0], 0), v1(m[1], 0), v2(m[2]) {}

    float3x3_aligned& operator=(const float3x3& rhs)
    {
        v0 = float4(rhs[0], 0);
        v1 = float4(rhs[1], 0);
        v2 = rhs[2];
        return *this;
    }

    operator float3x3() const
    {
        float3x3 m;
        m[0] = float3(v0);
        m[1] = float3(v1);
        m[2] = v2;
        return m;
    }
};

template <typename T, size_t Size>
union hlsl_type
{
    T       value;
    uint8_t padded[Size];

    hlsl_type& operator=(const T& rhs)
    {
        value = rhs;
        return *this;
    }
};

// clang-format off
template <size_t Size> using hlsl_float  = hlsl_type<float, Size>;
template <size_t Size> using hlsl_float2 = hlsl_type<float2, Size>;
template <size_t Size> using hlsl_float3 = hlsl_type<float3, Size>;
template <size_t Size> using hlsl_float4 = hlsl_type<float4, Size>;

template <size_t Size> using hlsl_float2x2 = hlsl_type<float2x2_aligned, Size>;
template <size_t Size> using hlsl_float3x3 = hlsl_type<float3x3_aligned, Size>;
template <size_t Size> using hlsl_float4x4 = hlsl_type<float4x4, Size>;

template <size_t Size> using hlsl_int  = hlsl_type<int, Size>;
template <size_t Size> using hlsl_int2 = hlsl_type<int2, Size>;
template <size_t Size> using hlsl_int3 = hlsl_type<int3, Size>;
template <size_t Size> using hlsl_int4 = hlsl_type<int4, Size>;

template <size_t Size> using hlsl_uint  = hlsl_type<uint, Size>;
template <size_t Size> using hlsl_uint2 = hlsl_type<uint2, Size>;
template <size_t Size> using hlsl_uint3 = hlsl_type<uint3, Size>;
template <size_t Size> using hlsl_uint4 = hlsl_type<uint4, Size>;
// clang-format on

PPX_HLSL_PACK_END();

// -------------------------------------------------------------------------------------------------

template <typename T>
T pi()
{
    return static_cast<T>(3.1415926535897932384626433832795);
}

// -------------------------------------------------------------------------------------------------

} // namespace ppx

#endif // ppx_math_config_h
