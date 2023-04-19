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

#include "ppx/metrics.h"

namespace ppx {
namespace metrics {

////////////////////////////////////////////////////////////////////////////////

const MetricMetadata& Metric::GetMetadata() const
{
    return mMetadata;
}

MetricType Metric::GetType() const
{
    return mType;
}

Metric::Metric(const MetricMetadata& metadata, MetricType type)
    : mMetadata(metadata), mType(type)
{
}

Metric::~Metric()
{
}

////////////////////////////////////////////////////////////////////////////////

MetricGauge::MetricGauge(const MetricMetadata& metadata)
    : Metric(metadata, MetricType::GAUGE)
{
}

void MetricGauge::RecordEntry(double seconds, double value)
{
    // TODO Add statistics support
    TimeSeriesEntry entry;
    entry.seconds = seconds;
    entry.value   = value;
    mTimeSeries.push_back(entry);
}

size_t MetricGauge::GetEntriesCount() const
{
    return mTimeSeries.size();
}

void MetricGauge::GetEntry(size_t index, double& seconds, double& value) const
{
    const TimeSeriesEntry entry = mTimeSeries[index];
    seconds                     = entry.seconds;
    value                       = entry.value;
}

const GaugeStatistics MetricGauge::GetStatistics() const
{
    GaugeStatistics statistics = {};
    // TODO Implement
    return statistics;
}

////////////////////////////////////////////////////////////////////////////////

MetricCounter::MetricCounter(const MetricMetadata& metadata)
    : Metric(metadata, MetricType::COUNTER)
{
}

uint64_t MetricCounter::Increment(uint64_t add)
{
    mCounter += add;
    return mCounter;
}

uint64_t MetricCounter::Get() const
{
    return mCounter;
}

////////////////////////////////////////////////////////////////////////////////

Run::Run(const char* name)
    : mName(name)
{
}

Run::~Run()
{
    // TODO Delete metrics
}

MetricGauge* Run::AddMetricGauge(MetricMetadata metadata)
{
    // TODO Check name double
    MetricGauge* metric = new MetricGauge(metadata);
    if (metric == nullptr) {
        return nullptr;
    }
    mMetrics.insert(make_pair(metadata.name, metric));
    return metric;
}

MetricCounter* Run::AddMetricCounter(MetricMetadata metadata)
{
    // TODO Check name double
    MetricCounter* metric = new MetricCounter(metadata);
    if (metric == nullptr) {
        return nullptr;
    }
    mMetrics.insert(make_pair(metadata.name, metric));
    return metric;
}

////////////////////////////////////////////////////////////////////////////////

Manager::Manager()
{
}

Manager::~Manager()
{
    for (auto [key, value] : mRuns) {
        delete value;
    }
    mRuns.clear();
}

Result Manager::AddRun(Run*& outRun, const char* name)
{
    outRun  = nullptr;
    auto it = mRuns.find(name);
    if (it != mRuns.end()) {
        return ERROR_DUPLICATE_ELEMENT;
    }
    Run* run = new Run(name);
    if (run == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }
    const auto ret = mRuns.insert({name, run});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding runs");
    outRun = run;
    return SUCCESS;
}

Run* Manager::GetRun(const char* name) const
{
    auto it = mRuns.find(name);
    return it == mRuns.end() ? nullptr : it->second;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx
