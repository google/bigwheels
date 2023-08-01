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

TEST(StringUtilTest, TrimLeft_NothingToTrim)
{
    std::string toTrim = "No left space  ";
    TrimLeft(toTrim);
    EXPECT_EQ(toTrim, "No left space  ");
}

TEST(StringUtilTest, TrimLeft_Spaces)
{
    std::string toTrim = "  Some left spaces  ";
    TrimLeft(toTrim);
    EXPECT_EQ(toTrim, "Some left spaces  ");
}

TEST(StringUtilTest, TrimRight_NothingToTrim)
{
    std::string toTrim = "    No right space";
    TrimRight(toTrim);
    EXPECT_EQ(toTrim, "    No right space");
}

TEST(StringUtilTest, TrimRight_Spaces)
{
    std::string toTrim = "  Some right spaces  ";
    TrimRight(toTrim);
    EXPECT_EQ(toTrim, "  Some right spaces");
}

TEST(StringUtilTest, TrimCopy_LeftAndRightSpaces)
{
    std::string toTrim  = "  Some spaces  ";
    std::string trimmed = TrimCopy(toTrim);
    EXPECT_EQ(trimmed, "Some spaces");
    EXPECT_EQ(toTrim, "  Some spaces  ");
}

TEST(StringUtilTest, TrimBothEnds_NothingToTrim)
{
    std::string_view toTrim  = "No spaces";
    std::string_view trimmed = TrimBothEnds(toTrim);
    EXPECT_EQ(trimmed, "No spaces");
    EXPECT_EQ(toTrim, "No spaces");
}

TEST(StringUtilTest, TrimBothEnds_LeftAndRightSpaces)
{
    std::string_view toTrim  = "  Some spaces  ";
    std::string_view trimmed = TrimBothEnds(toTrim);
    EXPECT_EQ(trimmed, "Some spaces");
    EXPECT_EQ(toTrim, "  Some spaces  ");
}

TEST(StringUtilTest, SplitInTwo_EmptyString)
{
    std::string_view toSplit = "";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(res, std::nullopt);
}

TEST(StringUtilTest, SplitInTwo_OneDelimiter)
{
    std::string_view toSplit = "Apple,Banana";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_NE(res, std::nullopt);
    EXPECT_EQ(res->first, "Apple");
    EXPECT_EQ(res->second, "Banana");
}

TEST(StringUtilTest, SplitInTwo_MultipleDelimiter)
{
    std::string_view toSplit = "Apple,Banana,Orange";
    auto             res     = SplitInTwo(toSplit, ',');
    EXPECT_NE(res, std::nullopt);
    EXPECT_EQ(res->first, "Apple");
    EXPECT_EQ(res->second, "Banana,Orange");
}

TEST(StringUtilTest, WrapText_EmptyString)
{
    std::string toWrap  = "";
    std::string wrapped = WrapText(toWrap, 10);
    EXPECT_EQ(wrapped, "");
    EXPECT_EQ(toWrap, "");
}

TEST(StringUtilTest, WrapText_IndentLargerThanWidth)
{
    std::string toWrap  = "Some text.";
    std::string wrapped = WrapText(toWrap, 5, 8);
    EXPECT_EQ(wrapped, toWrap);
}

