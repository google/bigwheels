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

#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace ppx {
namespace metrics {

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
    std::string name;
	std::string unit;
	MetricInterpretation interpretation = MetricInterpretation::NONE;
	Range expectedRange;
};

////////////////////////////////////////////////////////////////////////////////

enum class MetricType
{
    GAUGE,
    COUNTER,
};

class Metric
{
public:
	const MetricMetadata& GetMetadata() const;
    MetricType GetType() const;

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

	void RecordEntry(double seconds, double value);
	size_t GetEntriesCount() const;
	void GetEntry(size_t index, double& seconds, double& value) const;

	const GaugeStatistics GetStatistics() const;

private:
	struct TimeSeriesEntry
	{
		double seconds;
		double value;
	};

private:
    std::vector<TimeSeriesEntry> mTimeSeries;
};

////////////////////////////////////////////////////////////////////////////////

class MetricCounter final : public Metric
{
public:
	MetricCounter(const MetricMetadata& metadata);
	uint64_t Increment(uint64_t add);
	uint64_t Get() const;

private:
	uint64_t mCounter;
};

////////////////////////////////////////////////////////////////////////////////

class Run final
{
public:
    Run(const char* name);
    ~Run();
	MetricGauge* AddMetricGauge(MetricMetadata metadata);
	MetricCounter* AddMetricCounter(MetricMetadata metadata);

private:
    std::string mName;
    std::unordered_map<std::string, Metric*> mMetrics;
};

////////////////////////////////////////////////////////////////////////////////

class Manager final
{
public:
    Manager();
    ~Manager();
    Run* AddRun(const char* name);

private:
    std::unordered_map<std::string, Run*> mRuns;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace metrics
} // namespace ppx

#endif // ppx_metrics_h
