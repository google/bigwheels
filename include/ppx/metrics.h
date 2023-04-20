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

#define METRICS_NO_COPY(type)              \
    type(type&)                  = delete; \
    type(const type&)            = delete; \
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

enum class MetricType
{
    GAUGE,
    COUNTER,
};

class Metric
{
    friend class Run;

public:
    const MetricMetadata& GetMetadata() const;
    MetricType            GetType() const;

protected:
    Metric(const MetricMetadata& metadata, MetricType type);
    virtual ~Metric();

private:
    MetricMetadata mMetadata;
    MetricType     mType;
};

////////////////////////////////////////////////////////////////////////////////

struct GaugeStatistics
{
    double min;
    double max;
    double average;
    double median;
    double standardDeviation;
    double timeRatio;
};

class MetricGauge final : public Metric
{
public:
    MetricGauge(const MetricMetadata& metadata);

    void   RecordEntry(double seconds, double value);
    size_t GetEntriesCount() const;
    void   GetEntry(size_t index, double& seconds, double& value) const;

    const GaugeStatistics GetStatistics() const;

private:
    struct TimeSeriesEntry
    {
        double seconds;
        double value;
    };

private:
    ~MetricGauge();
    METRICS_NO_COPY(MetricGauge)

private:
    std::vector<TimeSeriesEntry> mTimeSeries;
};

////////////////////////////////////////////////////////////////////////////////

class MetricCounter final : public Metric
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
    uint64_t mCounter;
};

////////////////////////////////////////////////////////////////////////////////

class Run final
{
    friend class Manager;

public:
    Result       AddMetricGauge(MetricGauge*& outMetric, MetricMetadata metadata);
    MetricGauge* GetMetricGauge(const char* name) const;

    Result         AddMetricCounter(MetricCounter*& outMetric, MetricMetadata metadata);
    MetricCounter* GetMetricCounter(const char* name) const;

private:
    Run(const char* name);
    ~Run();
    METRICS_NO_COPY(Run)

    Metric* GetMetric(const char* name) const;

    template <typename T>
    Result AddMetric(T*& outMetric, MetricMetadata metadata);

private:
    std::string                              mName;
    std::unordered_map<std::string, Metric*> mMetrics;
};

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager();
    ~Manager();

    Result AddRun(Run*& outRun, const char* name);
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
