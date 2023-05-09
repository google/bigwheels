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

// ---------------------------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------------------------

class KnobTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ppx::Log::Initialize(ppx::LOG_MODE_CONSOLE, nullptr);
    }

    void TearDown() override
    {
        ppx::Log::Shutdown();
    }
};

class KnobManagerTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ppx::Log::Initialize(ppx::LOG_MODE_CONSOLE, nullptr);
    }

    void TearDown() override
    {
        ppx::Log::Shutdown();
    }

protected:
    ppx::KnobManager km;
};

// ---------------------------------------------------------------------------------------------
// KnobType
// ---------------------------------------------------------------------------------------------

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

    testkt = KnobType::Int_Slider;
    out << testkt;
    EXPECT_EQ(out.str(), "Int_Slider");
    out.str("");

    testkt = KnobType::Str_Dropdown;
    out << testkt;
    EXPECT_EQ(out.str(), "Str_Dropdown");
    out.str("");
}

namespace ppx {

// -------------------------------------------------------------------------------------------------
// Knob (Abstract class, so using KnobBoolCheckbox to test)
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, Knob_ParentChild)
{
    KnobBoolCheckbox boolKnob1 = KnobBoolCheckbox("Knob Name 1", "flag_name1", "description1", true);
    KnobBoolCheckbox boolKnob2 = KnobBoolCheckbox("Knob Name 2", "flag_name2", "description2", false);
    boolKnob1.AddChild(&boolKnob2);
    auto children = boolKnob1.GetChildren();
    EXPECT_FALSE(children.empty());
    EXPECT_EQ(children.front()->GetFlagName(), "flag_name2");
}

