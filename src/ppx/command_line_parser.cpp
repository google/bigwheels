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

void CliOptions::OverwriteOptions(const CliOptions& newOptions)
{
    for (auto& it : newOptions.mAllOptions) {
        mAllOptions[it.first] = it.second;
    }
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

std::optional<ppx::string_util::ParsingError> CommandLineParser::Parse(int argc, const char* argv[])
{
    // argc should be >= 1 and argv[0] the name of the executable.
    if (argc < 2) {
        return std::nullopt;
    }

    // Initial pass to trim the name of executable and to split any flag and parameters that are connected with '='
    std::vector<std::string_view> args;
    for (size_t i = 1; i < argc; ++i) {
        std::string_view argString(argv[i]);
        if (argString.find('=') != std::string_view::npos) {
            auto res = ppx::string_util::SplitInTwo(argString, '=');
            if (res == std::nullopt) {
                return "Malformed flag with '=': \"" + std::string(argString) + "\"";
            }
            args.emplace_back(res->first);
            args.emplace_back(res->second);
            continue;
        }
        args.emplace_back(argString);
    }

    // Another pass to identify JSON config files, add that option, and remove it from the argument list
    std::vector<std::string_view> argsFiltered;
    for (size_t i = 0; i < args.size(); ++i) {
        bool nextArgumentIsParameter = (i + 1 < args.size()) && !StartsWithDoubleDash(args[i + 1]);
        if ((args[i] == "--" + mJsonConfigFlagName) && nextArgumentIsParameter) {
            mOpts.AddOption(mJsonConfigFlagName, ppx::string_util::TrimBothEnds(args[i + 1]));
            ++i;
            continue;
        }
        argsFiltered.emplace_back(args[i]);
    }
    args = argsFiltered;
    argsFiltered.clear();

    // Flags inside JSON files are processed first
    // These are always lower priority than flags on the command-line
    std::vector<std::string> configJsonPaths;
    configJsonPaths = mOpts.GetOptionValueOrDefault(mJsonConfigFlagName, configJsonPaths);
    for (const auto& jsonPath : configJsonPaths) {
        std::ifstream f(jsonPath);
        if (f.fail()) {
            return "Cannot locate file --" + mJsonConfigFlagName + ": " + jsonPath;
        }

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

        CliOptions jsonOptions;
        if (auto error = ParseJson(jsonOptions, data)) {
            return error;
        }
        mOpts.OverwriteOptions(jsonOptions);
    }

    CliOptions commandlineOptions;
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

        if (auto error = ParseOption(commandlineOptions, name, value)) {
            return error;
        }
    }
    mOpts.OverwriteOptions(commandlineOptions);

    return std::nullopt;
}

std::optional<ppx::string_util::ParsingError> CommandLineParser::ParseJson(CliOptions& cliOptions, const nlohmann::json& jsonConfig)
{
    std::stringstream ss;
    for (auto it = jsonConfig.cbegin(); it != jsonConfig.cend(); ++it) {
        if ((it.value()).is_array()) {
            // Special case, arrays specified in JSON are added directly to cliOptions to avoid inserting element by element
            std::vector<std::string> jsonStringArray;
            for (const auto& elem : it.value()) {
                ss << elem;
                jsonStringArray.push_back(std::string(ppx::string_util::TrimBothEnds(ss.str(), " \t\"")));
                ss.str("");
            }
            cliOptions.AddOption(it.key(), jsonStringArray);
            continue;
        }

        ss << it.value();
        std::string value = ss.str();
        ss.str("");
        cliOptions.AddOption(it.key(), ppx::string_util::TrimBothEnds(value, " \t\""));
    }
    return std::nullopt;
}

std::optional<ppx::string_util::ParsingError> CommandLineParser::ParseOption(CliOptions& cliOptions, std::string_view optionName, std::string_view valueStr)
{
    if (optionName.length() > 2 && optionName.substr(0, 3) == "no-") {
        if (valueStr.length() > 0) {
            return "invalid prefix no- for option \"" + std::string(optionName) + "\" and value \"" + std::string(valueStr) + "\"";
        }
        optionName = optionName.substr(3);
        valueStr   = "0";
    }

    if (valueStr.find(',') != std::string_view::npos) {
        auto substringArray = ppx::string_util::Split(valueStr, ',');
        if (substringArray == std::nullopt) {
            return "invalid comma use for option \"" + std::string(optionName) + "\" and value \"" + std::string(valueStr) + "\"";
        }
        // Special case, comma-separated value lists specified on the commandline are added directly to cliOptions to avoid inserting element by element
        std::vector<std::string> substringStringArray(substringArray->cbegin(), substringArray->cend());
        cliOptions.AddOption(optionName, substringStringArray);
        return std::nullopt;
    }

    cliOptions.AddOption(optionName, valueStr);
    return std::nullopt;
}

} // namespace ppx
