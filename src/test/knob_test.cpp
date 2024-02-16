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

class KnobManagerWithKnobsTestFixture : public KnobManagerTestFixture
{
protected:
    void SetUp() override
    {
        ppx::Log::Initialize(ppx::LOG_MODE_CONSOLE, nullptr);

        km.InitKnob(&k1, "flag_name1", true);
        km.InitKnob(&k2, "flag_name2", true);
        km.InitKnob(&k3, "flag_name3", 5, 0, 10);
        km.InitKnob(&k4, "flag_name4", 1, choices4);
        km.InitKnob(&k5, "flag_name5", true);
        km.InitKnob(&k6, "flag_name6", 6.6f, 0.0f, 10.0f);
        km.InitKnob(&k7, "flag_name7", 8, 0, INT_MAX);
        km.InitKnob(&k8, "flag_name8", 5.0f, 0.0f, 10.0f);
        km.InitKnob(&k9, "flag_name9", std::make_pair(1, 2));
        km.InitKnob(&k10, "flag_name10", defaultValues10);
    }

protected:
    std::shared_ptr<ppx::KnobCheckbox>                       k1;
    std::shared_ptr<ppx::KnobCheckbox>                       k2;
    std::shared_ptr<ppx::KnobSlider<int>>                    k3;
    std::vector<std::string>                                 choices4 = {"c1", "c2", "c3 and more"};
    std::shared_ptr<ppx::KnobDropdown<std::string>>          k4;
    std::shared_ptr<ppx::KnobFlag<bool>>                     k5;
    std::shared_ptr<ppx::KnobFlag<float>>                    k6;
    std::shared_ptr<ppx::KnobFlag<int>>                      k7;
    std::shared_ptr<ppx::KnobSlider<float>>                  k8;
    std::shared_ptr<ppx::KnobFlag<std::pair<int, int>>>      k9;
    std::vector<std::string>                                 defaultValues10 = {"a", "b", "c", "d and more"};
    std::shared_ptr<ppx::KnobFlag<std::vector<std::string>>> k10;
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

TEST_F(KnobTestFixture, KnobSlider_CreateIntAndSetBasicMembers)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(k.GetValue(), 5);
}

TEST_F(KnobTestFixture, KnobSlider_CreateFloatAndSetBasicMembers)
{
    KnobSlider<float> k = KnobSlider<float>("flag_name1", 5.0f, 0.0f, 10.0f);
    EXPECT_EQ(k.GetValue(), 5.0f);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, KnobSlider_CreateIntInvalidRangeTooSmall)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> k = KnobSlider<int>("flag_name1", 10, 10, 10);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateFloatInvalidRangeTooSmall)
{
    EXPECT_DEATH(
        {
            KnobSlider<float> k = KnobSlider<float>("flag_name1", 10.0f, 10.0f, 10.0f);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateIntInvalidDefaultTooLow)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> k = KnobSlider<int>("flag_name1", -1, 0, 10);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateFloatInvalidDefaultTooLow)
{
    EXPECT_DEATH(
        {
            KnobSlider<float> k = KnobSlider<float>("flag_name1", -1.0f, 0.0f, 10.0f);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateIntInvalidDefaultTooHigh)
{
    EXPECT_DEATH(
        {
            KnobSlider<int> k = KnobSlider<int>("flag_name1", 11, 0, 10);
        },
        "");
}

TEST_F(KnobTestFixture, KnobSlider_CreateFloatInvalidDefaultTooHigh)
{
    EXPECT_DEATH(
        {
            KnobSlider<float> k = KnobSlider<float>("flag_name1", 11.0f, 0.0f, 10.0f);
        },
        "");
}
#endif

TEST_F(KnobTestFixture, KnobSlider_CanSetIntValue)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(k.GetValue(), 5);
    k.SetValue(10);
    EXPECT_EQ(k.GetValue(), 10);
}

TEST_F(KnobTestFixture, KnobSlider_CanSetFloatValue)
{
    KnobSlider<float> k = KnobSlider<float>("flag_name1", 5.0f, 0.0f, 10.0f);
    EXPECT_EQ(k.GetValue(), 5.0f);
    k.SetValue(5.5f);
    EXPECT_EQ(k.GetValue(), 5.5f);
}

TEST_F(KnobTestFixture, KnobSlider_CanDigestIntValueUpdate)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_TRUE(k.DigestUpdate());
    EXPECT_EQ(k.GetValue(), 5);

    EXPECT_FALSE(k.DigestUpdate());
    k.SetValue(10);
    EXPECT_EQ(k.GetValue(), 10);
    EXPECT_TRUE(k.DigestUpdate());
}

TEST_F(KnobTestFixture, KnobSlider_CanDigestFloatValueUpdate)
{
    KnobSlider<float> k = KnobSlider<float>("flag_name1", 5, 0, 10);
    EXPECT_TRUE(k.DigestUpdate());
    EXPECT_EQ(k.GetValue(), 5);

    EXPECT_FALSE(k.DigestUpdate());
    k.SetValue(10);
    EXPECT_EQ(k.GetValue(), 10);
    EXPECT_TRUE(k.DigestUpdate());
}

TEST_F(KnobTestFixture, KnobSlider_MinIntValueClamped)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    k.SetValue(-3);
    EXPECT_EQ(k.GetValue(), 5);
}

