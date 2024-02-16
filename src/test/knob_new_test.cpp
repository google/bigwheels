// Copyright 2024 Google LLC
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

#include "ppx/knob_new.h"

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

class KnobManagerNewTestFixture : public KnobTestFixture
{
protected:
    ppx::KnobManagerNew km;
};

class KnobManagerNewWithKnobsTestFixture : public KnobManagerNewTestFixture
{
protected:
    void SetUp() override
    {
        ppx::Log::Initialize(ppx::LOG_MODE_CONSOLE, nullptr);

        km.InitKnob(&pGeneralBoolean, "general_boolean", true);

        km.InitKnob(&pGeneralBooleanList, "general_boolean_list", std::vector<bool>{true, true, true});
        pGeneralBooleanList->SetValidator([](std::vector<bool> values) {
            for (auto v : values) {
                if (!v) {
                    return false;
                }
            }
            return true;
        });

        km.InitKnob(&pRange1Int, "range_1_int", 0);
        pRange1Int->SetMin(-10);
        pRange1Int->SetMax(10);

        km.InitKnob(&pRange3Int, "range_3_int", std::vector<int>{1, 2, 3});
        pRange3Int->SetAllMins(-10);
        pRange3Int->SetAllMaxes(10);

        km.InitKnob(&pRange3Float, "range_3_float", std::vector<float>{1.5f, 2.5f, 3.5f});
        pRange3Float->SetAllMins(-10.0f);
        pRange3Float->SetAllMaxes(10.0f);

        std::vector<ppx::OptionKnobEntry<int>> entries = {{"c1", 1}, {"c2", 2}, {"c3 and more", 3}};
        km.InitKnob(&pOptionInt, "option_int", 1, entries);
        pOptionInt->SetMask(0, false);

        km.InitKnob(&pOptionString, "option_string", 1, std::vector<std::string>{"c1", "c2", "c3 and more"});
        pOptionString->SetMask(0, false);
    }

protected:
    std::shared_ptr<ppx::GeneralKnob<bool>>              pGeneralBoolean;
    std::shared_ptr<ppx::GeneralKnob<std::vector<bool>>> pGeneralBooleanList;
    std::shared_ptr<ppx::RangeKnob<int>>                 pRange1Int;
    std::shared_ptr<ppx::RangeKnob<int>>                 pRange3Int;
    std::shared_ptr<ppx::RangeKnob<float>>               pRange3Float;
    std::shared_ptr<ppx::OptionKnob<int>>                pOptionInt;
    std::shared_ptr<ppx::OptionKnob<std::string>>        pOptionString;
};

namespace ppx {

// -------------------------------------------------------------------------------------------------
// GeneralKnob
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, GeneralKnob_CreateBoolean)
{
    GeneralKnob pKnob = GeneralKnob<bool>("flag_name1", true);
    EXPECT_TRUE(pKnob.GetValue());
}

