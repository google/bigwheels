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

#include "ppx/csv_file_log.h"

namespace ppx {

CSVFileLog::CSVFileLog()
    : CSVFileLog(PPX_DEFAULT_CSV_FILE)
{
}

CSVFileLog::CSVFileLog(const std::string& filepath)
{
    mFilePath = filepath;
    if (!mFilePath.empty()) {
        mFileStream.open(mFilePath.c_str());
    }
    Lock();
}

CSVFileLog::~CSVFileLog()
{
    Flush();
    Unlock();
    if (mFileStream.is_open()) {
        mFileStream.close();
    }
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wthread-safety-analysis"
#endif
void CSVFileLog::Lock()
{
    mWriteMutex.lock();
}

void CSVFileLog::Unlock()
{
    mWriteMutex.unlock();
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

void CSVFileLog::Write(const char* msg)
{
    if (mFileStream.is_open()) {
        mFileStream << msg;
    }
}
void CSVFileLog::Flush()
{
    // Write anything that's in the buffer
    Write(mBuffer.str().c_str());

    // Signal flush for file
    if (mFileStream.is_open()) {
        mFileStream.flush();
    }

    // Clear buffer
    mBuffer.str(std::string());
    mBuffer.clear();
}

} // namespace ppx