TEST_F(KnobTestFixture, KnobSlider_MinFloatValueClamped)
{
    KnobSlider<float> k = KnobSlider<float>("flag_name1", 5.0f, 0.0f, 10.0f);
    k.SetValue(-3.0f);
    EXPECT_EQ(k.GetValue(), 5.0f);
}

TEST_F(KnobTestFixture, KnobSlider_MaxIntValueClamped)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    k.SetValue(22);
    EXPECT_EQ(k.GetValue(), 5);
}

TEST_F(KnobTestFixture, KnobSlider_MaxFloatValueClamped)
{
    KnobSlider<float> k = KnobSlider<float>("flag_name1", 5.0f, 0.0f, 10.0f);
    k.SetValue(22.0f);
    EXPECT_EQ(k.GetValue(), 5.0f);
}

TEST_F(KnobTestFixture, KnobSlider_ResetToDefault)
{
    KnobSlider<int> k = KnobSlider<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(k.GetValue(), 5);
    k.SetValue(8);
    EXPECT_EQ(k.GetValue(), 8);
    k.ResetToDefault();
    EXPECT_EQ(k.GetValue(), 5);
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

TEST_F(KnobTestFixture, KnobDropdown_CreateNumbers)
{
    std::vector<int>  choices = {1, 2, 3, 5, 8};
    KnobDropdown<int> strKnob = KnobDropdown<int>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetValue(), 2);
    strKnob.SetIndex(2);
    EXPECT_EQ(strKnob.GetIndex(), 2);
    EXPECT_EQ(strKnob.GetValue(), 3);
}

