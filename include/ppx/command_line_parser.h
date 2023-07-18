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

#include <cstdint>
#include <ios>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ppx {
// -------------------------------------------------------------------------------------------------
// StandardOptions
// -------------------------------------------------------------------------------------------------
struct StandardOptions
{
    // Flags
    bool deterministic          = false;
    bool enable_metrics         = false;
    bool headless               = false;
    bool list_gpus              = false;
    bool overwrite_metrics_file = false;
    bool use_software_renderer  = false;

    // Options
    std::vector<std::string> assets_paths            = {};
    int                      frame_count             = 0;
    int                      gpu_index               = -1;
    std::string              metrics_filename        = "";
    std::pair<int, int>      resolution              = {-1, -1};
    int                      run_time_ms             = 0;
    int                      screenshot_frame_number = -1;
    std::string              screenshot_path         = "";
    uint32_t                 stats_frame_window      = 300;
#if defined(PPX_BUILD_XR)
    std::pair<int, int> xrUIResolution = {-1, -1};
#endif

    bool operator==(const StandardOptions&) const = default;
};

// -------------------------------------------------------------------------------------------------
// CliOptions
// -------------------------------------------------------------------------------------------------

// All commandline flags are stored as key-value pairs (string, list of strings)
// Value syntax:
// - strings cannot contain "," or "="
// - boolean values stored as "0" "false" "1" "true"
//
// GetOptionValueOrDefault() can be used to access value of specified type.
// If requesting a single element from a list, will use the last one.
class CliOptions
{
public:
    CliOptions() = default;

    bool HasExtraOption(const std::string& option) const
    {
        return allOptions.find(option) != allOptions.end();
    }

    size_t GetNumOptions() const { return allOptions.size(); }

    // Try to parse option string into the type of the default value and return it.
    // If the value fails to be converted, return the specified default value.
    template <typename T>
    T GetOptionValueOrDefault(const std::string& optionName, const T& defaultValue) const
    {
        if (allOptions.find(optionName) == allOptions.cend()) {
            return defaultValue;
        }
        auto valueStr = allOptions.at(optionName).back();
        return GetParsedOrDefault<T>(valueStr, defaultValue);
    }

    // Intended for list flags, specified on command line with comma-separated parameters and/or multiple instances of same flag
    template <typename T1>
    std::vector<T1> GetOptionValueOrDefault(const std::string& optionName, const std::vector<T1>& defaultValues) const
    {
        if (allOptions.find(optionName) == allOptions.cend()) {
            return defaultValues;
        }
        std::vector<T1> values;
        auto            valueList = allOptions.at(optionName);
        T1              nullValue{};
        for (size_t i = 0; i < valueList.size(); ++i) {
            std::string valueStr = "";
            values.emplace_back(GetParsedOrDefault<T1>(valueList[i], nullValue));
        }
        return values;
    }

    // Intended for resolution flags, specified on command line with <Width>x<Height>
    std::pair<int, int> GetOptionValueOrDefault(const std::string& optionName, const std::pair<int, int>& defaultValue) const;

    // (DEPRECATED)
    // Get the parameter value after converting it into the desired integral,
    // floating-point, or boolean type. If the value fails to be converted,
    // return the specified default value.
    template <typename T2>
    T2 GetExtraOptionValueOrDefault(const std::string& optionName, const T2& defaultValue) const
    {
        static_assert(std::is_integral_v<T2> || std::is_floating_point_v<T2> || std::is_same_v<T2, std::string>, "GetExtraOptionValueOrDefault must be called with an integral, floating-point, boolean, or std::string type");

        return GetOptionValueOrDefault<T2>(optionName, defaultValue);
    }

private:
    // Add new option unless an option with this name already exists
    void
    AddOption(const std::string& key, const std::string& value);

    template <typename T3>
    T3 GetParsedOrDefault(const std::string& valueStr, const T3& defaultValue) const
    {
        static_assert(std::is_integral_v<T3> || std::is_floating_point_v<T3> || std::is_same_v<T3, std::string>, "GetParsedOrDefault must be called with an integral, floating-point, boolean, or std::string type");
        if constexpr (std::is_same_v<T3, std::string>) {
            return valueStr == "" ? defaultValue : valueStr;
        }
        else if constexpr (std::is_same_v<T3, bool>) {
            return GetParsedBoolOrDefault(valueStr, defaultValue);
        }
        return GetParsedNumberOrDefault<T3>(valueStr, defaultValue);
    }

    // For boolean parameters
    //   interpreted as true: "true", 1, ""
    //   interpreted as false: "false", 0
    bool GetParsedBoolOrDefault(const std::string& valueStr, bool defaultValue) const;

    template <typename T4>
    T4 GetParsedNumberOrDefault(const std::string& valueStr, const T4 defaultValue) const
    {
        std::stringstream ss{valueStr};
        T4                valueAsNum;
        ss >> valueAsNum;
        if (ss.fail()) {
            return defaultValue;
        }
        return valueAsNum;
    }

private:
    // All flag names (string) and parameters (vector of strings) specified on the command line
    std::unordered_map<std::string, std::vector<std::string>> allOptions;

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
    const CliOptions&           GetOptions() const { return mOpts; }
    const StandardOptions&      GetStandardOptions() const { return mStandardOpts; }
    std::string                 GetUsageMsg() const { return mUsageMsg; }
    void                        AppendUsageMsg(const std::string& additionalMsg) { mUsageMsg += additionalMsg; }
    bool                        GetPrintHelp() const { return mPrintHelp; }

private:
    CliOptions      mOpts;
    StandardOptions mStandardOpts;
    bool            mPrintHelp = false;
    std::string     mUsageMsg  = R"(
USAGE
==============================
Boolean options can be turned on with:
  --flag-name true, --flag-name 1, --flag-name
And turned off with:
  --flag-name false, --flag-name 0, --no-flag-name
==============================
--help              Prints this help message and exits.

--assets-path       Add a path in front of the assets search path list (Can be
                    specified multiple times).

--deterministic     Disable non-deterministic behaviors, like clocks and
                    diagnostic display.

--enable-metrics    Enable metrics report output. See also:
                    `--metrics-filename` and `--overwrite-metrics-file`.

--frame-count <N>   Shutdown the application after successfully rendering N
                    frames. Default: 0 (infinite).

--run-time-ms <N>   Shutdown the application after N milliseconds. Default: 0
                    (infinite).

--gpu <index>       Select the gpu with the given index. To determine the set
                    of valid indices use --list-gpus.

--headless          Run the sample without creating windows.

--list-gpus         Prints a list of the available GPUs on the current system
                    with their index and exits (see --gpu).

--metrics-filename  If metrics are enabled, save the metrics report to the
                    provided filename (including path). If used, any "@"
                    symbols in the filename (not the path) will be replaced
                    with the current timestamp. If the filename does not end in
                    .json, it will be appended. Default: "report_@". See also:
                    `--enable-metrics` and `--overwrite-metrics-file`.

--overwrite-metrics-file
                    Only applies if metrics are enabled with
                    `--enable-metrics`. If an existing file at the path set
                    with `--metrics-filename` is found, it will be overwritten.
                    Default: false. See also: `--enable-metrics` and
                    `--metrics-filename`.
)"
#if defined(PPX_BUILD_XR)
                            R"(
--resolution <Width>x<Height>
                    Specify the per-eye resolution in pixels. Width and Height
                    must be two positive integers greater or equal to 1.
)"
#else
                            R"(
--resolution <Width>x<Height>
                    Specify the main window resolution in pixels. Width and
                    Height must be two positive integers greater or equal to 1.
)"
#endif
                            R"(
--screenshot-frame-number <N>
                    Take a screenshot of frame number N and save it in PPM
                    format. See also `--screenshot-path`.

--screenshot-path   Save the screenshot to this destination. If not specified,
                    BigWheels will create a "screenshot_frameN" file in the
                    current working directory.

--stats-frame-window <N>
                    Calculate frame statistics over the last N frames only.
                    Default: 0 (use all frames since the beginning of the
                    application).

--use-software-renderer
                    Use a software renderer instead of a hardware device, if
                    available.
)"
#if defined(PPX_BUILD_XR)
                            R"(
--xr-ui-resolution <Width>x<Height>
                    Specify the UI quad resolution in pixels. Width and Height
                    must be two positive integers greater or equal to 1.
)"
#endif
        ; // mUsageMsg
};

} // namespace ppx

#endif // ppx_command_line_parser_h
