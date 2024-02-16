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

#ifndef ppx_knob_new_h
#define ppx_knob_new_h

#include "ppx/options_new.h"
#include "ppx/config.h"
#include "ppx/imgui_impl.h"
#include "ppx/log.h"
#include "ppx/string_util.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace ppx {

// Knobs represent parameters that can be adjusted during the application runtime.
//
// Defining and registering a knob with the application's KnobManagerNew will create a parameter
// whose starting value is determined by (from high priority -> low):
// - A specified command-line flag
// - A value specified in a config file
// - The default value provided when the knob is created
//
// The startup value is saved when the KnobManagerNew finalizes the knobs.
//
// Then, while the application is running:
// - Users can manually adjust the knob through the UI
// - The application can access the knob's values through the knob getters and setters
// - JSON config files can be saved and loaded

enum class KnobDisplayType
{
    PLAIN,
    CHECKBOX,
    FAST_SLIDER,
    SLOW_SLIDER,
    DROPDOWN,
    COLOR
};

// ---------------------------------------------------------------------------------------------
// KnobNew Classes
// ---------------------------------------------------------------------------------------------

// KnobNew is an abstract class which contains common features for all knobs and knob hierarchy
class KnobNew
{
public:
    KnobNew(const std::string& flagName)
        : mDisplayIndent(0),
          mDisplayName(flagName),
          mDisplayType(KnobDisplayType::PLAIN),
          mDisplayVisible(true),
          mFinalized(false),
          mFlagName(flagName),
          mStartupDisplayVisible(false),
          mStartupOnly(false),
          mUpdatedFlag(false) {}
    virtual ~KnobNew() = default;

    // Customize knob functionality
    void SetStartupOnly();

    // Customize flag usage message
    void SetFlagDescription(const std::string& flagDescription) { mFlagDescription = flagDescription; }
    void SetFlagParameters(const std::string& flagParameters) { mFlagParameters = flagParameters; }

    // Customize how knob is drawn in the UI
    void SetDisplayType(KnobDisplayType newDisplayType) { mDisplayType = newDisplayType; }
    void SetDisplayName(const std::string& displayName) { mDisplayName = displayName; }
    void SetIndent(size_t indent) { mDisplayIndent = indent; }
    void SetVisible(bool visible) { mDisplayVisible = visible; }

    // Returns true if there has been an update to the knob value
    bool DigestUpdate();

protected:
    void RaiseUpdatedFlag() { mUpdatedFlag = true; }

    // Helper functions for drawing in UI
    void DrawPlain();
    void DrawToolTip();

private:
    // Called from KnobManagerNew
    virtual void        Draw()        = 0;
    virtual std::string ValueString() = 0;
    void                Finalize();
    void                ResetToStartup();
    std::string         GetFlagParameters();

    // Called by the ones above
    virtual void        FinalizeValues()           = 0;
    virtual void        ResetValuesToStartup()     = 0;
    virtual std::string GetDefaultFlagParameters() = 0;

    // Convert list of strings to knob values and vice versa
    virtual void                     Load(const std::vector<std::string>& valueStrings) = 0;
    virtual std::vector<std::string> Save()                                             = 0;

protected:
    std::string mFlagName;

    // Parameters for how the knob will be displayed in the UI
    std::string     mDisplayName;
    KnobDisplayType mDisplayType;

    // If true, startup values have been stored and can no longer be changed
    bool mFinalized;

    // If true, once finalized this knob cannot have its value changed
    bool mStartupOnly;

private:
    // Boolean flag tracking when the knob value has been updated
    bool mUpdatedFlag;

    // Parameters for how the knob is described in the usage message
    std::string mFlagParameters;
    std::string mFlagDescription; // also used in the tooltip

    // Parameters for how the knob will be displayed in the UI
    size_t          mDisplayIndent;
    bool            mDisplayVisible;
    bool            mStartupDisplayVisible;
    KnobDisplayType mStartupDisplayType;

private:
    friend class KnobManagerNew;
};

// GeneralKnob can hold varied types and can have a user-specified validator function.
template <typename T>
class GeneralKnob final
    : public KnobNew
{
public:
    GeneralKnob(const std::string& flagName, T defaultValue)
        : KnobNew(flagName)
    {
        PPX_ASSERT_MSG(IsValidValue(defaultValue), "GeneralKnob " + mFlagName + " cannot be created with value " + ppx::string_util::ToString(defaultValue));
        SetValue(defaultValue);
    }

    T GetValue() const { return mValue; }

    void SetValue(T newValue)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("GeneralKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        if (!IsValidValue(newValue)) {
            PPX_LOG_ERROR("GeneralKnob " + mFlagName + " cannot be set to value " + ppx::string_util::ToString(newValue));
            return;
        }
        if (mValue == newValue) {
            return;
        }
        mValue = newValue;
        RaiseUpdatedFlag();
    }

    void SetValidator(std::function<bool(T)> validatorFunc)
    {
        PPX_ASSERT_MSG(!mFinalized, "GeneralKnob " + mFlagName + " cannot have a validator set since it is finalized");
        PPX_ASSERT_MSG(validatorFunc(mValue), "GeneralKnob " + mFlagName + " cannot have a validator set that makes the current value invalid");

        mValidatorFunc = validatorFunc;
    }

private:
    void Draw() override
    {
        switch (mDisplayType) {
            case KnobDisplayType::PLAIN: {
                DrawPlain();
                return;
            }
            case KnobDisplayType::CHECKBOX: {
                if constexpr (std::is_same_v<T, bool>) {
                    bool displayValue = mValue;
                    bool interacted   = ImGui::Checkbox(mDisplayName.c_str(), &displayValue);
                    DrawToolTip();
                    if (!interacted) {
                        return;
                    }
                    SetValue(displayValue);
                    return;
                }
                PPX_ASSERT_MSG(false, "GeneralKnob " << mFlagName << " is incompatable with display type CHECKBOX");
                return;
            }
        }
        PPX_ASSERT_MSG(false, "GeneralKnob " << mFlagName << " does not support display type " << static_cast<int>(mDisplayType));
        return;
    }

    void FinalizeValues() override
    {
        mStartupValue = mValue;
    }

    void ResetValuesToStartup() override
    {
        mValue = mStartupValue;
        RaiseUpdatedFlag();
    }

    std::string GetDefaultFlagParameters() override
    {
        return ""; // Empty parameters for GeneralKnob
    }

    void Load(const std::vector<std::string>& valueStrings) override
    {
        LoadValue(mValue, valueStrings);
    }

    std::vector<std::string> Save() override
    {
        return SaveValue(mValue);
    }

    std::string ValueString() override
    {
        return ppx::string_util::ToString(mValue);
    }

    bool IsValidValue(T val)
    {
        if (!mValidatorFunc) {
            return true;
        }
        return mValidatorFunc(val);
    }

    // Tries to parse the option string into the type of the current value and load it into mValue.
    template <typename U>
    void LoadValue(const U& currentValue, const std::vector<std::string>& valueStrings)
    {
        // The knob value is not a vector, interpret from only the last value string
        auto valueStr = valueStrings.back();
        U    parsedValue;
        auto res = ppx::string_util::Parse(valueStr, parsedValue);
        if (Failed(res)) {
            PPX_LOG_ERROR("GeneralKnob " << mFlagName << " could not be loaded with string " << valueStr);
            return;
        }
        SetValue(parsedValue);
    }

    // Same as above, but intended for GeneralKnob of vector type.
    template <typename U>
    void LoadValue(const std::vector<U>& currentValues, const std::vector<std::string>& valueStrings)
    {
        // The knob value is a vector, interpret each value string as an element in the vector
        std::vector<U> parsedValues;
        for (size_t i = 0; i < valueStrings.size(); ++i) {
            U    parsedValue{};
            auto res = ppx::string_util::Parse(valueStrings.at(i), parsedValue);
            if (Failed(res)) {
                PPX_LOG_ERROR("GeneralKnob " << mFlagName << " element could not be loaded with string " << valueStrings.at(i));
                return;
            }
            parsedValues.emplace_back(parsedValue);
        }
        SetValue(parsedValues);
    }

    // Returns the current value (non-list) as a vector of strings with one element.
    template <typename U>
    std::vector<std::string> SaveValue(const U& currentValue)
    {
        std::vector<std::string> valueStrings = {};
        valueStrings.emplace_back(ppx::string_util::ToString(currentValue));
        return valueStrings;
    }

    // Same as above, but intended for GeneralKnob of vector type
    // Returns vector of strings with the same number of elements as the current value.
    template <typename U>
    std::vector<std::string> SaveValue(const std::vector<U>& currentValues)
    {
        std::vector<std::string> valueStrings = {};
        for (const auto& currentValue : currentValues) {
            valueStrings.emplace_back(ppx::string_util::ToString(currentValue));
        }
        return valueStrings;
    }

private:
    T mValue;
    T mStartupValue;

    std::function<bool(T)> mValidatorFunc;
};

// RangeKnob can hold arithmetic types and restricts the value to be between min~max range, inclusive.
template <typename T>
class RangeKnob final
    : public KnobNew
{
public:
    template <typename Iter>
    RangeKnob(const std::string& flagName, Iter defaultsBegin, Iter defaultsEnd)
        : KnobNew(flagName)
    {
        size_t                   i = 0;
        std::vector<std::string> displaySuffixes;
        for (auto iter = defaultsBegin; iter != defaultsEnd; ++iter) {
            mMaxValues.push_back(std::numeric_limits<T>::max());
            mMinValues.push_back(std::numeric_limits<T>::min());
            PPX_ASSERT_MSG(IsValidValue(i, *iter), "RangeKnob " << mFlagName << " cannot be created with value " << *iter << " at position " << i);
            mValues.push_back(*iter);
            displaySuffixes.push_back(ppx::string_util::ToString(i));
            i++;
        }
        mDisplayValues = mValues;
        SetDisplaySuffixes(displaySuffixes);
        RaiseUpdatedFlag();
    }

    RangeKnob(const std::string& flagName, T defaultValue)
        : RangeKnob(flagName, std::vector<T>{defaultValue}) {}

    RangeKnob(const std::string& flagName, std::vector<T> defaultValues)
        : RangeKnob(flagName, std::begin(defaultValues), std::end(defaultValues)) {}

    T GetValue(size_t i) const
    {
        PPX_ASSERT_MSG((i >= 0) && (i < mValues.size()), "RangeKnob " << mFlagName << " value cannot be accessed at position " << i);
        return mValues[i];
    }

    void SetValue(size_t i, T newValue)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("RangeKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        PPX_ASSERT_MSG((i >= 0) && (i < mValues.size()) && (i < mDisplayValues.size()), "RangeKnob " << mFlagName << " value cannot be accessed at position " << i);
        if (!IsValidValue(i, newValue)) {
            LogRange(i);
            PPX_LOG_ERROR("RangeKnob " + mFlagName + " position " + ppx::string_util::ToString(i) + " cannot be set to value " + ppx::string_util::ToString(newValue));
            return;
        }
        if (mValues[i] == newValue) {
            return;
        }
        mValues[i]        = newValue;
        mDisplayValues[i] = newValue;
        RaiseUpdatedFlag();
    }

    void SetMax(size_t i, T newMaxValue)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("RangeKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        PPX_ASSERT_MSG((i >= 0) && (i < mMaxValues.size()), "RangeKnob " << mFlagName << " max cannot be accessed at position " << i);
        PPX_ASSERT_MSG(newMaxValue >= mMinValues[i], "RangeKnob " << mFlagName << " max cannot be smaller than min at position " << i);
        mMaxValues[i] = newMaxValue;
        if (mValues[i] > newMaxValue) {
            SetValue(i, newMaxValue);
        }
    }

    void SetMin(size_t i, T newMinValue)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("RangeKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        PPX_ASSERT_MSG((i >= 0) && (i < mMinValues.size()), "RangeKnob " << mFlagName << " min cannot be accessed at position " << i);
        PPX_ASSERT_MSG(newMinValue <= mMaxValues[i], "RangeKnob " << mFlagName << " min cannot be larger than max at position " << i);
        mMinValues[i] = newMinValue;
        if (mValues[i] < newMinValue) {
            SetValue(i, newMinValue);
        }
    }

    // For N=1 case
    T GetValue() const
    {
        PPX_ASSERT_MSG(mValues.size() == 1, "specify index when RangeKnob N>1");
        return GetValue(0);
    }
    void SetValue(T newValue)
    {
        PPX_ASSERT_MSG(mValues.size() == 1, "specify index when RangeKnob N>1");
        SetValue(0, newValue);
    }
    void SetMax(T newMaxValue)
    {
        PPX_ASSERT_MSG(mValues.size() == 1, "specify index when RangeKnob N>1");
        SetMax(0, newMaxValue);
    }
    void SetMin(T newMaxValue)
    {
        PPX_ASSERT_MSG(mValues.size() == 1, "specify index when RangeKnob N>1");
        SetMin(0, newMaxValue);
    }

    // For N>1 cases
    void SetAllValues(T newValue)
    {
        for (size_t i = 0; i < mValues.size(); i++) {
            SetValue(i, newValue);
        }
    }
    void SetAllMaxes(T newMaxValue)
    {
        for (size_t i = 0; i < mMaxValues.size(); i++) {
            SetMax(i, newMaxValue);
        }
    }
    void SetAllMins(T newMinValue)
    {
        for (size_t i = 0; i < mMinValues.size(); i++) {
            SetMin(i, newMinValue);
        }
    }

    void SetDisplaySuffixes(const std::vector<std::string>& newSuffixes)
    {
        PPX_ASSERT_MSG(newSuffixes.size() == mValues.size(), "RangeKnob " << mFlagName << " must have " << mValues.size() << " suffixes set");
        mDisplaySuffixes = newSuffixes;
    }

private:
    void FinalizeValues() override
    {
        mStartupValues    = mValues;
        mStartupMinValues = mMinValues;
        mStartupMaxValues = mMaxValues;

        mDisplayValues = mValues;
    }

    void ResetValuesToStartup() override
    {
        mValues    = mStartupValues;
        mMinValues = mStartupMinValues;
        mMaxValues = mStartupMaxValues;

        mDisplayValues = mValues;
        RaiseUpdatedFlag();
    }

    std::string GetDefaultFlagParameters() override
    {
        std::vector<std::string> minStrings;
        for (const auto& min : mMinValues) {
            if (min == std::numeric_limits<T>::min()) {
                minStrings.push_back("MIN");
                continue;
            }
            minStrings.push_back(ppx::string_util::ToString(min));
        }
        std::vector<std::string> maxStrings;
        for (const auto& max : mMaxValues) {
            if (max == std::numeric_limits<T>::max()) {
                maxStrings.push_back("MAX");
                continue;
            }
            maxStrings.push_back(ppx::string_util::ToString(max));
        }
        return "<" + ppx::string_util::ToString(minStrings) + " ~ " + ppx::string_util::ToString(maxStrings) + ">";
    }

    void Draw() override
    {
        switch (mDisplayType) {
            case KnobDisplayType::PLAIN: {
                DrawPlain();
                return;
            }
            case KnobDisplayType::SLOW_SLIDER: {
                if constexpr (std::is_same_v<T, int>) {
                    for (size_t i = 0; i < mValues.size(); i++) {
                        std::string displayName = mDisplayName + " " + mDisplaySuffixes.at(i);
                        if (mValues.size() == 1) {
                            displayName = mDisplayName;
                        }
                        ImGui::SliderInt(displayName.c_str(), &mDisplayValues[i], mMinValues[i], mMaxValues[i], NULL, ImGuiSliderFlags_AlwaysClamp);
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            SetValue(i, mDisplayValues[i]);
                        }
                        DrawToolTip();
                    }
                }
                else if constexpr (std::is_same_v<T, float>) {
                    for (size_t i = 0; i < mValues.size(); i++) {
                        std::string displayName = mDisplayName + " " + mDisplaySuffixes.at(i);
                        if (mValues.size() == 1) {
                            displayName = mDisplayName;
                        }
                        ImGui::SliderFloat(displayName.c_str(), &mDisplayValues[i], mMinValues[i], mMaxValues[i], "%.3f", ImGuiSliderFlags_AlwaysClamp);
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            SetValue(i, mDisplayValues[i]);
                        }
                        DrawToolTip();
                    }
                }
                else {
                    PPX_ASSERT_MSG(false, "RangeKnob " << mFlagName << " is incompatible with display type SLOW_SLIDER");
                }
                return;
            }
            case KnobDisplayType::FAST_SLIDER: {
                if constexpr (std::is_same_v<T, int>) {
                    for (size_t i = 0; i < mValues.size(); i++) {
                        std::string displayName = mDisplayName + " " + mDisplaySuffixes.at(i);
                        if (mValues.size() == 1) {
                            displayName = mDisplayName;
                        }
                        if (ImGui::SliderInt(displayName.c_str(), &mValues[i], mMinValues[i], mMaxValues[i], NULL, ImGuiSliderFlags_AlwaysClamp)) {
                            RaiseUpdatedFlag();
                        }
                        DrawToolTip();
                    }
                }
                else if constexpr (std::is_same_v<T, float>) {
                    for (size_t i = 0; i < mValues.size(); i++) {
                        std::string displayName = mDisplayName + " " + mDisplaySuffixes.at(i);
                        if (mValues.size() == 1) {
                            displayName = mDisplayName;
                        }
                        if (ImGui::SliderFloat(displayName.c_str(), &mValues[i], mMinValues[i], mMaxValues[i], "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                            RaiseUpdatedFlag();
                        }
                        DrawToolTip();
                    }
                }
                else {
                    PPX_ASSERT_MSG(false, "RangeKnob " << mFlagName << " is incompatible with display type FAST_SLIDER");
                }
                return;
            }
        }
        PPX_ASSERT_MSG(false, "RangeKnob " << mFlagName << " does not support display type " << static_cast<int>(mDisplayType));
        return;
    }

    void Load(const std::vector<std::string>& valueStrings) override
    {
        // Only use the last value string
        std::string valueString = valueStrings.back();

        // Interpret the first non-numerical character as a delimeter
        // [0-9] and '.', '-' are all considered numerical
        char delimiter = ',';
        for (auto& c : valueString) {
            if (!std::isdigit(c) && c != '.' && c != '-') {
                delimiter = c;
                break;
            }
        }

        auto splitValues = ppx::string_util::Split(valueString, delimiter);
        if (splitValues.size() != mValues.size()) {
            PPX_LOG_ERROR("RangeKnob " << mFlagName << " could not be loaded with string " << valueString);
            return;
        }

        std::vector<T> parsedValues;
        for (size_t i = 0; i < splitValues.size(); ++i) {
            T    parsedValue;
            auto res = ppx::string_util::Parse(splitValues.at(i), parsedValue);
            if (Failed(res)) {
                PPX_LOG_ERROR("RangeKnob " << mFlagName << " element could not be loaded with string " << splitValues.at(i));
                return;
            }
            parsedValues.emplace_back(parsedValue);
        }

        for (size_t i = 0; i < parsedValues.size(); ++i) {
            SetValue(i, parsedValues.at(i));
        }
    }

    std::vector<std::string> Save() override
    {
        std::vector<std::string> valueStrings;
        // single comma-separated string
        valueStrings.emplace_back(ValueString());
        return valueStrings;
    }

    std::string ValueString() override
    {
        return ppx::string_util::ToString(mValues);
    }

    bool IsValidValue(size_t i, T val)
    {
        PPX_ASSERT_MSG((i >= 0) && (i < mMinValues.size()) && (i < mMaxValues.size()), "RangeKnob " << mFlagName << " index out of range: " << i);

        if ((val >= mMinValues[i]) && (val <= mMaxValues[i])) {
            return true;
        }
        return false;
    }

    void LogRange(size_t i)
    {
        PPX_LOG_INFO("RangeKnob " + mFlagName + " at position " + ppx::string_util::ToString(i) + " has range " + ppx::string_util::ToString(mMinValues[i]) + "~" + ppx::string_util::ToString(mMaxValues[i]));
    }

private:
    std::vector<T>           mValues;
    std::vector<T>           mStartupValues;
    std::vector<T>           mMinValues;
    std::vector<T>           mStartupMinValues;
    std::vector<T>           mMaxValues;
    std::vector<T>           mStartupMaxValues;
    std::vector<T>           mDisplayValues;
    std::vector<std::string> mDisplaySuffixes;
};

// OptionKnobEntry is used to supply choices to OptionKnob
template <typename T, typename NameT = const char*>
struct OptionKnobEntry
{
    NameT name;
    T     value;
};

// OptionKnob stores the index of a selected choice, a list of allowed options, and a mask of which options are valid
template <typename T>
class OptionKnob final
    : public KnobNew
{
private:
    using Entry = OptionKnobEntry<T, std::string>;
    Entry ConvertToEntry(const T& value)
    {
        using string_util::ToString;
        std::string name;
        if constexpr (std::is_same_v<T, std::string>) {
            name = value;
        }
        else {
            name = ToString(value);
        }
        return {std::move(name), value};
    }
    template <typename NameT>
    Entry ConvertToEntry(const OptionKnobEntry<T, NameT>& entry)
    {
        return {entry.name, entry.value};
    }

public:
    template <typename Iter>
    OptionKnob(
        const std::string& flagName,
        size_t             defaultIndex,
        Iter               choicesBegin,
        Iter               choicesEnd)
        : KnobNew(flagName), mIndex(defaultIndex), mStartupIndex(defaultIndex)
    {
        for (auto iter = choicesBegin; iter != choicesEnd; ++iter) {
            mChoices.push_back(ConvertToEntry(*iter));
            mMask.push_back(true);
        }
        PPX_ASSERT_MSG(defaultIndex < mChoices.size(), "defaultIndex is out of range");
        RaiseUpdatedFlag();
    }

    template <typename Container>
    OptionKnob(
        const std::string& flagName,
        size_t             defaultIndex,
        const Container&   container)
        : OptionKnob(flagName, defaultIndex, std::begin(container), std::end(container)) {}

    size_t   GetIndex() const { return mIndex; }
    const T& GetValue() const { return mChoices[mIndex].value; }

    void SetIndex(size_t newIndex)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("OptionKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        if (!IsValidIndex(newIndex)) {
            PPX_LOG_ERROR(mFlagName << " does not have this index in allowed choices: " << newIndex);
            return;
        }
        if (newIndex == mIndex) {
            return;
        }
        mIndex = newIndex;
        RaiseUpdatedFlag();
    }

    void SetMask(size_t i, bool newValue)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("OptionKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        PPX_ASSERT_MSG(i < mMask.size(), "OptionKnob " + mFlagName + " mask index out of range " + ppx::string_util::ToString(i));
        mMask[i] = newValue;

        if ((i == mIndex) && (!newValue)) {
            SetIndex(GetFirstValidIndex());
        }
    }

    void SetMask(const std::vector<bool>& newMask)
    {
        if (mFinalized && mStartupOnly) {
            PPX_LOG_ERROR("OptionKnob " + mFlagName + " is startup-only and cannot be set after finalization");
            return;
        }
        PPX_ASSERT_MSG(newMask.size() == mMask.size(), "OptionKnob " + mFlagName + " new mask must be same size");
        mMask = newMask;

        if (!mMask[mIndex]) {
            SetIndex(GetFirstValidIndex());
        }
    }

private:
    void FinalizeValues() override
    {
        mStartupIndex = mIndex;
        mStartupMask  = mMask;
    }

    void ResetValuesToStartup() override
    {
        mIndex = mStartupIndex;
        mMask  = mStartupMask;
        RaiseUpdatedFlag();
    }

    std::string GetDefaultFlagParameters() override
    {
        std::string choiceStr = "";
        for (size_t i = 0; i < mChoices.size(); i++) {
            if (mMask[i]) {
                auto choice   = mChoices[i];
                bool hasSpace = choice.name.find_first_of("\t ") != std::string::npos;
                if (hasSpace) {
                    choiceStr += "\"" + choice.name + "\"|";
                }
                else {
                    choiceStr += choice.name + "|";
                }
            }
        }
        if (!choiceStr.empty()) {
            choiceStr.pop_back();
        }
        choiceStr = "<" + choiceStr + ">";
        return choiceStr;
    }

    void Draw() override
    {
        switch (mDisplayType) {
            case KnobDisplayType::PLAIN: {
                DrawPlain();
                return;
            }
            case KnobDisplayType::DROPDOWN: {
                bool interacted = ImGui::BeginCombo(mDisplayName.c_str(), mChoices.at(mIndex).name.c_str());
                if (!interacted) {
                    DrawToolTip(); // Cannot display tooltip when Combo is open
                    return;
                }
                for (size_t i = 0; i < mChoices.size(); ++i) {
                    if (!mMask.at(i)) {
                        continue;
                    }
                    bool isSelected = (i == mIndex);
                    if (ImGui::Selectable(mChoices.at(i).name.c_str(), isSelected)) {
                        if (i != mIndex) { // A new choice is selected
                            mIndex = i;
                            RaiseUpdatedFlag();
                        }
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
                return;
            }
        }
        PPX_ASSERT_MSG(false, "OptionKnob " << mFlagName << " does not support display type " << static_cast<int>(mDisplayType));
        return;
    }

    void Load(const std::vector<std::string>& valueStrings) override
    {
        // Only use the last value string
        std::string valueString = valueStrings.back();

        auto temp = std::find_if(
            mChoices.cbegin(),
            mChoices.cend(),
            [&valueString](const Entry& entry) {
                return entry.name == valueString;
            });
        if (temp == mChoices.cend()) {
            PPX_LOG_ERROR("OptionKnob " + mFlagName + " could not be loaded with name: " + valueString);
        }
        size_t newIndex = std::distance(mChoices.cbegin(), temp);

        SetIndex(newIndex);
    }

    std::vector<std::string> Save() override
    {
        std::vector<std::string> valueStrings;
        valueStrings.emplace_back(ValueString());
        return valueStrings;
    }

    std::string ValueString() override
    {
        return mChoices[mIndex].name;
    }

    bool IsValidIndex(size_t index)
    {
        if (index >= mMask.size()) {
            return false;
        }
        return mMask[index];
    }

    size_t GetFirstValidIndex()
    {
        auto it = std::find(mMask.begin(), mMask.end(), true);
        PPX_ASSERT_MSG(it != mMask.end(), "OptionKnob " << mFlagName << " no longer has any valid options");
        return it - mMask.begin();
    }

private:
    // mIndex indicates which of the mChoices is selected
    size_t             mIndex;
    size_t             mStartupIndex;
    std::vector<Entry> mChoices;
    std::vector<bool>  mMask;
    std::vector<bool>  mStartupMask;
};

// KnobManagerNew holds the knobs in an application
class KnobManagerNew
{
public:
    KnobManagerNew()                      = default;
    KnobManagerNew(const KnobManagerNew&) = delete;
    KnobManagerNew& operator=(const KnobManagerNew&) = delete;

private:
    // Knobs are added on creation and never removed
    std::vector<std::shared_ptr<KnobNew>> mKnobs;

    // mFlagNames is kept to prevent multiple knobs having the same mFlagName
    std::unordered_set<std::string> mFlagNames;

public:
    bool IsEmpty() { return mKnobs.empty(); }

    // This will mark all the current knobs as startup-only
    // Called after the Standard knobs are registered
    void SetAllStartupOnly();

    // Save startup states and prevent registration of new knobs, can only be called once
    void FinalizeAll();

    // The knobs can be reset to startup values
    void ResetAllToStartup();

    // Initialize the knob in-place and register it with the knob manager
    template <typename T, typename... ArgsT>
    void InitKnob(std::shared_ptr<T>* ppKnob, const std::string& flagName, ArgsT&&... args)
    {
        PPX_ASSERT_MSG(ppKnob != nullptr, "output parameter is nullptr");
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists: " + flagName);

        std::shared_ptr<T> knobPtr(new T(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr);

        *ppKnob = knobPtr;
    }

    void        DrawAllKnobs(bool inExistingWindow = false);
    std::string GetUsageMsg();

    void Load(const OptionsNew& opts);
    void Save(OptionsNew& opts, bool excludeStartupOnly = false);

private:
    void RegisterKnob(const std::string& flagName, std::shared_ptr<KnobNew> newKnob);

private:
    bool mFinalized           = false;
    char mConfigFilePath[128] = "";

    std::string mUsageMsg = R"(
USAGE
==============================
Boolean options can be turned on with:
  --flag-name true, --flag-name 1, --flag-name
And turned off with:
  --flag-name false, --flag-name 0, --no-flag-name

--help : Prints this help message and exits.
==============================
)";
};

} // namespace ppx

#endif // ppx_knob_new_h
