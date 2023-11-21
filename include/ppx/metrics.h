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

#ifndef ppx_metrics_h
#define ppx_metrics_h

#include "nlohmann/json.hpp"
#include "ppx/config.h"

#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ppx {
namespace metrics {

#define METRICS_NO_COPY(TYPE__)     \
    TYPE__(TYPE__&&)      = delete; \
    TYPE__(const TYPE__&) = delete; \
    TYPE__& operator=(const TYPE__&) = delete;

////////////////////////////////////////////////////////////////////////////////

typedef uint32_t          MetricID;
static constexpr MetricID kInvalidMetricID = 0;

enum class MetricType
{
    GAUGE   = 1,
    COUNTER = 2,
};

enum class MetricInterpretation
{
    NONE,
    HIGHER_IS_BETTER,
    LOWER_IS_BETTER,
};

struct Range
{
    double lowerBound = std::numeric_limits<double>::min();
    double upperBound = std::numeric_limits<double>::max();
};

struct MetricMetadata
{
    MetricType           type;
    std::string          name;
    std::string          unit;
    MetricInterpretation interpretation = MetricInterpretation::NONE;
    Range                expectedRange;

    nlohmann::json Export() const;
};

struct GaugeData
{
    double seconds;
    double value;
};

struct CounterData
{
    uint64_t increment;
};

struct MetricData
{
    MetricType type;
    union
    {
        GaugeData   gauge;
        CounterData counter;
    };
};

////////////////////////////////////////////////////////////////////////////////

// Interface for all metric types.
class Metric
{
public:
    virtual bool           RecordEntry(const MetricData& data) = 0;
    virtual nlohmann::json Export() const                      = 0;
    virtual MetricType     GetType() const                     = 0;
    virtual ~Metric(){};
};

////////////////////////////////////////////////////////////////////////////////

// A gauge metric represents a value that may increase or decrease over time.
// The value is sampled frequently (e.g. every frame) and statistics can be
// derived from the sampling process.
// The most typical case is the frame time, but memory consumption and image
// quality are also good examples.

// Basic statistics are computed on the fly as metrics entries are recorded.
// They can be retrieved without any significant run-time cost.
struct GaugeBasicStatistics
{
    double min       = std::numeric_limits<double>::max();
    double max       = std::numeric_limits<double>::min();
    double average   = 0.0;
    double timeRatio = 0.0;
};

// Complex statistics cannot be computed on the fly.
// They require significant computation (e.g. sorting).
struct GaugeComplexStatistics
{
    double median            = 0.0;
    double standardDeviation = 0.0;
    double percentile01      = 0.0;
    double percentile05      = 0.0;
    double percentile10      = 0.0;
    double percentile90      = 0.0;
    double percentile95      = 0.0;
    double percentile99      = 0.0;
};

class MetricGauge final : public Metric
{
    friend class Run;

public:
    ~MetricGauge() override {}

    // Record a measurement for the metric at a particular point in time.
    // Each entry must have a positive 'seconds' that is greater than the
    // 'seconds' of the previous entry (i.e. 'seconds' must form a stricly
    // increasing positive function).
    // Note however that the system does NOT assume that the 'seconds' of
    // the first entry equal zero.
    // Returns whether the given entry data was valid and able to be recorded.
    bool RecordEntry(const MetricData& data) override;

    // Exports this metric in JSON format.
    nlohmann::json Export() const override;

    MetricType GetType() const override
    {
        return MetricType::GAUGE;
    }

    const GaugeBasicStatistics GetBasicStatistics() const { return mStats.basic; };

private:
    struct TimeSeriesEntry
    {
        double seconds;
        double value;
    };

    struct Stats
    {
        GaugeBasicStatistics   basic;
        GaugeComplexStatistics complex;
    };

private:
    MetricGauge(const MetricMetadata& metadata)
        : mMetadata(metadata)
    {
        PPX_ASSERT_MSG(mMetadata.type == MetricType::GAUGE, "Gauge must be instantiated with gauge-type metadata!");
    }
    METRICS_NO_COPY(MetricGauge)

