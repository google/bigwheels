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
    void SetUp() override
    {
        manager = new metrics::Manager();
        ASSERT_NE(manager, nullptr);
        run = manager->AddRun("default_run");
        ASSERT_NE(run, nullptr);
    }

    void TearDown() override
    {
        if (manager != nullptr) {
            delete manager;
            manager = nullptr;
        }
    }

protected:
    metrics::Manager* manager;
    metrics::Run*     run;
};

////////////////////////////////////////////////////////////////////////////////
// Manager
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, ManagerAddSingleRun)
{
    metrics::Manager manager;
    metrics::Run*    run = manager.AddRun("run");
    EXPECT_NE(run, nullptr);
}

TEST(MetricsTest, ManagerAddRunWithNullName)
{
    metrics::Manager manager;
    EXPECT_DEATH({
        metrics::Run* run = manager.AddRun(nullptr);
    },
                 "");
}

TEST(MetricsTest, ManagerAddMultipleRun)
{
    metrics::Manager manager;

    metrics::Run* run0;
    metrics::Run* run1;

    {
        run0 = manager.AddRun("run0");
        ASSERT_NE(run0, nullptr);
    }
    {
        run1 = manager.AddRun("run1");
        ASSERT_NE(run1, nullptr);
    }
    ASSERT_NE(run0, run1);
}

TEST(MetricsTest, ManagerAddDuplicateRun)
{
    metrics::Manager      manager;
    constexpr const char* RUN_NAME = "run";
    {
        metrics::Run* run = manager.AddRun(RUN_NAME);
        EXPECT_NE(run, nullptr);
    }
    {
        EXPECT_DEATH({
            manager.AddRun(RUN_NAME);
        },
                     "");
    }
}

////////////////////////////////////////////////////////////////////////////////
// Run
////////////////////////////////////////////////////////////////////////////////

TEST(MetricsTest, RunAddSingleMetric)
{
    metrics::Manager manager;
    {
        metrics::Run* run = manager.AddRun("run_gauge");
        ASSERT_NE(run, nullptr);

        metrics::MetricMetadata metadata = {};
        metadata.name                    = "metric";
        metrics::MetricGauge* metric     = run->AddMetric<metrics::MetricGauge>(metadata);
        ASSERT_NE(metric, nullptr);
    }

    {
        metrics::Run* run = manager.AddRun("run_counter");
        ASSERT_NE(run, nullptr);

        metrics::MetricMetadata metadata = {};
        metadata.name                    = "metric";
        metrics::MetricCounter* metric   = run->AddMetric<metrics::MetricCounter>(metadata);
        ASSERT_NE(metric, nullptr);
    }
}

TEST(MetricsTest, ManagerAddMetricWithNullName)
{
    metrics::Manager manager;
    metrics::Run*    run = manager.AddRun("run");
    ASSERT_NE(run, nullptr);

    metrics::MetricMetadata metadata = {};
    EXPECT_DEATH({
        run->AddMetric<metrics::MetricGauge>(metadata);
    },
                 "");
}

TEST(MetricsTest, RunAddMultipleMetric)
{
    metrics::Manager manager;
    metrics::Run*    run = manager.AddRun("run");
    ASSERT_NE(run, nullptr);

    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = "metric_gauge";

        metrics::MetricGauge* metricGauge = run->AddMetric<metrics::MetricGauge>(metadata);
        EXPECT_NE(metricGauge, nullptr);
    }

    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = "metric_counter";

        metrics::MetricCounter* metricCounter = run->AddMetric<metrics::MetricCounter>(metadata);
        EXPECT_NE(metricCounter, nullptr);
    }
}

TEST(MetricsTest, RunAddDuplicateMetric)
{
    metrics::Manager manager;
    metrics::Run*    run = manager.AddRun("run");
    ASSERT_NE(run, nullptr);

    metrics::MetricMetadata metadata = {};
    metadata.name                    = "metric";

    metrics::MetricGauge* metricGauge = run->AddMetric<metrics::MetricGauge>(metadata);
    EXPECT_NE(metricGauge, nullptr);

    EXPECT_DEATH({
        metricGauge = run->AddMetric<metrics::MetricGauge>(metadata);
    },
                 "");

    EXPECT_DEATH({
        metrics::MetricCounter* metricCounter = run->AddMetric<metrics::MetricCounter>(metadata);
    },
                 "");
}

