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

#include "ppx/log.h"

// Use current platform if one isn't defined
// clang-format off
#if ! (defined(PPX_LINUX) || defined(PPX_MSW))
#   if defined(__linux__)
#       define PPX_LINUX
#   elif defined(WIN32)
#       define PPX_MSW
#   endif
#endif
// clang-format on

// clang-format off
#if defined(PPX_MSW)
#   if ! defined(VC_EXTRALEAN)
#       define VC_EXTRALEAN
#   endif
#   if ! defined(WIN32_LEAN_AND_MEAN)
#   define WIN32_LEAN_AND_MEAN
#   endif
#   include <Windows.h>
#endif
// clang-format on

namespace ppx {

static Log sLogInstance;

Log::Log()
{
}

Log::~Log()
{
    Shutdown();
}

bool Log::Initialize(uint32_t mode, const char* filePath)
{
    // This means that instance is already initialized
    if (sLogInstance.mModes != LOG_MODE_OFF) {
        return false;
    }

    // Create internal objects
    bool result = sLogInstance.CreateObjects(mode, filePath);
    if (!result) {
        return false;
    }

    sLogInstance.Lock();
    {
        sLogInstance << "Logging started" << std::endl;
        sLogInstance.Flush();
    }
    sLogInstance.Unlock();

    // Success
    return true;
}

void Log::Shutdown()
{
    // Bail if not initialized
    if (sLogInstance.mModes == LOG_MODE_OFF) {
        return;
    }

    // Write last line of log
    sLogInstance.Lock();
    {
        sLogInstance << "Logging stopped" << std::endl;
        sLogInstance.Flush();

        // Destroy internal objects
        sLogInstance.DestroyObjects();
    }
    sLogInstance.Unlock();
}

Log* Log::Get()
{
    Log* ptr = nullptr;
#if !defined(PPX_DISABLE_AUTO_LOG)
    if (sLogInstance.mModes == LOG_MODE_OFF) {
        bool res = Log::Initialize(LOG_MODE_CONSOLE | LOG_MODE_FILE, PPX_LOG_DEFAULT_PATH);
        if (!res) {
            return nullptr;
        }
    }
#endif
    if (sLogInstance.mModes != LOG_MODE_OFF) {
        ptr = &sLogInstance;
    }
    return ptr;
}

bool Log::IsActive()
{
#if !defined(PPX_DISABLE_AUTO_LOG)
    Log::Get();
#endif
    bool result = (sLogInstance.mModes != LOG_MODE_OFF);
    return result;
}

bool Log::IsModeActive(LogMode mode)
{
    bool result = ((sLogInstance.mModes & mode) != 0);
    return result;
}

bool Log::CreateObjects(uint32_t modes, const char* filePath)
{
    //// Allocate mutex dynamically since it doesn't like being
    //// initialized before main...and lock will crash.
    ////
    //mWriteMutex = std::make_unique<std::mutex>();

    mModes = modes;
    if ((mModes & LOG_MODE_FILE) != 0) {
        mFilePath = filePath;
        if (!mFilePath.empty()) {
            mFileStream.open(mFilePath.c_str());
        }
    }
    return true;
}

void Log::DestroyObjects()
{
    mModes    = LOG_MODE_OFF;
    mFilePath = "";
    if (mFileStream.is_open()) {
        mFileStream.close();
    }
}

void Log::Write(const char* msg)
{
    // Console
    if ((mModes & LOG_MODE_CONSOLE) != 0) {
#if defined(PPX_MSW)
        if (IsDebuggerPresent()) {
            OutputDebugStringA(msg);
        }
        else {
            std::cout << msg;
        }
#else
        std::cout << msg;
#endif
    }
    // File
    if (((mModes & LOG_MODE_FILE) != 0) && (mFileStream.is_open())) {
        mFileStream << msg;
    }
}

void Log::Lock()
{
    mWriteMutex.lock();
}

void Log::Unlock()
{
    mWriteMutex.unlock();
}

void Log::Flush()
{
    // Write anything that's in the buffer
    Write(mBuffer.str().c_str());

    // Signal flush for console
    if ((mModes & LOG_MODE_CONSOLE) != 0) {
#if defined(PPX_MSW)
        if (!IsDebuggerPresent()) {
            std::cout << std::flush;
        }
#else
        std::cout << std::flush;
#endif
    }
    // Signal flush for file
    if (((mModes & LOG_MODE_FILE) != 0) && (mFileStream.is_open())) {
        mFileStream.flush();
    }

    // Clear buffer
    mBuffer.str(std::string());
    mBuffer.clear();
}

} // namespace ppx
