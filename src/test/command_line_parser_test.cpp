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
    EXPECT_FALSE(parser.Parse(0, nullptr));
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 0);
}

TEST(CommandLineParserTest, FirstArgumentIgnored)
{
    CommandLineParser parser;
    StandardOptions   defaultOptions;
    const char*       args[] = {"/path/to/executable"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 0);
}

TEST(CommandLineParserTest, StandardOptionsSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--list-gpus", "--gpu", "5", "--resolution", "1920x1080", "--frame-count", "11", "--use-software-renderer", "--screenshot-frame-number", "321", "--screenshot-path", "/path/to/screenshot/dir/filename"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));

    StandardOptions wantOptions;
    wantOptions.list_gpus               = true;
    wantOptions.gpu_index               = 5;
    wantOptions.resolution              = {1920, 1080};
    wantOptions.frame_count             = 11;
    wantOptions.use_software_renderer   = true;
    wantOptions.screenshot_frame_number = 321;
    wantOptions.screenshot_path         = "/path/to/screenshot/dir/filename";

    EXPECT_EQ(parser.GetStandardOptions(), wantOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 7);
}

TEST(CommandLineParserTest, MixedEqualSignsAllowed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--list-gpus", "--gpu=5", "--resolution=1920x1080", "--frame-count", "11", "--use-software-renderer", "--screenshot-frame-number=321", "--screenshot-path=/path/to/screenshot/dir/filename"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));

    StandardOptions wantOptions;
    wantOptions.list_gpus               = true;
    wantOptions.gpu_index               = 5;
    wantOptions.resolution              = {1920, 1080};
    wantOptions.frame_count             = 11;
    wantOptions.use_software_renderer   = true;
    wantOptions.screenshot_frame_number = 321;
    wantOptions.screenshot_path         = "/path/to/screenshot/dir/filename";

    EXPECT_EQ(parser.GetStandardOptions(), wantOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 7);
}

TEST(CommandLineParserTest, BooleansSuccessfullyParsed)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--list-gpus", "--deterministic", "1", "--enable-metrics", "true", "--no-use-software-renderer", "--headless", "0", "--overwrite_metrics_file", "false"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));

    StandardOptions wantOptions;
    wantOptions.list_gpus              = true;
    wantOptions.deterministic          = true;
    wantOptions.enable_metrics         = true;
    wantOptions.use_software_renderer  = false;
    wantOptions.headless               = false;
    wantOptions.overwrite_metrics_file = false;

    EXPECT_EQ(parser.GetStandardOptions(), wantOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 6);
}

TEST(CommandLineParserTest, LastValueIsTaken)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--gpu", "1", "--gpu", "2", "--gpu", "3"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));

    StandardOptions wantOptions;
    wantOptions.gpu_index = 3;

    EXPECT_EQ(parser.GetStandardOptions(), wantOptions);
    EXPECT_EQ(parser.GetOptions().GetNumOptions(), 1);
}

TEST(CommandLineParserTest, ExtraOptionsSuccessfullyParsed)
{
    CommandLineParser parser;
    StandardOptions   defaultOptions;
    const char*       args[] = {"/path/to/executable", "--extra-option-bool", "true", "--extra-option-int", "123", "--extra-option-no-param", "--extra-option-str", "option string value"};
    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));

    auto opts = parser.GetOptions();
    EXPECT_EQ(parser.GetStandardOptions(), defaultOptions);
    EXPECT_EQ(opts.GetNumOptions(), 4);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-bool", false), true);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault("extra-option-int", 0), 123);
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-str", ""), "option string value");
    EXPECT_EQ(opts.GetExtraOptionValueOrDefault<std::string>("extra-option-no-param", ""), "");
    EXPECT_TRUE(opts.HasExtraOption("extra-option-no-param"));
}

TEST(CommandLineParserTest, StandardOptionsParsingMultipleAssets)
{
    CommandLineParser parser;
    const char*       args[] = {"/path/to/executable", "--assets-path", "some-path", "--assets-path", "some-other-path,some-other-path-2,some-other-path-3", "--assets-path", "last-path"};

    EXPECT_FALSE(parser.Parse(sizeof(args) / sizeof(args[0]), args));
    auto opts = parser.GetOptions();

    const auto& paths = parser.GetStandardOptions().assets_paths;
    EXPECT_EQ(paths.size(), 5);
    EXPECT_EQ(paths[0], "some-path");
    EXPECT_EQ(paths[1], "some-other-path");
    EXPECT_EQ(paths[2], "some-other-path-2");
    EXPECT_EQ(paths[3], "some-other-path-3");
    EXPECT_EQ(paths[4], "last-path");
}

} // namespace
} // namespace ppx
