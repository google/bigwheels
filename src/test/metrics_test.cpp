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

TEST(MetricsTest, AddRunSingle)
{
    metrics::Manager      manager;
    metrics::Run*         run;
    constexpr const char* RUN_NAME = "run";
    const Result          result   = manager.AddRun(run, RUN_NAME);
    ASSERT_EQ(result, SUCCESS);
    ASSERT_EQ(run, manager.GetRun(RUN_NAME));
}

TEST(MetricsTest, AddRunMultiple)
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
    }
}

TEST(MetricsTest, AddRunDuplicate)
{
    metrics::Manager      manager;
    constexpr const char* RUN_NAME = "run";
    {
        metrics::Run* run;
        const Result  result = manager.AddRun(run, RUN_NAME);
        ASSERT_EQ(result, SUCCESS);
    }
    {
        metrics::Run* run;
        const Result  result = manager.AddRun(run, RUN_NAME);
        ASSERT_EQ(result, ERROR_DUPLICATE_ELEMENT);
    }
}

} // namespace ppx
