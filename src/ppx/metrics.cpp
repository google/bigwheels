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

nlohmann::json MetricMetadata::Export() const
{
    nlohmann::json object;
    object["name"]                 = name;
    object["unit"]                 = unit;
    object["interpretation"]       = interpretation;
    object["expected_lower_bound"] = expectedRange.lowerBound;
    object["expected_upper_bound"] = expectedRange.upperBound;
    return object;
}

////////////////////////////////////////////////////////////////////////////////

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
    nlohmann::json statisticsObject;

    metricObject["metadata"] = mMetadata.Export();

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

    for (const auto& entry : mTimeSeries) {
        metricObject["time_series"] += nlohmann::json::array({entry.seconds, entry.value});
    }

    return metricObject;
}

////////////////////////////////////////////////////////////////////////////////

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
    metricObject["metadata"] = mMetadata.Export();
    metricObject["value"]    = mCounter;
    return metricObject;
}

////////////////////////////////////////////////////////////////////////////////

void Run::AddMetric(std::unique_ptr<MetricGauge>&& metric)
{
    mGauges.emplace(metric->GetName(), std::move(metric));
}

void Run::AddMetric(std::unique_ptr<MetricCounter>&& metric)
{
    mCounters.emplace(metric->GetName(), std::move(metric));
}

bool Run::HasMetric(const std::string& name) const
{
    return mGauges.find(name) != mGauges.end() || mCounters.find(name) != mCounters.end();
}

nlohmann::json Run::Export() const
{
    nlohmann::json object;
    object["name"] = mName;
    for (const auto& [name, pMetric] : mGauges) {
        object["gauges"] += pMetric->Export();
    }
    for (const auto& [name, pMetric] : mCounters) {
        object["counters"] += pMetric->Export();
    }
    return object;
}

////////////////////////////////////////////////////////////////////////////////

// The lifetime of this pointer aligns with the lifetime of the parent Manager.
// It is the responsibility of the caller to guard against use-after-free accordingly.
Run* Manager::AddRun(const std::string& name)
{
    PPX_ASSERT_MSG(!name.empty(), "A run name must not be empty");
    PPX_ASSERT_MSG(mRuns.find(name) == mRuns.end(), "Runs must have unique names (duplicate name detected)");

    auto* pRun = new Run(name);
    auto  run  = std::unique_ptr<Run>(pRun);
    mRuns.emplace(name, std::move(run));
    return pRun;
}

void Manager::ExportToDisk(const std::string& reportPath) const
{
    nlohmann::json content;
    for (const auto& [name, pRun] : mRuns) {
        content["runs"] += pRun->Export();
    }

    // Keep the report internal to this function.
    // Since the json library relies on strings-as-pointers rather than self-allocated objects,
    // the lifecycle of the report is tied to the lifecycle of the runs and metrics owned by
    // the manager. But this is opaque.
    Report(std::move(content), reportPath).WriteToDisk();
}

////////////////////////////////////////////////////////////////////////////////

Manager::Report::Report(const nlohmann::json& content, const std::string& reportPath)
    : mContent(content)
{
    SetReportPath(reportPath);
}

Manager::Report::Report(nlohmann::json&& content, const std::string& reportPath)
    : mContent(content)
{
    SetReportPath(reportPath);
}

void Manager::Report::WriteToDisk(bool overwriteExisting) const
{
    PPX_ASSERT_MSG(!mFilePath.empty(), "Filepath must not be empty!");

    if (!overwriteExisting && std::filesystem::exists(mFilePath)) {
        PPX_LOG_ERROR("Metrics report file cannot be written to disk. Path [" << mFilePath << "] already exists.");
        return;
    }

    std::filesystem::create_directories(mFilePath.parent_path());
    std::ofstream outputFile(mFilePath, std::ofstream::out);
    if (!outputFile.is_open()) {
        PPX_LOG_ERROR("Failed to open metrics file at path [" << mFilePath << "] for writing!");
        return;
    }
    outputFile << mContent.dump(4) << std::endl;
    outputFile.close();

    PPX_LOG_INFO("Metrics report written to path [" << mFilePath << "]");
}

void Manager::Report::SetReportPath(const std::string& reportPath)
{
    std::filesystem::path path(reportPath.empty() ? std::string(kDefaultReportPath) : reportPath);
    auto                  filename   = path.filename().string();
    auto                  atLocation = filename.find("@");
    auto                  now        = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream     timeStream;
    timeStream << now;
    while (atLocation != std::string::npos) {
        filename.replace(atLocation, 1, timeStream.str());
        atLocation = filename.find("@");
    }
    path.replace_filename(std::filesystem::path(filename));
    if (path.extension() != std::filesystem::path(kFileExtension)) {
        path += kFileExtension;
    }
#if defined(PPX_ANDROID)
    mFilePath = ppx::fs::GetInternalDataPath();
#endif
    mFilePath /= path;
    mContent["filename"]     = mFilePath.filename().string();
    mContent["generated_at"] = timeStream.str();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx
