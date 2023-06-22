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
    EXPECT_TRUE(boolKnob.GetValue());
}

TEST_F(KnobTestFixture, KnobCheckbox_CanSetBoolValue)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", false);
    EXPECT_FALSE(boolKnob.GetValue());
    boolKnob.SetValue(true);
    EXPECT_TRUE(boolKnob.GetValue());
}

TEST_F(KnobTestFixture, KnobCheckbox_CanDigestBoolValueUpdate)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", false);
    EXPECT_TRUE(boolKnob.DigestUpdate());
    EXPECT_FALSE(boolKnob.GetValue());

    EXPECT_FALSE(boolKnob.DigestUpdate());
    boolKnob.SetValue(true);
    EXPECT_TRUE(boolKnob.GetValue());
    EXPECT_TRUE(boolKnob.DigestUpdate());
}

TEST_F(KnobTestFixture, KnobCheckbox_ResetToDefault)
{
    KnobCheckbox boolKnob = KnobCheckbox("flag_name1", true);
    EXPECT_TRUE(boolKnob.GetValue());
    boolKnob.SetValue(false);
    EXPECT_FALSE(boolKnob.GetValue());
    boolKnob.ResetToDefault();
    EXPECT_TRUE(boolKnob.GetValue());
}

// -------------------------------------------------------------------------------------------------
// KnobSlider
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobSlider_CreateAndSetBasicMembers)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetValue(), 5);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, KnobSlider_CreateInvalidRangeTooSmall)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 10, 10, 10);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateInvalidDefaultTooLow)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", -1, 0, 10);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateInvalidDefaultTooHigh)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 11, 0, 10);
        },
        "");
}
#endif

TEST_F(KnobTestFixture, KnobSlider_CanSetIntValue)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(intKnob.GetValue(), 5);
    intKnob.SetValue(10);
    EXPECT_EQ(intKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, KnobSlider_CanDigestIntValueUpdate)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_TRUE(intKnob.DigestUpdate());
    EXPECT_EQ(intKnob.GetValue(), 5);

    EXPECT_FALSE(intKnob.DigestUpdate());
    intKnob.SetValue(10);
    EXPECT_EQ(intKnob.GetValue(), 10);
    EXPECT_TRUE(intKnob.DigestUpdate());
}

TEST_F(KnobTestFixture, KnobSlider_MinIntValueClamped)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    intKnob.SetValue(-3);
    EXPECT_EQ(intKnob.GetValue(), 5);
}

TEST_F(KnobTestFixture, KnobSlider_MaxIntValueClamped)
{
    KnobSlider<int> intKnob = KnobSlider<int>("flag_name1", 5, 0, 10);
    intKnob.SetValue(22);
    EXPECT_EQ(intKnob.GetValue(), 5);
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

TEST_F(KnobTestFixture, KnobDropdown_CreateAndSetBasicMembers)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetValue(), "c2");
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
TEST_F(KnobTestFixture, KnobDropdown_CreateInvalidEmptyChoices)
{
    std::vector<std::string> choices = {};
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 0, choices.cbegin(), choices.cend());
        },
        "");
}

TEST_F(KnobTestFixture, KnobDropdown_CreateInvalidDefaultTooLow)
{
    std::vector<std::string> choices = {"c1", "c2"};
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", -1, choices.cbegin(), choices.cend());
        },
        "");
}

TEST_F(KnobTestFixture, KnobDropdown_CreateInvalidDefaultTooHigh)
{
    std::vector<std::string> choices = {"c1", "c2"};
    EXPECT_DEATH(
        {
            KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 2, choices.cbegin(), choices.cend());
        },
        "");
}
#endif

TEST_F(KnobTestFixture, KnobDropdown_CanSetIndexInt)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetIndex(), 1);
    strKnob.SetIndex(0);
    EXPECT_EQ(strKnob.GetIndex(), 0);
}

TEST_F(KnobTestFixture, KnobDropdown_CanSetIndexStr)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex("c1");
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_EQ(strKnob.GetValue(), "c1");
}

TEST_F(KnobTestFixture, KnobDropdown_MinIndexClamped)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex(-3);
    EXPECT_EQ(strKnob.GetIndex(), 1);
}

TEST_F(KnobTestFixture, KnobDropdown_MaxIndexClamped)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex(2);
    EXPECT_EQ(strKnob.GetIndex(), 1);
}

TEST_F(KnobTestFixture, KnobDropdown_WontSetUnknownStr)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    strKnob.SetIndex("c3");
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetValue(), "c2");
}

