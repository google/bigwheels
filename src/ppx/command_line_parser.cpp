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
#include <fstream>
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

std::pair<int, int> CliOptions::GetOptionValueOrDefault(const std::string& optionName, const std::pair<int, int>& defaultValue) const
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

void CliOptions::AddOption(std::string_view optionName, std::string_view value)
{
    std::string optionNameStr(optionName);
    std::string valueStr(value);
    auto        it = mAllOptions.find(optionNameStr);
    if (it == mAllOptions.cend()) {
        std::vector<std::string> v{std::move(valueStr)};
        mAllOptions.emplace(std::move(optionNameStr), std::move(v));
        return;
    }
    it->second.push_back(valueStr);
}

void CliOptions::AddOption(std::string_view optionName, const std::vector<std::string>& valueArray)
{
    std::string optionNameStr(optionName);
    auto        it = mAllOptions.find(optionNameStr);
    if (it == mAllOptions.cend()) {
        mAllOptions.emplace(std::move(optionNameStr), valueArray);
        return;
    }
    auto storedValueArray = it->second;
    storedValueArray.insert(storedValueArray.end(), valueArray.cbegin(), valueArray.cend());
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

    // Initial pass:
    // - Split flag and parameters connected with '='
    // - Identify JSON config files, add the option, and remove from argument list
    std::vector<std::string_view> args;
    for (size_t i = 1; i < argc; ++i) {
        std::string_view argString(argv[i]);
        auto             res = ppx::string_util::SplitInTwo(argString, '=');

        // Argument does not contain '='
        if (res == std::nullopt) {
            if (StartsWithDoubleDash(argString) &&
                argString == "--" + mJsonConfigFlagName &&
                i + 1 < argc &&
                !StartsWithDoubleDash(argv[i + 1])) {
                mOpts.AddOption(mJsonConfigFlagName, ppx::string_util::TrimBothEnds(argv[i + 1]));
                ++i;
                continue;
            }
            args.emplace_back(argString);
            continue;
        }

        // Argument contains '='
        if (res->first.empty() || res->second.empty()) {
            return "Malformed flag with '=': \"" + std::string(argString) + "\"";
        }
        if (res->second.find('=') != std::string_view::npos) {
            return "Unexpected number of '=' symbols in the following string: \"" + std::string(argString) + "\"";
        }
        if (StartsWithDoubleDash(res->first) && res->first == "--" + mJsonConfigFlagName) {
            mOpts.AddOption(mJsonConfigFlagName, ppx::string_util::TrimBothEnds(res->second));
            continue;
        }
        args.emplace_back(res->first);
        args.emplace_back(res->second);
    }

    // Flags inside JSON files are processed first
    // These are always lower priority than flags on the command-line, will be "overwritten"
    std::vector<std::string> configJsonPaths;
    configJsonPaths = mOpts.GetOptionValueOrDefault(mJsonConfigFlagName, configJsonPaths);
    for (const auto& jsonPath : configJsonPaths) {
        // Attempt to open JSON file at specified path
        std::ifstream f(jsonPath);
        if (f.fail()) {
            return "Cannot locate file --" + mJsonConfigFlagName + ": " + jsonPath;
        }

        // Expect that JSON file specifies a valid JSON object
        PPX_LOG_INFO("Parsing JSON config file: " << jsonPath);
        nlohmann::json data;
        try {
            data = nlohmann::json::parse(f);
        }
        catch (nlohmann::json::parse_error& e) {
            PPX_LOG_ERROR("nlohmann::json::parse error: " << e.what() << '\n'
                                                          << "exception id: " << e.id << '\n'
                                                          << "byte position of error: " << e.byte);
        }
        if (!(data.is_object())) {
            return "The following config file could not be parsed as a JSON object: " + jsonPath;
        }

        // Attempt to add all options specified in this JSON object
        if (auto error = AddJsonOptions(data)) {
            return error;
        }
    }

    // Main pass, process arguments into either standalone flags or options with parameters.
    for (size_t i = 0; i < args.size(); ++i) {
        std::string_view name = ppx::string_util::TrimBothEnds(args[i]);
        if (!StartsWithDoubleDash(name)) {
            return "Invalid command-line option: \"" + std::string(name) + "\"";
        }
        name = name.substr(2);

        std::string_view value = "";
        if (i + 1 < args.size()) {
            auto nextElem = ppx::string_util::TrimBothEnds(args[i + 1]);
            if (!StartsWithDoubleDash(nextElem)) {
                // The next element is a parameter for the current option
                value = nextElem;
                ++i;
            }
        }

        if (auto error = AddOption(name, value)) {
            return error;
        }
    }

    return std::nullopt;
}

std::optional<CommandLineParser::ParsingError> CommandLineParser::AddJsonOptions(const nlohmann::json& jsonConfig)
{
    std::stringstream ss;
    for (auto it = jsonConfig.cbegin(); it != jsonConfig.cend(); ++it) {
        ss << it.value();
        std::string value = ss.str();
        ss.str("");

        if ((it.value()).is_array()) {
            // Special case, arrays specified in JSON are added directly to mOpts to avoid inserting element by element
            std::vector<std::string> jsonStringArray;
            for (const auto& elem : it.value()) {
                ss << elem;
                jsonStringArray.push_back(std::string(ppx::string_util::TrimBothEnds(ss.str(), " \t\"")));
                ss.str("");
            }
            mOpts.AddOption(it.key(), jsonStringArray);
        }
        else if (auto error = AddOption(it.key(), ppx::string_util::TrimBothEnds(value, " \t\""))) {
            return error;
        }
    }
    return std::nullopt;
}

std::optional<CommandLineParser::ParsingError> CommandLineParser::AddOption(std::string_view optionName, std::string_view valueStr)
{
    if (optionName.length() > 2 && optionName.substr(0, 3) == "no-") {
        // Special case where "no-flag-name" syntax is used
        if (valueStr.length() > 0) {
            return "invalid prefix no- for option \"" + std::string(optionName) + "\" and value \"" + std::string(valueStr) + "\"";
        }
        optionName = optionName.substr(3);
        valueStr   = "0";
    }
    mOpts.AddOption(optionName, valueStr);
    return std::nullopt;
}

} // namespace ppx
