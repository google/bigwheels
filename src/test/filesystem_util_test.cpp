// Copyright 2023 Google LLC
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ppx/fs.h"

#if !defined(NDEBUG)
#define PERFORM_DEATH_TESTS
#endif

using namespace ppx::fs;

const std::filesystem::path root          = std::filesystem::current_path().root_path();
const std::filesystem::path defaultFolder = root / "default" / "folder" / "";

// -------------------------------------------------------------------------------------------------
// GetFullPath
// -------------------------------------------------------------------------------------------------

#if defined(PERFORM_DEATH_TESTS)
TEST(FilesystemUtilTest, GetFullPath_PartialPathEmpty)
{
    std::filesystem::path partialPath = "";
    EXPECT_DEATH(
        {
            GetFullPath(partialPath, defaultFolder);
        },
        "");
}

TEST(FilesystemUtilTest, GetFullPath_PartialPathIsFolder)
{
    std::filesystem::path partialPath = defaultFolder;
    EXPECT_DEATH(
        {
            GetFullPath(partialPath, defaultFolder);
        },
        "");
}
#endif

TEST(FilesystemUtilTest, GetFullPath_IsFull)
{
    std::filesystem::path partialPath = root / "nondefault" / "folder" / "filename.txt";
    std::filesystem::path wantPath    = partialPath;

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder);
    EXPECT_EQ(fullPath, wantPath);
}

TEST(FilesystemUtilTest, GetFullPath_NoRoot)
{
    std::filesystem::path partialPath = std::filesystem::path("nondefault") / "folder" / "filename.txt";
    std::filesystem::path wantPath    = defaultFolder / partialPath;

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder);
    EXPECT_EQ(fullPath, wantPath);
}

TEST(FilesystemUtilTest, GetFullPath_ReplaceNoSymbol)
{
    std::filesystem::path partialPath = root / "nondefault" / "folder" / "filename.txt";
    std::filesystem::path wantPath    = partialPath;

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder, "@", "REPLACED");
    EXPECT_EQ(fullPath, wantPath);
}

TEST(FilesystemUtilTest, GetFullPath_ReplaceOneSymbol)
{
    std::filesystem::path partialPath = root / "nondefault" / "folder" / "filename_@.txt";
    std::filesystem::path wantPath    = root / "nondefault" / "folder" / "filename_REPLACED.txt";

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder, "@", "REPLACED");
    EXPECT_EQ(fullPath, wantPath);
}

TEST(FilesystemUtilTest, GetFullPath_ReplaceMultipleSymbols)
{
    std::filesystem::path partialPath = root / "nondefault" / "folder" / "filename_@@.txt";
    std::filesystem::path wantPath    = root / "nondefault" / "folder" / "filename_REPLACEDREPLACED.txt";

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder, "@", "REPLACED");
    EXPECT_EQ(fullPath, wantPath);
}

TEST(FilesystemUtilTest, GetFullPath_DontReplaceSymbolInPath)
{
    std::filesystem::path partialPath = root / "nondefault" / "folder_@" / "filename_@.txt";
    std::filesystem::path wantPath    = root / "nondefault" / "folder_@" / "filename_REPLACED.txt";

    std::filesystem::path fullPath = GetFullPath(partialPath, defaultFolder, "@", "REPLACED");
    EXPECT_EQ(fullPath, wantPath);
}