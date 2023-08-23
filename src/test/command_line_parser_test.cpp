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

#include "ppx/command_line_parser.h"

namespace ppx {
namespace {

using ::testing::HasSubstr;

TEST(CommandLineParserTest, Parse_ZeroArguments)
{
    CommandLineParser parser;
    EXPECT_TRUE(Success(parser.Parse(0, nullptr)));
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserTest, Parse_FirstArgumentIgnored)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserTest, Parse_Booleans)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b", "1", "--c", "true", "--no-d", "--e", "0", "--f", "false"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 6);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("b", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("c", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("d", true), false);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("e", true), false);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("f", true), false);
}

TEST(CommandLineParserTest, Parse_Strings)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "filename with spaces", "--b", "filenameWithoutSpaces", "--c", "filename,with/.punctuation,", "--d", "", "--e"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("a", ""), "filename with spaces");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("b", ""), "filenameWithoutSpaces");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("c", ""), "filename,with/.punctuation,");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("d", "foo"), "");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("e", "foo"), "");
}

TEST(CommandLineParserTest, Parse_Integers)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "0", "--b", "-5", "--c", "300", "--d", "0", "--e", "1000"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("a", -1), 0);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", -1), -5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("c", -1), 300);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("d", -1), 0);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("e", -1), 1000);
}

TEST(CommandLineParserTest, Parse_Floats)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1.0", "--b", "-6.5", "--c", "300"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 3);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("a", 0.0f), 1.0f);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("b", 0.0f), -6.5f);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("c", 0.0f), 300.0f);
}

TEST(CommandLineParserTest, Parse_StringList)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "some-path", "--a", "some-other-path", "--a", "last-path"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 1);
    auto paths = gotOptions.GetOptionValueOrDefault<std::string>("a", {"a-path"});
    EXPECT_EQ(paths.size(), 3);
    if (paths.size() == 3) {
        EXPECT_EQ(paths[0], "some-path");
        EXPECT_EQ(paths[1], "some-other-path");
        EXPECT_EQ(paths[2], "last-path");
    }
}

TEST(CommandLineParserTest, Parse_Resolution)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1000x2000"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 1);
    auto res = gotOptions.GetOptionValueOrDefault("a", std::make_pair(0, 0));
    EXPECT_EQ(res.first, 1000);
    EXPECT_EQ(res.second, 2000);
}

TEST(CommandLineParserTest, Parse_ResolutionDefaulted)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1000X2000"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 1);
    auto res = gotOptions.GetOptionValueOrDefault("a", std::make_pair(0, 0));
    EXPECT_EQ(res.first, 0);
    EXPECT_EQ(res.second, 0);
}

TEST(CommandLineParserTest, Parse_EqualSigns)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b=5", "--c", "--d", "11"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 4);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", 0), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("c", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("d", 0), 11);
}

TEST(CommandLineParserTest, Parse_EqualSignsMultipleFail)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b=5=8", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
}

TEST(CommandLineParserTest, Parse_EqualSignsMalformedFail)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b=", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
}

TEST(CommandLineParserTest, Parse_LeadingParameterFail)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "10", "--a", "--b", "5", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
}

TEST(CommandLineParserTest, Parse_AdjacentParameterFail)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b", "5", "8", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
}

TEST(CommandLineParserTest, Parse_LastValueIsTaken)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1", "--b", "1", "--a", "2", "--a", "3"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 2);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("a", 0), 3);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", 0), 1);
}

TEST(CommandLineParserTest, Parse_ExtraOptions)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--extra-option-bool", "true", "--extra-option-int", "123", "--extra-option-no-param", "--extra-option-str", "option string value"};
    EXPECT_TRUE(Success(parser.Parse(sizeof(args) / sizeof(args[0]), args)));
    auto opts = parser.GetOptions();
    EXPECT_EQ(opts.GetNumUniqueOptions(), 4);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-bool", false), true);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-int", 0), 123);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-str", ""), "option string value");
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-no-param", ""), "");
    EXPECT_TRUE(opts.HasExtraOption("extra-option-no-param"));
}

TEST(CommandLineParserTest, ParseJson_Empty)
{
    CommandLineParser parser;
    CliOptions        opts;
    nlohmann::json    jsonConfig;
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserTest, ParseJson_Simple)
{
    CommandLineParser parser;
    CliOptions        opts;
    std::string       jsonText   = R"(
  {
    "a": true,
    "b": false,
    "c": 1.234,
    "d": 5,
    "e": "helloworld",
    "f": "hello world",
    "g": "200x300"
  }
)";
    nlohmann::json    jsonConfig = nlohmann::json::parse(jsonText);
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 7);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("b", true), false);
    EXPECT_FLOAT_EQ(opts.GetOptionValueOrDefault<float>("c", 6.0f), 1.234f);
    EXPECT_EQ(opts.GetOptionValueOrDefault<int>("d", 0), 5);
    EXPECT_EQ(opts.GetOptionValueOrDefault<std::string>("e", "foo"), "helloworld");
    EXPECT_EQ(opts.GetOptionValueOrDefault<std::string>("f", "foo"), "hello world");
    std::pair<int, int> gFlag = opts.GetOptionValueOrDefault("g", std::make_pair(1, 1));
    EXPECT_EQ(gFlag.first, 200);
    EXPECT_EQ(gFlag.second, 300);
}

