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

////////////////////////////////////////////////////////////////////////////////
// Fixture
////////////////////////////////////////////////////////////////////////////////

class MetricsTestFixture : public ::testing::Test
{
protected:
    inline static const char* DEFAULT_RUN_NAME = "default_run";

protected:
    void SetUp() override
    {
        pManager = new metrics::Manager();
        ASSERT_NE(pManager, nullptr);
        const Result result = pManager->AddRun(pRun, DEFAULT_RUN_NAME);
        ASSERT_EQ(result, SUCCESS);
        ASSERT_EQ(pRun, pManager->GetRun(DEFAULT_RUN_NAME));
        ASSERT_EQ(nullptr, pManager->GetRun("dummy"));
    }

    void TearDown() override
    {
        if (pManager != nullptr) {
            delete pManager;
            pManager = nullptr;
        }
    }

protected:
    metrics::Manager* pManager;
    metrics::Run*     pRun;
};

////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, ManagerAddSingleRun)
{
    metrics::Manager      manager;
    metrics::Run*         run;
    constexpr const char* RUN_NAME = "run";
    const Result          result   = manager.AddRun(run, RUN_NAME);
    ASSERT_EQ(result, SUCCESS);
    ASSERT_EQ(run, manager.GetRun(RUN_NAME));
    ASSERT_EQ(nullptr, manager.GetRun("dummy"));
}

TEST(MetricsTest, ManagerAddMultipleRun)
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

TEST(MetricsTest, ManagerAddDuplicateRun)
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

////////////////////////////////////////////////////////////////////////////////
// Run
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, RunAddSingleMetric)
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

TEST(MetricsTest, RunAddMultipleMetric)
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

TEST(MetricsTest, RunAddDuplicateMetric)
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

////////////////////////////////////////////////////////////////////////////////
// Metrics
////////////////////////////////////////////////////////////////////////////////

TEST_F(MetricsTestFixture, MetricsCounter)
{
    constexpr const char*   METRIC_NAME = "counter";
    metrics::MetricCounter* pMetric;
    metrics::MetricMetadata metadata;
    metadata.name       = METRIC_NAME;
    const Result result = pRun->AddMetricCounter(pMetric, metadata);
    ASSERT_EQ(pMetric->GetMetadata().name, METRIC_NAME);
    ASSERT_EQ(pMetric->GetType(), metrics::MetricType::COUNTER);
    ASSERT_EQ(pMetric->Get(), 0U);

    pMetric->Increment(1U);
    ASSERT_EQ(pMetric->Get(), 1U);
    pMetric->Increment(4U);
    ASSERT_EQ(pMetric->Get(), 5U);
}

TEST_F(MetricsTestFixture, MetricsGaugeEntries)
{
    constexpr const char*   METRIC_NAME = "frame_time";
    metrics::MetricGauge*   pMetric;
    metrics::MetricMetadata metadata;
    metadata.name       = METRIC_NAME;
    const Result result = pRun->AddMetricGauge(pMetric, metadata);
    ASSERT_EQ(pMetric->GetMetadata().name, METRIC_NAME);
    ASSERT_EQ(pMetric->GetType(), metrics::MetricType::GAUGE);

    ASSERT_EQ(pMetric->GetEntriesCount(), 0U);
    pMetric->RecordEntry(0.0000, 11.0);
    ASSERT_EQ(pMetric->GetEntriesCount(), 1U);
    pMetric->RecordEntry(0.0110, 11.7);
    ASSERT_EQ(pMetric->GetEntriesCount(), 2U);
    pMetric->RecordEntry(0.0227, 12.2);
    ASSERT_EQ(pMetric->GetEntriesCount(), 3U);
    pMetric->RecordEntry(0.0349, 10.8);
    ASSERT_EQ(pMetric->GetEntriesCount(), 4U);
    pMetric->RecordEntry(0.0457, 11.1);
    ASSERT_EQ(pMetric->GetEntriesCount(), 5U);

    double seconds;
    double value;
    pMetric->GetEntry(0, seconds, value);
    ASSERT_EQ(seconds, 0.0000);
    ASSERT_EQ(value, 11.0);
    pMetric->GetEntry(4, seconds, value);
    ASSERT_EQ(seconds, 0.0457);
    ASSERT_EQ(value, 11.1);
}

} // namespace ppx
