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

#include <algorithm>
#include <iostream>
#include <locale>
#include <vector>
#include <stdexcept>
#include <cctype>

#include "ppx/command_line_parser.h"
#include "ppx/string_util.h"

namespace {

// Whether this is an option or flag (CLI argument starting with '--').
bool IsOptionOrFlag(const std::string& s)
{
    if (s.size() > 3) {
        return s.substr(0, 2) == "--";
    }
    return false;
}

} // namespace

namespace ppx {

std::pair<int, int> CliOptions::GetOptionValueOrDefault(const std::string& optionName, const std::pair<int, int>& defaultValue) const
{
    if (allOptions.find(optionName) == allOptions.cend()) {
        return defaultValue;
    }
    auto   valueStr = allOptions.at(optionName).back();
    size_t xIndex   = valueStr.find("x");
    if (xIndex == std::string::npos) {
        // valueStr not in expected format "NxM"
        return defaultValue;
    }
    std::string substringN = valueStr.substr(0, xIndex);
    std::string substringM = valueStr.substr(xIndex + 1);
    int         N          = GetParsedNumberOrDefault(substringN, defaultValue.first);
    int         M          = GetParsedNumberOrDefault(substringM, defaultValue.second);
    return std::make_pair(N, M);
}

void CliOptions::AddOption(const std::string& key, const std::string& valueStr)
{
    // If valueStr contains commas, split into separate elements to store
    std::vector<std::string> elements;
    std::string              remainingValueStr = valueStr;
    std::string              elementSubstring;
    size_t                   commaIndex = 0;
    while (commaIndex != std::string::npos) {
        commaIndex = remainingValueStr.find(",");
        elements.push_back(remainingValueStr.substr(0, commaIndex));
        remainingValueStr = remainingValueStr.substr(commaIndex + 1);
    }

    if (allOptions.find(key) != allOptions.cend()) {
        allOptions[key].insert(allOptions[key].end(), elements.begin(), elements.end());
        return;
    }
    allOptions.emplace(key, elements);
}

bool CliOptions::GetParsedBoolOrDefault(const std::string& valueStr, bool defaultValue) const
{
    if (valueStr == "") {
        return true;
    }
    std::stringstream ss{valueStr};
    bool              valueAsBool;
    ss >> valueAsBool;
    if (ss.fail()) {
        ss.clear();
        ss >> std::boolalpha >> valueAsBool;
        if (ss.fail()) {
            return defaultValue;
        }
    }
    return valueAsBool;
}

std::optional<CommandLineParser::ParsingError> CommandLineParser::Parse(int argc, const char* argv[])
{
    // argc is always >= 1 and argv[0] is the name of the executable.
    if (argc < 2) {
        return std::nullopt;
    }

    // Split flag and parameters connected with '='
    std::vector<std::string> args;
    for (size_t i = 1; i < argc; ++i) {
        std::string argString(argv[i]);
        size_t      delimeterIndex = argString.find("=");
        if (delimeterIndex == std::string::npos) {
            args.emplace_back(argv[i]);
            continue;
        }
        args.emplace_back(argString.substr(0, delimeterIndex));
        args.emplace_back(argString.substr(delimeterIndex + 1));
    }

    // Process arguments into either standalone flags or options with parameters.
    for (size_t i = 0; i < args.size(); ++i) {
        std::string name = ppx::string_util::TrimCopy(args[i]);
        if (!IsOptionOrFlag(name)) {
            return "Invalid command-line option " + name;
        }
        name = name.substr(2);
        if (name == "help") {
            mPrintHelp = true;
            return std::nullopt;
        }

        std::string nextElem = (i + 1 < args.size()) ? ppx::string_util::TrimCopy(args[i + 1]) : "";
        if (nextElem.size() > 0 && !IsOptionOrFlag(nextElem)) {
            // We found an option with a parameter.
            mOpts.AddOption(name, nextElem);
            ++i;
        }
        else if (name.substr(0, 3) == "no-") {
            // Option does not have a parameter, check if it has a no- prefix
            mOpts.AddOption(name.substr(3), "0");
        }
        else {
            // Option has no parameter, likely a boolean flag but do not assign "true"
            // in case it is a non-boolean string flag that is missing a parameter
            mOpts.AddOption(name, "");
        }
    }

    // Fill out standard options.
    mStandardOpts.assets_paths            = mOpts.GetOptionValueOrDefault("assets-path", mStandardOpts.assets_paths);
    mStandardOpts.deterministic           = mOpts.GetOptionValueOrDefault("deterministic", mStandardOpts.deterministic);
    mStandardOpts.enable_metrics          = mOpts.GetOptionValueOrDefault("enable-metrics", mStandardOpts.enable_metrics);
    mStandardOpts.frame_count             = mOpts.GetOptionValueOrDefault("frame-count", mStandardOpts.frame_count);
    mStandardOpts.gpu_index               = mOpts.GetOptionValueOrDefault("gpu", mStandardOpts.gpu_index);
    mStandardOpts.headless                = mOpts.GetOptionValueOrDefault("headless", mStandardOpts.headless);
    mStandardOpts.list_gpus               = mOpts.GetOptionValueOrDefault("list-gpus", mStandardOpts.list_gpus);
    mStandardOpts.metrics_filename        = mOpts.GetOptionValueOrDefault("metrics-filename", mStandardOpts.metrics_filename);
    mStandardOpts.overwrite_metrics_file  = mOpts.GetOptionValueOrDefault("overwrite-metrics-file", mStandardOpts.overwrite_metrics_file);
    mStandardOpts.resolution              = mOpts.GetOptionValueOrDefault("resolution", mStandardOpts.resolution);
    mStandardOpts.run_time_ms             = mOpts.GetOptionValueOrDefault("run-time-ms", mStandardOpts.run_time_ms);
    mStandardOpts.stats_frame_window      = mOpts.GetOptionValueOrDefault("stats-frame-window", mStandardOpts.stats_frame_window);
    mStandardOpts.screenshot_frame_number = mOpts.GetOptionValueOrDefault("screenshot-frame-number", mStandardOpts.screenshot_frame_number);
    mStandardOpts.screenshot_path         = mOpts.GetOptionValueOrDefault("screenshot-path", mStandardOpts.screenshot_path);
    mStandardOpts.use_software_renderer   = mOpts.GetOptionValueOrDefault("use-software-renderer", mStandardOpts.use_software_renderer);
#if defined(PPX_BUILD_XR)
    mStandardOpts.xrUIResolution = mOpts.GetOptionValueOrDefault("xr-ui-resolution", mStandardOpts.xrUIResolution);
#endif
    return std::nullopt;
}

} // namespace ppx
