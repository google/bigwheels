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

void TrimLeft(std::string& s);
void TrimRight(std::string& s);

std::string TrimCopy(const std::string& s);

// Trims all characters specified in c from both the left and right sides of s
std::string_view TrimBothEnds(std::string_view s, std::string_view c = " \t");

// Splits s at the first instance of delimeter and returns two substrings
// Returns std::nullopt if s does not contain the delimeter
std::optional<std::pair<std::string_view, std::string_view>> SplitInTwo(std::string_view s, char delimiter);

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

} // namespace string_util
} // namespace ppx

#endif // PPX_STRING_UTIL_H