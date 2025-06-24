// Copyright 2024 Google LLC
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

#include "ppx/options_new.h"

namespace ppx {
namespace {

using ::testing::HasSubstr;

TEST(OptionsNewTest, NoOptions)
{
    OptionsNew gotOptions;
    EXPECT_FALSE(gotOptions.HasOption("test"));
    EXPECT_EQ(gotOptions.GetNumUniqueOptions(), 0);
}

TEST(OptionsNewTest, AddOption_OneValue)
{
    OptionsNew wantOptions = OptionsNew({{"name1", std::vector<std::string>{"value1"}}});
    OptionsNew gotOptions;
    gotOptions.AddOption("name1", "value1");
    EXPECT_EQ(gotOptions, wantOptions);

    EXPECT_TRUE(gotOptions.HasOption("name1"));
    EXPECT_EQ(gotOptions.GetNumUniqueOptions(), 1);
    std::vector<std::string> gotValue = gotOptions.GetValueStrings("name1");
    EXPECT_EQ(gotValue.at(0), "value1");
}

TEST(OptionsNewTest, AddOption_MultipleValues)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"value1", "value2"}}});
    OptionsNew gotOptions;
    gotOptions.AddOption("name1", "value1");
    gotOptions.AddOption("name1", "value2");
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(OptionsNewTest, AddOption_MultipleValuesList)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"value1", "value2"}}});
    OptionsNew gotOptions;
    gotOptions.AddOption("name1", std::vector<std::string>{"value1", "value2"});
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(OptionsNewTest, AddOption_MultipleOptions)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"value1"}},
         {"name2", std::vector<std::string>{"value3"}}});
    OptionsNew gotOptions;
    gotOptions.AddOption("name1", "value1");
    gotOptions.AddOption("name2", "value3");
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(OptionsNewTest, OverwriteOptions_EmptyOverwrite)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"value1", "value2"}},
         {"name2", std::vector<std::string>{"value3", "value4"}}});
    OptionsNew gotOptions;
    OptionsNew overwriteOptions;
    gotOptions.AddOption("name1", std::vector<std::string>{"value1", "value2"});
    gotOptions.AddOption("name2", std::vector<std::string>{"value3", "value4"});
    gotOptions.OverwriteOptions(overwriteOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(OptionsNewTest, OverwriteOptions_EmptyBase)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"value1", "value2"}},
         {"name2", std::vector<std::string>{"value3", "value4"}}});
    OptionsNew gotOptions;
    OptionsNew overwriteOptions;
    overwriteOptions.AddOption("name1", std::vector<std::string>{"value1", "value2"});
    overwriteOptions.AddOption("name2", std::vector<std::string>{"value3", "value4"});
    gotOptions.OverwriteOptions(overwriteOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(OptionsNewTest, OverwriteOptions_Complex)
{
    OptionsNew wantOptions = OptionsNew(
        {{"name1", std::vector<std::string>{"newvalue1"}},
         {"name2", std::vector<std::string>{"oldvalue3", "oldvalue4"}},
         {"name3", std::vector<std::string>{"newvalue5", "newvalue6"}}});
    OptionsNew gotOptions;
    gotOptions.AddOption("name1", std::vector<std::string>{"oldvalue1", "oldvalue2"});
    gotOptions.AddOption("name2", std::vector<std::string>{"oldvalue3", "oldvalue4"});
    OptionsNew overwriteOptions;
    overwriteOptions.AddOption("name1", std::vector<std::string>{"newvalue1"});
    overwriteOptions.AddOption("name3", std::vector<std::string>{"newvalue5", "newvalue6"});
    gotOptions.OverwriteOptions(overwriteOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(CommandLineParserNewTest, Parse_ZeroArguments)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    EXPECT_TRUE(Success(parser.ParseOptions(0, nullptr, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserNewTest, Parse_FirstArgumentIgnored)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserNewTest, Parse_Booleans)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--b", "1", "--c", "true", "--no-d", "--e", "0", "--f", "false"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 6);
    auto optsMap = opts.GetMap();
    EXPECT_EQ(optsMap.at("a"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("b"), std::vector<std::string>{"1"});
    EXPECT_EQ(optsMap.at("c"), std::vector<std::string>{"true"});
    EXPECT_EQ(optsMap.at("d"), std::vector<std::string>{"0"}); // no- prefix is interpreted as such
    EXPECT_EQ(optsMap.at("e"), std::vector<std::string>{"0"});
    EXPECT_EQ(optsMap.at("f"), std::vector<std::string>{"false"});
}

TEST(CommandLineParserNewTest, Parse_Values)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] =
        {"/path/to/executable",
         "--a",
         "filename with spaces",
         "--b",
         "filenameWithoutSpaces",
         "--c",
         "filename,with/.punctuation,",
         "--d",
         "",
         "--e",
         "--f",
         "-5",
         "--g",
         "10.4",
         "--h",
         "-300.0"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 8);
    auto optsMap = opts.GetMap();
    EXPECT_EQ(optsMap.at("a"), std::vector<std::string>{"filename with spaces"});
    EXPECT_EQ(optsMap.at("b"), std::vector<std::string>{"filenameWithoutSpaces"});
    EXPECT_EQ(optsMap.at("c"), std::vector<std::string>{"filename,with/.punctuation,"});
    EXPECT_EQ(optsMap.at("d"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("e"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("f"), std::vector<std::string>{"-5"});
    EXPECT_EQ(optsMap.at("g"), std::vector<std::string>{"10.4"});
    EXPECT_EQ(optsMap.at("h"), std::vector<std::string>{"-300.0"});
}

TEST(CommandLineParserNewTest, Parse_StringList)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "some-path", "--a", "some-other-path", "--a", "last-path"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 1);
    auto gotValues = opts.GetValueStrings("a");
    EXPECT_EQ(gotValues.size(), 3);
    if (gotValues.size() == 3) {
        EXPECT_EQ(gotValues[0], "some-path");
        EXPECT_EQ(gotValues[1], "some-other-path");
        EXPECT_EQ(gotValues[2], "last-path");
    }
}

TEST(CommandLineParserNewTest, Parse_EqualSigns)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--b=5", "--c", "--d", "11"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 4);
    auto optsMap = opts.GetMap();
    EXPECT_EQ(optsMap.at("a"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("b"), std::vector<std::string>{"5"});
    EXPECT_EQ(optsMap.at("c"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("d"), std::vector<std::string>{"11"});
}

TEST(CommandLineParserNewTest, Parse_EqualSignsMultipleFail)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--b=5=8", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
}

TEST(CommandLineParserNewTest, Parse_EqualSignsNoNameFail)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--=5", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
}

TEST(CommandLineParserNewTest, Parse_EqualSignsNoFlagFail)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "=5", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
}