TEST_F(KnobTestFixture, GeneralKnob_CreateInt)
{
    GeneralKnob pKnob = GeneralKnob<int>("flag_name1", 10);
    EXPECT_EQ(pKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, GeneralKnob_CreateBooleanList)
{
    GeneralKnob pKnob = GeneralKnob<std::vector<bool>>("flag_name1", std::vector<bool>{true, true, false});
    EXPECT_TRUE(pKnob.GetValue().at(0));
    EXPECT_TRUE(pKnob.GetValue().at(1));
    EXPECT_FALSE(pKnob.GetValue().at(2));
}

TEST_F(KnobTestFixture, GeneralKnob_CanSetBoolValue)
{
    GeneralKnob pKnob = GeneralKnob<bool>("flag_name1", false);
    EXPECT_FALSE(pKnob.GetValue());
    pKnob.SetValue(true);
    EXPECT_TRUE(pKnob.GetValue());
}

TEST_F(KnobTestFixture, GeneralKnob_CustomValidator)
{
    GeneralKnob pKnob = GeneralKnob<bool>("flag_name1", false);
    pKnob.SetValidator([](bool newVal) { return !newVal; });

    EXPECT_FALSE(pKnob.GetValue());
    pKnob.SetValue(true);
    EXPECT_FALSE(pKnob.GetValue());
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, GeneralKnob_CustomValidatorInvalid)
{
    GeneralKnob pKnob = GeneralKnob<bool>("flag_name1", false);
    EXPECT_DEATH(
        {
            pKnob.SetValidator([](bool newVal) { return newVal; });
        },
        "");
}
#endif

// -------------------------------------------------------------------------------------------------
// RangeKnob
// -------------------------------------------------------------------------------------------------

TEST_F(KnobTestFixture, RangeKnob1_CreateIntIterators)
{
    std::vector<int> defaultValues = {1};
    RangeKnob        pKnob         = RangeKnob<int>("flag_name1", defaultValues.begin(), defaultValues.end());
    EXPECT_EQ(pKnob.GetValue(0), 1);
}

TEST_F(KnobTestFixture, RangeKnob1_CreateIntContainer)
{
    std::vector<int> defaultValues = {1};
    RangeKnob        pKnob         = RangeKnob<int>("flag_name1", defaultValues);
    EXPECT_EQ(pKnob.GetValue(0), 1);
}

TEST_F(KnobTestFixture, RangeKnob1_CreateIntConvenient)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    EXPECT_EQ(pKnob.GetValue(), 1);
}

TEST_F(KnobTestFixture, RangeKnob1_CanSetValue)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetValue(5);
    EXPECT_EQ(pKnob.GetValue(), 5);
}

TEST_F(KnobTestFixture, RangeKnob1_SetMax)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetMax(0);
    EXPECT_EQ(pKnob.GetValue(), 0);
}

TEST_F(KnobTestFixture, RangeKnob1_SetMin)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetMin(10);
    EXPECT_EQ(pKnob.GetValue(), 10);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, RangeKnob1_SetMaxBelowMin)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetMin(0);
    EXPECT_DEATH(
        {
            pKnob.SetMax(-1);
        },
        "");
}

TEST_F(KnobTestFixture, RangeKnob1_SetMinAboveMax)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetMax(0);
    EXPECT_DEATH(
        {
            pKnob.SetMin(1);
        },
        "");
}
#endif

TEST_F(KnobTestFixture, RangeKnob1_SetOutOfRange)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", 1);
    pKnob.SetMin(0);
    pKnob.SetMax(10);
    pKnob.SetValue(22);
    EXPECT_EQ(pKnob.GetValue(), 1);
}

TEST_F(KnobTestFixture, RangeKnob3_CreateInt)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 3);
}

TEST_F(KnobTestFixture, RangeKnob3_CreateFloat)
{
    RangeKnob pKnob = RangeKnob<float>("flag_name1", std::vector<float>{1.5f, 2.5f, 3.5f});
    EXPECT_EQ(pKnob.GetValue(0), 1.5f);
    EXPECT_EQ(pKnob.GetValue(1), 2.5f);
    EXPECT_EQ(pKnob.GetValue(2), 3.5f);
}

TEST_F(KnobTestFixture, RangeKnob3_CanSetValue)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    pKnob.SetValue(2, 5);
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 5);
}

TEST_F(KnobTestFixture, RangeKnob3_SetMax)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    pKnob.SetMax(2, 0);
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 0);
}

TEST_F(KnobTestFixture, RangeKnob3_SetMin)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    pKnob.SetMin(2, 10);
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 10);
}

TEST_F(KnobTestFixture, RangeKnob3_SetOutOfRange)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    pKnob.SetMin(2, 0);
    pKnob.SetMax(2, 10);
    pKnob.SetValue(2, 22);
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 3);
}

TEST_F(KnobTestFixture, RangeKnob3_SetAllOutOfRange)
{
    RangeKnob pKnob = RangeKnob<int>("flag_name1", std::vector<int>{1, 2, 3});
    pKnob.SetAllMins(0);
    pKnob.SetAllMaxes(2);
    pKnob.SetAllValues(10);
    EXPECT_EQ(pKnob.GetValue(0), 1);
    EXPECT_EQ(pKnob.GetValue(1), 2);
    EXPECT_EQ(pKnob.GetValue(2), 2); // Was changed by the reduced max value
}

