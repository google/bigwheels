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

#include "ppx/string_util.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace ppx {
namespace string_util {

// -------------------------------------------------------------------------------------------------
// Misc
// -------------------------------------------------------------------------------------------------

void TrimLeft(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void TrimRight(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
}

std::string TrimCopy(const std::string& s)
{
    std::string sc = s;
    TrimLeft(sc);
    TrimRight(sc);
    return sc;
}

std::string_view TrimBothEnds(std::string_view s, std::string_view c)
{
    if (s.size() == 0) {
        return s;
    }
    auto strBegin = s.find_first_not_of(c);
    auto strEnd   = s.find_last_not_of(c);
    auto strRange = strEnd - strBegin + 1;
    return s.substr(strBegin, strRange);
}

std::optional<std::vector<std::string_view>> Split(std::string_view s, char delimiter)
{
    if (s.size() == 0) {
        return std::nullopt;
    }

    std::vector<std::string_view> substrings;
    std::string_view              remainingString = s;
    while (remainingString != "") {
        size_t delimeterIndex = remainingString.find(delimiter);
        if (delimeterIndex == std::string_view::npos) {
            substrings.push_back(remainingString);
            break;
        }

        std::string_view element = remainingString.substr(0, delimeterIndex);
        if (element.length() == 0) {
            return std::nullopt;
        }
        substrings.push_back(element);

        if (delimeterIndex == remainingString.length() - 1) {
            return std::nullopt;
        }
        remainingString = remainingString.substr(delimeterIndex + 1);
    }
    return substrings;
}

std::optional<std::pair<std::string_view, std::string_view>> SplitInTwo(std::string_view s, char delimiter)
{
    if (s.size() == 0) {
        return std::nullopt;
    }
    auto splitResult = Split(s, delimiter);
    if (splitResult == std::nullopt || splitResult->size() != 2) {
        return std::nullopt;
    }
    return std::make_pair(splitResult->at(0), splitResult->at(1));
}

// -------------------------------------------------------------------------------------------------
// Formatting Strings
// -------------------------------------------------------------------------------------------------

std::string WrapText(const std::string& s, size_t width, size_t indent)
{
    if (indent >= width) {
        return s;
    }
    auto remainingString = TrimCopy(s);

    size_t      textWidth   = width - indent;
    std::string wrappedText = "";
    while (remainingString != "") {
        // Section off the next line from the remaining string, format it, and append it to wrappedText
        size_t lineLength = remainingString.length();
        if (lineLength > textWidth) {
            // Break the line at textWidth
            lineLength = textWidth;
            // If the character after the line break is not empty, the line break will interrupt a word
            // so try to insert the line break at the last empty space on this line
            if (!std::isspace(remainingString[textWidth])) {
                lineLength = remainingString.find_last_of(" \t", textWidth);
                // In case there is no empty space on this entire line, go back to breaking the word at textWidth
                if (lineLength == std::string::npos) {
                    lineLength = textWidth;
                }
            }
        }
        std::string newLine = remainingString.substr(0, lineLength);
        TrimRight(newLine);
        wrappedText += std::string(indent, ' ') + newLine + "\n";

        // Update the remaining string
        remainingString = remainingString.substr(lineLength);
        TrimLeft(remainingString);
    }
    return wrappedText;
}

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

std::pair<std::pair<int, int>, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, const std::pair<int, int>& defaultValue)
{
    auto parseResolution = SplitInTwo(valueStr, 'x');
    if (parseResolution == std::nullopt) {
        return std::make_pair(defaultValue, "resolution flag must be in format <Width>x<Height>: " + std::string(valueStr));
    }
    auto parseNResult = ParseOrDefault(parseResolution->first, defaultValue.first);
    if (parseNResult.second != std::nullopt) {
        return std::make_pair(defaultValue, "width cannot be parsed: " + parseNResult.second->errorMsg);
    }
    auto parseMResult = ParseOrDefault(parseResolution->second, defaultValue.first);
    if (parseMResult.second != std::nullopt) {
        return std::make_pair(defaultValue, "height cannot be parsed: " + parseMResult.second->errorMsg);
    }
    return std::make_pair(std::make_pair(parseNResult.first, parseMResult.first), std::nullopt);
}

std::pair<std::string, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, const std::string& defaultValue)
{
    return std::make_pair(std::string(valueStr), std::nullopt);
}

std::pair<std::string, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, std::string_view defaultValue)
{
    return std::make_pair(std::string(valueStr), std::nullopt);
}

std::pair<bool, std::optional<ParsingError>> ParseOrDefault(std::string_view valueStr, bool defaultValue)
{
    if (valueStr == "") {
        return std::make_pair(true, std::nullopt);
    }
    std::stringstream ss{std::string(valueStr)};
    bool              valueAsBool;
    ss >> valueAsBool;
    if (ss.fail()) {
        ss.clear();
        ss >> std::boolalpha >> valueAsBool;
        if (ss.fail()) {
            return std::make_pair(defaultValue, "could not be parsed as bool: " + std::string(valueStr));
        }
    }
    return std::make_pair(valueAsBool, std::nullopt);
}

} // namespace string_util
} // namespace ppx