// -------------------------------------------------------------------------------------------------
// KnobBoolCheckbox
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobBoolCheckbox_CreateNoCallback)
{
    KnobBoolCheckbox boolKnob = KnobBoolCheckbox("Knob Name 1", "flag_name1", "description1", true);
    EXPECT_EQ(boolKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(boolKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(boolKnob.GetFlagDesc(), "description1");
    EXPECT_EQ(boolKnob.GetType(), KnobType::Bool_Checkbox);
    EXPECT_EQ(boolKnob.GetBoolValue(), true);
    EXPECT_EQ(boolKnob.FlagText(), "--flag_name1 <true/false> : description1\n");
}

TEST_F(KnobTestFixture, KnobBoolCheckbox_SetBoolValue)
{
    KnobBoolCheckbox boolKnob = KnobBoolCheckbox("Knob Name 1", "flag_name1", "description1", false);
    boolKnob.SetBoolValue(true);
    EXPECT_EQ(boolKnob.GetBoolValue(), true);
}

TEST_F(KnobTestFixture, KnobBoolCheckbox_ResetToDefault)
{
    KnobBoolCheckbox boolKnob = KnobBoolCheckbox("Knob Name 1", "flag_name1", "description1", true);
    EXPECT_EQ(boolKnob.GetBoolValue(), true);
    boolKnob.SetBoolValue(false);
    EXPECT_EQ(boolKnob.GetBoolValue(), false);
    boolKnob.ResetToDefault();
    EXPECT_EQ(boolKnob.GetBoolValue(), true);
}

TEST_F(KnobTestFixture, KnobBoolCheckbox_CreateCallback)
{
    bool tester = false;
    bool*             pTester  = &tester;
    KnobBoolCheckbox  boolKnob = KnobBoolCheckbox("Knob Name 1", "flag_name1", "description1", false, [pTester](bool b) { *pTester = b; });
    boolKnob.SetBoolValue(true); // trigger callback
    EXPECT_EQ(tester, true);
}

// -------------------------------------------------------------------------------------------------
// KnobIntSlider
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobIntSlider_CreateNoCallback)
{
    KnobIntSlider intKnob = KnobIntSlider("Knob Name 1", "flag_name1", "description1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(intKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(intKnob.GetFlagDesc(), "description1");
    EXPECT_EQ(intKnob.GetType(), KnobType::Int_Slider);
    EXPECT_EQ(intKnob.GetIntValue(), 5);
    EXPECT_EQ(intKnob.FlagText(), "--flag_name1 <0~10> : description1\n");
}

TEST_F(KnobTestFixture, KnobIntSlider_SetIntValue)
{
    KnobIntSlider intKnob = KnobIntSlider("Knob Name 1", "flag_name1", "description1", 5, 0, 10);
    auto res = intKnob.SetIntValue(10);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(intKnob.GetIntValue(), 10);

    // Below min, should not be set
    res = intKnob.SetIntValue(-3);
    EXPECT_EQ(res, ERROR_OUT_OF_RANGE);
    EXPECT_EQ(intKnob.GetIntValue(), 10);

    // Above max, should not be set
    res = intKnob.SetIntValue(22);
    EXPECT_EQ(res, ERROR_OUT_OF_RANGE);
    EXPECT_EQ(intKnob.GetIntValue(), 10);
}

TEST_F(KnobTestFixture, KnobIntSlider_ResetToDefault)
{
    KnobIntSlider intKnob = KnobIntSlider("Knob Name 1", "flag_name1", "description1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetIntValue(), 5);
    auto res = intKnob.SetIntValue(8);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(intKnob.GetIntValue(), 8);
    intKnob.ResetToDefault();
    EXPECT_EQ(intKnob.GetIntValue(), 5);
}

TEST_F(KnobTestFixture, KnobIntSlider_CreateCallback)
{
    int tester = 0;
    int* pTester  = &tester;
    KnobIntSlider intKnob = KnobIntSlider("Knob Name 1", "flag_name1", "description1", 5, 0, 10, [pTester](int i) { *pTester = i; });
    auto res = intKnob.SetIntValue(8); // trigger callback
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(tester, 8);
}

// -------------------------------------------------------------------------------------------------
// KnobStrDropdown
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobStrDropdown_CreateNoCallback)
{
    std::vector<std::string> choices = {"c1", "c2"};
    KnobStrDropdown strKnob = KnobStrDropdown("Knob Name 1", "flag_name1", "description1", 1, choices);
    EXPECT_EQ(strKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(strKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(strKnob.GetFlagDesc(), "description1");
    EXPECT_EQ(strKnob.GetType(), KnobType::Str_Dropdown);
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetStr(), "c2");
    EXPECT_EQ(strKnob.FlagText(), "--flag_name1 <\"c1\"|\"c2\"> : description1\n");
}

TEST_F(KnobTestFixture, KnobStrDropdown_SetIndexInt)
{
    std::vector<std::string> choices = {"c1", "c2"};
    KnobStrDropdown strKnob = KnobStrDropdown("Knob Name 1", "flag_name1", "description1", 1, choices);
    auto res = strKnob.SetIndex(0);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(strKnob.GetIndex(), 0);

    // Below min, should not be set
    res = strKnob.SetIndex(-3);
    EXPECT_EQ(res, ERROR_ELEMENT_NOT_FOUND);
    EXPECT_EQ(strKnob.GetIndex(), 0);

    // Above max, should not be set
    res = strKnob.SetIndex(2);
    EXPECT_EQ(res, ERROR_ELEMENT_NOT_FOUND);
    EXPECT_EQ(strKnob.GetIndex(), 0);
}

TEST_F(KnobTestFixture, KnobStrDropdown_SetIndexStr)
{
    std::vector<std::string> choices = {"c1", "c2"};
    KnobStrDropdown strKnob = KnobStrDropdown("Knob Name 1", "flag_name1", "description1", 1, choices);
    auto res = strKnob.SetIndex("c1");
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_EQ(strKnob.GetStr(), "c1");

    // Not in choices, should not be set
    res = strKnob.SetIndex("c3");
    EXPECT_EQ(res, ERROR_ELEMENT_NOT_FOUND);
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_EQ(strKnob.GetStr(), "c1");
    
}

TEST_F(KnobTestFixture, KnobStrDropdown_ResetToDefault)
{
    std::vector<std::string> choices = {"c1", "c2"};
    KnobStrDropdown strKnob = KnobStrDropdown("Knob Name 1", "flag_name1", "description1", 1, choices);
    EXPECT_EQ(strKnob.GetIndex(), 1);
    auto res = strKnob.SetIndex(0);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(strKnob.GetIndex(), 0);
    strKnob.ResetToDefault();
    EXPECT_EQ(strKnob.GetIndex(), 1);
}

TEST_F(KnobTestFixture, KnobStrDropdown_CreateCallback)
{
    int tester = 1;
    int* pTester  = &tester;
    std::vector<std::string> choices = {"c1", "c2"};
    KnobStrDropdown strKnob = KnobStrDropdown("Knob Name 1", "flag_name1", "description1", 1, choices, [pTester](int i) { *pTester = i; });
    auto res = strKnob.SetIndex(0); // trigger callback
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(tester, 0);
}

// -------------------------------------------------------------------------------------------------
// KnobManager
// -------------------------------------------------------------------------------------------------

TEST_F(KnobManagerTestFixture, KnobManager_Create)
{
    EXPECT_TRUE(km.IsEmpty());
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateBoolCheckbox)
{
    KnobBoolCheckbox* boolKnobPtr = km.CreateKnob<KnobBoolCheckbox>(nullptr, "Knob Name 1", "flag_name1", "description1", true);
    EXPECT_EQ(boolKnobPtr->GetBoolValue(), true);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateIntSlider)
{
    KnobIntSlider* intKnobPtr = km.CreateKnob<KnobIntSlider>(nullptr, "Knob Name 1", "flag_name1", "description1", 5, 0, 10);
    EXPECT_EQ(intKnobPtr->GetIntValue(), 5);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateStrDropdown)
{
    std::vector<std::string> choices = {"c1", "c2", "c3"};
    KnobStrDropdown* strKnobPtr = km.CreateKnob<KnobStrDropdown>(nullptr, "Knob Name 1", "flag_name1", "description1", 1, choices);
    EXPECT_EQ(strKnobPtr->GetIndex(), 1);
}

TEST_F(KnobManagerTestFixture, KnobManager_GetUsageMsg)
{
    KnobBoolCheckbox* boolKnobPtr1 = km.CreateKnob<KnobBoolCheckbox>(nullptr, "Knob Name 1", "flag_name1", "description1", true);
    KnobBoolCheckbox* boolKnobPtr2 = km.CreateKnob<KnobBoolCheckbox>(boolKnobPtr1, "Knob Name 2", "flag_name2", "description2", true);
    KnobIntSlider* intKnobPtr1 = km.CreateKnob<KnobIntSlider>(nullptr, "Knob Name 3", "flag_name3", "description3", 5, 0, 10);
    std::vector<std::string> choices1 = {"c1", "c2", "c3"};
    KnobStrDropdown* strKnobPtr1 = km.CreateKnob<KnobStrDropdown>(nullptr, "Knob Name 4", "flag_name4", "description4", 1, choices1);
    
    std::string usageMsg = R"(
Application-specific flags
--flag_name1 <true/false> : description1
--flag_name2 <true/false> : description2
--flag_name3 <0~10> : description3
--flag_name4 <"c1"|"c2"|"c3"> : description4
)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerTestFixture, KnobManager_ResetAllToDefault)
{
    KnobBoolCheckbox* boolKnobPtr1 = km.CreateKnob<KnobBoolCheckbox>(nullptr, "Knob Name 1", "flag_name1", "description1", true);
    KnobBoolCheckbox* boolKnobPtr2 = km.CreateKnob<KnobBoolCheckbox>(boolKnobPtr1, "Knob Name 2", "flag_name2", "description2", true);   
    KnobIntSlider* intKnobPtr1 = km.CreateKnob<KnobIntSlider>(nullptr, "Knob Name 3", "flag_name3", "description3", 5, 0, 10);
    std::vector<std::string> choices1 = {"c1", "c2", "c3"};
    KnobStrDropdown* strKnobPtr1 = km.CreateKnob<KnobStrDropdown>(nullptr, "Knob Name 4", "flag_name4", "description4", 1, choices1);
    
    // Change from default
    boolKnobPtr1->SetBoolValue(false);
    EXPECT_EQ(boolKnobPtr1->GetBoolValue(), false);
    boolKnobPtr2->SetBoolValue(false);
    EXPECT_EQ(boolKnobPtr2->GetBoolValue(), false);
    auto res = intKnobPtr1->SetIntValue(8);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(intKnobPtr1->GetIntValue(), 8);
    res = strKnobPtr1->SetIndex(0);
    EXPECT_EQ(res, SUCCESS);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 0);

    km.ResetAllToDefault();
    EXPECT_EQ(boolKnobPtr1->GetBoolValue(), true);
    EXPECT_EQ(boolKnobPtr2->GetBoolValue(), true);
    EXPECT_EQ(intKnobPtr1->GetIntValue(), 5);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 1);
}

