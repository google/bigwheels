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

#include "gtest/gtest.h"

#include "ppx/metrics.h"

namespace ppx {

TEST(MetricsTest, ManagerAddRunSingle)
{
    metrics::Manager      manager;
    metrics::Run*         run;
    constexpr const char* RUN_NAME = "run";
    const Result          result   = manager.AddRun(run, RUN_NAME);
    ASSERT_EQ(result, SUCCESS);
    ASSERT_EQ(run, manager.GetRun(RUN_NAME));
    ASSERT_EQ(nullptr, manager.GetRun("dummy"));
}

TEST(MetricsTest, ManagerAddRunMultiple)
{
    metrics::Manager manager;

    constexpr const char* RUN_NAME_0 = "run0";
    constexpr const char* RUN_NAME_1 = "run1";

    metrics::Run* run0;
    metrics::Run* run1;

    {
        const Result result = manager.AddRun(run0, RUN_NAME_0);
        ASSERT_EQ(result, SUCCESS);
    }
    {
        const Result result = manager.AddRun(run1, RUN_NAME_1);
        ASSERT_EQ(result, SUCCESS);
    }
    {
        ASSERT_EQ(run0, manager.GetRun(RUN_NAME_0));
        ASSERT_EQ(run1, manager.GetRun(RUN_NAME_1));
        ASSERT_EQ(nullptr, manager.GetRun("dummy"));
    }
}

TEST(MetricsTest, ManagerAddRunDuplicate)
{
    metrics::Manager      manager;
    constexpr const char* RUN_NAME = "run";
    {
        metrics::Run* run;
        const Result  result = manager.AddRun(run, RUN_NAME);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_NE(run, nullptr);
    }
    {
        metrics::Run* run;
        const Result  result = manager.AddRun(run, RUN_NAME);
        ASSERT_EQ(result, ERROR_DUPLICATE_ELEMENT);
        ASSERT_EQ(run, nullptr);
    }
}

TEST(MetricsTest, RunAddMetricSingle)
{
    metrics::Manager manager;
    {
        Result result;

        metrics::Run* run;
        result = manager.AddRun(run, "run_gauge");
        ASSERT_EQ(result, SUCCESS);

        constexpr const char*   METRIC_NAME = "metric";
        metrics::MetricMetadata metadata    = {};
        metadata.name                       = METRIC_NAME;
        metrics::MetricGauge* metric;
        result = run->AddMetricGauge(metric, metadata);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_EQ(metric, run->GetMetricGauge(METRIC_NAME));
        ASSERT_EQ(nullptr, run->GetMetricCounter(METRIC_NAME));
        ASSERT_EQ(nullptr, run->GetMetricGauge("dummy"));
    }

    {
        Result result;

        metrics::Run* run;
        result = manager.AddRun(run, "run_counter");
        ASSERT_EQ(result, SUCCESS);

        constexpr const char*   METRIC_NAME = "metric";
        metrics::MetricMetadata metadata    = {};
        metadata.name                       = METRIC_NAME;
        metrics::MetricCounter* metric;
        result = run->AddMetricCounter(metric, metadata);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_EQ(metric, run->GetMetricCounter(METRIC_NAME));
        ASSERT_EQ(nullptr, run->GetMetricGauge(METRIC_NAME));
        ASSERT_EQ(nullptr, run->GetMetricCounter("dummy"));
    }
}

TEST(MetricsTest, RunAddMetricMultiple)
{
    Result           result;
    metrics::Manager manager;
    metrics::Run*    run;
    result = manager.AddRun(run, "run");
    ASSERT_EQ(result, SUCCESS);

    constexpr const char* METRIC_NAME_GAUGE = "metric_gauge";
    metrics::MetricGauge* metricGauge;
    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = METRIC_NAME_GAUGE;

        result = run->AddMetricGauge(metricGauge, metadata);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_NE(metricGauge, nullptr);
    }

    constexpr const char*   METRIC_NAME_COUNTER = "metric_counter";
    metrics::MetricCounter* metricCounter;
    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = METRIC_NAME_COUNTER;

        result = run->AddMetricCounter(metricCounter, metadata);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_NE(metricCounter, nullptr);
    }

    ASSERT_EQ(run->GetMetricGauge(METRIC_NAME_GAUGE), metricGauge);
    ASSERT_EQ(run->GetMetricGauge(METRIC_NAME_COUNTER), nullptr);
    ASSERT_EQ(run->GetMetricCounter(METRIC_NAME_COUNTER), metricCounter);
    ASSERT_EQ(run->GetMetricCounter(METRIC_NAME_GAUGE), nullptr);
}

TEST(MetricsTest, RunAddMetricDuplicate)
{
    Result           result;
    metrics::Manager manager;
    metrics::Run*    run;
    result = manager.AddRun(run, "run");
    ASSERT_EQ(result, SUCCESS);

    constexpr const char*   METRIC_NAME = "metric";
    metrics::MetricMetadata metadata    = {};
    metadata.name                       = METRIC_NAME;

    metrics::MetricGauge* metricGauge;
    result = run->AddMetricGauge(metricGauge, metadata);
    ASSERT_EQ(result, SUCCESS);
    ASSERT_NE(metricGauge, nullptr);

    result = run->AddMetricGauge(metricGauge, metadata);
    ASSERT_EQ(result, ERROR_DUPLICATE_ELEMENT);
    ASSERT_EQ(metricGauge, nullptr);

    metrics::MetricCounter* metricCounter;
    result = run->AddMetricCounter(metricCounter, metadata);
    ASSERT_EQ(result, ERROR_DUPLICATE_ELEMENT);
    ASSERT_EQ(metricCounter, nullptr);
}

} // namespace ppx
