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

#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace ppx {
namespace metrics {

#define METRICS_NO_COPY(type)   \
    type(type&)       = delete; \
    type(const type&) = delete; \
    type& operator=(const type&) = delete;

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
};

////////////////////////////////////////////////////////////////////////////////

class Report final
{
    friend class Manager;

public:
    static constexpr const char* FILE_EXTENSION = ".json";

public:
    Report();
    ~Report();
    bool WriteToFile(const char* filename) const;

private:
    nlohmann::json mContent;
};

////////////////////////////////////////////////////////////////////////////////

// Basic statistics are computed on the fly as metrics entries are recorded.
// They can be retrieved without any significant run-time cost.
struct GaugeBasicStatistics
{
    double min       = std::numeric_limits<double>::min();
    double max       = std::numeric_limits<double>::max();
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
    MetricGauge(const MetricMetadata& metadata);

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

    void Export(nlohmann::json* pOutRunObject) const;

private:
    struct TimeSeriesEntry
    {
        double seconds;
        double value;
    };

private:
    ~MetricGauge();
    METRICS_NO_COPY(MetricGauge)

    void UpdateBasicStatistics(double seconds, double value);

private:
    MetricMetadata               mMetadata;
    std::vector<TimeSeriesEntry> mTimeSeries;
    GaugeBasicStatistics         mBasicStatistics;
    double                       mAccumulatedValue;
};

////////////////////////////////////////////////////////////////////////////////

// A counter metric represents a value that only goes up, e.g. the number
// of stutters or pipeline cache misses.
class MetricCounter final
{
    friend class Run;

public:
    MetricCounter(const MetricMetadata& metadata);

    void     Increment(uint64_t add);
    uint64_t Get() const;

    // Information from the metadata.
    std::string GetName() const
    {
        return mMetadata.name;
    };

    void Export(nlohmann::json* pOutRunObject) const;

private:
    ~MetricCounter();
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
    template <typename T>
    T* AddMetric(MetricMetadata metadata);

private:
    Run(const char* pName);
    ~Run();
    METRICS_NO_COPY(Run)

    void AddMetric(MetricGauge* pMetric);
    void AddMetric(MetricCounter* pMetric);
    bool HasMetric(const char* pName) const;

    void Export(nlohmann::json* pRunObject) const;

private:
    std::string                                     mName;
    std::unordered_map<std::string, MetricGauge*>   mGauges;
    std::unordered_map<std::string, MetricCounter*> mCounters;
};

template <typename T>
T* Run::AddMetric(MetricMetadata metadata)
{
    PPX_ASSERT_MSG(!metadata.name.empty(), "The metric name must not be empty");
    PPX_ASSERT_MSG(!HasMetric(metadata.name.c_str()), "Metrics must have unique names (duplicate name detected)");

    T* pMetric = new T(metadata);
    if (pMetric == nullptr) {
        return nullptr;
    }

    AddMetric(pMetric);
    return pMetric;
}

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager();
    ~Manager();

    Run* AddRun(const char* pName);
    void Export(const char* pName, Report* pOutReport) const;

private:
    METRICS_NO_COPY(Manager)

private:
    std::unordered_map<std::string, Run*> mRuns;
};

////////////////////////////////////////////////////////////////////////////////

#undef METRICS_NO_COPY

} // namespace metrics
} // namespace ppx

#endif // ppx_metrics_h
