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

#ifndef PPX_STRING_UTIL_H
#define PPX_STRING_UTIL_H

#include "ppx/config.h"

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ppx {
namespace string_util {

// -------------------------------------------------------------------------------------------------
// Misc
// -------------------------------------------------------------------------------------------------

std::string ToLowerCopy(const std::string& s);
std::string ToUpperCopy(const std::string& s);

void TrimLeft(std::string& s);
void TrimRight(std::string& s);

std::string TrimCopy(const std::string& s);

// Trims all characters specified in c from both the left and right sides of s
std::string_view TrimBothEnds(std::string_view s, std::string_view c = " \t");

// Splits s at the first instance of delimiter and returns two substrings
// Returns an empty second element if there is no delimiter
std::pair<std::string_view, std::string_view> SplitInTwo(std::string_view s, char delimiter);

// Splits s at all instances of delimiter and returns N substrings
// If the delimiter is at the beginning or the end of the string or right beside another delimiter,
// empty strings can be returned.
std::vector<std::string_view> Split(std::string_view s, char delimiter);

// -------------------------------------------------------------------------------------------------
// Formatting Strings
// -------------------------------------------------------------------------------------------------

// Formats string for printing with the specified width and left indent.
// Words will be pushed to the subsequent line to avoid line breaks in the
// middle of a word if possible.
// Leading and trailing whitespace is trimmed from each line.
std::string WrapText(const std::string& s, size_t width, size_t indent = 0);

// -------------------------------------------------------------------------------------------------
// Converting to Strings
// -------------------------------------------------------------------------------------------------

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
        ss << ToString<T>(value) << ",";
    }
    std::string valueStr = ss.str();
    return valueStr.substr(0, valueStr.length() - 1);
}

// Same as above, for pairs
template <typename T>
std::string ToString(std::pair<T, T> values)
{
    std::stringstream ss;
    ss << ToString<T>(values.first) << "," << ToString<T>(values.second);
    return ss.str();
}

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

// Parse() attempts to parse valueStr into the specified type
// If successful, overwrites parsedValue and returns std::nullopt
// If unsucessful, does not overwrite parsedValue, logs error and returns ERROR_FAILED instead

// For strings
// e.g. "a string" -> "a string"
Result Parse(std::string_view valueStr, std::string& parsedValue);

// For bool
// e.g. "true", "1", "" -> true
// e.g. "false", "0" -> false
Result Parse(std::string_view valueStr, bool& parsedValue);

// For integers, chars and floats
// e.g. "1.0" -> 1.0f or 1
// e.g. "-20" -> -20
// e.g. "c" -> 'c'
template <typename T>
Result Parse(std::string_view valueStr, T& parsedValue)
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "attempting to parse unsupported type");

    std::stringstream ss((std::string(valueStr)));
    T                 valueAsNum;
    ss >> valueAsNum;
    if (ss.fail()) {
        PPX_LOG_ERROR("could not be parsed as integral or float: " << valueStr);
        return ERROR_FAILED;
    }
    parsedValue = valueAsNum;

    if (std::is_integral_v<T> && (valueStr.find('.') != std::string_view::npos)) {
        PPX_LOG_WARN("value string is truncated: " << valueStr);
    }

    return SUCCESS;
}

// For resolution with x-separated string representation
// e.g. "600x800" -> (600, 800)
Result Parse(std::string_view valueStr, std::pair<int, int>& parsedValues);

} // namespace string_util
} // namespace ppx

#endif // PPX_STRING_UTIL_H