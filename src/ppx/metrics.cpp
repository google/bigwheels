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

#include <regex>
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

bool MetricGauge::RecordEntry(const MetricData& data)
{
    if (data.type != MetricType::GAUGE) {
        PPX_LOG_ERROR("Provided metric was not correct type; ignoring. Provided type: " << static_cast<uint32_t>(data.type));
        return false;
    }

    if (data.gauge.seconds < 0.0) {
        PPX_LOG_ERROR("Provided gauge metric had negative seconds value; ignoring.");
        return false;
    }

    auto entryCount = mTimeSeries.size();
    if (entryCount > 0 && data.gauge.seconds <= mTimeSeries.back().seconds) {
        PPX_LOG_ERROR("Provided gauge metric had old seconds value; ignoring.");
        return false;
    }

    TimeSeriesEntry entry;
    entry.seconds = data.gauge.seconds;
    entry.value   = data.gauge.value;
    // This entry will be added at the end; update the count now for calculations.
    ++entryCount;

    // Update the basic stats.
    mAccumulatedValue += entry.value;
    mBasicStats.min     = std::min(mBasicStats.min, entry.value);
    mBasicStats.max     = std::max(mBasicStats.max, entry.value);
    mBasicStats.average = mAccumulatedValue / entryCount;
    // Above checks guarantee the 'seconds' field monotonically increases with each entry.
    mBasicStats.timeRatio = (entryCount > 1)
                                ? mAccumulatedValue / (entry.seconds - mTimeSeries.front().seconds)
                                : entry.value;

    mTimeSeries.emplace_back(std::move(entry));
    return true;
}

ppx::metrics::GaugeComplexStatistics MetricGauge::ComputeComplexStats() const
{
    GaugeComplexStatistics complex;
    size_t                 entryCount = mTimeSeries.size();
    if (entryCount == 0) {
        return complex;
    }

    std::vector<TimeSeriesEntry> sorted = mTimeSeries;
    std::sort(
        sorted.begin(), sorted.end(), [](const TimeSeriesEntry& lhs, const TimeSeriesEntry& rhs) {
            return lhs.value < rhs.value;
        });

    auto medianIndex = entryCount / 2;
    // medianIndex is guaranteed to be > 0 when even from above check.
    complex.median = (entryCount % 2 == 0)
                         ? (sorted[medianIndex - 1].value + sorted[medianIndex].value) * 0.5
                         : sorted[medianIndex].value;

    double squareDiffSum = 0.0;
    for (const auto& entry : mTimeSeries) {
        double diff = entry.value - mBasicStats.average;
        squareDiffSum += (diff * diff);
    }
    double variance           = squareDiffSum / entryCount;
    complex.standardDeviation = sqrt(variance);

    complex.percentile01 = sorted[entryCount * 1 / 100].value;
    complex.percentile05 = sorted[entryCount * 5 / 100].value;
    complex.percentile10 = sorted[entryCount * 10 / 100].value;
    complex.percentile90 = sorted[entryCount * 90 / 100].value;
    complex.percentile95 = sorted[entryCount * 95 / 100].value;
    complex.percentile99 = sorted[entryCount * 99 / 100].value;

    return complex;
}

nlohmann::json MetricGauge::Export() const
{
    // JSON does not have +inf or -inf
    constexpr double kMinJsonNumber = std::numeric_limits<double>::lowest();
    constexpr double kMaxJsonNumber = std::numeric_limits<double>::max();

    nlohmann::json metricObject;
    nlohmann::json statsObject;

    metricObject["metadata"] = mMetadata.Export();

    GaugeComplexStatistics complex    = ComputeComplexStats();
    statsObject["min"]                = std::clamp(mBasicStats.min, kMinJsonNumber, kMaxJsonNumber);
    statsObject["max"]                = std::clamp(mBasicStats.max, kMinJsonNumber, kMaxJsonNumber);
    statsObject["average"]            = mBasicStats.average;
    statsObject["time_ratio"]         = mBasicStats.timeRatio;
    statsObject["median"]             = complex.median;
    statsObject["standard_deviation"] = complex.standardDeviation;
    statsObject["percentile_01"]      = complex.percentile01;
    statsObject["percentile_05"]      = complex.percentile05;
    statsObject["percentile_10"]      = complex.percentile10;
    statsObject["percentile_90"]      = complex.percentile90;
    statsObject["percentile_95"]      = complex.percentile95;
    statsObject["percentile_99"]      = complex.percentile99;

    metricObject["statistics"] = statsObject;

    metricObject["time_series"] = nlohmann::json::array();
    for (const auto& entry : mTimeSeries) {
        metricObject["time_series"] += nlohmann::json::array({entry.seconds, entry.value});
    }

    return metricObject;
}

////////////////////////////////////////////////////////////////////////////////

