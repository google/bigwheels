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

#ifndef ppx_options_new_h
#define ppx_options_new_h

#include "nlohmann/json.hpp"
#include "ppx/config.h"
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
// OptionsNew
// -------------------------------------------------------------------------------------------------
//
// OptionsNew is an intermediate format structure consisting of an unordered map of key-value pairs
// that contain information used in knobs, command line flags, and JSON config files
//
// keys (string): The knob's unique flag name
// values (list of strings): Values that have been specified for those keys from low to high priority
//
class OptionsNew
{
public:
    OptionsNew() = default;
    OptionsNew(const std::unordered_map<std::string, std::vector<std::string>>& newOptions) { mAllOptions = newOptions; }

    bool HasOption(const std::string& option) const { return mAllOptions.find(option) != mAllOptions.end(); }

    size_t GetNumUniqueOptions() const { return mAllOptions.size(); }

    std::unordered_map<std::string, std::vector<std::string>> GetMap() const { return mAllOptions; }

    std::vector<std::string> GetValueStrings(const std::string& optionName) const { return mAllOptions.at(optionName); }

    // Adds new option if the option does not already exist
    // Otherwise, the new value is appended to the end of the vector of stored parameters for this option
    void AddOption(std::string_view optionName, std::string_view value);

    // Same as above, but appends an array of values at the same key
    void AddOption(std::string_view optionName, const std::vector<std::string>& valueArray);

    // For all options existing in newOptions, current entries in mAllOptions will be replaced by them
    void OverwriteOptions(const OptionsNew& newOptions);

    bool operator==(const OptionsNew& rhs) const
    {
        return (mAllOptions == rhs.GetMap());
    }

private:
    // All flag names (string) and parameters (vector of strings) specified on the command line
    std::unordered_map<std::string, std::vector<std::string>> mAllOptions;
};

// -------------------------------------------------------------------------------------------------
// CommandLineParserNew
// -------------------------------------------------------------------------------------------------
//
// CommandLineParserNew can parse command-line arguments into OptionsNew intermediate format.
// This includes any JSON config files specified with the flag mJsonConfigFlagName.
//
class CommandLineParserNew
{
public:
    // Parse the given arguments into options. Return false if parsing
    // succeeded. Otherwise, return true if an error occurred,
    // and write the error to `out_error`.
    //
    // Value syntax:
    // - strings cannot contain "="
    // - boolean values stored as "0", "false", "1", "true", ""
    Result ParseOptions(int argc, const char* argv[], OptionsNew& options);

    std::string GetJsonConfigFlagName() const { return mJsonConfigFlagName; }

private:
    // Adds one instance of an option to the OptionsNew
    // Checks for the special no- prefix allowed for the command line flags
    Result AddOption(OptionsNew& opts, std::string_view optionName, std::string_view valueStr);

private:
    std::string mJsonConfigFlagName = "config-json-path";
};

// -------------------------------------------------------------------------------------------------
// JsonConverterNew
// -------------------------------------------------------------------------------------------------
//
// JsonConverterNew can convert JSON structures and files to and from OptionsNew intermediate format.
//
class JsonConverterNew
{
public:
    // Parses all options specified within the JSON file and adds them to options.
    Result ParseOptionsFromFile(const std::string& jsonPath, OptionsNew& options);

    // Creates/overwrites file at jsonPath with all options in JSON format
    Result ExportOptionsToFile(const OptionsNew& options, const std::string& jsonPath);

    // Exposed for testing
    void ParseOptions(const nlohmann::json& jsonConfig, OptionsNew& options);
    void ExportOptions(const OptionsNew& options, nlohmann::json& jsonConfig);
};

} // namespace ppx

#endif // ppx_options_new_h