TEST_F(KnobTestFixture, KnobDropdown_CreateCustom)
{
    std::vector<DropdownEntry<int>> choices = {{"a", 1}, {"b", 2}, {"c", 3}};
    KnobDropdown<int>               strKnob = KnobDropdown<int>("flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(strKnob.GetIndex(), 1);
    EXPECT_EQ(strKnob.GetValue(), 2);
    strKnob.SetIndex(2);
    EXPECT_EQ(strKnob.GetIndex(), 2);
    EXPECT_EQ(strKnob.GetValue(), 3);
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
// KnobFlag
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, KnobFlag_CreateBool)
{
    KnobFlag<bool> k = KnobFlag<bool>("flag_name1", false);
    EXPECT_EQ(k.GetValue(), false);
}

TEST_F(KnobTestFixture, KnobFlag_CreateString)
{
    KnobFlag<std::string> k = KnobFlag<std::string>("flag_name1", "placeholder");
    EXPECT_EQ(k.GetValue(), "placeholder");
}

TEST_F(KnobTestFixture, KnobFlag_CreateInt)
{
    KnobFlag<int> k = KnobFlag<int>("flag_name1", 0);
    EXPECT_EQ(k.GetValue(), 0);
}

TEST_F(KnobTestFixture, KnobFlag_CreateFloat)
{
    KnobFlag<float> k = KnobFlag<float>("flag_name1", 1.5f);
    EXPECT_EQ(k.GetValue(), 1.5f);
}

TEST_F(KnobTestFixture, KnobFlag_CreateIntWithRange)
{
    KnobFlag<int> k = KnobFlag<int>("flag_name1", 5, 0, 10);
    EXPECT_EQ(k.GetValue(), 5);
}

TEST_F(KnobTestFixture, KnobFlag_CreateFloatWithRange)
{
    KnobFlag<float> k = KnobFlag<float>("flag_name1", 1.5f, 0.0f, 3.0f);
    EXPECT_EQ(k.GetValue(), 1.5f);
}

TEST_F(KnobTestFixture, KnobFlag_CreatePairInts)
{
    KnobFlag<std::pair<int, int>> k = KnobFlag<std::pair<int, int>>("flag_name1", std::make_pair(1, 2));
    EXPECT_EQ(k.GetValue().first, 1);
    EXPECT_EQ(k.GetValue().second, 2);
}

TEST_F(KnobTestFixture, KnobFlag_CreateListStrings)
{
    KnobFlag<std::vector<std::string>> k = KnobFlag<std::vector<std::string>>("flag_name1", {"hi1", "hi2", "hi3"});
    EXPECT_EQ(k.GetValue().size(), 3);
    EXPECT_EQ(k.GetValue().at(0), "hi1");
    EXPECT_EQ(k.GetValue().at(1), "hi2");
    EXPECT_EQ(k.GetValue().at(2), "hi3");
}

TEST_F(KnobTestFixture, KnobFlag_CreateListInts)
{
    KnobFlag<std::vector<int>> k = KnobFlag<std::vector<int>>("flag_name1", {1, 2, 3});
    EXPECT_EQ(k.GetValue().size(), 3);
    EXPECT_EQ(k.GetValue().at(0), 1);
    EXPECT_EQ(k.GetValue().at(1), 2);
    EXPECT_EQ(k.GetValue().at(2), 3);
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
    std::shared_ptr<KnobCheckbox> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_TRUE(pKnob->GetValue());
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateIntSlider)
{
    std::shared_ptr<KnobSlider<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 5, 0, 10);
    EXPECT_EQ(pKnob->GetValue(), 5);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateFloatSlider)
{
    std::shared_ptr<KnobSlider<float>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 5.0f, 0.0f, 10.0f);
    EXPECT_EQ(pKnob->GetValue(), 5.0f);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateStrDropdown)
{
    std::vector<std::string>                   choices = {"c1", "c2", "c3"};
    std::shared_ptr<KnobDropdown<std::string>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 1, choices.cbegin(), choices.cend());
    EXPECT_EQ(pKnob->GetIndex(), 1);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagBool)
{
    std::shared_ptr<KnobFlag<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_EQ(pKnob->GetValue(), true);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagStr)
{
    std::shared_ptr<KnobFlag<std::string>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", "placeholder");
    EXPECT_EQ(pKnob->GetValue(), "placeholder");
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagInt)
{
    std::shared_ptr<KnobFlag<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 5);
    EXPECT_EQ(pKnob->GetValue(), 5);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagFloat)
{
    std::shared_ptr<KnobFlag<float>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 5.5f);
    EXPECT_EQ(pKnob->GetValue(), 5.5f);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagIntWithRange)
{
    std::shared_ptr<KnobFlag<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 5, 0, 10);
    EXPECT_EQ(pKnob->GetValue(), 5);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagFloatWithRange)
{
    std::shared_ptr<KnobFlag<float>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 1.5f, 0.0f, 3.0f);
    EXPECT_EQ(pKnob->GetValue(), 1.5f);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagPairInts)
{
    std::shared_ptr<KnobFlag<std::pair<int, int>>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", std::make_pair(1, 2));
    EXPECT_EQ(pKnob->GetValue().first, 1);
    EXPECT_EQ(pKnob->GetValue().second, 2);
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagListStrings)
{
    std::vector<std::string>                            stringList = {"hi1", "hi2", "hi3"};
    std::shared_ptr<KnobFlag<std::vector<std::string>>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", stringList);
    EXPECT_EQ(pKnob->GetValue().size(), 3);
    EXPECT_EQ(pKnob->GetValue().at(0), "hi1");
    EXPECT_EQ(pKnob->GetValue().at(1), "hi2");
    EXPECT_EQ(pKnob->GetValue().at(2), "hi3");
}

TEST_F(KnobManagerTestFixture, KnobManager_CreateKnobFlagListInts)
{
    std::vector<int>                            intList = {1, 2, 3};
    std::shared_ptr<KnobFlag<std::vector<int>>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", intList);
    EXPECT_EQ(pKnob->GetValue().size(), 3);
    EXPECT_EQ(pKnob->GetValue().at(0), 1);
    EXPECT_EQ(pKnob->GetValue().at(1), 2);
    EXPECT_EQ(pKnob->GetValue().at(2), 3);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobManagerTestFixture, KnobManager_CreateUniqueName)
{
    std::shared_ptr<KnobCheckbox> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_DEATH(
        {
            std::shared_ptr<KnobCheckbox> pKnob;
            km.InitKnob(&pKnob, "flag_name1", true);
        },
        "");
}
#endif

TEST_F(KnobManagerWithKnobsTestFixture, KnobManager_GetBasicUsageMsg)
{
    std::string usageMsg = R"(
Flags:
--flag_name1 <true|false>
                    (Default: true)

--flag_name2 <true|false>
                    (Default: true)

--flag_name3 <0~10>
                    (Default: 5)

--flag_name4 <c1|c2|"c3 and more">
                    (Default: c2)

--flag_name5
                    (Default: true)

--flag_name6
                    (Default: 6.6)

--flag_name7
                    (Default: 8)

--flag_name8 <0.0~10.0>
                    (Default: 5)

--flag_name9
                    (Default: 1,2)

--flag_name10
                    (Default: a,b,c,d and more)

)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerWithKnobsTestFixture, KnobManager_GetCustomizedUsageMsg)
{
    k1->SetFlagParameters("<bool>");
    k1->SetFlagDescription("description1");
    k3->SetFlagParameters("<N>");
    k3->SetFlagDescription("description3");
    k4->SetFlagDescription("description4");
    k5->SetFlagParameters("<0|1>");
    k6->SetFlagParameters("<0.0~10.0>");
    k6->SetFlagDescription("description6");
    k7->SetFlagParameters("<0~INT_MAX>");
    k8->SetFlagParameters("<0.000~10.000>");
    k9->SetFlagDescription("description9");
    k10->SetFlagParameters("<string>");

    std::string usageMsg = R"(
Flags:
--flag_name1 <bool>
                    (Default: true)
                    description1

--flag_name2 <true|false>
                    (Default: true)

--flag_name3 <N>
                    (Default: 5)
                    description3

--flag_name4 <c1|c2|"c3 and more">
                    (Default: c2)
                    description4

--flag_name5 <0|1>
                    (Default: true)

--flag_name6 <0.0~10.0>
                    (Default: 6.6)
                    description6

--flag_name7 <0~INT_MAX>
                    (Default: 8)

--flag_name8 <0.000~10.000>
                    (Default: 5)

--flag_name9
                    (Default: 1,2)
                    description9

--flag_name10 <string>
                    (Default: a,b,c,d and more)

)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerWithKnobsTestFixture, KnobManager_ResetAllToDefault)
{
    // Change from default
    k1->SetValue(false);
    EXPECT_FALSE(k1->GetValue());
    k2->SetValue(false);
    EXPECT_FALSE(k2->GetValue());
    k3->SetValue(8);
    EXPECT_EQ(k3->GetValue(), 8);
    k4->SetIndex(0);
    EXPECT_EQ(k4->GetIndex(), 0);
    k8->SetValue(8.0f);

    km.ResetAllToDefault();
    EXPECT_TRUE(k1->GetValue());
    EXPECT_TRUE(k2->GetValue());
    EXPECT_EQ(k3->GetValue(), 5);
    EXPECT_EQ(k4->GetIndex(), 1);
    EXPECT_EQ(k8->GetValue(), 5.0f);
}

void UpdateDependentKnobs(std::shared_ptr<KnobCheckbox> p1, std::shared_ptr<KnobSlider<int>> p2, std::shared_ptr<KnobDropdown<std::string>> p3)
{
    // Example where changing either the slider or the dropdown will uncheck the box.
    if (p2->DigestUpdate()) {
        std::cout << "digested int slider knob" << std::endl;
        p1->SetValue(false);
    }
    if (p3->DigestUpdate()) {
        std::cout << "digested string dropdown knob" << std::endl;
        p1->SetValue(false);
    }
    p1->DigestUpdate();
    std::cout << "current value of checkbox: " << p1->GetValue() << std::endl;
}

TEST_F(KnobManagerWithKnobsTestFixture, KnobManager_UpdateDependentKnobs)
{
    UpdateDependentKnobs(k1, k3, k4);
    k1->SetValue(true);
    k3->SetValue(8);
    k4->SetIndex(2);
    EXPECT_TRUE(k1->GetValue());

    std::cout << "about to digest second time" << std::endl;
    UpdateDependentKnobs(k1, k3, k4);
    EXPECT_EQ(k3->GetValue(), 8);
    EXPECT_EQ(k4->GetIndex(), 2);
    EXPECT_FALSE(k1->GetValue());
}

} // namespace ppx
