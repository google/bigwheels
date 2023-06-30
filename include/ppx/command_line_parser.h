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
    bool deterministic         = false;
    bool headless              = false;
    bool help                  = false;
    bool list_gpus             = false;
    bool use_software_renderer = false;

    // Options
    std::vector<std::string> assets_paths            = {};
    int                      gpu_index               = -1;
    int                      frame_count             = 0;
    int                      run_time_ms             = 0;
    std::pair<int, int>      resolution              = {-1, -1};
    int                      screenshot_frame_number = -1;
    std::string              screenshot_path         = "";
    uint32_t                 stats_frame_window      = 300;
#if defined(PPX_BUILD_XR)
    std::pair<int, int> xrUIResolution = {-1, -1};
#endif

    bool        operator==(const StandardOptions&) const = default;
};

// -------------------------------------------------------------------------------------------------
// CliOptions
// -------------------------------------------------------------------------------------------------
struct CliOptions
{
    struct Option
    {
    public:
        Option(const std::string& name, const std::string& value)
            : name(name), value(value) {}

        const std::string& GetName() const { return name; }
        bool               HasValue() const { return !value.empty(); }

        // Get the option value after converting it into the desired integral,
        // floating-point, or boolean type. If the value fails to be converted,
        // return the specified default value.
        template <typename T>
        T GetValueOrDefault(const T& defaultValue) const
        {
            static_assert(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>, "GetValueOrDefault must be called with an integral, floating-point, boolean, or std::string type");
            if constexpr (std::is_same_v<T, std::string>) {
                return value;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return GetValueAsBool(defaultValue);
            }

            std::stringstream ss{value};
            T                 val;
            ss >> val;
            if (ss.fail()) {
                return defaultValue;
            }
            return val;
        }

    private:
        // For boolean options, accept "true" and "false" as well as numeric values.
        bool GetValueAsBool(bool defaultValue) const
        {
            std::stringstream ss{value};
            bool              val;
            ss >> val;
            if (ss.fail()) {
                ss.clear();
                ss >> std::boolalpha >> val;
                if (ss.fail()) {
                    return defaultValue;
                }
            }
            return val;
        }

        std::string name;
        std::string value;
    };

public:
    const StandardOptions& GetStandardOptions() const
    {
        return standardOptions;
    }

    bool HasExtraOption(const std::string& option) const
    {
        return extraOptions.find(option) != extraOptions.end();
    }

    size_t GetNumExtraOptions() const { return extraOptions.size(); }

    // Get the option value after converting it into the desired integral,
    // floating-point, or boolean type. If the option does not exist or the
    // value fails to be converted, return the specified default value.
    template <typename T>
    T GetExtraOptionValueOrDefault(const std::string& option, const T& defaultValue) const
    {
        if (!HasExtraOption(option)) {
            return defaultValue;
        }
        return extraOptions.at(option).GetValueOrDefault<T>(defaultValue);
    }

private:
    void AddExtraOption(const Option& opt)
    {
        extraOptions.insert(std::make_pair(opt.GetName(), opt));
    }

    std::unordered_map<std::string, Option> extraOptions;
    StandardOptions                         standardOptions;

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
    std::string                 GetUsageMsg() const { return mUsageMsg; }
    void                        AppendUsageMsg(const std::string& additionalMsg)
    {
        mUsageMsg += additionalMsg;
    }

private:
    CliOptions mOpts;
#if defined(PPX_BUILD_XR)
    std::string mUsageMsg = R"(
--help                        Prints this help message and exits.

--assets-path                 Add a path in front of the assets search path list (Can be specified multiple times).
--deterministic               Disable non-deterministic behaviors, like clocks.
--frame-count <N>             Shutdown the application after successfully rendering N frames.
--run-time-ms <N>             Shutdown the application after N milliseconds.
--gpu <index>                 Select the gpu with the given index. To determine the set of valid indices use --list-gpus.
--headless                    Run the sample without creating windows.
--list-gpus                   Prints a list of the available GPUs on the current system with their index and exits (see --gpu).
--resolution <Width>x<Height> Specify the per-eye resolution in pixels. Width and Height must be two positive integers greater or equal to 1.
--xr-ui-resolution <Width>x<Height> Specify the UI quad resolution in pixels. Width and Height must be two positive integers greater or equal to 1.
--screenshot-frame-number <N> Take a screenshot of frame number N and save it in PPM format.
                              See also `--screenshot-path`.
--screenshot-path             Save the screenshot to this path. If not specified, BigWheels will create a
                              "screenshot_frameN" file in the current working directory.
--stats-frame-window <N>      Calculate frame statistics over the last N frames only.
                              Set to 0 to use all frames since the beginning of the application.
--use-software-renderer       Use a software renderer instead of a hardware device, if available.
)";
#else
    std::string mUsageMsg = R"(
--help                        Prints this help message and exits.

--assets-path                 Add a path in front of the assets search path list (Can be specified multiple times).
--deterministic               Disable non-deterministic behaviors, like clocks.
--frame-count <N>             Shutdown the application after successfully rendering N frames. Default: 0 (infinite).
--run-time-ms <N>             Shutdown the application after N milliseconds. Default: 0 (infinite).
--gpu <index>                 Select the gpu with the given index. To determine the set of valid indices use --list-gpus.
--headless                    Run the sample without creating windows.
--list-gpus                   Prints a list of the available GPUs on the current system with their index and exits (see --gpu).
--resolution <Width>x<Height> Specify the main window resolution in pixels. Width and Height must be two positive integers greater or equal to 1.
--screenshot-frame-number <N> Take a screenshot of frame number N and save it in PPM format.
                              See also `--screenshot-path`.
--screenshot-path             Save the screenshot to this path. If not specified, BigWheels will create a
                              "screenshot_frameN" file in the current working directory.
--stats-frame-window <N>      Calculate frame statistics over the last N frames only.
                              Set to 0 to use all frames since the beginning of the application.
--use-software-renderer       Use a software renderer instead of a hardware device, if available.
)";
#endif
};
} // namespace ppx

#endif // ppx_command_line_parser_h