TEST_F(KnobTestFixture, KnobDropdown_CanDigestIndexUpdate)
{
    std::vector<std::string>  choices = {"c1", "c2"};
    KnobDropdown<std::string> strKnob = KnobDropdown<std::string>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_TRUE(strKnob.DigestUpdate());
    EXPECT_EQ(strKnob.GetIndex(), 1);

    EXPECT_FALSE(strKnob.DigestUpdate());
    strKnob.SetIndex(0);
    EXPECT_EQ(strKnob.GetIndex(), 0);
    EXPECT_TRUE(strKnob.DigestUpdate());
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
    EXPECT_TRUE(boolKnobPtr->GetValue());
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

TEST_F(KnobManagerTestFixture, KnobManager_GetBasicUsageMsg)
{
    std::shared_ptr<KnobCheckbox>              k1(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    std::shared_ptr<KnobCheckbox>              k2(km.CreateKnob<KnobCheckbox>("flag_name2", true));
    std::shared_ptr<KnobSlider<int>>           k3(km.CreateKnob<KnobSlider<int>>("flag_name3", 5, 0, 10));
    std::vector<std::string>                   choices4 = {"c1", "c2", "c3 and more"};
    std::shared_ptr<KnobDropdown<std::string>> k4(km.CreateKnob<KnobDropdown<std::string>>("flag_name4", 1, choices4.cbegin(), choices4.cend()));

    std::string usageMsg = R"(
Application-Specific Flags:
--flag_name1 <true|false>
--flag_name2 <true|false>
--flag_name3 <0~10>
--flag_name4 <c1|c2|"c3 and more">
)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerTestFixture, KnobManager_GetCustomizedUsageMsg)
{
    std::shared_ptr<KnobCheckbox> k1(km.CreateKnob<KnobCheckbox>("flag_name1", true));
    k1->SetFlagParameters(" <bool>");
    k1->SetFlagDesc("description1");
    std::shared_ptr<KnobCheckbox>    k2(km.CreateKnob<KnobCheckbox>("flag_name2", true));
    std::shared_ptr<KnobSlider<int>> k3(km.CreateKnob<KnobSlider<int>>("flag_name3", 5, 0, 10));
    k3->SetFlagParameters(" <N>");
    k3->SetFlagDesc("description3");
    std::vector<std::string>                   choices4 = {"c1", "c2", "c3 and more"};
    std::shared_ptr<KnobDropdown<std::string>> k4(km.CreateKnob<KnobDropdown<std::string>>("flag_name4", 1, choices4.cbegin(), choices4.cend()));
    k4->SetFlagDesc("description4");

    std::string usageMsg = R"(
Application-Specific Flags:
--flag_name1 <bool> : description1
--flag_name2 <true|false>
--flag_name3 <N> : description3
--flag_name4 <c1|c2|"c3 and more"> : description4
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
    EXPECT_FALSE(boolKnobPtr1->GetValue());
    boolKnobPtr2->SetValue(false);
    EXPECT_FALSE(boolKnobPtr2->GetValue());
    intKnobPtr1->SetValue(8);
    EXPECT_EQ(intKnobPtr1->GetValue(), 8);
    strKnobPtr1->SetIndex(0);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 0);

    km.ResetAllToDefault();
    EXPECT_TRUE(boolKnobPtr1->GetValue());
    EXPECT_TRUE(boolKnobPtr2->GetValue());
    EXPECT_EQ(intKnobPtr1->GetValue(), 5);
    EXPECT_EQ(strKnobPtr1->GetIndex(), 1);
}

void UpdateDependentKnobs(std::shared_ptr<KnobCheckbox> p1, std::shared_ptr<KnobSlider<int>> p2, std::shared_ptr<KnobDropdown<std::string>> p3)
{
    // Example where changing either the slider or the dropdown will uncheck the box.
    if (p2->DigestUpdate()) {
        std::cout << "digested p2" << std::endl;
        p1->SetValue(false);
    }
    if (p3->DigestUpdate()) {
        std::cout << "digested p3" << std::endl;
        p1->SetValue(false);
    }
    p1->DigestUpdate();
    std::cout << "current value of p1: " << p1->GetValue() << std::endl;
}

TEST_F(KnobManagerTestFixture, KnobManager_UpdateDependentKnobs)
{
    std::shared_ptr<KnobCheckbox>              pKnob1(km.CreateKnob<ppx::KnobCheckbox>("knob1", false));
    std::shared_ptr<KnobSlider<int>>           pKnob2(km.CreateKnob<ppx::KnobSlider<int>>("knob2", 5, 0, 10));
    std::vector<std::string>                   knob3Choices = {"one", "two", "three"};
    std::shared_ptr<KnobDropdown<std::string>> pKnob3(km.CreateKnob<ppx::KnobDropdown<std::string>>("knob3", 1, knob3Choices));

    UpdateDependentKnobs(pKnob1, pKnob2, pKnob3);
    pKnob1->SetValue(true);
    pKnob2->SetValue(8);
    pKnob3->SetIndex(2);
    EXPECT_TRUE(pKnob1->GetValue());

    std::cout << "about to digest second time" << std::endl;
    UpdateDependentKnobs(pKnob1, pKnob2, pKnob3);
    EXPECT_EQ(pKnob2->GetValue(), 8);
    EXPECT_EQ(pKnob3->GetIndex(), 2);
    EXPECT_FALSE(pKnob1->GetValue());
}

} // namespace ppx
