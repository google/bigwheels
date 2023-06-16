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

#include "ppx/timer.h"
#include "ppx/config.h"

#include <cassert>
#include <cmath>

#if defined(PPX_LINUX) || defined(PPX_ANDROID)
#include <time.h>
#include <unistd.h>
#endif

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

// =============================================================================
// Static Data
// =============================================================================
#if defined(PPX_MSW)

static double sNanosPerCount = 0.0;

#endif // defined(PPX_MSW)

// =============================================================================
// Win32SleepNanos
// =============================================================================
#if defined(PPX_MSW)
static TimerResult Win32SleepNanos(double nanos)
{
    const uint64_t k_min_sleep_threshold = 2 * PPX_TIMER_MILLIS_TO_NANOS;

    uint64_t    now    = 0;
    TimerResult result = Timer::Timestamp(&now);
    if (result != TIMER_RESULT_SUCCESS) {
        return result;
    }

    uint64_t target = now + (uint64_t)nanos;

    for (;;) {
        result = Timer::Timestamp(&now);
        if (result != TIMER_RESULT_SUCCESS) {
            return result;
        }

        if (now >= target) {
            break;
        }

        uint64_t diff = target - now;
        if (diff >= k_min_sleep_threshold) {
            DWORD millis = (DWORD)(diff * PPX_TIMER_NANOS_TO_MILLIS);
            Sleep(millis);
        }
        else {
            Sleep(0);
        }
    }

    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// SleepSeconds
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
static TimerResult SleepSeconds(double seconds)
{
    double nanos = seconds * (double)PPX_TIMER_SECONDS_TO_NANOS;
    double secs  = floor(nanos * (double)PPX_TIMER_NANOS_TO_SECONDS);

    struct timespec req;
    req.tv_sec  = (time_t)secs;
    req.tv_nsec = (long)(nanos - (secs * (double)PPX_TIMER_SECONDS_TO_NANOS));

    int result = nanosleep(&req, NULL);
    assert(result == 0);
    if (result != 0) {
        return TIMER_RESULT_ERROR_SLEEP_FAILED;
    }

    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
static TimerResult SleepSeconds(double seconds)
{
    double nanos = seconds * PPX_TIMER_SECONDS_TO_NANOS;
    Win32SleepNanos(nanos);
    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// SleepMillis
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
TimerResult SleepMillis(double millis)
{
    double nanos = millis * (double)PPX_TIMER_MILLIS_TO_NANOS;
    double secs  = floor(nanos * (double)PPX_TIMER_NANOS_TO_SECONDS);

    struct timespec req;
    req.tv_sec  = (time_t)secs;
    req.tv_nsec = (long)(nanos - (secs * (double)PPX_TIMER_SECONDS_TO_NANOS));

    int result = nanosleep(&req, NULL);
    assert(result == 0);
    if (result != 0) {
        return TIMER_RESULT_ERROR_SLEEP_FAILED;
    }

    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
static TimerResult SleepMillis(double millis)
{
    double nanos = millis * (double)PPX_TIMER_MILLIS_TO_NANOS;
    Win32SleepNanos(nanos);
    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// SleepMicros
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
static TimerResult SleepMicros(double micros)
{
    double nanos = micros * (double)PPX_TIMER_MICROS_TO_NANOS;
    double secs  = floor(nanos * (double)PPX_TIMER_NANOS_TO_SECONDS);

    struct timespec req;
    req.tv_sec  = (time_t)secs;
    req.tv_nsec = (long)(nanos - (secs * (double)PPX_TIMER_SECONDS_TO_NANOS));

    int result = nanosleep(&req, NULL);
    assert(result == 0);
    if (result != 0) {
        return TIMER_RESULT_ERROR_SLEEP_FAILED;
    }

    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
static TimerResult SleepMicros(double micros)
{
    double nanos = micros * (double)PPX_TIMER_MICROS_TO_NANOS;
    Win32SleepNanos(nanos);
    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// SleepNanos
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
static TimerResult SleepNanos(double nanos)
{
    double secs = floor(nanos * (double)PPX_TIMER_NANOS_TO_SECONDS);

    struct timespec req;
    req.tv_sec  = (time_t)secs;
    req.tv_nsec = (long)(nanos - (secs * (double)PPX_TIMER_SECONDS_TO_NANOS));

    int result = nanosleep(&req, NULL);
    assert(result == 0);
    if (result != 0) {
        return TIMER_RESULT_ERROR_SLEEP_FAILED;
    }

    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
static TimerResult SleepNanos(double nanos)
{
    Win32SleepNanos(nanos);
    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// Timer::InitializeStaticData
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
TimerResult Timer::InitializeStaticData()
{
    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
TimerResult Timer::InitializeStaticData()
{
    LARGE_INTEGER frequency;
    BOOL          result = QueryPerformanceFrequency(&frequency);
    assert(result != FALSE);
    if (result == FALSE) {
        return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
    }
    sNanosPerCount = 1.0e9 / (double)frequency.QuadPart;
    return TIMER_RESULT_SUCCESS;
}
#endif

// =============================================================================
// Timer::Timestamp
// =============================================================================
#if defined(PPX_LINUX) || defined(PPX_ANDROID)
TimerResult Timer::Timestamp(uint64_t* pTimestamp)
{
    assert(pTimestamp != NULL);
#if !defined(PPX_TIMER_DISABLE_NULL_POINTER_CHECK)
    if (pTimestamp == NULL) {
        return TIMER_RESULT_ERROR_NULL_POINTER;
    }
#endif

    // Read
    struct timespec tp;
    int             result = clock_gettime(PPX_TIMER_CLK_ID, &tp);
    assert(result == 0);
    if (result != 0) {
        return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
    }

    // Convert seconds to nanoseconds
    uint64_t timestamp = (uint64_t)tp.tv_sec *
                         (uint64_t)PPX_TIMER_SECONDS_TO_NANOS;
    // Add nanoseconds
    timestamp += (uint64_t)tp.tv_nsec;

    *pTimestamp = timestamp;

    return TIMER_RESULT_SUCCESS;
}
#elif defined(PPX_MSW)
TimerResult Timer::Timestamp(uint64_t* pTimestamp)
{
    assert(pTimestamp != NULL);
#if !defined(PPX_TIMER_DISABLE_NULL_POINTER_CHECK)
    if (pTimestamp == NULL) {
        return TIMER_RESULT_ERROR_NULL_POINTER;
    }
#endif

    // Read
    LARGE_INTEGER counter;
    BOOL          result = QueryPerformanceCounter(&counter);
    assert(result != FALSE);
    if (result == FALSE) {
        return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
    }

    //
    // QPC: https://msdn.microsoft.com/en-us/library/ms644904(v=VS.85).aspx
    //
    // According to the QPC link above, QueryPerformanceCounter returns a
    // timestamp that's < 1us.
    //
    double timestamp = (double)counter.QuadPart * sNanosPerCount;

    *pTimestamp = static_cast<uint64_t>(timestamp);

    return TIMER_RESULT_SUCCESS;
}
#endif // defined(PPX_LINUX) || defined(PPX_ANDROID)

// =============================================================================
// Timer::TimestampToSeconds
// =============================================================================
double Timer::TimestampToSeconds(uint64_t timestamp)
{
    double seconds = (double)timestamp * (double)PPX_TIMER_NANOS_TO_SECONDS;
    return seconds;
}

// =============================================================================
// Timer::TimestampToMillis
// =============================================================================
double Timer::TimestampToMillis(uint64_t timestamp)
{
    double millis = (double)timestamp * (double)PPX_TIMER_NANOS_TO_MILLIS;
    return millis;
}

// =============================================================================
// Timer::TimestampToMicros
// =============================================================================
double Timer::TimestampToMicros(uint64_t timestamp)
{
    double micros = (double)timestamp * (double)PPX_TIMER_NANOS_TO_MICROS;
    return micros;
}

// =============================================================================
// Timer::TimestampToNanos
// =============================================================================
double Timer::TimestampToNanos(uint64_t timestamp)
{
    double nanos = (double)timestamp;
    return nanos;
}

// =============================================================================
// Timer::SleepSeconds
// =============================================================================
TimerResult Timer::SleepSeconds(double seconds)
{
    return ppx::SleepSeconds(seconds);
}

// =============================================================================
// Timer::SleepMillis
// =============================================================================
TimerResult Timer::SleepMillis(double millis)
{
    return ppx::SleepMillis(millis);
}

// =============================================================================
// Timer::SleepMicros
// =============================================================================
TimerResult Timer::SleepMicros(double micros)
{
    return ppx::SleepMicros(micros);
}

// =============================================================================
// Timer::SleepNanos
// =============================================================================
TimerResult Timer::SleepNanos(double nanos)
{
    return ppx::SleepNanos(nanos);
}

// =============================================================================
// Timer::Start
// =============================================================================
TimerResult Timer::Start()
{
    TimerResult result = Timestamp(&mStartTimestamp);
    return result;
}

// =============================================================================
// Timer::Stop
// =============================================================================
TimerResult Timer::Stop()
{
    TimerResult result = Timestamp(&mStopTimestamp);
    return result;
}

// =============================================================================
// Timer::StartTimestamp
// =============================================================================
uint64_t Timer::StartTimestamp() const
{
    return mStartTimestamp;
}

// =============================================================================
// Timer::StopTimestamp
// =============================================================================
uint64_t Timer::StopTimestamp() const
{
    return mStopTimestamp;
}

// =============================================================================
// Timer::Seconds
// =============================================================================
double Timer::SecondsSinceStart() const
{
    uint64_t diff = 0;
    if (mStartTimestamp > 0) {
        if (mStopTimestamp > 0) {
            diff = mStopTimestamp - mStartTimestamp;
        }
        else {
            uint64_t now = 0;
            Timestamp(&now);
            diff = now - mStartTimestamp;
        }
    }
    double seconds = TimestampToSeconds(diff);
    return seconds;
}

// =============================================================================
// Timer::Millis
// =============================================================================
double Timer::MillisSinceStart() const
{
    uint64_t diff = 0;
    if (mStartTimestamp > 0) {
        if (mStopTimestamp > 0) {
            diff = mStopTimestamp - mStartTimestamp;
        }
        else {
            uint64_t now = 0;
            Timestamp(&now);
            diff = now - mStartTimestamp;
        }
    }
    double millis = TimestampToMillis(diff);
    return millis;
}

// =============================================================================
// Timer::Micros
// =============================================================================
double Timer::MicrosSinceStart() const
{
    uint64_t diff = 0;
    if (mStartTimestamp > 0) {
        if (mStopTimestamp > 0) {
            diff = mStopTimestamp - mStartTimestamp;
        }
        else {
            uint64_t now = 0;
            Timestamp(&now);
            diff = now - mStartTimestamp;
        }
    }
    double micros = TimestampToMicros(diff);
    return micros;
}

// =============================================================================
// Timer::Nanos
// =============================================================================
double Timer::NanosSinceStart() const
{
    uint64_t diff = 0;
    if (mStartTimestamp > 0) {
        if (mStopTimestamp > 0) {
            diff = mStopTimestamp - mStartTimestamp;
        }
        else {
            uint64_t now = 0;
            Timestamp(&now);
            diff = now - mStartTimestamp;
        }
    }
    double nanos = TimestampToMicros(diff);
    return nanos;
}

ScopedTimer::ScopedTimer(const std::string& message)
    : mMessage(message)
{
    PPX_ASSERT_MSG(mTimer.Start() == TIMER_RESULT_SUCCESS, "Timer start failed.");
}

ScopedTimer::~ScopedTimer()
{
    PPX_ASSERT_MSG(mTimer.Stop() == TIMER_RESULT_SUCCESS, "Timer stop failed.");
    const float elapsed = static_cast<float>(mTimer.SecondsSinceStart());
    PPX_LOG_INFO(mMessage + ": " + FloatString(elapsed) << " seconds.");
}

} // namespace ppx
