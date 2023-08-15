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

#ifndef PPX_STRING_UTIL_H
#define PPX_STRING_UTIL_H

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ppx {
namespace string_util {

// -------------------------------------------------------------------------------------------------
// Misc
// -------------------------------------------------------------------------------------------------

void TrimLeft(std::string& s);
void TrimRight(std::string& s);

std::string TrimCopy(const std::string& s);

// Trims all characters specified in c from both the left and right sides of s
std::string_view TrimBothEnds(std::string_view s, std::string_view c = " \t");

// Splits s at every instance of delimeter and returns a vector of substrings
// Returns std::nullopt if s contains: leading/trailing/consecutive delimiters
std::optional<std::vector<std::string_view>> Split(std::string_view s, char delimiter);

// Splits s at the first instance of delimeter and returns two substrings
// Returns std::nullopt if s is not in expected format of string-delimeter-string
std::optional<std::pair<std::string_view, std::string_view>> SplitInTwo(std::string_view s, char delimiter);

// -------------------------------------------------------------------------------------------------
// Formatting Strings
// -------------------------------------------------------------------------------------------------

// Formats string for printing with the specified width and left indent.
// Words will be pushed to the subsequent line to avoid line breaks in the
// middle of a word if possible.
// Leading and trailing whitespace is trimmed from each line.
std::string WrapText(const std::string& s, size_t width, size_t indent = 0);

// Provides string representation of value for printing or display
template <typename T>
std::string ToString(T value)
{
    std::stringstream ss;
    if constexpr (std::is_same_v<T, bool>) {
        ss << std::boolalpha;
    }
    ss << value;
    return ss.str();
}

// Same as above, for vectors
template <typename T>
std::string ToString(std::vector<T> values)
{
    std::stringstream ss;
    for (const auto& value : values) {
        ss << ToString<T>(value) << ", ";
    }
    std::string valueStr = ss.str();
    return valueStr.substr(0, valueStr.length() - 2);
}

// Same as above, for pairs
template <typename T>
std::string ToString(std::pair<T, T> values)
{
    std::stringstream ss;
    ss << ToString<T>(values.first) << ", " << ToString<T>(values.second);
    return ss.str();
}

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

struct ParsingError
{
    ParsingError(const std::string& error)
        : errorMsg(error) {}
    std::string errorMsg;
};

// ParseOrDefault() attempts to parse valueStr into the same type as defaultValue
// If successful, returns the parsed value and std::nullopt
// If unsucessful, returns defaultValue and ParsingError

// For strings
// e.g. "a string" -> "a string"
std::pair<std::string, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, const std::string& defaultValue);
std::pair<std::string, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, std::string_view defaultValue);

// For bool
// e.g. "true", "1", "" -> true
// e.g. "false", "0" -> false
std::pair<bool, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, bool defaultValue);

// For integers, chars and floats
// e.g. "1.0" -> 1.0f
// e.g. "-20" -> -20
// e.g. "c" -> 'c'
template <typename T>
std::pair<T, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, T defaultValue)
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Attempted to parse invalid type for ParseOrDefault");

    std::stringstream ss((std::string(valueStr)));
    T                 valueAsNum;
    ss >> valueAsNum;
    if (ss.fail()) {
        return std::make_pair(defaultValue, "could not be parsed as integral or float: " + std::string(valueStr));
    }
    return std::make_pair(valueAsNum, std::nullopt);
}

// For lists with comma-separated string representation
// e.g. "i1,i2,i3 with spaces,i4" -> {"i1", "i2", "i3 with spaces", "i4"}
template <typename T>
std::pair<typename std::vector<T>, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, const std::vector<T>& defaultValues)
{
    std::vector<std::string> splitStrings;
    auto                     res = Split(valueStr, ',');
    if (res == std::nullopt) {
        // String contains no commas
        splitStrings.emplace_back(valueStr);
    }
    else {
        for (const auto sv : res.value()) {
            splitStrings.emplace_back(std::string(sv));
        }
    }

    std::vector<T> parsedValues;
    T              nullValue{};
    for (const auto& singleStr : splitStrings) {
        auto res = ParseOrDefault(singleStr, nullValue);
        if (res.second != std::nullopt) {
            return std::make_pair(defaultValues, res.second);
        }
        parsedValues.emplace_back(res.first);
    }
    return std::make_pair(parsedValues, std::nullopt);
}

// For resolution with x-separated string representation
// e.g. "600x800" -> (600, 800)
std::pair<std::pair<int, int>, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, const std::pair<int, int>& defaultValue);

} // namespace string_util
} // namespace ppx

#endif // PPX_STRING_UTIL_H