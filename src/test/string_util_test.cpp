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
#include "gmock/gmock.h"

#include "ppx/string_util.h"

using namespace ppx::string_util;

// -------------------------------------------------------------------------------------------------
// Misc
// -------------------------------------------------------------------------------------------------

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

TEST(StringUtilTest, Split_EmptyString)
{
    std::string_view              toSplit = "";
    std::vector<std::string_view> want    = {};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, Split_NoDelimiter)
{
    std::string_view              toSplit = "Apple";
    std::vector<std::string_view> want    = {"Apple"};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got.size(), want.size());
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, Split_OneDelimiter)
{
    std::string_view              toSplit = "Apple,Banana";
    std::vector<std::string_view> want    = {"Apple", "Banana"};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got.size(), want.size());
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, Split_MultipleElements)
{
    std::string_view              toSplit = "Apple,Banana,Orange,Pear";
    std::vector<std::string_view> want    = {"Apple", "Banana", "Orange", "Pear"};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got.size(), want.size());
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, Split_LeadingTrailingDelimiter)
{
    std::string_view              toSplit = ",Apple,";
    std::vector<std::string_view> want    = {"", "Apple", ""};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got.size(), want.size());
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, Split_ConsecutiveDelimiters)
{
    std::string_view              toSplit = "Apple,,,Banana";
    std::vector<std::string_view> want    = {"Apple", "", "", "Banana"};
    std::vector<std::string_view> got     = Split(toSplit, ',');
    EXPECT_EQ(got.size(), want.size());
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_EmptyString)
{
    std::string_view                              toSplit = "";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_Pass)
{
    std::string_view                              toSplit = "Apple,Banana";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("Apple", "Banana");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_NoDelimiter)
{
    std::string_view                              toSplit = "Apple";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_MissingFirstHalf)
{
    std::string_view                              toSplit = ",Banana";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_MissingSecondHalf)
{
    std::string_view                              toSplit = "Apple,";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_MoreThanTwoElements)
{
    std::string_view                              toSplit = "Apple,Banana,Orange";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_TwoElementsWithLeadingTrailingDelimeters)
{
    std::string_view                              toSplit = ",Apple,Banana,";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

TEST(StringUtilTest, SplitInTwo_TwoElementsWithConsecutiveDelimeters)
{
    std::string_view                              toSplit = "Apple,,Banana";
    std::pair<std::string_view, std::string_view> want    = std::make_pair("", "");
    std::pair<std::string_view, std::string_view> got     = SplitInTwo(toSplit, ',');
    EXPECT_EQ(got, want);
}

// -------------------------------------------------------------------------------------------------
// Formatting Strings
// -------------------------------------------------------------------------------------------------

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

TEST(StringUtilTest, ToString_BoolTrue)
{
    bool        b          = true;
    std::string wantString = "true";
    std::string gotString  = ToString(b);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_BoolFalse)
{
    bool        b          = false;
    std::string wantString = "false";
    std::string gotString  = ToString(b);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_IntPositive)
{
    int         i          = 4;
    std::string wantString = "4";
    std::string gotString  = ToString(i);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_IntNegative)
{
    int         i          = -10;
    std::string wantString = "-10";
    std::string gotString  = ToString(i);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_FloatPositive)
{
    float       f          = 4.5f;
    std::string wantString = "4.5";
    std::string gotString  = ToString(f);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_FloatNegative)
{
    float       f          = -3.1415f;
    std::string wantString = "-3.1415";
    std::string gotString  = ToString(f);
    EXPECT_EQ(gotString, wantString);
}

TEST(StringUtilTest, ToString_FloatNoTrailingZeroes)
{
    float       f          = 80.8000f;
    std::string wantString = "80.8";
    std::string gotString  = ToString(f);
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

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

TEST(StringUtilTest, Parse_String)
{
    std::string toParse     = "foo";
    std::string parsedValue = "default";
    std::string wantValue   = "foo";

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_StringWithSpace)
{
    std::string toParse     = "foo bar";
    std::string parsedValue = "default";
    std::string wantValue   = "foo bar";

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolTrueText)
{
    std::string toParse     = "true";
    bool        parsedValue = false;
    bool        wantValue   = true;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolTrueOne)
{
    std::string toParse     = "1";
    bool        parsedValue = false;
    bool        wantValue   = true;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolTrueEmpty)
{
    std::string toParse     = "";
    bool        parsedValue = false;
    bool        wantValue   = true;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolFalseText)
{
    std::string toParse     = "false";
    bool        parsedValue = true;
    bool        wantValue   = false;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolFalseZero)
{
    std::string toParse     = "0";
    bool        parsedValue = true;
    bool        wantValue   = false;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_BoolFail)
{
    std::string toParse     = "foo";
    bool        parsedValue = true;
    bool        wantValue   = true;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_IntegerPass)
{
    std::string toParse     = "-10";
    int         parsedValue = 0;
    int         wantValue   = -10;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_IntegerFail)
{
    std::string toParse     = "foo";
    int         parsedValue = 0;
    int         wantValue   = 0;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_IntegerEmptyFail)
{
    std::string toParse     = "";
    int         parsedValue = 1;
    int         wantValue   = 1;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_SizetPass)
{
    std::string toParse     = "5";
    size_t      parsedValue = 0;
    size_t      wantValue   = 5;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_SizetFail)
{
    std::string toParse     = "foo";
    size_t      parsedValue = 0;
    size_t      wantValue   = 0;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_FloatPass)
{
    std::string toParse     = "5.6";
    float       parsedValue = 0.0f;
    float       wantValue   = 5.6f;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_FloatFail)
{
    std::string toParse     = "foo";
    float       parsedValue = 0.0f;
    float       wantValue   = 0.0f;

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_ResolutionPass)
{
    std::string         toParse     = "100x200";
    std::pair<int, int> parsedValue = std::make_pair(-1, -1);
    std::pair<int, int> wantValue   = std::make_pair(100, 200);

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Success(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_ResolutionNoDelimeterFail)
{
    std::string         toParse     = "100X200";
    std::pair<int, int> parsedValue = std::make_pair(-1, -1);
    std::pair<int, int> wantValue   = std::make_pair(-1, -1);

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_ResolutionWidthFail)
{
    std::string         toParse     = "foox200";
    std::pair<int, int> parsedValue = std::make_pair(-1, -1);
    std::pair<int, int> wantValue   = std::make_pair(-1, -1);

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}

TEST(StringUtilTest, Parse_ResolutionHeightFail)
{
    std::string         toParse     = "100xfoo";
    std::pair<int, int> parsedValue = std::make_pair(-1, -1);
    std::pair<int, int> wantValue   = std::make_pair(-1, -1);

    auto res = Parse(toParse, parsedValue);
    EXPECT_TRUE(ppx::Failed(res));
    EXPECT_EQ(parsedValue, wantValue);
}
