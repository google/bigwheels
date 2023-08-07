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
#include "ppx/log.h"
#include "ppx/string_util.h"

namespace {

bool StartsWithDoubleDash(std::string_view s)
{
    if (s.size() >= 3) {
        return s.substr(0, 2) == "--";
    }
    return false;
}

} // namespace

namespace ppx {

std::pair<int, int> CliOptions::GetOptionValueOrDefault(std::string_view optionName, const std::pair<int, int>& defaultValue) const
{
    auto it = mAllOptions.find(optionName);
    if (it == mAllOptions.cend()) {
        return defaultValue;
    }
    auto valueStr = it->second.back();
    auto res      = ppx::string_util::SplitInTwo(valueStr, 'x');
    if (res == std::nullopt) {
        PPX_LOG_ERROR("resolution flag must be in format <Width>x<Height>: " << valueStr);
        return defaultValue;
    }
    int N = GetParsedOrDefault(res->first, defaultValue.first);
    int M = GetParsedOrDefault(res->second, defaultValue.second);
    return std::make_pair(N, M);
}

void CliOptions::AddOption(std::string_view optionName, std::string_view valueStr)
{
    auto it = mAllOptions.find(optionName);
    if (it == mAllOptions.cend()) {
        std::vector<std::string_view> v{valueStr};
        mAllOptions.emplace(optionName, v);
        return;
    }
    it->second.push_back(valueStr);
}

bool CliOptions::Parse(std::string_view valueStr, bool defaultValue) const
{
    if (valueStr == "") {
        return true;
    }
    std::stringstream ss{std::string(valueStr)};
    bool              valueAsBool;
    ss >> valueAsBool;
    if (ss.fail()) {
        ss.clear();
        ss >> std::boolalpha >> valueAsBool;
        if (ss.fail()) {
            PPX_LOG_ERROR("could not be parsed as bool: " << valueStr);
            return defaultValue;
        }
    }
    return valueAsBool;
}

std::optional<CommandLineParser::ParsingError> CommandLineParser::Parse(int argc, const char* argv[])
{
    // argc should be >= 1 and argv[0] the name of the executable.
    if (argc < 2) {
        return std::nullopt;
    }

    // Split flag and parameters connected with '='
    std::vector<std::string_view> args;
    for (size_t i = 1; i < argc; ++i) {
        std::string_view argString(argv[i]);
        auto             res = ppx::string_util::SplitInTwo(argString, '=');
        if (res == std::nullopt) {
            args.emplace_back(argString);
            continue;
        }
        if (res->first.empty() || res->second.empty()) {
            return "Malformed flag with '=': \"" + std::string(argString) + "\"";
        }
        else if (res->second.find('=') != std::string_view::npos) {
            return "Unexpected number of '=' symbols in the following string: \"" + std::string(argString) + "\"";
        }
        args.emplace_back(res->first);
        args.emplace_back(res->second);
    }

    // Process arguments into either standalone flags or options with parameters.
    for (size_t i = 0; i < args.size(); ++i) {
        std::string_view name = ppx::string_util::TrimBothEnds(args[i]);
        if (!StartsWithDoubleDash(name)) {
            return "Invalid command-line option: \"" + std::string(name) + "\"";
        }
        name = name.substr(2);

        if (i + 1 < args.size()) {
            std::string_view nextElem = ppx::string_util::TrimBothEnds(args[i + 1]);
            if (!StartsWithDoubleDash(nextElem)) {
                // We found an option with a parameter.
                mOpts.AddOption(name, nextElem);
                ++i;
                continue;
            }
        }
        // There is no parameter so it's likely a flag
        if (name.substr(0, 3) == "no-") {
            mOpts.AddOption(name.substr(3), "0");
        }
        else {
            // Do not assign "1" in case it's an option lacking a parameter
            mOpts.AddOption(name, "");
        }
    }

    return std::nullopt;
}

} // namespace ppx
