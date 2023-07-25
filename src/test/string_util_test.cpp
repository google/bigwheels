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

#include "gtest/gtest.h"

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

TEST(StringUtilTest, TrimBothEndsNothingToTrim)
{
    std::string_view toTrim  = "No spaces";
    std::string_view trimmed = TrimBothEnds(toTrim);
    EXPECT_EQ(trimmed, "No spaces");
    EXPECT_EQ(toTrim, "No spaces");
}

TEST(StringUtilTest, TrimBothEndsLeftAndRightSpaces)
{
    std::string_view toTrim  = "  Some spaces  ";
    std::string_view trimmed = TrimBothEnds(toTrim);
    EXPECT_EQ(trimmed, "Some spaces");
    EXPECT_EQ(toTrim, "  Some spaces  ");
}

TEST(StringUtilTest, SplitInTwoEmptyString)
{
    std::string_view toSplit = "";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(res, std::nullopt);
}

TEST(StringUtilTest, SplitInTwoNullDelimiter)
{
    std::string_view toSplit = "Apple,Banana";
    auto             res     = SplitInTwo(toSplit, '\0');
    EXPECT_EQ(res, std::nullopt);
}

TEST(StringUtilTest, SplitInTwoOneDelimiter)
{
    std::string_view toSplit = "Apple,Banana";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_NE(res, std::nullopt);
    EXPECT_EQ(res->first, "Apple");
    EXPECT_EQ(res->second, "Banana");
}

TEST(StringUtilTest, SplitInTwoMultipleDelimiter)
{
    std::string_view toSplit = "Apple,Banana,Orange";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_NE(res, std::nullopt);
    EXPECT_EQ(res->first, "Apple");
    EXPECT_EQ(res->second, "Banana,Orange");
}