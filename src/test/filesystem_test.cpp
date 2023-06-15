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

// Those tests uses some OS-specific libraries to handled filesystem and make sure the FS library
// behaves correctly. This should probably be implemented on other platforms, but for now we test is on
// linux and relies on the app working for other platforms.
#if defined(PPX_LINUX)

#include "gtest/gtest.h"
#include "ppx/config.h"
#include "ppx/fs.h"

#include <dirent.h>
#include <filesystem>
#include <stdio.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

constexpr std::string_view kDefaultFileContent = "some content";

// Counts the number of filedescriptors open by the current process.
static size_t getOpenFDCount()
{
    DIR* fdDir = opendir("/proc/self/fd");
    assert(fdDir != nullptr);

    size_t fd_count = 0;
    while (readdir(fdDir)) {
        ++fd_count;
    }

    closedir(fdDir);
    return fd_count;
}

class FsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        readableFileHandle = createFile(kDefaultFileContent);
        readableFile       = filenameFromFile(readableFileHandle);

        directory = readableFile.parent_path();

        {
            char* filename  = tmpnam(nullptr);
            nonExistantFile = std::filesystem::path(filename);
        }
    }

    void TearDown() override
    {
        fclose(readableFileHandle);
    }

    std::filesystem::path readableFile;
    FILE*                 readableFileHandle;

    std::filesystem::path nonExistantFile;
    std::filesystem::path directory;

private:
    static std::filesystem::path filenameFromFile(FILE* file)
    {
        return std::filesystem::path("/proc/self/fd/") / std::to_string(fileno(file));
    }

    FILE* createFile(const std::string_view& content)
    {
        FILE* file = tmpfile();
        assert(file != nullptr);

        const size_t totalSize = content.size() * sizeof(content[0]);
        const size_t written   = fwrite(content.data(), sizeof(content[0]), content.size(), file);
        assert(written == totalSize);

        // See man(3) fopen: it is required to call a fseek/fsetpos between output and input to a File
        // to make sure buffers are flushed.
        // rewind is a wrapper to fseek(file, 0, SEEK_SET);
        rewind(file);

        return file;
    }
};

namespace ppx {

TEST_F(FsTest, InitializedFileIsBad)
{
    fs::File file;

    EXPECT_FALSE(file.IsValid());
}

TEST_F(FsTest, SimpleOpen)
{
    fs::File file;
    EXPECT_TRUE(file.Open(readableFile));
}

TEST_F(FsTest, SimpleOpenValid)
{
    fs::File file;
    EXPECT_TRUE(file.Open(readableFile));

    EXPECT_TRUE(file.IsValid());
}

TEST_F(FsTest, OpenAndRead)
{
    fs::File file;
    EXPECT_TRUE(file.Open(readableFile));

    std::string  buffer(kDefaultFileContent.size(), '\0');
    const size_t readCount = file.Read(buffer.data(), buffer.size());
    EXPECT_EQ(readCount, kDefaultFileContent.size());
    EXPECT_EQ(buffer, kDefaultFileContent);
}

TEST_F(FsTest, OpenAndReadCursor)
{
    fs::File file;
    EXPECT_TRUE(file.Open(readableFile));

    assert(kDefaultFileContent.size() == 12 && "Test needs update if the value changes.");
    std::string part1(6, '\0');
    std::string part2(6, '\0');

    {
        const size_t readCount = file.Read(part1.data(), part1.size());
        EXPECT_EQ(readCount, part1.size());
    }
    {
        const size_t readCount = file.Read(part2.data(), part2.size());
        EXPECT_EQ(readCount, part2.size());
    }

    EXPECT_EQ(part1, "some c");
    EXPECT_EQ(part2, "ontent");
}

TEST_F(FsTest, GetSizeIgnoresCursor)
{
    fs::File file;
    EXPECT_TRUE(file.Open(readableFile));
    EXPECT_EQ(file.GetLength(), kDefaultFileContent.size());

    std::string buffer(kDefaultFileContent.size(), '\0');
    EXPECT_EQ(file.Read(buffer.data(), buffer.size()), kDefaultFileContent.size());

    EXPECT_EQ(file.GetLength(), kDefaultFileContent.size());
}

TEST_F(FsTest, RAIIClosure)
{
    const size_t fdCountBefore = getOpenFDCount();

    {
        fs::File file;
        EXPECT_TRUE(file.Open(readableFile));
        // Sanity check, in case some weird libc implems uses the same FD.
        EXPECT_EQ(getOpenFDCount(), fdCountBefore + 1);
    }

    EXPECT_EQ(getOpenFDCount(), fdCountBefore);
}

} // namespace ppx
#endif