TEST(StringUtilTest, WrapText_NoIndent)
{
    std::string toWrap   = "The quick brown fox jumps over the lazy dog.";
    std::string wantWrap = R"(The quick
brown fox
jumps over
the lazy
dog.
)";
    std::string wrapped  = WrapText(toWrap, 10);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_WithIndent)
{
    std::string toWrap   = "The quick brown fox jumps over the lazy dog.";
    std::string wantWrap = R"(   The quick
   brown fox
   jumps over
   the lazy
   dog.
)";
    std::string wrapped  = WrapText(toWrap, 13, 3);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_LeadingTrailingSpaces)
{
    std::string toWrap   = "    The quick brown fox jumps over the lazy dog.    ";
    std::string wantWrap = R"(The quick
brown fox
jumps over
the lazy
dog.
)";
    std::string wrapped  = WrapText(toWrap, 10);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_WithTabs)
{
    std::string toWrap   = "\t\tThe quick brown \tfox jumps over \tthe lazy dog.\t";
    std::string wantWrap = "The quick\nbrown \tfox\njumps over\nthe lazy\ndog.\n";
    std::string wrapped  = WrapText(toWrap, 10);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_MixedTabsAndSpaces)
{
    std::string toWrap   = "    \t\tThe quick brown \tfox       jumps over \tthe lazy dog. \t  ";
    std::string wantWrap = "The quick\nbrown \tfox\njumps over\nthe lazy\ndog.\n";
    std::string wrapped  = WrapText(toWrap, 10, 0);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_LongWord)
{
    std::string toWrap   = "The quick brown fox jumps over the extremely-long-word-here lazy dog.";
    std::string wantWrap = R"(The quick
brown fox
jumps over
the
extremely-
long-word-
here lazy
dog.
)";
    std::string wrapped  = WrapText(toWrap, 10, 0);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, WrapText_LongTextWithIndent)
{
    std::string toWrap   = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. \
Cras dapibus finibus nibh, id volutpat odio porta eget. Curabitur lacus urna, \
placerat tempus consequat id, vulputate eget urna. Suspendisse et massa eget erat \
pretium convallis elementum quis nunc. Suspendisse lacinia justo tellus, a fermentum \
metus cursus sed. Phasellus rhoncus ante nec augue rhoncus, id interdum nunc condimentum. \
Pellentesque vel urna ac tellus euismod finibus quis ac magna. Cras sit amet sapien id \
neque lobortis aliquam. Vivamus porttitor neque eu eros mollis imperdiet. Vivamus \
blandit neque sed nisl pretium, quis volutpat dui pharetra.";
    std::string wantWrap = R"(                    Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                    Cras dapibus finibus nibh, id volutpat odio porta eget.
                    Curabitur lacus urna, placerat tempus consequat id,
                    vulputate eget urna. Suspendisse et massa eget erat pretium
                    convallis elementum quis nunc. Suspendisse lacinia justo
                    tellus, a fermentum metus cursus sed. Phasellus rhoncus ante
                    nec augue rhoncus, id interdum nunc condimentum.
                    Pellentesque vel urna ac tellus euismod finibus quis ac
                    magna. Cras sit amet sapien id neque lobortis aliquam.
                    Vivamus porttitor neque eu eros mollis imperdiet. Vivamus
                    blandit neque sed nisl pretium, quis volutpat dui pharetra.
)";
    std::string wrapped  = WrapText(toWrap, 80, 20);
    EXPECT_EQ(wrapped, wantWrap);
}

TEST(StringUtilTest, ToString_Bool)
{
    bool        b          = false;
    std::string wantString = "false";
    std::string gotString  = ToString(b);
    EXPECT_EQ(gotString, wantString);

    b          = true;
    wantString = "true";
    gotString  = ToString(b);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_Int)
{
    int         i          = 4;
    std::string wantString = "4";
    std::string gotString  = ToString(i);
    EXPECT_EQ(gotString, wantString);

    i          = -10;
    wantString = "-10";
    gotString  = ToString(i);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_Float)
{
    float       f          = 4.5f;
    std::string wantString = "4.5";
    std::string gotString  = ToString(f);
    EXPECT_EQ(gotString, wantString);

    f          = -3.1415f;
    wantString = "-3.1415";
    gotString  = ToString(f);
    EXPECT_EQ(gotString, wantString);

    // Trims trailing zeros
    f          = 80.000f;
    wantString = "80";
    gotString  = ToString(f);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_PairInt)
{
    std::pair<int, int> pi         = std::make_pair(10, 20);
    std::string         wantString = "10, 20";
    std::string         gotString  = ToString(pi);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_VectorString)
{
    std::vector<std::string> vs         = {"hello", "world", "!"};
    std::string              wantString = "hello, world, !";
    std::string              gotString  = ToString(vs);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_VectorBool)
{
    std::vector<bool> vb         = {true, false, true, true, false};
    std::string       wantString = "true, false, true, true, false";
    std::string       gotString  = ToString(vb);
    EXPECT_EQ(gotString, wantString);
}
