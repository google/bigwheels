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

#include "ppx/log.h"
#include "ppx/config.h"

#include <string>

namespace ppx {

class LogStaticTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Some other unrelated tests might have run before the logging tests and
        // already initialized logging. Since we share global state and run all
        // tests in a single process, we need to shut down any existing logging.
        // If the logging was not initialized this operation is a no-op.
        Log::Shutdown();
    }

    void TearDown() override
    {
        Log::Shutdown();
    }
};

class LogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Some other unrelated tests might have run before the logging tests and
        // already initialized logging. Since we share global state and run all
        // tests in a single process, we need to shut down any existing logging.
        // If the logging was not initialized this operation is a no-op.
        Log::Shutdown();
        Log::Initialize(LOG_MODE_CONSOLE, nullptr, &mOut);
        mOut.str(std::string());
        mOut.clear();
    }

    void TearDown() override
    {
        Log::Shutdown();
    }

    std::stringstream mOut;
};

TEST_F(LogStaticTest, LogInitialized)
{
    std::stringstream out;

    Log::Initialize(LOG_MODE_CONSOLE, nullptr, &out);

    std::string expected = "Logging started\n";
    std::string actual   = out.str();

    EXPECT_EQ(expected, actual);

    Log::Shutdown();
}

TEST_F(LogStaticTest, LogShutdown)
{
    std::stringstream out;
    Log::Initialize(LOG_MODE_CONSOLE, nullptr, &out);
    out.str(std::string());
    out.clear();

    Log::Shutdown();

    std::string expected = "Logging stopped\n";
    std::string actual   = out.str();

    EXPECT_EQ(expected, actual);
}

TEST_F(LogTest, LogRaw)
{
    PPX_LOG_RAW("test " << 123 << PPX_ENDL << " ");

    std::string expected = "test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogInfo)
{
    PPX_LOG_INFO("test " << 123 << PPX_ENDL << " ");

    std::string expected = "test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogWarn)
{
    PPX_LOG_WARN("test " << 123 << PPX_ENDL << " ");

    std::string expected = "[WARNING] test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogDebug)
{
    PPX_LOG_DEBUG("test " << 123 << PPX_ENDL << " ");

    std::string expected = "[DEBUG] test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogError)
{
    PPX_LOG_ERROR("test " << 123 << PPX_ENDL << " ");

    std::string expected = "[ERROR] test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogFatal)
{
    PPX_LOG_FATAL("test " << 123 << PPX_ENDL << " ");

    std::string expected = "[FATAL ERROR] test 123\n \n";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogManualEmptyFlush)
{
    Log::Get()->Flush(LOG_LEVEL_DEFAULT);

    std::string expected = "";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogManualMultipleFlush)
{
    Log::Get()->Flush(LOG_LEVEL_ERROR);
    Log::Get()->Flush(LOG_LEVEL_ERROR);

    std::string expected = "";
    EXPECT_EQ(expected, mOut.str());
}

TEST_F(LogTest, LogDifferentLevels)
{
    PPX_LOG_ERROR("Error " << 1);
    PPX_LOG_WARN("Warn 2");
    PPX_LOG_ERROR("Error " + std::to_string(3));

    std::string expected =
        "[ERROR] Error 1\n"
        "[WARNING] Warn 2\n"
        "[ERROR] Error 3\n";
    EXPECT_EQ(expected, mOut.str());
}

} // namespace ppx