////////////////////////////////////////////////////////////////////////////////
// Metrics
////////////////////////////////////////////////////////////////////////////////

TEST_F(MetricsTestFixture, MetricsCounter)
{
    metrics::MetricMetadata metadata;
    metadata.name                  = "counter";
    metrics::MetricCounter* metric = run->AddMetric<metrics::MetricCounter>(metadata);
    EXPECT_EQ(metric->Get(), 0U);

    metric->Increment(1U);
    EXPECT_EQ(metric->Get(), 1U);
    metric->Increment(4U);
    EXPECT_EQ(metric->Get(), 5U);
}

TEST_F(MetricsTestFixture, MetricsGaugeEntries)
{
    metrics::MetricMetadata metadata;
    metadata.name                = "frame_time";
    metrics::MetricGauge* metric = run->AddMetric<metrics::MetricGauge>(metadata);

    EXPECT_EQ(metric->GetEntriesCount(), 0U);
    metric->RecordEntry(0.0000, 11.0);
    EXPECT_EQ(metric->GetEntriesCount(), 1U);
    metric->RecordEntry(0.0110, 11.7);
    EXPECT_EQ(metric->GetEntriesCount(), 2U);
    metric->RecordEntry(0.0227, 12.2);
    EXPECT_EQ(metric->GetEntriesCount(), 3U);
    metric->RecordEntry(0.0349, 10.8);
    EXPECT_EQ(metric->GetEntriesCount(), 4U);
    metric->RecordEntry(0.0457, 11.1);
    EXPECT_EQ(metric->GetEntriesCount(), 5U);

    double seconds;
    double value;
    metric->GetEntry(0, seconds, value);
    EXPECT_EQ(seconds, 0.0000);
    EXPECT_EQ(value, 11.0);
    metric->GetEntry(4, seconds, value);
    EXPECT_EQ(seconds, 0.0457);
    EXPECT_EQ(value, 11.1);
}

TEST_F(MetricsTestFixture, RecordNegativeSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.name                = "frame_time";
    metrics::MetricGauge* metric = run->AddMetric<metrics::MetricGauge>(metadata);

    EXPECT_DEATH({
        metric->RecordEntry(-1.0, 10.868892007019612);
    },
                 "");
}

TEST_F(MetricsTestFixture, RecordNonIncreasingSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.name                = "frame_time";
    metrics::MetricGauge* metric = run->AddMetric<metrics::MetricGauge>(metadata);

    metric->RecordEntry(0.0, 10.868892007019612);
    metric->RecordEntry(1.0, 10.868892007019612);
    EXPECT_DEATH({
        metric->RecordEntry(0.9, 10.868892007019612);
    },
                 "");
}

TEST_F(MetricsTestFixture, RecordNonStrictlyIncreasingSeconds)
{
    metrics::MetricMetadata metadata;
    metadata.name                = "frame_time";
    metrics::MetricGauge* metric = run->AddMetric<metrics::MetricGauge>(metadata);

    metric->RecordEntry(0.0, 10.868892007019612);
    metric->RecordEntry(1.0, 10.868892007019612);
    EXPECT_DEATH({
        metric->RecordEntry(1.0, 10.868892007019612);
    },
                 "");
}

TEST_F(MetricsTestFixture, Statistics)
{
    metrics::MetricMetadata metadata;
    metadata.name                = "frame_time";
    metrics::MetricGauge* metric = run->AddMetric<metrics::MetricGauge>(metadata);

    metric->RecordEntry(0.0, 10.868892007019612);
    metric->RecordEntry(0.010868892007019612, 11.245153538647925);
    metric->RecordEntry(0.022114045545667538, 11.602910062251805);
    metric->RecordEntry(0.03371695560791935, 11.33761713476685);
    metric->RecordEntry(0.0450545727426862, 11.898861108180402);
    metric->RecordEntry(0.0569534338508666, 12.4339009501692);
    metric->RecordEntry(0.0693873348010358, 11.898241466973156);
    metric->RecordEntry(0.08128557626800896, 11.578552223971503);
    metric->RecordEntry(0.09286412849198046, 11.866067498772232);
    metric->RecordEntry(0.10473019599075269, 11.060070436041686);
    metric->RecordEntry(0.11579026642679437, 12.052120427214446);
    metric->RecordEntry(0.1278423868540088, 11.23341128678147);
    metric->RecordEntry(0.13907579814079027, 12.200557497396941);
    metric->RecordEntry(0.1512763556381872, 12.36049827984556);
    metric->RecordEntry(0.16363685391803276, 11.549563113595383);
    metric->RecordEntry(0.17518641703162816, 10.802797167019325);
    metric->RecordEntry(0.18598921419864747, 12.283888464432493);
    metric->RecordEntry(0.19827310266307996, 11.071345155888102);
    metric->RecordEntry(0.20934444781896805, 12.434753867028249);
    metric->RecordEntry(0.2217792016859963, 11.61296462432844);
    metric->RecordEntry(0.23339216631032472, 11.28622818582004);
    metric->RecordEntry(0.24467839449614476, 11.49636551023874);
    metric->RecordEntry(0.2561747600063835, 11.25958164228463);
    metric->RecordEntry(0.26743434164866814, 11.323910161619201);
    metric->RecordEntry(0.27875825181028735, 11.873158233564933);
    metric->RecordEntry(0.2906314100438523, 12.141777965793269);
    metric->RecordEntry(0.30277318800964553, 12.188989971932937);
    metric->RecordEntry(0.3149621779815785, 12.019919594110705);
    metric->RecordEntry(0.3269820975756892, 10.642457556401965);
    metric->RecordEntry(0.33762455513209116, 10.946841233564584);
    metric->RecordEntry(0.34857139636565576, 10.775581025208819);
    metric->RecordEntry(0.3593469773908646, 10.674050454315147);
    metric->RecordEntry(0.3700210278451797, 10.559965750942794);
    metric->RecordEntry(0.3805809935961225, 10.6470760580965);
    metric->RecordEntry(0.391228069654219, 11.815641763719412);
    metric->RecordEntry(0.40304371141793843, 11.583674887818198);
    metric->RecordEntry(0.41462738630575663, 11.268057958679512);
    metric->RecordEntry(0.42589544426443615, 10.805038189858271);
    metric->RecordEntry(0.43670048245429444, 10.526121077848554);
    metric->RecordEntry(0.447226603532143, 10.50685559634115);
    metric->RecordEntry(0.4577334591284841, 12.343618211531659);
    metric->RecordEntry(0.4700770773400158, 11.803992180985341);
    metric->RecordEntry(0.4818810695210011, 11.62310428246178);
    metric->RecordEntry(0.4935041738034629, 11.45193421131918);
    metric->RecordEntry(0.5049561080147821, 12.11556153467419);
    metric->RecordEntry(0.5170716695494563, 11.786296410371959);
    metric->RecordEntry(0.5288579659598283, 12.109353454810051);
    metric->RecordEntry(0.5409673194146383, 11.481262532191405);
    metric->RecordEntry(0.5524485819468297, 11.886935788528675);
    metric->RecordEntry(0.5643355177353584, 11.22097303285219);
    metric->RecordEntry(0.5755564907682106, 11.21611365134226);
    metric->RecordEntry(0.5867726044195528, 12.038979734460957);
    metric->RecordEntry(0.5988115841540138, 11.865370493140473);
    metric->RecordEntry(0.6106769546471543, 10.961889249183846);
    metric->RecordEntry(0.6216388438963382, 11.029523446967511);
    metric->RecordEntry(0.6326683673433057, 11.363081128774372);
    metric->RecordEntry(0.64403144847208, 11.044254285638864);
    metric->RecordEntry(0.6550757027577189, 11.304579227095472);
    metric->RecordEntry(0.6663802819848144, 11.518339967718285);
    metric->RecordEntry(0.6778986219525327, 11.535175932647867);
    metric->RecordEntry(0.6894337978851806, 11.438807944873988);
    metric->RecordEntry(0.7008726058300545, 11.995862817066628);
    metric->RecordEntry(0.7128684686471212, 12.180350761995374);
    metric->RecordEntry(0.7250488194091166, 11.849875908074102);
    metric->RecordEntry(0.7368986953171907, 11.25893134410846);
    metric->RecordEntry(0.7481576266612991, 11.29556518338785);
    metric->RecordEntry(0.759453191844687, 12.100159818336204);
    metric->RecordEntry(0.7715533516630232, 11.268292831597137);
    metric->RecordEntry(0.7828216444946203, 10.876697084092664);
    metric->RecordEntry(0.793698341578713, 11.41067769966513);
    metric->RecordEntry(0.8051090192783781, 10.764130551471954);
    metric->RecordEntry(0.81587314982985, 11.516565679587151);
    metric->RecordEntry(0.8273897155094372, 12.258346383658813);
    metric->RecordEntry(0.839648061893096, 12.042792906933066);
    metric->RecordEntry(0.8516908548000292, 12.006355840344012);
    metric->RecordEntry(0.8636972106403732, 12.436359135554602);
    metric->RecordEntry(0.8761335697759278, 11.877641201985803);
    metric->RecordEntry(0.8880112109779136, 10.567107149500245);
    metric->RecordEntry(0.8985783181274138, 11.911189275994568);
    metric->RecordEntry(0.9104895074034084, 10.889245781607388);
    metric->RecordEntry(0.9213787531850158, 12.465256618853836);
    metric->RecordEntry(0.9338440098038696, 11.223065556424801);
    metric->RecordEntry(0.9450670753602944, 11.730574094963503);
    metric->RecordEntry(0.9567976494552579, 11.325378236493892);
    metric->RecordEntry(0.9681230276917518, 11.874416573715038);
    metric->RecordEntry(0.9799974442654669, 11.868266253898241);
    metric->RecordEntry(0.9918657105193651, 10.700628941411287);
    metric->RecordEntry(1.0025663394607764, 11.199358872070935);
    metric->RecordEntry(1.0137656983328474, 10.863209130580797);
    metric->RecordEntry(1.024628907463428, 12.04804735335845);
    metric->RecordEntry(1.0366769548167865, 12.060037474628244);
    metric->RecordEntry(1.0487369922914147, 12.07516039943628);
    metric->RecordEntry(1.060812152690851, 11.636401196731727);
    metric->RecordEntry(1.0724485538875828, 11.325091973540063);
    metric->RecordEntry(1.083773645861123, 12.367583932075869);
    metric->RecordEntry(1.0961412297931987, 12.100536564029815);
    metric->RecordEntry(1.1082417663572286, 10.709781403850688);
    metric->RecordEntry(1.1189515477610792, 11.965315843361148);
    metric->RecordEntry(1.1309168636044404, 11.3946157918795);
    metric->RecordEntry(1.1423114793963198, 11.310652174786224);

    const metrics::GaugeBasicStatistics basicStatistics = metric->GetBasicStatistics();
    EXPECT_EQ(basicStatistics.min, 10.50685559634115);
    EXPECT_EQ(basicStatistics.max, 12.465256618853836);
    EXPECT_DOUBLE_EQ(basicStatistics.average, 11.53622131571106);
    EXPECT_DOUBLE_EQ(basicStatistics.timeRatio, 1009.9015482018648);

    const metrics::GaugeComplexStatistics complexStatistics = metric->ComputeComplexStatistics();
    EXPECT_EQ(complexStatistics.median, 11.526757950183075);
    EXPECT_DOUBLE_EQ(complexStatistics.standardDeviation, 0.5296000886136008);
    EXPECT_EQ(complexStatistics.percentile90, 12.200557497396941);
    EXPECT_EQ(complexStatistics.percentile95, 12.367583932075869);
    EXPECT_EQ(complexStatistics.percentile99, 12.465256618853836);
}

} // namespace ppx
