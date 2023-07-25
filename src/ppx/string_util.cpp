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

namespace ppx {
namespace string_util {

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

std::optional<std::pair<std::string_view, std::string_view>> SplitInTwo(std::string_view s, char delimiter)
{
    if (s.size() == 0 || delimiter == '\0') {
        return std::nullopt;
    }
    size_t delimeterIndex = s.find(delimiter);
    if (delimeterIndex == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view firstSubstring  = s.substr(0, delimeterIndex);
    std::string_view secondSubstring = s.substr(delimeterIndex + 1);
    return std::make_pair(firstSubstring, secondSubstring);
}

} // namespace string_util
} // namespace ppx