bool MetricCounter::RecordEntry(const MetricData& data)
{
    if (data.type != MetricType::COUNTER) {
        PPX_LOG_ERROR("Provided metric was not correct type! Provided type: " << static_cast<uint32_t>(data.type));
        return false;
    }

    mCounter += data.counter.increment;
    ++mEntryCount;
    return true;
}

nlohmann::json MetricCounter::Export() const
{
    nlohmann::json metricObject;
    metricObject["metadata"]    = mMetadata.Export();
    metricObject["value"]       = mCounter;
    metricObject["entry_count"] = mEntryCount;
    return metricObject;
}

////////////////////////////////////////////////////////////////////////////////

LiveMetric::LiveMetric(double halfLive)
    : mHalfLife(halfLive)
{
}

bool LiveMetric::RecordEntry(const MetricData& data)
{
    if (data.type != MetricType::GAUGE) {
        PPX_LOG_ERROR("Provided metric was not correct type; ignoring. Provided type: " << static_cast<uint32_t>(data.type));
        return false;
    }

    if (data.gauge.seconds < 0.0) {
        PPX_LOG_ERROR("Provided gauge metric had negative seconds value; ignoring.");
        return false;
    }

    if (data.gauge.seconds <= mStats.seconds) {
        PPX_LOG_ERROR("Provided gauge metric had old seconds value; ignoring.");
        return false;
    }

    Append(data.gauge.seconds, data.gauge.value);

    return true;
}

void LiveMetric::ClearHistory()
{
    // Keep the latest and timestamp, since they are not part of history.
    const double latest  = mStats.latest;
    const double seconds = mStats.seconds;

    mVar           = {};
    mStats         = {};
    mStats.latest  = latest;
    mStats.seconds = seconds;
}

void LiveMetric::Append(double seconds, double value)
{
    constexpr double kNewWeight = 1.0;

    const double lastTimestamp = mStats.seconds;
    if (mHalfLife > 0.0) {
        const double elapsed          = static_cast<double>(seconds - lastTimestamp);
        const double elapsedHalfLifes = elapsed / mHalfLife;
        const double multiplier       = std::exp2(-elapsedHalfLifes);

        mVar = {mVar.weight * multiplier, mVar.mean, mVar.accVar * multiplier};
    }

    mVar = ParallelVariance<double>::Combine(mVar, {kNewWeight, value, 0.0});

    mStats.latest   = value;
    mStats.seconds  = seconds;
    mStats.mean     = mVar.Mean();
    mStats.variance = mVar.PopulationVariance();
    mStats.weight   = mVar.Weight();
    mStats.min      = std::min<double>(mStats.min, value);
    mStats.max      = std::max<double>(mStats.max, value);
}

////////////////////////////////////////////////////////////////////////////////

Metric* Run::AddMetric(const MetricMetadata& metadata)
{
    if (metadata.name.empty()) {
        PPX_LOG_ERROR("Metric name must not be empty; dropping.");
        return nullptr;
    }

    if (mMetricNames.count(metadata.name) > 0) {
        PPX_LOG_ERROR("Duplicate metric name provided: " << metadata.name << "; dropping.");
        return nullptr;
    }

    Metric* pMetric = nullptr;
    switch (metadata.type) {
        case MetricType::GAUGE:
            pMetric = new MetricGauge(metadata);
            break;
        case MetricType::COUNTER:
            pMetric = new MetricCounter(metadata);
            break;
        default:
            return nullptr;
    }
    auto metric = std::unique_ptr<Metric>(pMetric);
    mMetrics.emplace_back(std::move(metric));
    mMetricNames.insert(metadata.name);
    return pMetric;
}

bool Run::HasMetric(const std::string& name) const
{
    return (mMetricNames.count(name) != 0);
}

nlohmann::json Run::Export() const
{
    nlohmann::json object;
    object["name"]     = mName;
    object["gauges"]   = nlohmann::json::array();
    object["counters"] = nlohmann::json::array();
    for (const auto& metric : mMetrics) {
        std::string typeString;
        switch (metric->GetType()) {
            case MetricType::GAUGE:
                typeString = "gauges";
                break;
            case MetricType::COUNTER:
                typeString = "counters";
                break;
            default:
                PPX_LOG_ERROR("Unrecognized metric type at export: " << static_cast<uint32_t>(metric->GetType()));
                continue;
        }
        object[typeString] += metric->Export();
    }
    return object;
}

////////////////////////////////////////////////////////////////////////////////

void Manager::StartRun(const std::string& name)
{
    PPX_ASSERT_MSG(!name.empty(), "A run name must not be empty");
    PPX_ASSERT_MSG(mRuns.find(name) == mRuns.end(), "All runs must have unique names (duplicate name detected)");
    PPX_ASSERT_MSG(mActiveRun == nullptr, "Only one run may be active at a time!");

    mActiveRun = new Run(name);
    auto run   = std::unique_ptr<Run>(mActiveRun);
    mRuns.emplace(name, std::move(run));
}

void Manager::EndRun()
{
    if (mActiveRun == nullptr) {
        PPX_LOG_ERROR("Requested to end run with no active run!");
    }

    mActiveRun = nullptr;
    mActiveMetrics.clear();
}

