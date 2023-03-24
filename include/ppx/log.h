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

#ifndef PPX_LOG_H
#define PPX_LOG_H

#include "math_config.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

#define PPX_LOG_DEFAULT_PATH "ppx.log"

// Output overloads for common data types.
std::ostream& operator<<(std::ostream& os, const ppx::float2& i);
std::ostream& operator<<(std::ostream& os, const ppx::float3& i);
std::ostream& operator<<(std::ostream& os, const ppx::float4& i);

namespace ppx {

enum LogMode
{
    LOG_MODE_OFF     = 0x0,
    LOG_MODE_CONSOLE = 0x1,
    LOG_MODE_FILE    = 0x2,
};

#define PPX_LOG_ENDL std::endl

//! @class Log
//!
//!
class Log
{
public:
    Log();
    ~Log();

    static bool Initialize(uint32_t modes, const char* filePath = nullptr);
    static void Shutdown();

    static Log* Get();

    static bool IsActive();
    static bool IsModeActive(LogMode mode);

    void Lock();
    void Unlock();
    void Flush();

    template <typename T>
    Log& operator<<(const T& value)
    {
        mBuffer << value;
        return *this;
    }

    Log& operator<<(std::ostream& (*manip)(std::ostream&))
    {
        if (manip == (std::basic_ostream<char> & (*)(std::basic_ostream<char>&)) & std::endl) {
            mBuffer << std::endl;
            Flush();
        }
        return *this;
    }

private:
    bool CreateObjects(uint32_t mode, const char* filePath);
    void DestroyObjects();

    void Write(const char* msg);

private:
    uint32_t          mModes = LOG_MODE_OFF;
    std::string       mFilePath;
    std::ofstream     mFileStream;
    std::stringstream mBuffer;
    std::mutex        mWriteMutex;
};

} // namespace ppx

// clang-format off
#define PPX_LOG_RAW(MSG)                           \
    if (ppx::Log::IsActive()) {                    \
        ppx::Log::Get()->Lock();                   \
        (*ppx::Log::Get()) << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                  \
        ppx::Log::Get()->Unlock();                 \
    }

#define PPX_LOG_INFO(MSG)                          \
    if (ppx::Log::IsActive()) {                    \
        ppx::Log::Get()->Lock();                   \
        (*ppx::Log::Get()) << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                  \
        ppx::Log::Get()->Unlock();                 \
    }

#define PPX_LOG_WARN(MSG)                                          \
    if (ppx::Log::IsActive()) {                                    \
        ppx::Log::Get()->Lock();                                   \
        (*ppx::Log::Get()) << "[WARNING] " << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                                  \
        ppx::Log::Get()->Unlock();                                 \
    }

#define PPX_LOG_DEBUG(MSG)                                       \
    if (ppx::Log::IsActive()) {                                  \
        ppx::Log::Get()->Lock();                                 \
        (*ppx::Log::Get()) << "[DEBUG] " << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                                \
        ppx::Log::Get()->Unlock();                               \
    }

#define PPX_LOG_ERROR(MSG)                                       \
    if (ppx::Log::IsActive()) {                                  \
        ppx::Log::Get()->Lock();                                 \
        (*ppx::Log::Get()) << "[ERROR] " << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                                \
        ppx::Log::Get()->Unlock();                               \
    }

#define PPX_LOG_FATAL(MSG)                                             \
    if (ppx::Log::IsActive()) {                                        \
        ppx::Log::Get()->Lock();                                       \
        (*ppx::Log::Get()) << "[FATAL ERROR] " << MSG << PPX_LOG_ENDL; \
        ppx::Log::Get()->Flush();                                      \
        ppx::Log::Get()->Unlock();                                     \
    }
// clang-format on

#endif // PPX_LOG_H
