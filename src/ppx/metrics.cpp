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

Metric::Metric(const MetricMetadata& metadata, MetricType type)
    : mMetadata(metadata), mType(type)
{
}

Metric::~Metric()
{
}

const MetricMetadata& Metric::GetMetadata() const
{
    return mMetadata;
}

MetricType Metric::GetType() const
{
    return mType;
}

////////////////////////////////////////////////////////////////////////////////

MetricGauge::MetricGauge(const MetricMetadata& metadata)
    : Metric(metadata, MetricType::GAUGE)
{
    memset(&mRealTimeStatistics, 0, sizeof(mRealTimeStatistics));
    mRealTimeStatistics.min = std::numeric_limits<double>::max();
    mRealTimeStatistics.max = std::numeric_limits<double>::min();
}

MetricGauge::~MetricGauge()
{
}

void MetricGauge::RecordEntry(double seconds, double value)
{
    PPX_ASSERT_MSG(mTimeSeries.size() == 0 || seconds >= mTimeSeries.back().seconds, "The entries' seconds must form a monotically increasing function");

    TimeSeriesEntry entry;
    entry.seconds = seconds;
    entry.value   = value;
    mTimeSeries.push_back(entry);

    UpdateRealTimeStatistics(seconds, value);
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

const GaugeStatistics MetricGauge::GetStatistics(bool realtime) const
{
    GaugeStatistics statistics   = mRealTimeStatistics;
    const size_t    entriesCount = mTimeSeries.size();
    if (!realtime && entriesCount > 0) {
        std::vector<TimeSeriesEntry> sorted = mTimeSeries;
        std::sort(
            sorted.begin(), sorted.end(), [](const TimeSeriesEntry& lhs, const TimeSeriesEntry& rhs) {
                return lhs.value < rhs.value;
            });

        const size_t medianIndex = (entriesCount / 2U) + (((entriesCount & 0x1U) && entriesCount > 1) ? 1U : 0U);
        statistics.median        = sorted[medianIndex].value;

        {
            double squareDiffSum = 0.0;
            for (auto entry : mTimeSeries) {
                const double diff = entry.value - mRealTimeStatistics.average;
                squareDiffSum += (diff * diff);
            }
            const double variance        = squareDiffSum / entriesCount;
            statistics.standardDeviation = sqrt(variance);
        }

        const size_t percentileIndex90 = entriesCount * 90 / 100;
        statistics.percentile90        = sorted[percentileIndex90].value;
        const size_t percentileIndex95 = entriesCount * 95 / 100;
        statistics.percentile95        = sorted[percentileIndex95].value;
        const size_t percentileIndex99 = entriesCount * 99 / 100;
        statistics.percentile99        = sorted[percentileIndex99].value;
    }
    return statistics;
}

void MetricGauge::UpdateRealTimeStatistics(double seconds, double value)
{
    mAccumlatedValue += value;

    mRealTimeStatistics.min = std::min(mRealTimeStatistics.min, value);
    mRealTimeStatistics.max = std::max(mRealTimeStatistics.max, value);

    const size_t entriesCount   = mTimeSeries.size();
    mRealTimeStatistics.average = mAccumlatedValue / entriesCount;
    if (entriesCount > 1) {
        const double timeSpan = mTimeSeries.back().value - mTimeSeries.front().value;
        statistics.timeRatio  = mAccumlatedValue / timeSpan;
    }
    else {
        statistics.timeRatio = mTimeSeries.front().value;
    }

    // TODO Implement median, standard deviation and percentiles approximations if necessary at runtime
}

////////////////////////////////////////////////////////////////////////////////

MetricCounter::MetricCounter(const MetricMetadata& metadata)
    : Metric(metadata, MetricType::COUNTER), mCounter(0U)
{
}

MetricCounter::~MetricCounter()
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
    PPX_ASSERT_MSG(name != nullptr, "A run name cannot be null");
}

Run::~Run()
{
    for (auto [name, metric] : mMetrics) {
        delete metric;
    }
    mMetrics.clear();
}

template <typename T>
Result Run::AddMetric(T*& outMetric, MetricMetadata metadata)
{
    PPX_ASSERT_MSG(outMetric != nullptr, "The metric pointer must not be null");
    PPX_ASSERT_MSG(!metadata.name.empty(), "The metric name must not be empty");

    outMetric = nullptr;
    if (GetMetric(metadata.name.c_str()) != nullptr) {
        return ERROR_DUPLICATE_ELEMENT;
    }

    T* metric = new T(metadata);
    if (metric == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }

    const auto ret = mMetrics.insert({metadata.name, metric});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding a metric");
    outMetric = metric;
    return SUCCESS;
}

Result Run::AddMetricGauge(MetricGauge*& outMetric, MetricMetadata metadata)
{
    return AddMetric(outMetric, metadata);
}

MetricGauge* Run::GetMetricGauge(const char* name) const
{
    Metric* metric = GetMetric(name);
    if (metric != nullptr && metric->GetType() == MetricType::GAUGE) {
        return static_cast<MetricGauge*>(metric);
    }
    return nullptr;
}

Result Run::AddMetricCounter(MetricCounter*& outMetric, MetricMetadata metadata)
{
    return AddMetric(outMetric, metadata);
}

MetricCounter* Run::GetMetricCounter(const char* name) const
{
    Metric* metric = GetMetric(name);
    if (metric != nullptr && metric->GetType() == MetricType::COUNTER) {
        return static_cast<MetricCounter*>(metric);
    }
    return nullptr;
}

Metric* Run::GetMetric(const char* name) const
{
    auto it = mMetrics.find(name);
    return mMetrics.find(name) == mMetrics.end() ? nullptr : it->second;
}

////////////////////////////////////////////////////////////////////////////////

Manager::Manager()
{
}

Manager::~Manager()
{
    for (auto [name, run] : mRuns) {
        delete run;
    }
    mRuns.clear();
}

Result Manager::AddRun(Run*& outRun, const char* name)
{
    PPX_ASSERT_MSG(outRun != nullptr, "The run pointer must not be null");
    PPX_ASSERT_MSG(name != nullptr, "A run name must not be null");

    outRun = nullptr;
    if (GetRun(name) != nullptr) {
        return ERROR_DUPLICATE_ELEMENT;
    }

    Run* run = new Run(name);
    if (run == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }

    const auto ret = mRuns.insert({name, run});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding a run");
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
