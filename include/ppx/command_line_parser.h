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

#ifndef ppx_command_line_parser_h
#define ppx_command_line_parser_h

#include <ios>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <type_traits>
#include <optional>

namespace ppx {

// -------------------------------------------------------------------------------------------------
// CliOptions
// -------------------------------------------------------------------------------------------------

class CliOptions
{
public:
    CliOptions() = default;

    bool HasExtraOption(const std::string& option) const
    {
        return allOptions.find(option) != allOptions.end();
    }

    size_t GetNumExtraOptions() const { return allOptions.size(); }

    // Get the parameter value after converting it into the desired integral,
    // floating-point, or boolean type. If the value fails to be converted,
    // return the specified default value.
    template <typename T>
    T GetExtraOptionValueOrDefault(const std::string& optionName, const T& defaultValue) const
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>, "GetValueOrDefault must be called with an integral, floating-point, boolean, or std::string type");

        // Return default value for all unspecified flags
        if (allOptions.find(optionName) == allOptions.cend()) {
            return defaultValue;
        }

        auto valueStr = allOptions.at(optionName);
        return GetParsedOrDefault(valueStr, defaultValue);
    }

    template <typename T1, typename T2>
    std::pair<T1, T2> GetIntPairValueOrDefault(const std::string& optionName, const std::pair<T1, T2>& defaultValue) const
    {
        // Return default value for all unspecified flags
        if (allOptions.find(optionName) == allOptions.cend()) {
            return defaultValue;
        }

        // valueStr in format NxM
        auto   valueStr = allOptions.at(optionName);
        size_t xIndex   = valueStr.find("x");
        if (xIndex == std::string::npos) {
            return defaultValue;
        }
        std::string substringN = valueStr.substr(0, xIndex);
        std::string substringM = valueStr.substr(xIndex + 1);
        T1          N          = GetParsedOrDefault(substringN, defaultValue.first);
        T2          M          = GetParsedOrDefault(substringM, defaultValue.second);
        return std::make_pair(N, M);
    }

private:
    // Add new option unless an option with this name already exists
    void AddOption(const std::string& key, const std::string& value);

    // For boolean parameters
    //   interpreted as true: "true", 1, ""
    //   interpreted as false: "false", 0
    bool GetParsedBoolOrDefault(const std::string& value, bool defaultValue) const;

    template <typename T1>
    T1 GetParsedNumberOrDefault(const std::string& value, T1 defaultValue) const
    {
        std::stringstream ss{value};
        T1                valueAsNum;
        ss >> valueAsNum;
        if (ss.fail()) {
            return defaultValue;
        }
        return valueAsNum;
    }

    template <typename T>
    T GetParsedOrDefault(const std::string& valueStr, const T& defaultValue) const
    {
        if constexpr (std::is_same_v<T, std::string>) {
            return valueStr == "" ? defaultValue : valueStr;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return GetParsedBoolOrDefault(valueStr, defaultValue);
        }
        return GetParsedNumberOrDefault(valueStr, defaultValue);
    }

    std::unordered_map<std::string, std::string> allOptions;

    friend class CommandLineParser;
};

// -------------------------------------------------------------------------------------------------
// CommandLineParser
// -------------------------------------------------------------------------------------------------
class CommandLineParser
{
public:
    struct ParsingError
    {
        ParsingError(const std::string& error)
            : errorMsg(error) {}
        std::string errorMsg;
    };

    // Parse the given arguments into options. Return false if parsing
    // succeeded. Otherwise, return true if an error occurred,
    // and write the error to `out_error`.
    std::optional<ParsingError> Parse(int argc, const char* argv[]);
    std::optional<ParsingError> ParseJsonConfig(const std::string& jsonConfigPath);

    const CliOptions& GetOptions() const { return mOpts; }
    std::string       GetUsageMsg() const { return mUsageMsg; }
    void              AppendUsageMsg(const std::string& additionalMsg) { mUsageMsg += additionalMsg; }
    bool              GetPrintHelp() const { return mPrintHelp; }

private:
    CliOptions  mOpts;
    bool        mPrintHelp = false;
    std::string mUsageMsg  = R"(
USAGE
==============================
Options can be turned on with:
  --flag_name true
  --flag_name

And turned off with:
  --flag_name false
  --no-flag_name
==============================

--help  Prints this help message and exits.  
)";
};
} // namespace ppx

#endif // ppx_command_line_parser_h