    Stats ComputeStats() const;

private:
    MetricMetadata               mMetadata;
    std::vector<TimeSeriesEntry> mTimeSeries;
    Stats                        mStats;
    double                       mAccumulatedValue = 0.0;
};

////////////////////////////////////////////////////////////////////////////////

// A counter metric represents a value that only goes up, e.g. the number
// of stutters or pipeline cache misses.
class MetricCounter final : public Metric
{
    friend class Run;

public:
    ~MetricCounter() override {}

    bool RecordEntry(const MetricData& data) override;

    // Exports this metric in JSON format.
    nlohmann::json Export() const override;

    MetricType GetType() const override
    {
        return MetricType::COUNTER;
    }

private:
    MetricCounter(const MetricMetadata& metadata)
        : mMetadata(metadata)
    {
        PPX_ASSERT_MSG(mMetadata.type == MetricType::COUNTER, "Counter must be instantiated with counter-type metadata!");
    }
    METRICS_NO_COPY(MetricCounter)

private:
    MetricMetadata mMetadata;
    uint64_t       mCounter    = 0;
    size_t         mEntryCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

// A run gathers metrics relevant to the execution of a benchmark.
// It is expected that a new run is created each time parameters that affect the
// metrics measurements are changed.
class Run final
{
    friend class Manager;

public:
    // The lifetime of this pointer aligns with the lifetime of the parent Run.
    // It is the responsibility of the caller to guard against use-after-free accordingly.
    Metric* AddMetric(const MetricMetadata& metadata);

    // Exports the run in JSON format.
    nlohmann::json Export() const;

private:
    Run(const std::string& name)
        : mName(name) {}
    METRICS_NO_COPY(Run)

    bool HasMetric(const std::string& name) const;

private:
    std::string                          mName;
    std::unordered_set<std::string>      mMetricNames;
    std::vector<std::unique_ptr<Metric>> mMetrics;
};

////////////////////////////////////////////////////////////////////////////////

// A report contains runs and metrics information meant to be saved to disk.
class Report final
{
public:
    // Copy constructor for content.
    Report(const nlohmann::json& content, const std::string& reportPath);
    // Move constructor for content.
    Report(nlohmann::json&& content, const std::string& reportPath);

    void WriteToDisk(bool overwriteExisting = false) const;

    std::string GetContentString() const;

private:
    static constexpr const char* kFileExtension     = ".json";
    static constexpr const char* kDefaultReportPath = "report_@";

private:
    void SetReportPath(const std::string& reportPath);

    nlohmann::json        mContent;
    std::filesystem::path mFilePath;
};

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager() = default;

    // Starts a run. There may only be one active run at a time.
    void StartRun(const std::string& name);
    // Concludes the current run.
    void EndRun();
    // Returns whether a run is active.
    bool HasActiveRun() const;

    // Adds a metric to the current run. A run must be started to add a metric.
    // Failure to add a metric returns kInvalidMetricID.
    MetricID AddMetric(const MetricMetadata& metadata);

    // Records data for the given metric ID. Metrics for completed runs will be discarded.
    bool RecordMetricData(MetricID id, const MetricData& data);

    // Exports all the runs and metrics information into a report. Does NOT close the
    // current run.
    Report CreateReport(const std::string& reportPath) const;

    // Get Gauge Basic Statistics, only works for type GAUGE
    GaugeBasicStatistics GetGaugeBasicStatistics(MetricID id) const;

private:
    METRICS_NO_COPY(Manager)

private:
    std::unordered_map<std::string, std::unique_ptr<Run>> mRuns;

    // Set while there's an active Run, otherwise null.
    Run* mActiveRun = nullptr;

    // Must be stored with the manager; Runs should not share MetricIDs.
    MetricID mNextMetricID = kInvalidMetricID + 1;

    // Convenient to store with the manager, so the hop of going through the Run isn't necessary.
    std::unordered_map<MetricID, Metric*> mActiveMetrics;
};

////////////////////////////////////////////////////////////////////////////////

#undef METRICS_NO_COPY

} // namespace metrics
} // namespace ppx

#endif // ppx_metrics_h
