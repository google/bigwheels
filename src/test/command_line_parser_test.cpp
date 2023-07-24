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

TEST(CommandLineParserTest, ZeroArguments)
{
    CommandLineParser parser;
    StandardOptions   defaultOptions;
    if (auto error = parser.Parse(0, nullptr)) {
        FAIL() << error->errorMsg;
    }
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserTest, FirstArgumentIgnored)
{
    CommandLineParser parser;
    StandardOptions   defaultOptions;
    const char*       args[] = {"/path/to/executable"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 0);
}

TEST(CommandLineParserTest, BooleansSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b", "1", "--c", "true", "--no-d", "--e", "0", "--f", "false"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 6);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("b", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("c", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("d", true), false);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("e", true), false);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("f", true), false);
}

TEST(CommandLineParserTest, StringsSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "filename with spaces", "--b", "filenameWithoutSpaces", "--c", "filename,with/.punctuation,", "--d", "", "--e"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("a", ""), "filename with spaces");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("b", ""), "filenameWithoutSpaces");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("c", ""), "filename,with/.punctuation,");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("d", "foo"), "");
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<std::string>("e", "foo"), "");
}

TEST(CommandLineParserTest, IntegersSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "0", "--b", "-5", "--c", "300", "--d", "0", "--e", "1000"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("a", -1), 0);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", -1), -5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("c", -1), 300);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("d", -1), 0);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("e", -1), 1000);
}

TEST(CommandLineParserTest, FloatsSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1.0", "--b", "-6.5", "--c", "300"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 3);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("a", 0.0f), 1.0f);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("b", 0.0f), -6.5f);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<float>("c", 0.0f), 300.0f);
}

TEST(CommandLineParserTest, StringListSuccesfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "some-path", "--a", "some-other-path", "--a", "last-path"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
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

TEST(CommandLineParserTest, ResolutionSuccesfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1000x2000"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 1);
    auto res = gotOptions.GetOptionValueOrDefault("a", std::make_pair(0, 0));
    EXPECT_EQ(res.first, 1000);
    EXPECT_EQ(res.second, 2000);
}

TEST(CommandLineParserTest, ResolutionSuccessfullyParsedButDefaulted)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1000X2000"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 1);
    auto res = gotOptions.GetOptionValueOrDefault("a", std::make_pair(0, 0));
    EXPECT_EQ(res.first, 0);
    EXPECT_EQ(res.second, 0);
}

TEST(CommandLineParserTest, EqualSignsSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b=5", "--c", "--d", "11"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 4);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("a", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", 0), 5);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<bool>("c", false), true);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("d", 0), 11);
}

TEST(CommandLineParserTest, EqualSignsFailedParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b=5=8", "--c", "--d", "11"};
    auto              error  = parser.Parse(sizeof(args) / sizeof(args[0]), args);
    EXPECT_TRUE(error);
    EXPECT_THAT(error->errorMsg, HasSubstr("Unexpected number of '=' symbols in following string"));
}

TEST(CommandLineParserTest, LeadingParameterFailedParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "10", "--a", "--b", "5", "--c", "--d", "11"};
    auto              error  = parser.Parse(sizeof(args) / sizeof(args[0]), args);
    EXPECT_TRUE(error);
    EXPECT_THAT(error->errorMsg, HasSubstr("Invalid command-line option"));
}

TEST(CommandLineParserTest, AdjacentParameterFailedParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "--b", "5", "8", "--c", "--d", "11"};
    auto              error  = parser.Parse(sizeof(args) / sizeof(args[0]), args);
    EXPECT_TRUE(error);
    EXPECT_THAT(error->errorMsg, HasSubstr("Invalid command-line option"));
}

TEST(CommandLineParserTest, LastValueIsTaken)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--a", "1", "--b", "1", "--a", "2", "--a", "3"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    CliOptions gotOptions = parser.GetOptions();
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 2);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("a", 0), 3);
    EXPECT_EQ(gotOptions.GetOptionValueOrDefault<int>("b", 0), 1);
}

TEST(CommandLineParserTest, StandardOptionsSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {
        "/path/to/executable",
        "--list-gpus",
        "--deterministic",
        "1",
        "--enable-metrics",
        "true",
        "--no-use-software-renderer",
        "--headless",
        "0",
        "--overwrite_metrics_file",
        "false",
        "--metrics-filename",
        "filename with spaces",
        "--screenshot-path",
        "filenameWithoutSpaces",
        "--frame-count",
        "0",
        "--gpu",
        "5",
        "--run-time-ms",
        "300",
        "--screenshot-frame-number",
        "0",
        "--stats-frame-window",
        "1000",
        "--assets-path",
        "foo1",
        "--assets-path",
        "foo2",
        "--resolution",
        "100x200"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    EXPECT_EQ(parser.GetOptions().GetNumUniqueOptions(), 15);
    StandardOptions gotStandardOptions = parser.GetStandardOptions();
    EXPECT_EQ(gotStandardOptions.list_gpus, true);
    EXPECT_EQ(gotStandardOptions.deterministic, true);
    EXPECT_EQ(gotStandardOptions.enable_metrics, true);
    EXPECT_EQ(gotStandardOptions.use_software_renderer, false);
    EXPECT_EQ(gotStandardOptions.headless, false);
    EXPECT_EQ(gotStandardOptions.overwrite_metrics_file, false);
    EXPECT_EQ(gotStandardOptions.metrics_filename, "filename with spaces");
    EXPECT_EQ(gotStandardOptions.screenshot_path, "filenameWithoutSpaces");
    EXPECT_EQ(gotStandardOptions.frame_count, 0);
    EXPECT_EQ(gotStandardOptions.gpu_index, 5);
    EXPECT_EQ(gotStandardOptions.run_time_ms, 300);
    EXPECT_EQ(gotStandardOptions.screenshot_frame_number, 0);
    EXPECT_EQ(gotStandardOptions.stats_frame_window, 1000);
    std::vector<std::string> assetsPaths{"foo1", "foo2"};
    EXPECT_EQ(gotStandardOptions.assets_paths, assetsPaths);
    EXPECT_EQ(gotStandardOptions.resolution, std::make_pair(100, 200));
}

TEST(CommandLineParserTest, ExtraOptionsSuccessfullyParsed)
{
    CommandLineParser parser;
    StandardOptions   defaultOptions;
    const char*       args[] = {"/path/to/executable", "--extra-option-bool", "true", "--extra-option-int", "123", "--extra-option-no-param", "--extra-option-str", "option string value"};
    if (auto error = parser.Parse(sizeof(args) / sizeof(args[0]), args)) {
        FAIL() << error->errorMsg;
    }
    auto opts = parser.GetOptions();
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(opts.GetNumUniqueOptions(), 4);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-bool", false), true);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-int", 0), 123);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-str", ""), "option string value");
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-no-param", ""), "");
    EXPECT_TRUE(opts.HasExtraOption("extra-option-no-param"));
}

} // namespace
} // namespace ppx