TEST(CommandLineParserNewTest, Parse_EqualSignsEmptyValuePass)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--b=", "--c", "--d", "11"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 4);
    auto optsMap = opts.GetMap();
    EXPECT_EQ(optsMap.at("a"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("b"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("c"), std::vector<std::string>{""});
    EXPECT_EQ(optsMap.at("d"), std::vector<std::string>{"11"});
}

TEST(CommandLineParserNewTest, Parse_LeadingParameterFail)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "10", "--a", "--b", "5", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
}

TEST(CommandLineParserNewTest, Parse_AdjacentParameterFail)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "--b", "5", "8", "--c", "--d", "11"};
    EXPECT_TRUE(Failed(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
}

TEST(CommandLineParserNewTest, Parse_MultipleUsageFlag)
{
    CommandLineParserNew parser;
    OptionsNew           opts;
    const char*          args[] = {"/path/to/executable", "--a", "1", "--b", "1", "--a", "2", "--a", "3"};
    EXPECT_TRUE(Success(parser.ParseOptions(sizeof(args) / sizeof(args[0]), args, opts)));
    EXPECT_EQ(opts.GetNumUniqueOptions(), 2);
    auto optsMap = opts.GetMap();
    EXPECT_EQ(optsMap.at("a").at(0), "1");
    EXPECT_EQ(optsMap.at("a").at(1), "2");
    EXPECT_EQ(optsMap.at("a").at(2), "3");
    EXPECT_EQ(optsMap.at("b"), std::vector<std::string>{"1"});
}

TEST(JsonConverterTest, ParseOptions_Empty)
{
    JsonConverterNew parser;
    OptionsNew       opts;
    nlohmann::json   jsonConfig;
    parser.ParseOptions(jsonConfig, opts);
    EXPECT_EQ(opts.GetNumUniqueOptions(), 0);
}

TEST(JsonConverterTest, ParseOptions_Simple)
{
    JsonConverterNew parser;
    std::string      jsonText = R"(
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

    OptionsNew wantOptions = OptionsNew(
        {{"a", std::vector<std::string>{"true"}},
         {"b", std::vector<std::string>{"false"}},
         {"c", std::vector<std::string>{"1.234"}},
         {"d", std::vector<std::string>{"5"}},
         {"e", std::vector<std::string>{"helloworld"}},
         {"f", std::vector<std::string>{"hello world"}},
         {"g", std::vector<std::string>{"200x300"}}});
    OptionsNew     gotOptions = {};
    nlohmann::json jsonConfig = nlohmann::json::parse(jsonText);
    parser.ParseOptions(jsonConfig, gotOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(JsonConverterTest, ParseOptions_NestedStructureFlattened)
{
    JsonConverterNew parser;
    std::string      jsonText = R"(
  {
    "a": true,
    "b": {
        "c" : 1,
        "d" : 2
    }
  }
)";

    OptionsNew wantOptions = OptionsNew(
        {{"a", std::vector<std::string>{"true"}},
         {"b", std::vector<std::string>{"{\"c\":1,\"d\":2}"}}});
    OptionsNew     gotOptions = {};
    nlohmann::json jsonConfig = nlohmann::json::parse(jsonText);
    parser.ParseOptions(jsonConfig, gotOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(JsonConverterTest, ParseOptions_IntArray)
{
    JsonConverterNew parser;
    std::string      jsonText = R"(
  {
    "a": true,
    "b": [1, 2, 3]
  }
)";

    OptionsNew wantOptions = OptionsNew(
        {{"a", std::vector<std::string>{"true"}},
         {"b", std::vector<std::string>{"1", "2", "3"}}});
    OptionsNew     gotOptions = {};
    nlohmann::json jsonConfig = nlohmann::json::parse(jsonText);
    parser.ParseOptions(jsonConfig, gotOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST(JsonConverterTest, ParseOptions_HeterogeneousArray)
{
    JsonConverterNew parser;
    std::string      jsonText = R"(
  {
    "a": true,
    "b": [1, "2", {"c" : 3}, 4.0]
  }
)";

    OptionsNew wantOptions = OptionsNew(
        {{"a", std::vector<std::string>{"true"}},
         {"b", std::vector<std::string>{"1", "2", "{\"c\":3}", "4.0"}}});
    OptionsNew     gotOptions = {};
    nlohmann::json jsonConfig = nlohmann::json::parse(jsonText);
    parser.ParseOptions(jsonConfig, gotOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

} // namespace
} // namespace ppx
