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

std::pair<std::string_view, std::string_view> SplitInTwo(std::string_view s, char delimiter)
{
    std::pair<std::string_view, std::string_view> stringViewPair;
    if (s.size() == 0) {
        return stringViewPair;
    }

    size_t delimiterIndex = s.find(delimiter);
    if (delimiterIndex == std::string_view::npos) {
        stringViewPair.first = s;
        return stringViewPair;
    }

    stringViewPair.first  = s.substr(0, delimiterIndex);
    stringViewPair.second = s.substr(delimiterIndex + 1);
    return stringViewPair;
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

} // namespace string_util

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

Result string_util::Parse(std::string_view valueStr, std::string& parsedValue)
{
    parsedValue = std::string(valueStr);
    return SUCCESS;
}

Result string_util::Parse(std::string_view valueStr, bool& parsedValue)
{
    if (valueStr == "") {
        parsedValue = true;
        return SUCCESS;
    }
    std::stringstream ss{std::string(valueStr)};
    bool              valueAsBool;
    ss >> valueAsBool;
    if (ss.fail()) {
        ss.clear();
        ss >> std::boolalpha >> valueAsBool;
        if (ss.fail()) {
            PPX_LOG_ERROR("could not be parsed as bool: " << valueStr);
            return ERROR_FAILED;
        }
    }
    parsedValue = valueAsBool;
    return SUCCESS;
}

Result string_util::Parse(std::string_view valueStr, std::pair<int, int>& parsedValues)
{
    if (std::count(valueStr.cbegin(), valueStr.cend(), 'x') != 1) {
        PPX_LOG_ERROR("invalid number of 'x', resolution string must be in format <Width>x<Height>: " << valueStr);
        return ERROR_FAILED;
    }
    std::pair<std::string_view, std::string_view> parseResolution = SplitInTwo(valueStr, 'x');
    if (parseResolution.first.length() == 0 || parseResolution.second.length() == 0) {
        PPX_LOG_ERROR("both width and height must be defined, resolution string must be in format <Width>x<Height>: " << valueStr);
        return ERROR_FAILED;
    }
    int  N, M;
    auto res = Parse(parseResolution.first, N);
    if (Failed(res)) {
        PPX_LOG_ERROR("width cannot be parsed");
        return res;
    }
    res = Parse(parseResolution.second, M);
    if (Failed(res)) {
        PPX_LOG_ERROR("height cannot be parsed");
        return res;
    }
    parsedValues.first  = N;
    parsedValues.second = M;
    return SUCCESS;
}

} // namespace ppx