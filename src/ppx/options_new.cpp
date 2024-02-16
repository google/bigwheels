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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <locale>
#include <vector>
#include <stdexcept>
#include <cctype>

#include "ppx/options_new.h"

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

// -------------------------------------------------------------------------------------------------
// OptionsNew
// -------------------------------------------------------------------------------------------------
void OptionsNew::OverwriteOptions(const OptionsNew& newOptions)
{
    for (auto& it : newOptions.mAllOptions) {
        mAllOptions[it.first] = it.second;
    }
}

void OptionsNew::AddOption(std::string_view optionName, std::string_view value)
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

void OptionsNew::AddOption(std::string_view optionName, const std::vector<std::string>& valueArray)
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

// -------------------------------------------------------------------------------------------------
// CommandLineParserNew
// -------------------------------------------------------------------------------------------------

Result CommandLineParserNew::ParseOptions(int argc, const char* argv[], OptionsNew& options)
{
    // argc should be >= 1 and argv[0] the name of the executable.
    if (argc < 2) {
        return SUCCESS;
    }

    // Initial pass to trim the name of executable and to split any flag and parameters that are connected with '='
    std::vector<std::string_view> args;
    for (size_t i = 1; i < argc; ++i) {
        std::string_view argString(argv[i]);
        if (argString.find('=') != std::string_view::npos) {
            if (std::count(argString.cbegin(), argString.cend(), '=') != 1) {
                PPX_LOG_ERROR("invalid number of '=' in flag: \"" << argString << "\"");
                return ERROR_FAILED;
            }
            std::pair<std::string_view, std::string_view> optionAndValue = ppx::string_util::SplitInTwo(argString, '=');
            args.emplace_back(optionAndValue.first);
            args.emplace_back(optionAndValue.second);
            continue;
        }
        args.emplace_back(argString);
    }

    // Another pass to identify JSON config files, add that option, and remove it from the argument list
    std::vector<std::string_view> argsFiltered;
    std::vector<std::string>      jsonConfigFilePaths;
    for (size_t i = 0; i < args.size(); ++i) {
        bool nextArgumentIsParameter = (i + 1 < args.size()) && !StartsWithDoubleDash(args[i + 1]);
        if ((args[i] == "--" + mJsonConfigFlagName) && nextArgumentIsParameter) {
            jsonConfigFilePaths.emplace_back(ppx::string_util::TrimBothEnds(args[i + 1]));
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
    JsonConverterNew         jsonConverter;
    OptionsNew               jsonOptions;
    for (const auto& jsonPath : jsonConfigFilePaths) {
        auto res = jsonConverter.ParseOptionsFromFile(jsonPath, jsonOptions);
        if (Failed(res)) {
            return res;
        }
        options.OverwriteOptions(jsonOptions);
    }

    OptionsNew commandlineOptions;
    // Main pass, process arguments into either standalone flags or options with parameters.
    for (size_t i = 0; i < args.size(); ++i) {
        std::string_view name = ppx::string_util::TrimBothEnds(args[i]);
        if (!StartsWithDoubleDash(name)) {
            PPX_LOG_ERROR("Invalid command-line option: \"" << name << "\"");
            return ERROR_FAILED;
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
        auto res = AddOption(commandlineOptions, name, value);
        if (Failed(res)) {
            return res;
        }
    }
    options.OverwriteOptions(commandlineOptions);

    return SUCCESS;
}

Result CommandLineParserNew::AddOption(OptionsNew& opts, std::string_view optionName, std::string_view valueStr)
{
    if (optionName.length() > 2 && optionName.substr(0, 3) == "no-") {
        if (valueStr.length() > 0) {
            PPX_LOG_ERROR("invalid prefix no- for option \"" << optionName << "\" and value \"" << valueStr << "\"");
            return ERROR_FAILED;
        }
        optionName = optionName.substr(3);
        valueStr   = "0";
    }

    opts.AddOption(optionName, valueStr);
    return SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// JsonConverterNew
// -------------------------------------------------------------------------------------------------

Result JsonConverterNew::ParseOptionsFromFile(const std::string& jsonPath, OptionsNew& opts)
{
    std::ifstream f(jsonPath);
    if (f.fail()) {
        PPX_LOG_ERROR("Cannot locate JSON file : " << jsonPath);
        return ERROR_FAILED;
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
        PPX_LOG_ERROR("The following config file could not be parsed as a JSON object: " << jsonPath);
        return ERROR_FAILED;
    }

    ParseOptions(data, opts);

    return SUCCESS;
}

Result JsonConverterNew::ExportOptionsToFile(const OptionsNew& options, const std::string& jsonPath)
{
    std::ofstream f(jsonPath);
    if (f.fail()) {
        PPX_LOG_ERROR("Cannot write to JSON file : " << jsonPath);
        return ERROR_FAILED;
    }

    nlohmann::json data;
    ExportOptions(options, data);
    f << data.dump(0);
    f.close();

    return SUCCESS;
}

void JsonConverterNew::ParseOptions(const nlohmann::json& jsonConfig, OptionsNew& opts)
{
    std::stringstream ss;
    for (auto it = jsonConfig.cbegin(); it != jsonConfig.cend(); ++it) {
        if ((it.value()).is_array()) {
            // Special case, arrays specified in JSON are added directly to opts to avoid inserting element by element
            std::vector<std::string> jsonStringArray;
            for (const auto& elem : it.value()) {
                ss << elem;
                jsonStringArray.push_back(std::string(ppx::string_util::TrimBothEnds(ss.str(), " \t\"")));
                ss.str("");
            }
            opts.AddOption(it.key(), jsonStringArray);
            continue;
        }

        ss << it.value();
        std::string value = ss.str();
        ss.str("");
        opts.AddOption(it.key(), ppx::string_util::TrimBothEnds(value, " \t\""));
    }
}

void JsonConverterNew::ExportOptions(const OptionsNew& options, nlohmann::json& jsonConfig)
{
    auto optMap = options.GetMap();
    for (const auto& iter : optMap) {
        jsonConfig[iter.first] = ppx::string_util::ToString(iter.second);
    }
}

} // namespace ppx
