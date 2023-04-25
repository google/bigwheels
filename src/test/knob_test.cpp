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

#include "ppx/knob.h"

#include <sstream>

TEST(KnobTest, KnobTypeStream)
{
    std::stringstream out;
    KnobType          testkt = KnobType::Unknown;
    out << testkt;
    EXPECT_EQ(out.str(), "Unknown");
    out.str("");
}

namespace ppx {

TEST(KnobTest, KnobBase)
{
    // Basic knob properties
    KnobConfig config  = {};
    config.displayName = "Knob Name 1";
    config.flagName    = "flag_name1";
    config.flagDesc    = "description1";

    Knob k1 = Knob(config, KnobType::Unknown);
    EXPECT_EQ(k1.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(k1.GetFlagName(), "flag_name1");
    EXPECT_EQ(k1.GetFlagDesc(), "description1");
    EXPECT_EQ(k1.GetType(), KnobType::Unknown);

    // Child knob
    config             = {};
    config.displayName = "Knob Name 2";
    config.flagName    = "flag_name2";
    config.flagDesc    = "description2";
    Knob k2            = Knob(config, KnobType::Unknown);
    k1.AddChild(&k2);
    EXPECT_FALSE(k1.GetChildren().empty());
    EXPECT_EQ(k1.GetChildren()[0]->GetFlagName(), "flag_name2");
}

} // namespace ppx