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
#include "ppx/string_util.h"

#include "nlohmann/json.hpp"

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

void CliOptions::AddOption(const std::string& key, const std::string& value)
{
    if (allOptions.find(key) != allOptions.cend()) {
        return;
    }
    allOptions.insert(std::make_pair(key, value));
}

bool CliOptions::GetParsedBoolOrDefault(const std::string& value, bool defaultValue) const
{
    if (value == "") {
        return true;
    }
    std::stringstream ss{value};
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

    // Process arguments into either standalone flags or options with parameters.
    // For boolean flags, accepted formats:
    // --flag-name, --flag-name true
    // --no-flag-name, --flag-name false
    std::vector<std::string> args(argv + 1, argv + argc);
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
            mOpts.AddOption(name, "0");
        }
        else {
            // Option has no parameter, likely a boolean flag but do not assign "true"
            // in case it is a non-boolean string flag that is missing a parameter
            mOpts.AddOption(name, "");
        }
    }
    return std::nullopt;
}

std::optional<CommandLineParser::ParsingError> CommandLineParser::ParseJsonConfig(const std::string& jsonFilePath)
{
    std::ifstream  f(jsonFilePath);
    nlohmann::json jsonStruct = nlohmann::json::parse(f);

    // Flattening the JSON structure to fit in mUnorganizedOpts
    // Storing any nested objects and arrays as strings
    for (auto it = jsonStruct.cbegin(); it != jsonStruct.cend(); ++it) {
        std::stringstream ss;
        ss << it.value();
        std::string value = ss.str();
        if (value.back() == '"' && value.at(0) == '"') {
            value.erase(value.begin());
            value.pop_back();
        }
        mOpts.AddOption(it.key(), value);
    }
    return std::nullopt;
}

} // namespace ppx
