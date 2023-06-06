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

#if !defined(NDEBUG)
#define PERFORM_DEATH_TESTS
#endif

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

class KnobManagerTestFixture : public KnobTestFixture
{
protected:
    ppx::KnobManager km;
};

namespace ppx {

// -------------------------------------------------------------------------------------------------
// KnobCheckbox
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobCheckbox_Create)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", true);
    EXPECT_EQ(boolKnob.GetFlagHelpText(), "--flag_name1 <true|false>\n");

    boolKnob.SetDisplayName("Knob Name 1");
    boolKnob.SetFlagDesc("description1");
    boolKnob.SetIndent(3);

    EXPECT_EQ(boolKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(boolKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(boolKnob.GetFlagHelp(), "description1");
    EXPECT_EQ(boolKnob.GetIndent(), 3);
    EXPECT_EQ(boolKnob.GetValue(), true);
    EXPECT_EQ(boolKnob.GetFlagHelpText(), "--flag_name1 <true|false> : description1\n");
}

TEST_F(KnobTestFixture, KnobCheckbox_SetBoolValue)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", false);
    boolKnob.SetValue(true);
    EXPECT_EQ(boolKnob.GetValue(), true);
}

TEST_F(KnobTestFixture, KnobCheckbox_ResetToDefault)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", true);
    EXPECT_EQ(boolKnob.GetValue(), true);
    boolKnob.SetValue(false);
    EXPECT_EQ(boolKnob.GetValue(), false);
    boolKnob.ResetToDefault();
    EXPECT_EQ(boolKnob.GetValue(), true);
}

// -------------------------------------------------------------------------------------------------
// KnobSlider
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobSlider_Create)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetFlagHelpText(), "--flag_name1 <0~10>\n");

    intKnob.SetDisplayName("Knob Name 1");
    intKnob.SetFlagDesc("description1");
    intKnob.SetIndent(3);

    EXPECT_EQ(intKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(intKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(intKnob.GetFlagHelp(), "description1");
    EXPECT_EQ(intKnob.GetIndent(), 3);
    EXPECT_EQ(intKnob.GetValue(), 5);
    EXPECT_EQ(intKnob.GetFlagHelpText(), "--flag_name1 <0~10> : description1\n");
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, KnobSlider_CreateInvalid)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 10, 10, 10);
        },
        "");
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", -1, 0, 10);
        },
        "");
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 11, 0, 10);
        },
        "");
}
#endif

TEST_F(KnobTestFixture, KnobSlider_SetIntValue)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    intKnob.SetValue(10);
    EXPECT_EQ(intKnob.GetValue(), 10);

    // Below min, should not be set
    intKnob.SetValue(-3);
    EXPECT_EQ(intKnob.GetValue(), 10);

    // Above max, should not be set
    intKnob.SetValue(22);
    EXPECT_EQ(intKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, KnobSlider_ResetToDefault)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetValue(), 5);
    intKnob.SetValue(8);
    EXPECT_EQ(intKnob.GetValue(), 8);
    intKnob.ResetToDefault();
    EXPECT_EQ(intKnob.GetValue(), 5);
}

// -------------------------------------------------------------------------------------------------
// KnobDropdown
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobDropdown_Create)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetFlagHelpText(), "--flag_name1 <\"c1\"|\"c2\">\n");

    strKnob.SetDisplayName("Knob Name 1");
    strKnob.SetFlagDesc("description1");
    strKnob.SetIndent(3);

    EXPECT_EQ(strKnob.GetDisplayName(), "Knob Name 1");
    EXPECT_EQ(strKnob.GetFlagName(), "flag_name1");
    EXPECT_EQ(strKnob.GetFlagHelp(), "description1");
    EXPECT_EQ(strKnob.GetIndent(), 3);
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetValue(), "c2");
    EXPECT_EQ(strKnob.GetFlagHelpText(), "--flag_name1 <\"c1\"|\"c2\"> : description1\n");
}

TEST_F(KnobTestFixture, KnobDropdown_CreateVaried)
{
    std::vector<std::string>  choices1 = {"c1", "c2"};
    KnobDropdown<std::string> strKnob  = KnobDropdown<std::string>("flag_name1", 1, choices1);
    EXPECT_EQ(strKnob.GetIndex(), 1);

    std::vector<const char*> choices2 = {"c1", "c2"};
    strKnob                           = KnobDropdown<std::string>("flag_name2", 0, choices2);
    EXPECT_EQ(strKnob.GetIndex(), 0);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, KnobDropdown_CreateInvalid)
{
    std::vector<std::string> choices = {};
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 0, choices.cbegin(), choices.cend());
        },
        "");

    choices = {"c1", "c2"};
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name2", -1, choices.cbegin(), choices.cend());
        },
        "");
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name3", 2, choices.cbegin(), choices.cend());
        },
        "");
}
#endif

