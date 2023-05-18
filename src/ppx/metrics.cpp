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
#include "metrics_report.pb.h"

namespace ppx {
namespace metrics {

////////////////////////////////////////////////////////////////////////////////

static void ExportMetadata(const MetricMetadata& metadata, reporting::MetricMetadata* pOutMetadata)
{
    pOutMetadata->set_name(metadata.name);
    pOutMetadata->set_unit(metadata.unit);
    pOutMetadata->set_interpretation(static_cast<reporting::Interpretation>(metadata.interpretation));
    auto pExpectedRange = pOutMetadata->mutable_expected_range();
    pExpectedRange->set_lower_bound(metadata.expectedRange.lowerBound);
    pExpectedRange->set_upper_bound(metadata.expectedRange.upperBound);
}

////////////////////////////////////////////////////////////////////////////////

MetricGauge::MetricGauge(const MetricMetadata& metadata)
    : mMetadata(metadata), mAccumulatedValue(0)
{
    memset(&mBasicStatistics, 0, sizeof(mBasicStatistics));
    mBasicStatistics.min = std::numeric_limits<double>::max();
    mBasicStatistics.max = std::numeric_limits<double>::min();
}

MetricGauge::~MetricGauge()
{
}

void MetricGauge::RecordEntry(double seconds, double value)
{
    PPX_ASSERT_MSG(seconds >= 0.0, "The entries' seconds must always be positive");
    PPX_ASSERT_MSG(mTimeSeries.size() == 0 || seconds > mTimeSeries.back().seconds, "The entries' seconds must form a stricly increasing function");

    TimeSeriesEntry entry;
    entry.seconds = seconds;
    entry.value   = value;
    mTimeSeries.push_back(entry);

    UpdateBasicStatistics(seconds, value);
}

size_t MetricGauge::GetEntriesCount() const
{
    return mTimeSeries.size();
}

void MetricGauge::GetEntry(size_t index, double* pSeconds, double* pValue) const
{
    PPX_ASSERT_MSG(index < mTimeSeries.size(), "The entry index is invalid");
    PPX_ASSERT_NULL_ARG(pSeconds);
    PPX_ASSERT_NULL_ARG(pValue);
    const TimeSeriesEntry entry = mTimeSeries[index];
    *pSeconds                   = entry.seconds;
    *pValue                     = entry.value;
}

const GaugeBasicStatistics MetricGauge::GetBasicStatistics() const
{
    return mBasicStatistics;
}

const GaugeComplexStatistics MetricGauge::ComputeComplexStatistics() const
{
    GaugeComplexStatistics statistics   = {};
    const size_t           entriesCount = mTimeSeries.size();
    if (entriesCount == 0) {
        return statistics;
    }

    std::vector<TimeSeriesEntry> sorted = mTimeSeries;
    std::sort(
        sorted.begin(), sorted.end(), [](const TimeSeriesEntry& lhs, const TimeSeriesEntry& rhs) {
            return lhs.value < rhs.value;
        });

    if (entriesCount % 2 == 0) {
        const size_t medianIndex = entriesCount / 2;
        PPX_ASSERT_MSG(medianIndex > 0, "Unexpected median index");
        statistics.median = (sorted[medianIndex - 1].value + sorted[medianIndex].value) * 0.5;
    }
    else {
        const size_t medianIndex = entriesCount / 2;
        statistics.median        = sorted[medianIndex].value;
    }

    {
        double squareDiffSum = 0.0;
        for (auto entry : mTimeSeries) {
            const double diff = entry.value - mBasicStatistics.average;
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

    return statistics;
}

void MetricGauge::UpdateBasicStatistics(double seconds, double value)
{
    mAccumulatedValue += value;

    mBasicStatistics.min = std::min(mBasicStatistics.min, value);
    mBasicStatistics.max = std::max(mBasicStatistics.max, value);

    const size_t entriesCount = mTimeSeries.size();
    mBasicStatistics.average  = mAccumulatedValue / entriesCount;
    if (entriesCount > 1) {
        const double timeSpan      = mTimeSeries.back().seconds - mTimeSeries.front().seconds;
        mBasicStatistics.timeRatio = mAccumulatedValue / timeSpan;
    }
    else {
        mBasicStatistics.timeRatio = mTimeSeries.front().value;
    }
}

void MetricGauge::Export(reporting::MetricGauge* pReportMetric) const
{
    ExportMetadata(mMetadata, pReportMetric->mutable_metadata());

    reporting::GaugeStatistics* pReportStatistics = pReportMetric->mutable_statistics();

    const GaugeBasicStatistics basicStatistics = GetBasicStatistics();
    pReportStatistics->set_min(basicStatistics.min);
    pReportStatistics->set_max(basicStatistics.max);
    pReportStatistics->set_average(basicStatistics.average);
    pReportStatistics->set_time_ratio(basicStatistics.timeRatio);

    const GaugeComplexStatistics complexStatistics = ComputeComplexStatistics();
    pReportStatistics->set_median(complexStatistics.median);
    pReportStatistics->set_standard_deviation(complexStatistics.standardDeviation);
    pReportStatistics->set_percentile_90(complexStatistics.percentile90);
    pReportStatistics->set_percentile_95(complexStatistics.percentile95);
    pReportStatistics->set_percentile_99(complexStatistics.percentile99);

    for (const TimeSeriesEntry& entry : mTimeSeries) {
        reporting::TimeSeriesEntry* pReportEntry = pReportMetric->add_time_series();
        pReportEntry->set_seconds(entry.seconds);
        pReportEntry->set_value(entry.value);
    }
}

////////////////////////////////////////////////////////////////////////////////

MetricCounter::MetricCounter(const MetricMetadata& metadata)
    : mMetadata(metadata), mCounter(0U)
{
}

MetricCounter::~MetricCounter()
{
}

void MetricCounter::Increment(uint64_t add)
{
    mCounter += add;
}

uint64_t MetricCounter::Get() const
{
    return mCounter;
}

void MetricCounter::Export(reporting::MetricCounter* pReportMetric) const
{
    ExportMetadata(mMetadata, pReportMetric->mutable_metadata());
    pReportMetric->set_value(mCounter);
}

////////////////////////////////////////////////////////////////////////////////

Run::Run(const char* pName)
    : mName(pName)
{
    PPX_ASSERT_MSG(pName != nullptr, "A run name cannot be null");
}

Run::~Run()
{
    for (auto [name, pMetric] : mGauges) {
        delete pMetric;
    }
    mGauges.clear();
    for (auto [name, pMetric] : mCounters) {
        delete pMetric;
    }
    mCounters.clear();
}

void Run::AddMetric(MetricGauge* pMetric)
{
    PPX_ASSERT_NULL_ARG(pMetric);
    const auto ret = mGauges.insert({pMetric->GetName(), pMetric});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding a metric");
}

void Run::AddMetric(MetricCounter* pMetric)
{
    PPX_ASSERT_NULL_ARG(pMetric);
    const auto ret = mCounters.insert({pMetric->GetName(), pMetric});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding a metric");
}

bool Run::HasMetric(const char* pName) const
{
    return mGauges.find(pName) != mGauges.end() || mCounters.find(pName) != mCounters.end();
}

void Run::Export(reporting::Run* pReportRun) const
{
    pReportRun->set_name(mName);
    for (auto [name, pMetric] : mGauges) {
        reporting::MetricGauge* pReportGauge = pReportRun->add_gauges();
        pMetric->Export(pReportGauge);
    }
    for (auto [name, pMetric] : mCounters) {
        reporting::MetricCounter* pReportCounter = pReportRun->add_counters();
        pMetric->Export(pReportCounter);
    }
}

////////////////////////////////////////////////////////////////////////////////

Manager::Manager()
{
}

Manager::~Manager()
{
    for (auto [name, pRun] : mRuns) {
        delete pRun;
    }
    mRuns.clear();
}

Run* Manager::AddRun(const char* pName)
{
    PPX_ASSERT_MSG(pName != nullptr, "A run name must not be null");
    PPX_ASSERT_MSG(mRuns.find(pName) == mRuns.end(), "Runs must have unique names (duplicate name detected)");

    Run* pRun = new Run(pName);
    if (pRun == nullptr) {
        return nullptr;
    }

    const auto ret = mRuns.insert({pName, pRun});
    PPX_ASSERT_MSG(ret.second, "An insertion shall always take place when adding a run");
    return pRun;
}

void Manager::Export(const char* name, reporting::Report* pOutReport) const
{
    pOutReport->set_name(name);
    for (auto [name, pRun] : mRuns) {
        reporting::Run* pReportRun = pOutReport->add_runs();
        pRun->Export(pReportRun);
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx
