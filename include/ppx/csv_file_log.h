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

#ifndef PPX_CSV_FILE_LOG_H
#define PPX_CSV_FILE_LOG_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

const std::string PPX_DEFAULT_CSV_FILE{"stats.csv"};

namespace ppx {

//! @class CSVFileLog
//!
//!
class CSVFileLog
{
public:
    CSVFileLog();
    CSVFileLog(const std::string& filepath);
    ~CSVFileLog();

    template <typename T>
    CSVFileLog& operator<<(const T& value)
    {
        mBuffer << value;
        return *this;
    }

    template <typename T>
    void LogField(const T& value)
    {
        (*this) << value << ",";
    }
    template <typename T>
    void LastField(const T& value)
    {
        (*this) << value << "\n";
    }
    void NewLine()
    {
        (*this) << "\n";
    }

private:
    void Write(const char* msg);
    void Lock();
    void Unlock();
    void Flush();

private:
    std::string       mFilePath;
    std::ofstream     mFileStream;
    std::stringstream mBuffer;
    std::mutex        mWriteMutex;
};

} // namespace ppx

#endif // PPX_CSV_FILE_LOG_H