// -------------------------------------------------------------------------------------------------
// OptionKnob
// -------------------------------------------------------------------------------------------------
TEST_F(KnobTestFixture, OptionKnob_CreateIntEntryIterators)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices.begin(), choices.end());
    EXPECT_EQ(pKnob.GetIndex(), 1);
    EXPECT_EQ(pKnob.GetValue(), 20);
}

TEST_F(KnobTestFixture, OptionKnob_CreateIntEntryContainer)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    EXPECT_EQ(pKnob.GetIndex(), 1);
    EXPECT_EQ(pKnob.GetValue(), 20);
}

TEST_F(KnobTestFixture, OptionKnob_CreateString)
{
    std::vector<std::string> choices = {"name1", "name2"};
    OptionKnob               pKnob   = OptionKnob<std::string>("flag_name1", 1, choices);
    EXPECT_EQ(pKnob.GetIndex(), 1);
    EXPECT_EQ(pKnob.GetValue(), "name2");
}

TEST_F(KnobTestFixture, OptionKnob_CanSetIndex)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    pKnob.SetIndex(0);
    EXPECT_EQ(pKnob.GetIndex(), 0);
    EXPECT_EQ(pKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, OptionKnob_SetMaskByIndex)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    pKnob.SetMask(1, false);
    EXPECT_EQ(pKnob.GetIndex(), 0);
    EXPECT_EQ(pKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, OptionKnob_SetMaskEntire)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    std::vector<bool>                 mask    = {true, false};
    pKnob.SetMask(mask);
    EXPECT_EQ(pKnob.GetIndex(), 0);
    EXPECT_EQ(pKnob.GetValue(), 10);
}

TEST_F(KnobTestFixture, OptionKnob_SetOutOfRange)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    pKnob.SetIndex(3);
    EXPECT_EQ(pKnob.GetIndex(), 1);
    EXPECT_EQ(pKnob.GetValue(), 20);
}

TEST_F(KnobTestFixture, OptionKnob_SetOutOfMaskRange)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    std::vector<bool>                 mask    = {false, true};
    pKnob.SetMask(mask);
    pKnob.SetIndex(0);
    EXPECT_EQ(pKnob.GetIndex(), 1);
    EXPECT_EQ(pKnob.GetValue(), 20);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobTestFixture, OptionKnob_SetMaskIndexInvalid)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);

    EXPECT_DEATH(
        {
            pKnob.SetMask(2, false);
        },
        "");
}

TEST_F(KnobTestFixture, OptionKnob_SetMaskEntireInvalid)
{
    std::vector<OptionKnobEntry<int>> choices = {{"name1", 10}, {"name2", 20}};
    OptionKnob                        pKnob   = OptionKnob<int>("flag_name1", 1, choices);
    std::vector<bool>                 mask    = {true, false, true};

    EXPECT_DEATH(
        {
            pKnob.SetMask(mask);
        },
        "");
}
#endif