TEST(CommandLineParserTest, ParseJson_NestedStructure)
{
    CommandLineParser parser;
    CliOptions        opts;
    std::string       jsonText   = R"(
  {
    "a": true,
    "b": {
        "c" : 1,
        "d" : 2
    }
  }
)";
    nlohmann::json    jsonConfig = nlohmann::json::parse(jsonText);
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 2);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_TRUE(opts.HasExtraOption("b"));
    EXPECT_EQ(opts.GetOptionValueOrDefault<std::string>("b", "default"), "{\"c\":1,\"d\":2}");
    EXPECT_FALSE(opts.HasExtraOption("c"));
    EXPECT_FALSE(opts.HasExtraOption("d"));
}

TEST(CommandLineParserTest, ParseJson_IntArray)
{
    CommandLineParser parser;
    CliOptions        opts;
    std::string       jsonText   = R"(
  {
    "a": true,
    "b": [1, 2, 3]
  }
)";
    nlohmann::json    jsonConfig = nlohmann::json::parse(jsonText);
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 2);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_TRUE(opts.HasExtraOption("b"));
    std::vector<int> defaultB = {100};
    std::vector<int> gotB     = opts.GetOptionValueOrDefault<int>("b", defaultB);
    EXPECT_EQ(gotB.size(), 3);
    EXPECT_EQ(gotB.at(0), 1);
    EXPECT_EQ(gotB.at(1), 2);
    EXPECT_EQ(gotB.at(2), 3);
}

TEST(CommandLineParserTest, ParseJson_StrArray)
{
    CommandLineParser parser;
    CliOptions        opts;
    std::string       jsonText   = R"(
  {
    "a": true,
    "b": ["first", "second", "third"]
  }
)";
    nlohmann::json    jsonConfig = nlohmann::json::parse(jsonText);
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 2);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_TRUE(opts.HasExtraOption("b"));
    std::vector<std::string> defaultB = {};
    std::vector<std::string> gotB     = opts.GetOptionValueOrDefault<std::string>("b", defaultB);
    EXPECT_EQ(gotB.size(), 3);
    EXPECT_EQ(gotB.at(0), "first");
    EXPECT_EQ(gotB.at(1), "second");
    EXPECT_EQ(gotB.at(2), "third");
}

TEST(CommandLineParserTest, ParseJson_HeterogeneousArray)
{
    CommandLineParser parser;
    CliOptions        opts;
    std::string       jsonText   = R"(
  {
    "a": true,
    "b": [1, "2", {"c" : 3}, 4.0]
  }
)";
    nlohmann::json    jsonConfig = nlohmann::json::parse(jsonText);
    EXPECT_TRUE(Success(parser.ParseJson(opts, jsonConfig)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 2);
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_TRUE(opts.HasExtraOption("b"));
    std::vector<std::string> defaultB;
    std::vector<std::string> gotB = opts.GetOptionValueOrDefault<std::string>("b", defaultB);
    EXPECT_EQ(gotB.size(), 4);
    EXPECT_EQ(gotB.at(0), "1");
    EXPECT_EQ(gotB.at(1), "2");
    EXPECT_EQ(gotB.at(2), "{\"c\":3}");
    EXPECT_EQ(gotB.at(3), "4.0");
}

TEST(CommandLineParserTest, ParseOption_Simple)
{
    CommandLineParser parser;
    CliOptions        opts;
    EXPECT_TRUE(Success(parser.ParseOption(opts, "flag-name", "true")));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 1);
    EXPECT_TRUE(opts.HasExtraOption("flag-name"));
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("flag-name", false), true);
}

TEST(CommandLineParserTest, ParseOption_NoPrefix)
{
    CommandLineParser parser;
    CliOptions        opts;
    EXPECT_TRUE(Success(parser.ParseOption(opts, "no-flag-name", "")));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 1);
    EXPECT_TRUE(opts.HasExtraOption("flag-name"));
    EXPECT_EQ(opts.GetOptionValueOrDefault<bool>("flag-name", true), false);
}

TEST(CommandLineParserTest, ParseOption_NoPrefixWithValueFail)
{
    CommandLineParser parser;
    CliOptions        opts;
    EXPECT_TRUE(Failed(parser.ParseOption(opts, "no-flag-name", "value")));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 0);
    EXPECT_FALSE(opts.HasExtraOption("flag-name"));
}

} // namespace
} // namespace ppx
