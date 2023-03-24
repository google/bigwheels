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
#include "ppx/string_util.h"

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

std::optional<CommandLineParser::ParsingError> CommandLineParser::Parse(int argc, const char* argv[])
{
    // argc is always >= 1 and argv[0] is the name of the executable.
    if (argc < 2) {
        return std::nullopt;
    }

    // Process arguments into either standalone flags or
    // options with parameters.
    std::vector<std::string>        args(argv + 1, argv + argc);
    std::vector<CliOptions::Option> options;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string name = ppx::string_util::TrimCopy(args[i]);
        if (!IsOptionOrFlag(name)) {
            return "Invalid command-line option " + name;
        }
        name = name.substr(2);

        std::string parameter = (i + 1 < args.size()) ? ppx::string_util::TrimCopy(args[i + 1]) : "";
        if (!IsOptionOrFlag(parameter)) {
            // We found an option with a parameter.
            options.emplace_back(name, parameter);
            ++i;
        }
        else {
            options.emplace_back(name, "");
        }
    }

    for (const auto& opt : options) {
        // Process standard options.
        if (opt.GetName() == "help") {
            mOpts.standardOptions.help = true;
        }
        else if (opt.GetName() == "list-gpus") {
            mOpts.standardOptions.list_gpus = true;
        }
        else if (opt.GetName() == "use-software-renderer") {
            mOpts.standardOptions.use_software_renderer = true;
        }
        else if (opt.GetName() == "headless") {
            mOpts.standardOptions.headless = true;
        }
        else if (opt.GetName() == "gpu") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --gpu requires a parameter");
            }
            mOpts.standardOptions.gpu_index = opt.GetValueOrDefault<int>(-1);
            if (mOpts.standardOptions.gpu_index < 0) {
                return std::string("Command-line option --gpu requires a positive integer as the parameter");
            }
        }
        else if (opt.GetName() == "resolution") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --resolution requires a parameter");
            }

            // Resolution is passed as <Width>x<Height>.
            std::string       val = opt.GetValueOrDefault<std::string>("");
            std::stringstream ss{val};
            int               width = -1, height = -1;
            char              x;
            ss >> width >> x >> height;
            if (ss.fail() || x != 'x') {
                return std::string("Parameter for command-line option --resolution must be in <Width>x<Height> format, got " + val + " instead");
            }
            if (width < 1 || height < 1) {
                return std::string("Parameter for command-line option --resolution must be in <Width>x<Height> format where Width and Height are integers greater or equal to 1");
            }

            mOpts.standardOptions.resolution = {width, height};
        }
        else if (opt.GetName() == "frame-count") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --frame-count requires a parameter");
            }
            mOpts.standardOptions.frame_count = opt.GetValueOrDefault<int>(-1);
        }
        else if (opt.GetName() == "stats-frame-window") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --stats-frame-window requires a parameter");
            }
            mOpts.standardOptions.stats_frame_window = opt.GetValueOrDefault<uint32_t>(300);
        }
        else if (opt.GetName() == "screenshot-frame-number") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --screenshot-frame-number requires a parameter");
            }
            mOpts.standardOptions.screenshot_frame_number = opt.GetValueOrDefault<int>(-1);
        }
        else if (opt.GetName() == "screenshot-out-dir") {
            if (!opt.HasValue()) {
                return std::string("Command-line option --screenshot-out-dir requires a parameter");
            }
            mOpts.standardOptions.screenshot_out_dir = opt.GetValueOrDefault<std::string>("");
        }
        else {
            // Non-standard option.
            mOpts.AddExtraOption(opt);
        }
    }

    return std::nullopt;
}

} // namespace ppx