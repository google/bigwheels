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

#include "ppx/profiler.h"
#include "ppx/timer.h"

#define PPX_MAX_THREAD_PROFILERS 64

namespace ppx {

static Profiler           sPerThreadProfilers[PPX_MAX_THREAD_PROFILERS];
static std::mutex         sThreadIndexMutex;
static unsigned int       sThreadCount = 0;
thread_local unsigned int sThreadIndex = UINT32_MAX;

static unsigned int GetThreadIndex()
{
    if (sThreadIndex == UINT32_MAX) {
        std::lock_guard<std::mutex> lock(sThreadIndexMutex);
        sThreadIndex = sThreadCount;
        sThreadCount = sThreadCount + 1;
    };
    return sThreadIndex;
}

// -------------------------------------------------------------------------------------------------

ProfilerScopedEventSample::ProfilerScopedEventSample(ProfilerEventToken token)
    : mToken(token)
{
    Timer::Timestamp(&mSample.startTimestamp);
}

ProfilerScopedEventSample::~ProfilerScopedEventSample()
{
    Timer::Timestamp(&mSample.endTimestamp);

    Profiler* pProfiler = Profiler::GetProfilerForThread();
    if (IsNull(pProfiler)) {
        PPX_ASSERT_MSG(false, "profiler is null!");
    }

    pProfiler->RecordSample(mToken, mSample);
}

// -------------------------------------------------------------------------------------------------

ProfilerEvent::ProfilerEvent(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, const ProfilerEventToken& token)
    : mType(type),
      mName(name),
      mAction(recordAction),
      mToken(token)
{
}

ProfilerEvent::~ProfilerEvent()
{
}

void ProfilerEvent::RecordSample(const ProfilerEventSample& sample)
{
    if (mAction == PROFILER_EVENT_RECORD_ACTION_INSERT) {
        mSamples.push_back(sample);
    }
    else if (mAction == PROFILER_EVENT_RECORD_ACTION_AVERAGE) {
        uint64_t diff = (sample.endTimestamp - sample.startTimestamp);
        mSampleCount += 1;
        mSampleTotal += diff;
        mSampleMin = (diff < mSampleMin) ? diff : mSampleMin;
        mSampleMax = (diff > mSampleMax) ? diff : mSampleMax;
    }
    else {
        PPX_ASSERT_MSG(false, "unknown event record action");
    }
}

// -------------------------------------------------------------------------------------------------
// Profiler
// -------------------------------------------------------------------------------------------------
Profiler::Profiler()
{
}

Profiler::~Profiler()
{
}

Profiler* Profiler::GetProfilerForThread()
{
    unsigned int threadIndex = GetThreadIndex();
    Profiler*    pProfiler   = nullptr;
    if (threadIndex < PPX_MAX_THREAD_PROFILERS) {
        pProfiler = &sPerThreadProfilers[threadIndex];
    }
    return pProfiler;
}

Result Profiler::RegisterEvent(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, ProfilerEventToken* pToken)
{
    PPX_ASSERT_NULL_ARG(pToken);
    if (IsNull(pToken)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(sThreadIndexMutex);

    ProfilerEventToken token = XXH64(name.c_str(), name.length(), 0xDEADBEEF);

    for (size_t i = 0; i < PPX_MAX_THREAD_PROFILERS; ++i) {
        Result ppxres = sPerThreadProfilers[i].RegisterEventInternal(type, name, recordAction, token);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    *pToken = token;

    return ppx::SUCCESS;
}

Result Profiler::RegisterGrfxApiFnEvent(const std::string& name, ProfilerEventToken* pToken)
{
    Result ppxres = RegisterEvent(PROFILER_EVENT_TYPE_GRFX_API_FN, name, PROFILER_EVENT_RECORD_ACTION_AVERAGE, pToken);
    return ppxres;
}

Result Profiler::RegisterEventInternal(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, ProfilerEventToken token)
{
    auto it = FindIf(
        mEvents,
        [token](const ProfilerEvent& elem) -> bool {
            bool isSame = (elem.mToken == token);
            return isSame; });
    if (it != std::end(mEvents)) {
        return ppx::ERROR_DUPLICATE_ELEMENT;
    }

    mEvents.emplace_back(type, name, recordAction, token);

    return ppx::SUCCESS;
}

void Profiler::RecordSample(const ProfilerEventToken& token, const ProfilerEventSample& sample)
{
    auto it = FindIf(
        mEvents,
        [token](const ProfilerEvent& elem) -> bool {
            bool isSame = (elem.mToken == token);
            return isSame; });
    if (it != std::end(mEvents)) {
        ProfilerEvent& event = *it;
        event.RecordSample(sample);
    }
}

} // namespace ppx