TEST_F(KnobTestFixture, KnobDropdown_SetIndexInt)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex(0);
    EXPECT_EQ(strKnob.GetIndex(), 0);

    // Below min, should not be set
    strKnob.SetIndex(-3);
    EXPECT_EQ(strKnob.GetIndex(), 0);

    // Above max, should not be set
    strKnob.SetIndex(2);
    EXPECT_EQ(strKnob.GetIndex(), 0);
}

TEST_F(KnobTestFixture, KnobDropdown_SetIndexStr)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex("c1");
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_EQ(strKnob.GetValue(), "c1");

    // Not in choices, should not be set
    strKnob.SetIndex("c3");
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_EQ(strKnob.GetValue(), "c1");
}

TEST_F(KnobTestFixture, KnobDropdown_ResetToDefault)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetIndex(), 1);
    strKnob.SetIndex(0);
    EXPECT_EQ(strKnob.GetIndex(), 0);
    strKnob.ResetToDefault();
    EXPECT_EQ(strKnob.GetIndex(), 1);
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
    std::shared_ptr<KnobCheckbox> boolKnobPtr(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    EXPECT_EQ(boolKnobPtr->GetValue(), true);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateIntSlider)
{
    std::shared_ptr<KnobSlider<int>> intKnobPtr(km.CreateKnob<KnobSlider<int>>("flag_name1", 5, 0, 10));
    EXPECT_EQ(intKnobPtr->GetValue(), 5);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateStrDropdown)
{
    std::vector<std::string>                   choices = {"c1", "c2", "c3"};
    std::shared_ptr<KnobDropdown<std::string>> strKnobPtr(km.CreateKnob<KnobDropdown<std::string>>("flag_name1", 1, choices.cbegin(), choices.cend()));
    EXPECT_EQ(strKnobPtr->GetIndex(), 1);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobManagerTestFixture, KnobManager_CreateUniqueName)
{
    std::shared_ptr<KnobCheckbox> boolKnobPtr1(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    EXPECT_DEATH(
        {
            std::shared_ptr<KnobCheckbox> boolKnobPtr2(km.CreateKnob<KnobCheckbox>("flag_name1", true));
        },
        "");
}
#endif

TEST_F(KnobManagerTestFixture, KnobManager_GetUsageMsg)
{
    std::shared_ptr<KnobCheckbox>              boolKnobPtr1(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    std::shared_ptr<KnobCheckbox>              boolKnobPtr2(km.CreateKnob<KnobCheckbox>("flag_name2", true));
    std::shared_ptr<KnobSlider<int>>           intKnobPtr1(km.CreateKnob<KnobSlider<int>>("flag_name3", 5, 0, 10));
    std::vector<std::string>                   choices1 = {"c1", "c2", "c3"};
    std::shared_ptr<KnobDropdown<std::string>> strKnobPtr1(km.CreateKnob<KnobDropdown<std::string>>("flag_name4", 1, choices1.cbegin(), choices1.cend()));

    std::string usageMsg = R"(
Application-specific flags
--flag_name1 <true|false>
--flag_name2 <true|false>
--flag_name3 <0~10>
--flag_name4 <"c1"|"c2"|"c3">
)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerTestFixture, KnobManager_ResetAllToDefault)
{
    std::shared_ptr<KnobCheckbox>              boolKnobPtr1(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    std::shared_ptr<KnobCheckbox>              boolKnobPtr2(km.CreateKnob<KnobCheckbox>("flag_name2", true));
    std::shared_ptr<KnobSlider<int>>           intKnobPtr1(km.CreateKnob<KnobSlider<int>>("flag_name3", 5, 0, 10));
    std::vector<std::string>                   choices1 = {"c1", "c2", "c3"};
    std::shared_ptr<KnobDropdown<std::string>> strKnobPtr1(km.CreateKnob<KnobDropdown<std::string>>("flag_name4", 1, choices1.cbegin(), choices1.cend()));

    // Change from default
    boolKnobPtr1->SetValue(false);
    EXPECT_EQ(boolKnobPtr1->GetValue(), false);
    boolKnobPtr2->SetValue(false);
    EXPECT_EQ(boolKnobPtr2->GetValue(), false);
    intKnobPtr1->SetValue(8);
    EXPECT_EQ(intKnobPtr1->GetValue(), 8);
    strKnobPtr1->SetIndex(0);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 0);

    km.ResetAllToDefault();
    EXPECT_EQ(boolKnobPtr1->GetValue(), true);
    EXPECT_EQ(boolKnobPtr2->GetValue(), true);
    EXPECT_EQ(intKnobPtr1->GetValue(), 5);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 1);
}

} // namespace ppx