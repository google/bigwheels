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

#include "nlohmann/json.hpp"
#include "ppx/log.h"
#include "ppx/string_util.h"

#include <cstdint>
#include <ios>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ppx {

// -------------------------------------------------------------------------------------------------
// CliOptions
// -------------------------------------------------------------------------------------------------

// All commandline flags are stored as key-value pairs (string, list of strings)
// Value syntax:
// - strings cannot contain "=" or ","
// - boolean values stored as "0" "false" "1" "true"
//
// GetOptionValueOrDefault() can be used to access value of specified type.
// If requesting a single element from a list, will use the last one.
class CliOptions
{
public:
    CliOptions() = default;

    bool HasExtraOption(const std::string& option) const { return mAllOptions.find(option) != mAllOptions.end(); }

    // Returns the number of unique options and flags that were specified on the commandline,
    // not counting multiple appearances of the same flag such as: --assets-path a --assets-path b
    size_t GetNumUniqueOptions() const { return mAllOptions.size(); }

    // Tries to parse the option string into the type of the default value and return it.
    // If the value fails to be converted, return the specified default value.
    // Warning: If this is called instead of the vector overload for multiple-value flags,
    //          only the last value will be returned.
    template <typename T>
    T GetOptionValueOrDefault(const std::string& optionName, const T& defaultValue) const
    {
        auto it = mAllOptions.find(optionName);
        if (it == mAllOptions.cend()) {
            return defaultValue;
        }
        auto valueStr = it->second.back();
        T    parsedValue;
        auto error = ppx::string_util::Parse(valueStr, parsedValue);
        if (error != std::nullopt) {
            PPX_LOG_ERROR(error->errorMsg);
            return defaultValue;
        }
        return parsedValue;
    }

    // Same as above, but intended for list flags that are specified on the command line
    // with multiple instances of the same flag, or with comma-separated values
    template <typename T>
    std::vector<T> GetOptionValueOrDefault(const std::string& optionName, const std::vector<T>& defaultValues) const
    {
        auto it = mAllOptions.find(optionName);
        if (it == mAllOptions.cend()) {
            return defaultValues;
        }
        std::vector<T> parsedValues;
        for (size_t i = 0; i < it->second.size(); ++i) {
            T    parsedValue{};
            auto error = ppx::string_util::Parse(it->second.at(i), parsedValue);
            if (error != std::nullopt) {
                PPX_LOG_ERROR(error->errorMsg);
                return parsedValues;
            }
            parsedValues.emplace_back(parsedValue);
        }
        return parsedValues;
    }

    // (WILL BE DEPRECATED, USE KNOBS INSTEAD)
    // Get the parameter value after converting it into the desired integral,
    // floating-point, or boolean type. If the value fails to be converted,
    // return the specified default value.
    template <typename T>
    T GetExtraOptionValueOrDefault(const std::string& optionName, const T& defaultValue) const
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>, "GetExtraOptionValueOrDefault must be called with an integral, floating-point, boolean, or std::string type");

        return GetOptionValueOrDefault<T>(optionName, defaultValue);
    }

private:
    // Adds new option if the option does not already exist
    // Otherwise, the new value is appended to the end of the vector of stored parameters for this option
    void AddOption(std::string_view optionName, std::string_view value);

    // Same as above, but appends an array of values at the same key
    void AddOption(std::string_view optionName, const std::vector<std::string>& valueArray);

    // For all options existing in newOptions, current entries in mAllOptions will be replaced by them
    void OverwriteOptions(const CliOptions& newOptions);

private:
    // All flag names (string) and parameters (vector of strings) specified on the command line
    std::unordered_map<std::string, std::vector<std::string>> mAllOptions;

    friend class CommandLineParser;
};

// -------------------------------------------------------------------------------------------------
// CommandLineParser
// -------------------------------------------------------------------------------------------------
class CommandLineParser
{
public:
    // Parse the given arguments into options. Return false if parsing
    // succeeded. Otherwise, return true if an error occurred,
    // and write the error to `out_error`.
    std::optional<ppx::string_util::ParsingError> Parse(int argc, const char* argv[]);

    // Parses all options specified within jsonConfig and adds them to cliOptions.
    std::optional<ppx::string_util::ParsingError> ParseJson(CliOptions& cliOptions, const nlohmann::json& jsonConfig);

    // Parses an option, handles the special --no-flag-name case, then adds the option to cliOptions
    // Expects option names without the "--" prefix.
    std::optional<ppx::string_util::ParsingError> ParseOption(CliOptions& cliOptions, std::string_view optionName, std::string_view valueStr);

    std::string       GetJsonConfigFlagName() const { return mJsonConfigFlagName; }
    const CliOptions& GetOptions() const { return mOpts; }
    std::string       GetUsageMsg() const { return mUsageMsg; }

    void AppendUsageMsg(const std::string& additionalMsg) { mUsageMsg += additionalMsg; }

private:
    CliOptions  mOpts;
    std::string mJsonConfigFlagName = "config-json-path";
    std::string mUsageMsg           = R"(
USAGE
==============================
Boolean options can be turned on with:
  --flag-name true, --flag-name 1, --flag-name
And turned off with:
  --flag-name false, --flag-name 0, --no-flag-name

--help : Prints this help message and exits.
==============================
)";
};

} // namespace ppx

#endif // ppx_command_line_parser_h
