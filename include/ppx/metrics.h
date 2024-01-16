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

#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ppx {
namespace metrics {

#define METRICS_NO_COPY(TYPE__)                \
    TYPE__(TYPE__&&)                 = delete; \
    TYPE__(const TYPE__&)            = delete; \
    TYPE__& operator=(const TYPE__&) = delete;

////////////////////////////////////////////////////////////////////////////////

typedef uint32_t          MetricID;
static constexpr MetricID kInvalidMetricID = 0;

// +inf/-inf if exist, otherwise the max/lowest floating point value.
static constexpr double kGaugePositiveInf =
    (std::numeric_limits<double>::has_infinity
         ? +std::numeric_limits<double>::infinity()
         : std::numeric_limits<double>::max());
static constexpr double kGaugeNegativeInf =
    (std::numeric_limits<double>::has_infinity
         ? -std::numeric_limits<double>::infinity()
         : std::numeric_limits<double>::lowest());

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
    // Note the default range is (0.0, +inf), not (-inf, +inf)
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
    double min       = kGaugePositiveInf;
    double max       = kGaugeNegativeInf;
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

    const GaugeBasicStatistics GetBasicStatistics() const { return mBasicStats; };

private:
    struct TimeSeriesEntry
    {
        double seconds;
        double value;
    };

private:
    MetricGauge(const MetricMetadata& metadata)
        : mMetadata(metadata)
    {
        PPX_ASSERT_MSG(mMetadata.type == MetricType::GAUGE, "Gauge must be instantiated with gauge-type metadata!");
    }
    METRICS_NO_COPY(MetricGauge)

    GaugeComplexStatistics ComputeComplexStats() const;

private:
    MetricMetadata               mMetadata;
    std::vector<TimeSeriesEntry> mTimeSeries;
    GaugeBasicStatistics         mBasicStats;
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

// Live statistics are computerd on the fly, for recently reported metric.
// The weight assigned to each entry is assigned as follows
//   w_i = exp((t_i-t_now)/halfLife), default halfLife = 0.5s
// Min/Max are not affected by the weight.
// They can be retrieved without any significant run-time cost.
struct LiveStatistics
{
    double Latest() const { return latest; }
    double Min() const { return min; }
    double Max() const { return max; }
    double Mean() const { return mean; }
    double Variance() const { return PopulationVariance(); }
    double StandardDeviation() const { return std::sqrt(PopulationVariance()); }

    double SampleVariance() const { return variance * weight / (weight - 1.0); }
    double PopulationVariance() const { return variance; }

    double latest   = 0.0;
    double seconds  = 0.0;
    double mean     = 0.0;
    double variance = 0.0;
    double weight   = 0.0;
    double min      = kGaugePositiveInf;
    double max      = kGaugeNegativeInf;
};

template <typename FloatT>
struct ParallelVariance
{
    static constexpr FloatT kInvalidVariance = std::numeric_limits<FloatT>::quiet_NaN();

    FloatT weight = 0.0;
    FloatT mean   = 0.0;
    FloatT accVar = 0.0;

    FloatT Weight() const { return weight; }
    FloatT Mean() const { return mean; }
    FloatT PopulationVariance() const
    {
        if (weight == 0.0) {
            return kInvalidVariance;
        }
        return accVar / weight;
    }
    FloatT SampleVariance() const
    {
        if (weight - 1.0 < std::numeric_limits<FloatT>::epsilon()) {
            return kInvalidVariance;
        }
        return accVar / (weight - 1.0);
    }

    static ParallelVariance Combine(const ParallelVariance& a, const ParallelVariance& b)
    {
        // Chan et al. online variance algorithm
        // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm
        //
        // Note: Large weights might cause problem with mean calculation.
        const FloatT delta  = b.mean - a.mean;
        const FloatT weight = a.weight + b.weight;
        const FloatT mean   = a.mean + delta * (b.weight / weight);
        const FloatT accVar = a.accVar + b.accVar + (delta * delta) * (a.weight * b.weight / weight);
        return {weight, mean, accVar};
    }
};

class LiveMetric // Not an exportable metric type
{
public:
    static constexpr double kDefaultHalfLife = 0.5;

    // halfLive is the weight assigned to each data point, does not affect min/max.
    LiveMetric(double halfLive = kDefaultHalfLife);

    const LiveStatistics& GetLiveStatistics() const { return mStats; }

    bool RecordEntry(const MetricData& data);
    void ClearHistory();

private:
    double                   mHalfLife = 0.5;
    LiveStatistics           mStats    = {};
    ParallelVariance<double> mVar      = {};

    void Append(double seconds, double value);
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

    // Allocate a MetricID.
    MetricID AllocateID() { return EnsureAllocateID(kInvalidMetricID); }

    // Adds a metric to the current run. A run must be started to add a metric.
    // Failure to add a metric returns kInvalidMetricID.
    // Optionally bind the metric to an existing metricID.
    MetricID AddMetric(
        const MetricMetadata& metadata,
        MetricID              metricID = kInvalidMetricID);

    // Adds a live metric.
    // Optionally bind the metric to an existing metricID.
    MetricID AddLiveMetric(
        double   halfLife = LiveMetric::kDefaultHalfLife,
        MetricID metricID = kInvalidMetricID);

    // Adds a live metric to current run.
    void BindMetric(MetricID liveMetricID, const MetricMetadata& metadata);

    // Records data for the given metric ID. Metrics for completed runs will be discarded.
    bool RecordMetricData(MetricID id, const MetricData& data);

    // Exports all the runs and metrics information into a report. Does NOT close the
    // current run.
    Report CreateReport(const std::string& reportPath) const;

    // Get Gauge Basic Statistics, only works for type GAUGE
    GaugeBasicStatistics GetGaugeBasicStatistics(MetricID id) const;

    // Get Live Statistics for a LiveMetric, only works for type GAUGE
    LiveStatistics GetLiveStatistics(MetricID id) const;

    // Reset live statistics.
    void ClearLiveMetricsHistory();

private:
    METRICS_NO_COPY(Manager)

private:
    std::unordered_map<std::string, std::unique_ptr<Run>> mRuns;

    // Set while there's an active Run, otherwise null.
    Run* mActiveRun = nullptr;

    // Must be stored with the manager; Runs should not share MetricIDs.
    MetricID mNextMetricID = kInvalidMetricID + 1;
    MetricID EnsureAllocateID(MetricID reuseID);

    // Convenient to store with the manager, so the hop of going through the Run isn't necessary.
    std::unordered_map<MetricID, Metric*> mActiveMetrics;

    // Record live statistics, even without an active run.
    std::unordered_map<MetricID, LiveMetric> mLiveMetrics;
};

////////////////////////////////////////////////////////////////////////////////

#undef METRICS_NO_COPY

} // namespace metrics
} // namespace ppx

#endif // ppx_metrics_h
