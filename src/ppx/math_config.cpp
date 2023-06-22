
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

#include <ostream>
#include "ppx/math_config.h"

std::ostream& operator<<(std::ostream& os, const ppx::float2& i)
{
    os << "(" << i.x << ", " << i.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ppx::float3& i)
{
    os << "(" << i.x << ", " << i.y << ", " << i.z << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ppx::float4& i)
{
    os << "(" << i.r << ", " << i.g << ", " << i.b << ", " << i.a << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ppx::uint3& i)
{
    os << "(" << i.x << ", " << i.y << ", " << i.z << ")";
    return os;
}