// -------------------------------------------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, FlattenDepthFirst_Empty)
{
    std::vector<Knob*> rootKnobs = {};
    auto flat = FlattenDepthFirst(rootKnobs);
    EXPECT_TRUE(flat.empty());
}

TEST_F(KnobTestFixture, FlattenDepthFirst_Flat)
{
    KnobBoolCheckbox p1 = KnobBoolCheckbox("", "1", "", true);
    KnobBoolCheckbox p2 = KnobBoolCheckbox("", "2", "", true);
    KnobBoolCheckbox p3 = KnobBoolCheckbox("", "3", "", true);
    std::vector<Knob*> rootKnobs = {&p1, &p2, &p3};
    std::stringstream out;

    auto flat = FlattenDepthFirst(rootKnobs);
    for (auto knobPtr : flat) {
        out << knobPtr->GetFlagName();
    }
    EXPECT_EQ(out.str(), "123");
    out.str("");
}

TEST_F(KnobTestFixture, FlattenDepthFirst_Tree)
{
    //     1    8
    //    /|   /|
    //   2 3  9 10
    //    /|
    //   4 7
    //  /|
    // 5 6
    //
    // Depth-first order: [1 2 3 4 5 6 7 8 9 10]
    KnobBoolCheckbox p1 = KnobBoolCheckbox("", "1", "", true);
    KnobBoolCheckbox p2 = KnobBoolCheckbox("", "2", "", true);
    KnobBoolCheckbox p3 = KnobBoolCheckbox("", "3", "", true);
    KnobBoolCheckbox p4 = KnobBoolCheckbox("", "4", "", true);
    KnobBoolCheckbox p5 = KnobBoolCheckbox("", "5", "", true);
    KnobBoolCheckbox p6 = KnobBoolCheckbox("", "6", "", true);
    KnobBoolCheckbox p7 = KnobBoolCheckbox("", "7", "", true);
    KnobBoolCheckbox p8 = KnobBoolCheckbox("", "8", "", true);
    KnobBoolCheckbox p9 = KnobBoolCheckbox("", "9", "", true);
    KnobBoolCheckbox p10 = KnobBoolCheckbox("", "10", "", true);
    p1.AddChild(&p2);  
    p1.AddChild(&p3); 
    p3.AddChild(&p4);
    p3.AddChild(&p7);
    p4.AddChild(&p5);
    p4.AddChild(&p6);
    p8.AddChild(&p9);
    p8.AddChild(&p10);

    std::vector<Knob*> rootKnobs = {&p1, &p8};
    auto flat1 = FlattenDepthFirst(rootKnobs);
    std::stringstream out;
    for (auto knobPtr : flat1) {
        out << knobPtr->GetFlagName();
    }
    EXPECT_EQ(out.str(), "12345678910");
    out.str("");
}

} // namespace ppx