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

#include <sstream>

#include "ppx/fs.h"

namespace ppx {
namespace metrics {

////////////////////////////////////////////////////////////////////////////////

static nlohmann::json ExportMetadata(const MetricMetadata& metadata)
{
    nlohmann::json object;
    object["name"]                 = metadata.name;
    object["unit"]                 = metadata.unit;
    object["interpretation"]       = metadata.interpretation;
    object["expected_lower_bound"] = metadata.expectedRange.lowerBound;
    object["expected_upper_bound"] = metadata.expectedRange.upperBound;
    return object;
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

nlohmann::json MetricGauge::Export() const
{
    nlohmann::json metricObject;

    {
        metricObject["metadata"] = ExportMetadata(mMetadata);
    }

    {
        nlohmann::json statisticsObject;

        const GaugeBasicStatistics basicStatistics = GetBasicStatistics();
        statisticsObject["min"]                    = basicStatistics.min;
        statisticsObject["max"]                    = basicStatistics.max;
        statisticsObject["average"]                = basicStatistics.average;
        statisticsObject["time_ratio"]             = basicStatistics.timeRatio;

        const GaugeComplexStatistics complexStatistics = ComputeComplexStatistics();
        statisticsObject["median"]                     = complexStatistics.median;
        statisticsObject["standard_deviation"]         = complexStatistics.standardDeviation;
        statisticsObject["percentile_90"]              = complexStatistics.percentile90;
        statisticsObject["percentile_95"]              = complexStatistics.percentile95;
        statisticsObject["percentile_99"]              = complexStatistics.percentile99;

        metricObject["statistics"] = statisticsObject;
    }

    for (const TimeSeriesEntry& entry : mTimeSeries) {
        metricObject["time_series"] += nlohmann::json::array({entry.seconds, entry.value});
    }

    return metricObject;
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

nlohmann::json MetricCounter::Export() const
{
    nlohmann::json metricObject;
    metricObject["metadata"] = ExportMetadata(mMetadata);
    metricObject["value"]    = mCounter;
    return metricObject;
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

nlohmann::json Run::Export() const
{
    nlohmann::json object;
    object["name"] = mName;
    for (auto [name, pMetric] : mGauges) {
        object["gauges"] += pMetric->Export();
    }
    for (auto [name, pMetric] : mCounters) {
        object["counters"] += pMetric->Export();
    }
    return object;
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

void Manager::ExportToDisk(const std::string& baseReportName) const
{
    PPX_ASSERT_MSG(!baseReportName.empty(), "Base report name must not be empty!");

    nlohmann::json content;
    for (auto [name, pRun] : mRuns) {
        content["runs"] += pRun->Export();
    }

    // Keep the report internal to this function.
    // Since the json library relies on strings-as-pointers rather than self-allocated objects,
    // the lifecycle of the report is tied to the lifecycle of the runs and metrics owned by
    // the manager. But this is opaque.
    Report(std::move(content), baseReportName).WriteToDisk();
}

////////////////////////////////////////////////////////////////////////////////

Manager::Report::Report(const nlohmann::json& content, const std::string& baseFilename)
    : mContent(content)
{
    SetReportName(baseFilename);
}

Manager::Report::Report(nlohmann::json&& content, const std::string& baseFilename)
    : mContent(content)
{
    SetReportName(baseFilename);
}

void Manager::Report::WriteToDisk(bool overwriteExisting) const
{
    PPX_ASSERT_MSG(!mFilePath.empty(), "Filepath must not be empty!");

    if (!overwriteExisting && std::filesystem::exists(mFilePath)) {
        PPX_LOG_ERROR("Metrics report file cannot be written to disk. Path [" << mFilePath << "] already exists.");
        return;
    }

    std::filesystem::create_directories(mFilePath.parent_path());
    std::ofstream outputFile(mFilePath.c_str(), std::ofstream::out);
    outputFile << mContent.dump(4) << std::endl;
    outputFile.close();

    PPX_LOG_INFO("Metrics report written to path [" << mFilePath << "]");
}

void Manager::Report::SetReportName(const std::string& baseReportName)
{
    const time_t      currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream reportName;
    reportName << baseReportName << "_" << currentTime;
    mReportName      = reportName.str();
    mContent["name"] = mReportName.c_str();
#if defined(PPX_ANDROID)
    mFilePath = ppx::fs::GetInternalDataPath();
#endif
    mFilePath /= (mReportName + kFileExtension);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx
