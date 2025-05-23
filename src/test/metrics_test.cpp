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

#include "nlohmann/json.hpp"

#include <memory>
#include <limits>
#include <regex>

#if !defined(NDEBUG)
#define PERFORM_DEATH_TESTS
#endif

namespace ppx {

////////////////////////////////////////////////////////////////////////////////
// Fixture
////////////////////////////////////////////////////////////////////////////////

class MetricsTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        pManager = std::make_unique<metrics::Manager>();
        pManager->StartRun("default_run");
    }

    void TearDown() override
    {
        pManager.reset();
    }

protected:
    std::unique_ptr<metrics::Manager> pManager;
};

////////////////////////////////////////////////////////////////////////////////
// Manager Tests
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, ManagerActiveRunInit)
{
    metrics::Manager manager;
    EXPECT_FALSE(manager.HasActiveRun());
}

TEST(MetricsTest, ManagerActiveRunAfterStart)
{
    metrics::Manager manager;
    manager.StartRun("run");
    EXPECT_TRUE(manager.HasActiveRun());
}

TEST(MetricsTest, ManagerActiveRunAfterStop)
{
    metrics::Manager manager;
    manager.StartRun("run");
    manager.EndRun();
    EXPECT_FALSE(manager.HasActiveRun());
}

TEST(MetricsTest, ManagerActiveRunAfterStopAndStart)
{
    metrics::Manager manager;
    manager.StartRun("run1");
    manager.EndRun();
    manager.StartRun("run2");
    EXPECT_TRUE(manager.HasActiveRun());
}

#if defined(PERFORM_DEATH_TESTS)
TEST(MetricsTest, ManagerAddRunWithEmptyNameFails)
{
    metrics::Manager manager;
    EXPECT_DEATH({ manager.StartRun(""); }, "");
}

TEST(MetricsTest, ManagerStartSimultaneousRunsFails)
{
    metrics::Manager manager;
    manager.StartRun("run0");
    EXPECT_DEATH({ manager.StartRun("run1"); }, "");
}

TEST(MetricsTest, ManagerStartDuplicateRunFails)
{
    metrics::Manager manager;
    manager.StartRun("run");
    manager.EndRun();
    EXPECT_DEATH({ manager.StartRun("run"); }, "");
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Run Tests
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, ManagerAddMetricWithNoRun)
{
    metrics::Manager        manager;
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric";
    auto metricId                    = manager.AddMetric(metadata);
    EXPECT_EQ(metricId, metrics::kInvalidMetricID);
}

TEST_F(MetricsTestFixture, ManagerAddSingleMetricCounter)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric";
    auto metricId                    = pManager->AddMetric(metadata);
    EXPECT_NE(metricId, metrics::kInvalidMetricID);
}

TEST_F(MetricsTestFixture, ManagerAddSingleMetricGauge)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::GAUGE;
    metadata.name                    = "metric";
    auto metricId                    = pManager->AddMetric(metadata);
    EXPECT_NE(metricId, metrics::kInvalidMetricID);
}

TEST_F(MetricsTestFixture, ManagerAddMultipleMetrics)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric1";
    auto metricId1                   = pManager->AddMetric(metadata);
    // Change the name to guarantee this is JUST about the name.
    metadata.name  = "metric2";
    auto metricId2 = pManager->AddMetric(metadata);
    EXPECT_NE(metricId1, metrics::kInvalidMetricID);
    EXPECT_NE(metricId2, metrics::kInvalidMetricID);
    EXPECT_NE(metricId1, metricId2);
}

TEST_F(MetricsTestFixture, ManagerAddManyTypesOfMetrics)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric1";
    auto metricId1                   = pManager->AddMetric(metadata);
    metadata.type                    = metrics::MetricType::GAUGE;
    metadata.name                    = "metric2";
    auto metricId2                   = pManager->AddMetric(metadata);
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric3";
    auto metricId3                   = pManager->AddMetric(metadata);
    EXPECT_NE(metricId1, metrics::kInvalidMetricID);
    EXPECT_NE(metricId2, metrics::kInvalidMetricID);
    EXPECT_NE(metricId3, metrics::kInvalidMetricID);
    EXPECT_NE(metricId1, metricId2);
    EXPECT_NE(metricId1, metricId3);
    EXPECT_NE(metricId2, metricId3);
}

TEST_F(MetricsTestFixture, ManagerAddMetricWithEmptyName)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    auto metricId                    = pManager->AddMetric(metadata);
    EXPECT_EQ(metricId, metrics::kInvalidMetricID);
}

TEST_F(MetricsTestFixture, ManagerAddMetricWithInvalidType)
{
    metrics::MetricMetadata metadata = {};
    metadata.name                    = "metric";
    auto metricId                    = pManager->AddMetric(metadata);
    EXPECT_EQ(metricId, metrics::kInvalidMetricID);
}

TEST_F(MetricsTestFixture, ManagerAddDuplicateMetricName)
{
    metrics::MetricMetadata metadata = {};
    metadata.type                    = metrics::MetricType::COUNTER;
    metadata.name                    = "metric";
    auto metricId1                   = pManager->AddMetric(metadata);
    ASSERT_NE(metricId1, metrics::kInvalidMetricID);
    // Change the type to guarantee this is JUST about the name.
    metadata.type  = metrics::MetricType::GAUGE;
    auto metricId2 = pManager->AddMetric(metadata);
    EXPECT_EQ(metricId2, metrics::kInvalidMetricID);
}

////////////////////////////////////////////////////////////////////////////////
// Report Tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(MetricsTestFixture, ReportEmptyRun)
{
    auto           result = pManager->CreateReport("report1.json").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    EXPECT_EQ(parsed["filename"], "report1.json");
    ASSERT_EQ(parsed["runs"].size(), 1);
    auto run = parsed["runs"][0];
    EXPECT_EQ(run["name"], "default_run");
    EXPECT_EQ(run["gauges"].size(), 0);
    EXPECT_EQ(run["counters"].size(), 0);
}

TEST_F(MetricsTestFixture, ReportAddsTimestampWithAt)
{
    auto           result   = pManager->CreateReport("re_@_port_@.json").GetContentString();
    nlohmann::json parsed   = nlohmann::json::parse(result);
    std::string    filename = parsed["filename"];
    std::regex     regex("re_([\\d]+)_port_([\\d]+).json");
    std::smatch    match;
    ASSERT_TRUE(std::regex_match(filename, match, regex));
    ASSERT_EQ(match.size(), 3);
    // The timestamps should be the same.
    EXPECT_EQ(match[1].str(), match[2].str());
}

TEST_F(MetricsTestFixture, ReportEmptyCounter)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::COUNTER;
    metadata.name = "counter1";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    ASSERT_EQ(parsed["runs"].size(), 1);
    auto run = parsed["runs"][0];
    ASSERT_EQ(run["counters"].size(), 1);
    auto counter = run["counters"][0];
    EXPECT_EQ(counter["metadata"]["name"], "counter1");
    EXPECT_EQ(counter["value"], 0);
    EXPECT_EQ(counter["entry_count"], 0);
}

TEST_F(MetricsTestFixture, ReportEmptyGauge)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::GAUGE;
    metadata.name = "gauge1";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    ASSERT_EQ(parsed["runs"].size(), 1);
    auto run = parsed["runs"][0];
    ASSERT_EQ(run["gauges"].size(), 1);
    auto gauge = run["gauges"][0];
    EXPECT_EQ(gauge["metadata"]["name"], "gauge1");
    EXPECT_EQ(gauge["time_series"].size(), 0);
    auto stats = gauge["statistics"];
    EXPECT_EQ(stats["min"], std::numeric_limits<double>::max());
    EXPECT_EQ(stats["max"], std::numeric_limits<double>::min());
    EXPECT_EQ(stats["average"], 0);
    EXPECT_EQ(stats["time_ratio"], 0);
    EXPECT_EQ(stats["median"], 0);
    EXPECT_EQ(stats["standard_deviation"], 0);
    EXPECT_EQ(stats["percentile_01"], 0);
    EXPECT_EQ(stats["percentile_05"], 0);
    EXPECT_EQ(stats["percentile_10"], 0);
    EXPECT_EQ(stats["percentile_90"], 0);
    EXPECT_EQ(stats["percentile_95"], 0);
    EXPECT_EQ(stats["percentile_99"], 0);
}

////////////////////////////////////////////////////////////////////////////////
// Metrics Tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(MetricsTestFixture, MetricsBasicCounter)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::COUNTER;
    metadata.name = "counter";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result  = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed  = nlohmann::json::parse(result);
    auto           counter = parsed["runs"][0]["counters"][0];
    EXPECT_EQ(counter["value"], 0);
    EXPECT_EQ(counter["entry_count"], 0);

    metrics::MetricData data = {metrics::MetricType::COUNTER};
    data.counter.increment   = 1;
    pManager->RecordMetricData(metricId, data);

    result  = pManager->CreateReport("report").GetContentString();
    parsed  = nlohmann::json::parse(result);
    counter = parsed["runs"][0]["counters"][0];
    EXPECT_EQ(counter["value"], 1);
    EXPECT_EQ(counter["entry_count"], 1);

    data.counter.increment = 4;
    pManager->RecordMetricData(metricId, data);

    result  = pManager->CreateReport("report").GetContentString();
    parsed  = nlohmann::json::parse(result);
    counter = parsed["runs"][0]["counters"][0];
    EXPECT_EQ(counter["value"], 5);
    EXPECT_EQ(counter["entry_count"], 2);
}

TEST_F(MetricsTestFixture, MetricsCounterIgnoresGaugeData)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::COUNTER;
    metadata.name = "counter";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result  = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed  = nlohmann::json::parse(result);
    auto           counter = parsed["runs"][0]["counters"][0];
    EXPECT_EQ(counter["value"], 0);
    EXPECT_EQ(counter["entry_count"], 0);

    metrics::MetricData data = {metrics::MetricType::GAUGE};
    data.counter.increment   = 1;
    EXPECT_FALSE(pManager->RecordMetricData(metricId, data));

    result  = pManager->CreateReport("report").GetContentString();
    parsed  = nlohmann::json::parse(result);
    counter = parsed["runs"][0]["counters"][0];
    EXPECT_EQ(counter["value"], 0);
    EXPECT_EQ(counter["entry_count"], 0);
}

TEST_F(MetricsTestFixture, MetricsBasicGauge)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::GAUGE;
    metadata.name = "gauge";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    auto           gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 0);

    metrics::MetricData data = {metrics::MetricType::GAUGE};
    data.gauge.seconds       = 0.0;
    data.gauge.value         = 10.0;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.1;
    data.gauge.value   = 10.5;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.2;
    data.gauge.value   = 11.0;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.3;
    data.gauge.value   = 11.0;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.4;
    data.gauge.value   = 12.5;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.5;
    data.gauge.value   = 10.5;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.6;
    data.gauge.value   = 11.5;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.7;
    data.gauge.value   = 11.0;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.8;
    data.gauge.value   = 10.0;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 0.9;
    data.gauge.value   = 10.5;
    pManager->RecordMetricData(metricId, data);
    data.gauge.seconds = 1.0;
    data.gauge.value   = 12.0;
    pManager->RecordMetricData(metricId, data);

    result = pManager->CreateReport("report").GetContentString();
    parsed = nlohmann::json::parse(result);
    gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 11);
    EXPECT_EQ(gauge["time_series"][0][0], 0.0000);
    EXPECT_EQ(gauge["time_series"][0][1], 10.0);
    EXPECT_EQ(gauge["time_series"][5][0], 0.5);
    EXPECT_EQ(gauge["time_series"][5][1], 10.5);
    EXPECT_EQ(gauge["time_series"][10][0], 1.0);
    EXPECT_EQ(gauge["time_series"][10][1], 12.0);

    auto stats = gauge["statistics"];
    EXPECT_EQ(stats["min"], 10.0);
    EXPECT_EQ(stats["max"], 12.5);
    EXPECT_EQ(stats["average"], 10.954545454545455);
    EXPECT_EQ(stats["time_ratio"], 120.5);
    EXPECT_EQ(stats["median"], 11.0);
    EXPECT_EQ(stats["standard_deviation"], 0.7524066071475841);
    EXPECT_EQ(stats["percentile_01"], 10.0);
    EXPECT_EQ(stats["percentile_05"], 10.0);
    EXPECT_EQ(stats["percentile_10"], 10.0);
    EXPECT_EQ(stats["percentile_90"], 12.0);
    EXPECT_EQ(stats["percentile_95"], 12.5);
    EXPECT_EQ(stats["percentile_99"], 12.5);
}

TEST_F(MetricsTestFixture, MetricsGaugeIgnoresNegativeSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::GAUGE;
    metadata.name = "gauge";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    auto           gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 0);

    metrics::MetricData data = {metrics::MetricType::GAUGE};
    data.gauge.seconds       = -1.000;
    data.gauge.value         = 10.0;
    pManager->RecordMetricData(metricId, data);

    // Invalid data point should be ignored.
    result = pManager->CreateReport("report").GetContentString();
    parsed = nlohmann::json::parse(result);
    gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 0);
}

TEST_F(MetricsTestFixture, MetricsGaugeIgnoresDecreasingSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::GAUGE;
    metadata.name = "gauge";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    auto           gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 0);

    metrics::MetricData data = {metrics::MetricType::GAUGE};
    data.gauge.seconds       = 0.000;
    data.gauge.value         = 10.0;
    EXPECT_TRUE(pManager->RecordMetricData(metricId, data));
    data.gauge.seconds = 1.000;
    data.gauge.value   = 11.0;
    EXPECT_TRUE(pManager->RecordMetricData(metricId, data));
    data.gauge.seconds = 0.500;
    data.gauge.value   = 12.0;
    EXPECT_FALSE(pManager->RecordMetricData(metricId, data));

    // Invalid data point should be ignored.
    result = pManager->CreateReport("report").GetContentString();
    parsed = nlohmann::json::parse(result);
    gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 2);
    EXPECT_EQ(gauge["time_series"][0][0], 0.0000);
    EXPECT_EQ(gauge["time_series"][0][1], 10.0);
    EXPECT_EQ(gauge["time_series"][1][0], 1.000);
    EXPECT_EQ(gauge["time_series"][1][1], 11.0);
}

TEST_F(MetricsTestFixture, MetricsGaugeIgnoresSameSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.type = metrics::MetricType::GAUGE;
    metadata.name = "gauge";
    auto metricId = pManager->AddMetric(metadata);
    ASSERT_NE(metricId, metrics::kInvalidMetricID);

    auto           result = pManager->CreateReport("report").GetContentString();
    nlohmann::json parsed = nlohmann::json::parse(result);
    auto           gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 0);

    metrics::MetricData data = {metrics::MetricType::GAUGE};
    data.gauge.seconds       = 0.000;
    data.gauge.value         = 10.0;
    EXPECT_TRUE(pManager->RecordMetricData(metricId, data));
    data.gauge.seconds = 1.000;
    data.gauge.value   = 11.0;
    EXPECT_TRUE(pManager->RecordMetricData(metricId, data));
    data.gauge.seconds = 1.000;
    data.gauge.value   = 12.0;
    EXPECT_FALSE(pManager->RecordMetricData(metricId, data));

    // Invalid data point should be ignored.
    result = pManager->CreateReport("report").GetContentString();
    parsed = nlohmann::json::parse(result);
    gauge  = parsed["runs"][0]["gauges"][0];
    EXPECT_EQ(gauge["time_series"].size(), 2);
    EXPECT_EQ(gauge["time_series"][0][0], 0.0000);
    EXPECT_EQ(gauge["time_series"][0][1], 10.0);
    EXPECT_EQ(gauge["time_series"][1][0], 1.000);
    EXPECT_EQ(gauge["time_series"][1][1], 11.0);
}

} // namespace ppx
