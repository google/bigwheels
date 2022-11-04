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

#ifndef PPX_PROFILER_H
#define PPX_PROFILER_H

#include "ppx/config.h"
#include "xxhash.h"

namespace ppx {

enum ProfilerEventType
{
    PROFILER_EVENT_TYPE_UNDEFINED   = 0,
    PROFILER_EVENT_TYPE_GRFX_API_FN = 1,
};

enum ProfileEventRecordAction
{
    PROFILER_EVENT_RECORD_ACTION_INSERT  = 0,
    PROFILER_EVENT_RECORD_ACTION_AVERAGE = 1,
};

// -------------------------------------------------------------------------------------------------

using ProfilerEventToken = XXH64_hash_t;

// -------------------------------------------------------------------------------------------------

struct ProfilerEventSample
{
    uint64_t startTimestamp;
    uint64_t endTimestamp;
};

// -------------------------------------------------------------------------------------------------

class ProfilerScopedEventSample
{
public:
    ProfilerScopedEventSample(ProfilerEventToken token);
    ~ProfilerScopedEventSample();

private:
    ProfilerEventToken  mToken;
    ProfilerEventSample mSample;
};

// -------------------------------------------------------------------------------------------------

class ProfilerEvent
{
public:
    ProfilerEvent(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, const ProfilerEventToken& token);
    ~ProfilerEvent();

    ProfilerEventType                GetType() const { return mType; }
    const std::string&               GetName() const { return mName; }
    const ProfilerEventToken&        GetToken() const { return mToken; }
    std::vector<ProfilerEventSample> GetSamples() const { return mSamples; }
    uint64_t                         GetSampleCount() const { return mSampleCount; }
    uint64_t                         GetSampleTotal() const { return mSampleTotal; }
    uint64_t                         GetSampleMin() const { return mSampleMin; }
    uint64_t                         GetSampleMax() const { return mSampleMax; }

    void RecordSample(const ProfilerEventSample& sample);

private:
    friend class Profiler;

private:
    ProfilerEventType                mType = PROFILER_EVENT_TYPE_UNDEFINED;
    std::string                      mName;
    ProfileEventRecordAction         mAction;
    ProfilerEventToken               mToken = 0;
    std::vector<ProfilerEventSample> mSamples;                  // PROFILER_EVENT_RECORD_ACTION_INSERT
    uint64_t                         mSampleCount = 0;          // PROFILER_EVENT_RECORD_ACTION_AVERAGE
    uint64_t                         mSampleTotal = 0;          // PROFILER_EVENT_RECORD_ACTION_AVERAGE
    uint64_t                         mSampleMin   = UINT64_MAX; // PROFILER_EVENT_RECORD_ACTION_AVERAGE
    uint64_t                         mSampleMax   = 0;          // PROFILER_EVENT_RECORD_ACTION_AVERAGE
};

// -------------------------------------------------------------------------------------------------

class Profiler
{
public:
    Profiler();
    virtual ~Profiler();

    static Profiler* GetProfilerForThread();

    static Result RegisterEvent(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, ProfilerEventToken* pToken);
    static Result RegisterGrfxApiFnEvent(const std::string& name, ProfilerEventToken* pToken);

    void RecordSample(const ProfilerEventToken& token, const ProfilerEventSample& sample);

    const std::vector<ProfilerEvent>& GetEvents() const { return mEvents; }

private:
    Result RegisterEventInternal(ProfilerEventType type, const std::string& name, ProfileEventRecordAction recordAction, ProfilerEventToken token);

private:
    std::vector<ProfilerEvent> mEvents;
};

} // namespace ppx

#endif //PPX_PROFILER_H