// -------------------------------------------------------------------------------------------------
// KnobManagerNew
// -------------------------------------------------------------------------------------------------

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_Empty)
{
    EXPECT_TRUE(km.IsEmpty());
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterGeneralKnobBool)
{
    std::shared_ptr<GeneralKnob<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_FALSE(km.IsEmpty());
    EXPECT_EQ(pKnob->GetValue(), true);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterGeneralKnobBoolList)
{
    std::shared_ptr<GeneralKnob<std::vector<bool>>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", std::vector<bool>{true, false});
    EXPECT_FALSE(km.IsEmpty());
    std::vector<bool> gotValue = pKnob->GetValue();
    EXPECT_EQ(gotValue.size(), 2);
    EXPECT_EQ(gotValue.at(0), true);
    EXPECT_EQ(gotValue.at(1), false);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterRangeKnob1Int)
{
    std::shared_ptr<RangeKnob<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 1);
    EXPECT_FALSE(km.IsEmpty());
    EXPECT_EQ(pKnob->GetValue(), 1);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterRangeKnob3Int)
{
    std::shared_ptr<RangeKnob<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", std::vector<int>{1, 2, 3});
    EXPECT_FALSE(km.IsEmpty());
    EXPECT_EQ(pKnob->GetValue(0), 1);
    EXPECT_EQ(pKnob->GetValue(1), 2);
    EXPECT_EQ(pKnob->GetValue(2), 3);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterOptionKnobInt)
{
    std::shared_ptr<OptionKnob<int>> pKnob;
    std::vector<int>                 choices = {1, 2, 3};
    km.InitKnob(&pKnob, "flag_name1", 1, choices);
    EXPECT_FALSE(km.IsEmpty());
    EXPECT_EQ(pKnob->GetIndex(), 1);
    EXPECT_EQ(pKnob->GetValue(), 2);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobManagerNewTestFixture, KnobManagerNew_RegisterDuplicateNameFail)
{
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob;
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob2;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_DEATH(
        {
            km.InitKnob(&pKnob2, "flag_name1", true);
        },
        "");
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_BeforeFinalizationResetFail)
{
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_DEATH(
        {
            km.ResetAllToStartup();
        },
        "");
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_BeforeFinalizationDigestFail)
{
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    EXPECT_DEATH(
        {
            pKnob->DigestUpdate();
        },
        "");
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_AfterFinalizationSetStartupOnlyFail)
{
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    km.FinalizeAll();
    EXPECT_DEATH(
        {
            pKnob->SetStartupOnly();
        },
        "");
}
#endif

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_StartupGeneral)
{
    std::shared_ptr<ppx::GeneralKnob<bool>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", true);
    pKnob->SetStartupOnly();
    km.FinalizeAll();

    EXPECT_EQ(pKnob->GetValue(), true);
    pKnob->SetValue(false);
    EXPECT_EQ(pKnob->GetValue(), true);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_StartupRange)
{
    std::shared_ptr<ppx::RangeKnob<int>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 3);
    pKnob->SetStartupOnly();
    km.FinalizeAll();

    EXPECT_EQ(pKnob->GetValue(), 3);
    pKnob->SetValue(4);
    EXPECT_EQ(pKnob->GetValue(), 3);
    pKnob->SetMin(4);
    EXPECT_EQ(pKnob->GetValue(), 3);
    pKnob->SetMax(2);
    EXPECT_EQ(pKnob->GetValue(), 3);
}

TEST_F(KnobManagerNewTestFixture, KnobManagerNew_StartupOption)
{
    std::shared_ptr<ppx::OptionKnob<std::string>> pKnob;
    km.InitKnob(&pKnob, "flag_name1", 1, std::vector<std::string>{"c1", "c2"});
    pKnob->SetStartupOnly();
    km.FinalizeAll();

    EXPECT_EQ(pKnob->GetIndex(), 1);
    pKnob->SetIndex(0);
    EXPECT_EQ(pKnob->GetIndex(), 1);
    pKnob->SetMask(1, false);
    EXPECT_EQ(pKnob->GetIndex(), 1);
    pKnob->SetMask(std::vector<bool>{true, false});
    EXPECT_EQ(pKnob->GetIndex(), 1);
}