bool Manager::HasActiveRun() const
{
    return (mActiveRun != nullptr);
}

MetricID Manager::AllocateID()
{
    return mNextMetricID++;
}

bool Manager::BindMetric(MetricID metricID, const MetricMetadata& metadata)
{
    if (metricID == kInvalidMetricID) {
        PPX_LOG_WARN("Attempted to bind to an invalid metric id.");
        return false;
    }

    if (mActiveRun == nullptr) {
        PPX_LOG_WARN("Attempted to bind to an inactive run.");
        return false;
    }

    auto* metric = mActiveRun->AddMetric(metadata);
    if (metric == nullptr) {
        return false;
    }
    mActiveMetrics.emplace(metricID, metric);
    return true;
}

bool Manager::BindLiveMetric(MetricID metricID, double halfLife)
{
    if (metricID == kInvalidMetricID) {
        PPX_LOG_WARN("Attempted to bind to an invalid metric id.");
        return false;
    }
    if (mLiveMetrics.count(metricID) > 0) {
        PPX_LOG_WARN("Attempted to bind to an existing live metric.");
        return false;
    }
    mLiveMetrics[metricID] = LiveMetric(halfLife);
    return true;
}

bool Manager::RecordMetricData(MetricID id, const MetricData& data)
{
    auto liveMetricIter = mLiveMetrics.find(id);
    bool hasLiveMetric  = (liveMetricIter != mLiveMetrics.end());
    if (hasLiveMetric) {
        liveMetricIter->second.RecordEntry(data);
    }
    if (mActiveRun == nullptr) {
        if (!hasLiveMetric) {
            PPX_LOG_WARN("Attempted to record a metric entry with no active run.");
            return false;
        }
        return true;
    }
    auto findResult = mActiveMetrics.find(id);
    if (findResult == mActiveMetrics.end()) {
        if (!hasLiveMetric) {
            PPX_LOG_ERROR("Attempted to record a metric entry against an invalid ID.");
            return false;
        }
        return true;
    }
    return findResult->second->RecordEntry(data);
}

Report Manager::CreateReport(const std::string& reportPath) const
{
    nlohmann::json content;
    content["runs"] = nlohmann::json::array();
    for (const auto& [name, pRun] : mRuns) {
        content["runs"] += pRun->Export();
    }

    return Report(std::move(content), reportPath);
}

GaugeBasicStatistics Manager::GetGaugeBasicStatistics(MetricID id) const
{
    if (mActiveRun == nullptr) {
        PPX_LOG_WARN("Attempted to record a metric entry with no active run.");
        return GaugeBasicStatistics();
    }
    auto findResult = mActiveMetrics.find(id);
    if (findResult == mActiveMetrics.end()) {
        PPX_LOG_ERROR("Attempted to record a metric entry against an invalid ID.");
        return GaugeBasicStatistics();
    }

    if (findResult->second->GetType() != MetricType::GAUGE) {
        PPX_LOG_ERROR("Attempted to get gauge basic statistics from non MetricType::GAUGE type.");
        return GaugeBasicStatistics();
    }

    return static_cast<MetricGauge*>(findResult->second)->GetBasicStatistics();
}

// Get Live Statistics for a LiveMetric, only works for type GAUGE
LiveStatistics Manager::GetLiveStatistics(MetricID id) const
{
    auto liveMetricIter = mLiveMetrics.find(id);
    if (liveMetricIter == mLiveMetrics.end()) {
        PPX_LOG_ERROR("Attempted to get live statistics from an invalid ID.");
        return LiveStatistics{};
    }
    return liveMetricIter->second.GetLiveStatistics();
}

// Reset live statistics.
void Manager::ClearLiveMetricsHistory()
{
    for (auto& kv : mLiveMetrics) {
        kv.second.ClearHistory();
    }
}

////////////////////////////////////////////////////////////////////////////////

Report::Report(const nlohmann::json& content, const std::string& reportPath)
    : mContent(content)
{
    SetReportPath(reportPath);
}

Report::Report(nlohmann::json&& content, const std::string& reportPath)
    : mContent(content)
{
    SetReportPath(reportPath);
}

void Report::WriteToDisk(bool overwriteExisting) const
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
    outputFile << GetContentString() << std::endl;
    outputFile.close();

    PPX_LOG_INFO("Metrics report written to path [" << mFilePath << "]");
}

std::string Report::GetContentString() const
{
    return mContent.dump(4);
}

void Report::SetReportPath(const std::string& reportPath)
{
    std::stringstream timeStream;
    timeStream << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    mFilePath = ppx::fs::GetDefaultOutputDirectory();
    mFilePath = ppx::fs::GetFullPath(std::filesystem::path(reportPath), mFilePath, "@", timeStream.str());

    mContent["filename"]     = mFilePath.filename().string();
    mContent["generated_at"] = timeStream.str();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx
