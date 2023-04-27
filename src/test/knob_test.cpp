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

    testkt = KnobType::Bool_Checkbox;
    out << testkt;
    EXPECT_EQ(out.str(), "Bool_Checkbox");
    out.str("");
}

namespace ppx {

TEST(KnobTest, KnobBool)
{
    // Basic knob properties
    BoolCheckboxConfig config = {};
    config.displayName        = "Knob Name 1";
    config.flagName           = "flag_name1";
    config.flagDesc           = "description1";
    config.defaultValue       = true;

    auto k1 = KnobBoolCheckbox(config);
    EXPECT_EQ(k1.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(k1.GetFlagName(), "flag_name1");
    EXPECT_EQ(k1.GetFlagDesc(), "description1");
    EXPECT_EQ(k1.GetType(), KnobType::Bool_Checkbox);

    // Initialize knob manager
    KnobManager km;
    EXPECT_TRUE(km.IsEmpty());

    // Register bool knob with the knob manager
    km.CreateBoolCheckbox(1, config);
    EXPECT_EQ(km.GetKnobBoolValue(1), true);
    EXPECT_FALSE(km.IsEmpty());

    // Child knob
    config              = {};
    config.displayName  = "Knob Name 2";
    config.flagName     = "flag_name2";
    config.flagDesc     = "description2";
    config.defaultValue = false;
    config.parentId     = 1;
    km.CreateBoolCheckbox(2, config);

    auto knobPtr = km.GetKnob(1);
    EXPECT_NE(knobPtr, nullptr);
    EXPECT_FALSE(knobPtr->GetChildren().empty());
    EXPECT_EQ(knobPtr->GetChildren()[0]->GetFlagName(), "flag_name2");

    // Test changing bool knob value
    EXPECT_EQ(km.GetKnobBoolValue(2), false);
    km.SetKnobBoolValue(2, true);
    EXPECT_EQ(km.GetKnobBoolValue(2), true);

    // Test reset
    km.Reset();
    EXPECT_EQ(km.GetKnobBoolValue(2), false);
}

} // namespace ppx