// -------------------------------------------------------------------------------------------------
// KnobManagerNew (complex)
// -------------------------------------------------------------------------------------------------

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_DigestUpdatesSetters)
{
    km.FinalizeAll();

    // Clear updates
    EXPECT_TRUE(pGeneralBoolean->DigestUpdate());
    EXPECT_FALSE(pGeneralBoolean->DigestUpdate());
    EXPECT_TRUE(pRange3Int->DigestUpdate());
    EXPECT_FALSE(pRange3Int->DigestUpdate());
    EXPECT_TRUE(pOptionInt->DigestUpdate());
    EXPECT_FALSE(pOptionInt->DigestUpdate());

    // Change with setter
    pGeneralBoolean->SetValue(false);
    EXPECT_TRUE(pGeneralBoolean->DigestUpdate());
    pRange3Int->SetValue(0, 8);
    EXPECT_TRUE(pRange3Int->DigestUpdate());
    pOptionInt->SetIndex(2);
    EXPECT_TRUE(pOptionInt->DigestUpdate());
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_DigestUpdatesReset)
{
    km.FinalizeAll();

    // Clear updates
    EXPECT_TRUE(pGeneralBoolean->DigestUpdate());
    EXPECT_FALSE(pGeneralBoolean->DigestUpdate());
    EXPECT_TRUE(pRange3Int->DigestUpdate());
    EXPECT_FALSE(pRange3Int->DigestUpdate());
    EXPECT_TRUE(pOptionInt->DigestUpdate());
    EXPECT_FALSE(pOptionInt->DigestUpdate());

    // Change with reset to startup
    km.ResetAllToStartup();
    EXPECT_TRUE(pGeneralBoolean->DigestUpdate());
    EXPECT_TRUE(pRange3Int->DigestUpdate());
    EXPECT_TRUE(pOptionInt->DigestUpdate());
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_SetAllStartupOnly)
{
    km.SetAllStartupOnly();
    km.FinalizeAll();

    // Try to change from startup
    pGeneralBoolean->SetValue(false);
    pRange3Int->SetValue(0, 8);
    pOptionInt->SetIndex(2);

    EXPECT_TRUE(pGeneralBoolean->GetValue());
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pOptionInt->GetIndex(), 1);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_ResetAllToStartup)
{
    km.FinalizeAll();

    // Change from startup
    pGeneralBoolean->SetValue(false);
    pRange3Int->SetValue(0, 8);
    pOptionInt->SetIndex(2);

    km.ResetAllToStartup();
    EXPECT_TRUE(pGeneralBoolean->GetValue());
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pOptionInt->GetIndex(), 1);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_GetBasicUsageMsg)
{
    std::string usageMsg = R"(
Flags:
--general_boolean
                    (Default: true)

--general_boolean_list
                    (Default: true,true,true)

--range_1_int <-10 ~ 10>
                    (Default: 0)

--range_3_int <-10,-10,-10 ~ 10,10,10>
                    (Default: 1,2,3)

--range_3_float <-10,-10,-10 ~ 10,10,10>
                    (Default: 1.5,2.5,3.5)

--option_int <c2|"c3 and more">
                    (Default: c2)

--option_string <c2|"c3 and more">
                    (Default: c2)

)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_GetCustomizedUsageMsg)
{
    pGeneralBoolean->SetFlagParameters("<bool>");
    pGeneralBoolean->SetFlagDescription("pGeneralBoolean description");
    pRange3Int->SetMax(1, INT_MAX);
    pRange3Int->SetMin(0, INT_MIN);
    pRange3Int->SetFlagDescription("pRange3Int description");
    pRange3Float->SetMax(1, FLT_MAX);
    pRange3Float->SetMin(0, FLT_MIN);
    pRange3Float->SetFlagDescription("pRange3Float description");

    std::string usageMsg = R"(
Flags:
--general_boolean <bool>
                    (Default: true)
                    pGeneralBoolean description

--general_boolean_list
                    (Default: true,true,true)

--range_1_int <-10 ~ 10>
                    (Default: 0)

--range_3_int <MIN,-10,-10 ~ 10,MAX,10>
                    (Default: 1,2,3)
                    pRange3Int description

--range_3_float <MIN,-10,-10 ~ 10,MAX,10>
                    (Default: 1.5,2.5,3.5)
                    pRange3Float description

--option_int <c2|"c3 and more">
                    (Default: c2)

--option_string <c2|"c3 and more">
                    (Default: c2)

)";
    EXPECT_EQ(km.GetUsageMsg(), usageMsg);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_SaveAll)
{
    km.FinalizeAll();

    OptionsNew wantOptions = OptionsNew(
        {{"general_boolean", std::vector<std::string>{"true"}},
         {"general_boolean_list", std::vector<std::string>{"true", "true", "true"}},
         {"range_1_int", std::vector<std::string>{"0"}},
         {"range_3_int", std::vector<std::string>{"1,2,3"}},
         {"range_3_float", std::vector<std::string>{"1.5,2.5,3.5"}},
         {"option_int", std::vector<std::string>{"c2"}},
         {"option_string", std::vector<std::string>{"c2"}}});
    OptionsNew gotOptions;
    km.Save(gotOptions);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_SaveNonStartupOnly)
{
    pGeneralBoolean->SetStartupOnly();
    pRange3Float->SetStartupOnly();
    km.FinalizeAll();

    OptionsNew wantOptions = OptionsNew(
        {{"general_boolean_list", std::vector<std::string>{"true", "true", "true"}},
         {"range_1_int", std::vector<std::string>{"0"}},
         {"range_3_int", std::vector<std::string>{"1,2,3"}},
         {"option_int", std::vector<std::string>{"c2"}},
         {"option_string", std::vector<std::string>{"c2"}}});
    OptionsNew gotOptions;
    km.Save(gotOptions, true);
    EXPECT_EQ(gotOptions, wantOptions);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_Load)
{
    km.FinalizeAll();

    OptionsNew loadOptions = OptionsNew(
        {{"general_boolean", std::vector<std::string>{"false"}},
         {"general_boolean_list", std::vector<std::string>{"true", "true"}},
         {"range_1_int", std::vector<std::string>{"5"}},
         {"range_3_int", std::vector<std::string>{"1,2,3"}},
         {"range_3_float", std::vector<std::string>{"5.0,5.0,5.0"}},
         {"option_int", std::vector<std::string>{"c3 and more"}},
         {"option_string", std::vector<std::string>{"c3 and more"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pGeneralBoolean->GetValue(), false);
    EXPECT_EQ(pGeneralBooleanList->GetValue().size(), 2);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(0), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(1), true);
    EXPECT_EQ(pRange1Int->GetValue(), 5);
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pRange3Int->GetValue(1), 2);
    EXPECT_EQ(pRange3Int->GetValue(2), 3);
    EXPECT_EQ(pRange3Float->GetValue(0), 5.0f);
    EXPECT_EQ(pRange3Float->GetValue(1), 5.0f);
    EXPECT_EQ(pRange3Float->GetValue(2), 5.0f);
    EXPECT_EQ(pOptionInt->GetIndex(), 2);
    EXPECT_EQ(pOptionInt->GetValue(), 3);
    EXPECT_EQ(pOptionString->GetIndex(), 2);
    EXPECT_EQ(pOptionString->GetValue(), "c3 and more");
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadBeforeFinalize)
{
    OptionsNew loadOptions = OptionsNew(
        {{"general_boolean", std::vector<std::string>{"false"}},
         {"general_boolean_list", std::vector<std::string>{"true", "true"}},
         {"range_1_int", std::vector<std::string>{"5"}},
         {"range_3_int", std::vector<std::string>{"1,2,3"}},
         {"range_3_float", std::vector<std::string>{"5.0,5.0,5.0"}},
         {"option_int", std::vector<std::string>{"c3 and more"}},
         {"option_string", std::vector<std::string>{"c3 and more"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pGeneralBoolean->GetValue(), false);
    EXPECT_EQ(pGeneralBooleanList->GetValue().size(), 2);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(0), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(1), true);
    EXPECT_EQ(pRange1Int->GetValue(), 5);
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pRange3Int->GetValue(1), 2);
    EXPECT_EQ(pRange3Int->GetValue(2), 3);
    EXPECT_EQ(pRange3Float->GetValue(0), 5.0f);
    EXPECT_EQ(pRange3Float->GetValue(1), 5.0f);
    EXPECT_EQ(pRange3Float->GetValue(2), 5.0f);
    EXPECT_EQ(pOptionInt->GetIndex(), 2);
    EXPECT_EQ(pOptionInt->GetValue(), 3);
    EXPECT_EQ(pOptionString->GetIndex(), 2);
    EXPECT_EQ(pOptionString->GetValue(), "c3 and more");
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadNonStartupOnly)
{
    pGeneralBoolean->SetStartupOnly();
    pRange3Float->SetStartupOnly();
    km.FinalizeAll();

    OptionsNew loadOptions = OptionsNew(
        {{"general_boolean", std::vector<std::string>{"false"}},
         {"general_boolean_list", std::vector<std::string>{"true", "true"}},
         {"range_1_int", std::vector<std::string>{"5"}},
         {"range_3_int", std::vector<std::string>{"1,2,3"}},
         {"range_3_float", std::vector<std::string>{"5.0,5.0,5.0"}},
         {"option_int", std::vector<std::string>{"c3 and more"}},
         {"option_string", std::vector<std::string>{"c3 and more"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pGeneralBoolean->GetValue(), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().size(), 2);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(0), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(1), true);
    EXPECT_EQ(pRange1Int->GetValue(), 5);
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pRange3Int->GetValue(1), 2);
    EXPECT_EQ(pRange3Int->GetValue(2), 3);
    EXPECT_EQ(pRange3Float->GetValue(0), 1.5f);
    EXPECT_EQ(pRange3Float->GetValue(1), 2.5f);
    EXPECT_EQ(pRange3Float->GetValue(2), 3.5f);
    EXPECT_EQ(pOptionInt->GetIndex(), 2);
    EXPECT_EQ(pOptionInt->GetValue(), 3);
    EXPECT_EQ(pOptionString->GetIndex(), 2);
    EXPECT_EQ(pOptionString->GetValue(), "c3 and more");
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadGeneralKnobInvalid)
{
    OptionsNew loadOptions = OptionsNew(
        {{"general_boolean_list", std::vector<std::string>{"false", "false", "false"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pGeneralBooleanList->GetValue().size(), 3);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(0), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(1), true);
    EXPECT_EQ(pGeneralBooleanList->GetValue().at(2), true);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadRangeKnobValidDelimiters)
{
    OptionsNew loadOptions = OptionsNew(
        {{"range_3_int", std::vector<std::string>{"-4X3X-10"}},
         {"range_3_float", std::vector<std::string>{"1.0X-3.0X4.0"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pRange3Int->GetValue(0), -4);
    EXPECT_EQ(pRange3Int->GetValue(1), 3);
    EXPECT_EQ(pRange3Int->GetValue(2), -10);
    EXPECT_EQ(pRange3Float->GetValue(0), 1.0f);
    EXPECT_EQ(pRange3Float->GetValue(1), -3.0f);
    EXPECT_EQ(pRange3Float->GetValue(2), 4.0f);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadRangeKnobInvalid)
{
    km.FinalizeAll();

    OptionsNew loadOptions = OptionsNew(
        {{"range_3_int", std::vector<std::string>{"-4X3XX-10"}},
         {"range_3_float", std::vector<std::string>{"1.0X-3.0,4.0"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pRange3Int->GetValue(0), 1);
    EXPECT_EQ(pRange3Int->GetValue(1), 2);
    EXPECT_EQ(pRange3Int->GetValue(2), 3);
    EXPECT_EQ(pRange3Float->GetValue(0), 1.5f);
    EXPECT_EQ(pRange3Float->GetValue(1), 2.5f);
    EXPECT_EQ(pRange3Float->GetValue(2), 3.5f);
}

TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadOptionKnobInvalid)
{
    km.FinalizeAll();

    OptionsNew loadOptions = OptionsNew(
        {{"option_int", std::vector<std::string>{"c1"}},
         {"option_string", std::vector<std::string>{"anything"}}});
    km.Load(loadOptions);
    EXPECT_EQ(pOptionInt->GetIndex(), 1);
    EXPECT_EQ(pOptionString->GetIndex(), 1);
}

#if defined(PERFORM_DEATH_TESTS)
TEST_F(KnobManagerNewWithKnobsTestFixture, KnobManagerNew_LoadUnknownKnobFail)
{
    km.FinalizeAll();

    OptionsNew loadOptions = OptionsNew(
        {{"UNKNOWN", std::vector<std::string>{"hello world"}}});
    EXPECT_DEATH(
        {
            km.Load(loadOptions);
        },
        "");
}
#endif

} // namespace ppx
