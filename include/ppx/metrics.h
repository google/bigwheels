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
#include <vector>

namespace ppx {
namespace metrics {

#define METRICS_NO_COPY(TYPE__)     \
    TYPE__(TYPE__&)       = delete; \
    TYPE__(const TYPE__&) = delete; \
    TYPE__& operator=(const TYPE__&) = delete;

////////////////////////////////////////////////////////////////////////////////

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
    std::string          name;
    std::string          unit;
    MetricInterpretation interpretation = MetricInterpretation::NONE;
    Range                expectedRange;

    nlohmann::json Export() const;
};

////////////////////////////////////////////////////////////////////////////////

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
    double percentile90      = 0.0;
    double percentile95      = 0.0;
    double percentile99      = 0.0;
};

// A gauge metric represents a value that may increase or decrease over time.
// The value is sampled frequently (e.g. every frame) and statistics can be
// derived from the sampling process.
// The most typical case is the frame time, but memory consumption and image
// quality are also good examples.
class MetricGauge final
{
    friend class Run;

public:
    // Record a measurement for the metric at a particular point in time.
    // Each entry must have a positive 'seconds' that is greater than the
    // 'seconds' of the previous entry (i.e. 'seconds' must form a stricly
    // increasing positive function).
    // Note however that the system does NOT assume that the 'seconds' of
    // the first entry equal zero.
    void RecordEntry(double seconds, double value);

    // Entries can be retrieved using these two functions.
    // 'index' should be between 0 and 'GetEntriesCount() - 1'.
    size_t GetEntriesCount() const;
    void   GetEntry(size_t index, double* pSeconds, double* pValue) const;

    // Information from the metadata.
    std::string GetName() const
    {
        return mMetadata.name;
    };

    const GaugeBasicStatistics   GetBasicStatistics() const;
    const GaugeComplexStatistics ComputeComplexStatistics() const;

    nlohmann::json Export() const;

private:
    struct TimeSeriesEntry
    {
        double seconds;
        double value;
    };

private:
    MetricGauge(const MetricMetadata& metadata)
        : mMetadata(metadata) {}
    METRICS_NO_COPY(MetricGauge)

    void UpdateBasicStatistics(double seconds, double value);

private:
    MetricMetadata               mMetadata;
    std::vector<TimeSeriesEntry> mTimeSeries;
    GaugeBasicStatistics         mBasicStatistics;
    double                       mAccumulatedValue = 0.0;
};

////////////////////////////////////////////////////////////////////////////////

// A counter metric represents a value that only goes up, e.g. the number
// of stutters or pipeline cache misses.
class MetricCounter final
{
    friend class Run;

public:
    void     Increment(uint64_t add);
    uint64_t Get() const;

    // Information from the metadata.
    std::string GetName() const
    {
        return mMetadata.name;
    };

    nlohmann::json Export() const;

private:
    MetricCounter(const MetricMetadata& metadata)
        : mMetadata(metadata), mCounter(0U) {}
    METRICS_NO_COPY(MetricCounter)

private:
    MetricMetadata mMetadata;
    uint64_t       mCounter;
};

////////////////////////////////////////////////////////////////////////////////

// A run gathers metrics relevant to the execution of a benchmark.
// It is expected that a new run is created each time parameters that affect the
// metrics measurements are changed.
class Run final
{
    friend class Manager;

public:
    // TODO(slumpwuffle): The lifetime of the pointer returned by this function is not well-defined
    // to the caller, and its expiration cannot be known directly. Instead, we may want to use either
    // strong/weak pointers or functional access to help prevent unexpected use-after-free situations.
    template <typename T>
    T* AddMetric(MetricMetadata metadata)
    {
        PPX_ASSERT_MSG(!metadata.name.empty(), "The metric name must not be empty");
        PPX_ASSERT_MSG(!HasMetric(metadata.name), "Metrics must have unique names (duplicate name detected)");

        auto* pMetric = new T(metadata);
        auto  metric  = std::unique_ptr<T>(pMetric);
        AddMetric(std::move(metric));
        return pMetric;
    }

private:
    Run(const std::string& name)
        : mName(name) {}
    METRICS_NO_COPY(Run)

    void AddMetric(std::unique_ptr<MetricGauge>&& metric);
    void AddMetric(std::unique_ptr<MetricCounter>&& metric);
    bool HasMetric(const std::string& name) const;

    nlohmann::json Export() const;

private:
    std::string                                     mName;
    std::unordered_map<std::string, std::unique_ptr<MetricGauge>>   mGauges;
    std::unordered_map<std::string, std::unique_ptr<MetricCounter>> mCounters;
};

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager() = default;

    Run* AddRun(const std::string& name);

    // Exports all the runs and metrics information into a report to disk.
    void ExportToDisk(const std::string& baseReportName) const;

private:
    METRICS_NO_COPY(Manager)

    // A report contains runs and metrics information meant to be saved to disk.
    // Because the json object at its core relies on strings-as-pointers, the
    // lifecycle of a report is tied to the lifecycle of the runs and metrics
    // owned by the Manager. But this is opaque. To protect misuse, the class
    // is a private member.
    class Report final
    {
    public:
        static constexpr const char* kFileExtension = ".json";

    public:
        // Copy constructor for content.
        Report(const nlohmann::json& content, const std::string& baseReportName);
        // Move constructor for content.
        Report(nlohmann::json&& content, const std::string& baseReportName);

        void WriteToDisk(bool overwriteExisting = false) const;

    private:
        void SetReportName(const std::string& baseReportName);

        nlohmann::json        mContent;
        std::string           mReportName;
        std::filesystem::path mFilePath;
    };

private:
    std::unordered_map<std::string, std::unique_ptr<Run>> mRuns;
};

////////////////////////////////////////////////////////////////////////////////

#undef METRICS_NO_COPY

} // namespace metrics
} // namespace ppx

#endif // ppx_metrics_h
