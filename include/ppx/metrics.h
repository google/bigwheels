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

// Basic statistics are computed on the fly as metrics entries are recorded.
// They can be retrieved without any significant run-time cost.
struct GaugeBasicStatistics
{
    double min;
    double max;
    double average;
    double timeRatio;
};

// Complex statistics cannot be computed on the fly.
// They require significant computation (e.g. sorting).
struct GaugeComplexStatistics
{
    double median;
    double standardDeviation;
    double percentile90;
    double percentile95;
    double percentile99;
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
    void   RecordEntry(double seconds, double value);

    size_t GetEntriesCount() const;
    void   GetEntry(size_t index, double& seconds, double& value) const;

    const GaugeBasicStatistics GetBasicStatistics() const;
    const GaugeComplexStatistics ComputeComplexStatistics() const;

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
    MetricMetadata mMetadata;
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

    uint64_t Increment(uint64_t add);
    uint64_t Get() const;

private:
    ~MetricCounter();
    METRICS_NO_COPY(MetricCounter)

private:
    MetricMetadata mMetadata;
    uint64_t mCounter;
};

////////////////////////////////////////////////////////////////////////////////

class Run final
{
    friend class Manager;

public:
    template <typename T>
    T* AddMetric(MetricMetadata metadata);

private:
    Run(const char* name);
    ~Run();
    METRICS_NO_COPY(Run)

    void AddMetric(MetricGauge* metric);
    void AddMetric(MetricCounter* metric);
    bool HasMetric(const char* name) const;

private:
    std::string                              mName;
    std::unordered_map<std::string, MetricGauge*> mGauges;
    std::unordered_map<std::string, MetricCounter*> mCounters;
};

template <typename T>
T* Run::AddMetric(MetricMetadata metadata)
{
    PPX_ASSERT_MSG(!metadata.name.empty(), "The metric name must not be empty");
    PPX_ASSERT_MSG(!HasMetric(metadata.name.c_str()), "Metrics must have unique names (duplicate name detected)");

    T* metric = new T(metadata);
    if (metric == nullptr) {
        return nullptr;
    }

    AddMetric(metric);
    return metric;
}

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager();
    ~Manager();

    Result AddRun(const char* name, Run** outRun);
    Run*   GetRun(const char* name) const;

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
