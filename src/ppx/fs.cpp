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

#include "ppx/fs.h"

#include <fstream>
#include <filesystem>
#include <vector>
#include <optional>

namespace ppx::fs {

std::optional<std::vector<char>> load_file(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        return std::nullopt;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return std::nullopt;
    }

    std::vector<char> data;
    do {
        stream.seekg(0, std::ios::end);
        size_t size = stream.tellg();
        if (size == 0) {
            break;
        }

        data.resize(size);
        stream.seekg(0, std::ios::beg);
        stream.read(reinterpret_cast<char*>(data.data()), data.size());
    } while (false);
    stream.close();

    return data;
}

} // namespace ppx::fs
