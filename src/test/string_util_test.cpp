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

#include <gtest/gtest.h>

#include "ppx/string_util.h"

using namespace ppx::string_util;

TEST(StringUtilTest, TrimLeftNothingToTrim)
{
    std::string toTrim = "No left space  ";
    TrimLeft(toTrim);
    EXPECT_EQ(toTrim, "No left space  ");
}

TEST(StringUtilTest, TrimLeftSpaces)
{
    std::string toTrim = "  Some left spaces  ";
    TrimLeft(toTrim);
    EXPECT_EQ(toTrim, "Some left spaces  ");
}

TEST(StringUtilTest, TrimRightNothingToTrim)
{
    std::string toTrim = "    No right space";
    TrimRight(toTrim);
    EXPECT_EQ(toTrim, "    No right space");
}

TEST(StringUtilTest, TrimRightSpaces)
{
    std::string toTrim = "  Some right spaces  ";
    TrimRight(toTrim);
    EXPECT_EQ(toTrim, "  Some right spaces");
}

TEST(StringUtilTest, TrimCopyLeftAndRightSpaces)
{
    std::string toTrim  = "  Some spaces  ";
    std::string trimmed = TrimCopy(toTrim);
    EXPECT_EQ(trimmed, "Some spaces");
    EXPECT_EQ(toTrim, "  Some spaces  ");
}
