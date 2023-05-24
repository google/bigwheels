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

#ifndef ppx_knob_h
#define ppx_knob_h

#include "ppx/command_line_parser.h"
#include "ppx/config.h"
#include "ppx/imgui_impl.h"
#include "ppx/log.h"

#include <iostream>
#include <unordered_set>
#include <vector>

namespace ppx {

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

// Knob is an abstract class which contains common features for all knobs and knob hierarchy
class Knob
{
public:
    Knob(const std::string& flagName)
        : mFlagName(flagName), mDisplayName(flagName), mIndent(0) {}
    virtual ~Knob() = default;

    const std::string& GetDisplayName() const { return mDisplayName; }
    const std::string& GetFlagName() const { return mFlagName; }
    const std::string& GetFlagHelp() const { return mFlagDesc; }
    size_t             GetIndent() const { return mIndent; }

    // Populate flag members
    void SetDisplayName(const std::string& displayName) { mDisplayName = displayName; }
    void SetFlagDesc(const std::string& flagDesc) { mFlagDesc = flagDesc; }
    void SetIndent(size_t indent) { mIndent = indent; }

    virtual void Draw() = 0;

    // Flag description in usage message
    virtual std::string GetFlagHelpText() const = 0;

    // Updates knob value from commandline flag
    virtual void UpdateFromFlags(const CliOptions& opts) = 0;

    // Default values are the values when the application starts. The knobs can be reset to default by a button in the UI
    virtual void ResetToDefault() = 0;

protected:
    std::string mDisplayName;
    std::string mFlagName;
    std::string mFlagDesc;
    size_t      mIndent;
};

// KnobCheckbox will be displayed as a checkbox in the UI
class KnobCheckbox final
    : public Knob
{
public:
    KnobCheckbox(const std::string& flagName, bool defaultValue)
        : Knob(flagName)
    {
        SetDefaultAndValue(defaultValue);
    }

    void Draw() override
    {
        ImGui::Checkbox(mDisplayName.c_str(), &mValue);
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    // GetFlagHelpText format:
    // --flag_name <true|false> : flag description
    std::string GetFlagHelpText() const override
    {
        std::string flagHelpText = "--" + mFlagName + " <true|false>";
        if (!mFlagDesc.empty()) {
            flagHelpText += " : " + mFlagDesc;
        }
        return flagHelpText + "\n";
    }

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <true|false>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(mFlagName, mValue));
    }

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // Updates mValue
    void SetValue(bool newVal)
    {
        mValue = newVal;
    }

private:
    void SetDefaultAndValue(bool newVal)
    {
        mDefaultValue = newVal;
        ResetToDefault();
    }

private:
    bool mValue;
    bool mDefaultValue;
};

// KnobSlider will be displayed as a slider in the UI
// ImGui sliders can also become input boxes with ctrl + right click
template <typename T>
class KnobSlider final
    : public Knob
{
public:
    KnobSlider(const std::string& flagName, T defaultValue, T minValue, T maxValue)
        : Knob(flagName)
    {
        static_assert(std::is_same_v<T, int>, "KnobSlider must be created with type: int");

        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize slider");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");

        mMinValue = minValue;
        mMaxValue = maxValue;
        SetDefaultAndValue(defaultValue);
    }

    void Draw() override
    {
        ImGui::SliderInt(mDisplayName.c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    // GetFlagHelpText format:
    // --flag_name <mMinValue~mMaxValue> : flag description
    std::string GetFlagHelpText() const override
    {
        std::string flagHelpText = "--" + mFlagName + " <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">";
        if (!mFlagDesc.empty()) {
            flagHelpText += " : " + mFlagDesc;
        }
        return flagHelpText + "\n";
    }

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <int>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(mFlagName, mValue));
    }

    T GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // Updates mValue
    void SetValue(T newVal)
    {
        if (!IsValidValue(newVal)) {
            PPX_LOG_ERROR(mFlagName << " cannot be set to " << newVal << " because it's out of range " << mMinValue << "~" << mMaxValue);
            return;
        }

        mValue = newVal;
    }

private:
    // IsValidValue returns true if val is within allowed range
    bool IsValidValue(T val)
    {
        if (val < mMinValue || val > mMaxValue)
            return false;
        return true;
    }

    void SetDefaultAndValue(T newVal)
    {
        PPX_ASSERT_MSG(IsValidValue(newVal), "invalid default value");
        mDefaultValue = newVal;
        ResetToDefault();
    }

private:
    T mValue;
    T mDefaultValue;

    // mValue will be clamped to the mMinValue to mMaxValue range, inclusive
    T mMinValue;
    T mMaxValue;
};

// KnobDropdown will be displayed as a dropdown in the UI
// The knob stores the index of a selected choice from a list of allowed options
template <typename T>
class KnobDropdown final
    : public Knob
{
public:
    KnobDropdown(
        const std::string&                flagName,
        size_t                            defaultIndex,
        typename std::vector<T>::iterator choicesBegin,
        typename std::vector<T>::iterator choicesEnd)
        : Knob(flagName)
    {
        static_assert(std::is_same_v<T, std::string>, "KnobDropdown must be created with type: std::string");

        PPX_ASSERT_MSG(static_cast<int>(defaultIndex) >= 0, "defaultIndex must be non-negative");
        PPX_ASSERT_MSG(static_cast<int>(defaultIndex) < std::distance(choicesBegin, choicesEnd), "defaultIndex is out of range");

        mChoices = std::vector<T>(choicesBegin, choicesEnd);
        SetDefaultAndIndex(defaultIndex);
    }

    void Draw() override
    {
        if (ImGui::BeginCombo(mDisplayName.c_str(), mChoices.at(mIndex).c_str())) {
            for (size_t i = 0; i < mChoices.size(); ++i) {
                bool isSelected = (i == mIndex);
                if (ImGui::Selectable(mChoices.at(i).c_str(), isSelected)) {
                    if (i != mIndex) { // A new choice is selected
                        mIndex = i;
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void ResetToDefault() override { SetIndex(mDefaultIndex); }

    // GetFlagHelpText format:
    // --flag_name <"choice1"|"choice2"|...> : flag description
    std::string GetFlagHelpText() const override
    {
        std::string choiceStr = "";
        for (const auto& choice : mChoices) {
            choiceStr += '\"' + choice + '\"' + "|";
        }
        if (!choiceStr.empty())
            choiceStr.pop_back();

        std::string flagHelpText = "--" + mFlagName + " <" + choiceStr + ">";
        if (!mFlagDesc.empty()) {
            flagHelpText += " : " + mFlagDesc;
        }
        return flagHelpText + "\n";
    }

    // Updates mIndex and mDefaultIndex, expected flag format:
    // --flag_name <str>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndIndex(opts.GetExtraOptionValueOrDefault(mFlagName, GetStr()));
    }

    size_t             GetIndex() const { return mIndex; }
    const std::string& GetStr() const { return mChoices[mIndex]; }

    // Used for when mIndex needs to be updated outside of UI
    // Updates mIndex
    void SetIndex(size_t newI)
    {
        if (!IsValidIndex(newI)) {
            PPX_LOG_ERROR(mFlagName << " does not have this index in allowed choices: " << newI);
            return;
        }

        mIndex = newI;
    }

    // Used for setting from flags
    // Updates mIndex
    void SetIndex(const std::string& newVal)
    {
        auto temp = std::find(mChoices.cbegin(), mChoices.cend(), newVal);
        if (temp == mChoices.cend()) {
            PPX_LOG_ERROR(mFlagName << " does not have this value in allowed range: " << newVal);
            return;
        }

        mIndex = std::distance(mChoices.cbegin(), temp);
    }

private:
    // IsValidIndex returns true if i is within allowed range
    bool IsValidIndex(size_t i)
    {
        if (i >= mChoices.size())
            return false;
        return true;
    }

    void SetDefaultAndIndex(size_t newI)
    {
        PPX_ASSERT_MSG(IsValidIndex(newI), "invalid default index");
        mDefaultIndex = newI;
        ResetToDefault();
    }

    void SetDefaultAndIndex(std::string newVal)
    {
        auto temp = std::find(mChoices.cbegin(), mChoices.cend(), newVal);
        PPX_ASSERT_MSG(temp != mChoices.cend(), "invalid default value");

        mDefaultIndex = std::distance(mChoices.cbegin(), temp);
        ResetToDefault();
    }

private:
    // mIndex indicates which of the mChoices is selected
    size_t         mIndex;
    size_t         mDefaultIndex;
    std::vector<T> mChoices;
};

// KnobManager owns the knobs in an application
class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&) = delete;
    KnobManager& operator=(const KnobManager&) = delete;

private:
    // mKnobs holds all knobs
    // Knobs are added on creation and never deleted until knobManager is destroyed
    std::vector<Knob*> mKnobs;

    // mFlagNames is kept to prevent multiple knobs having the same mFlagName
    std::unordered_set<std::string> mFlagNames;

public:
    // Utilities
    bool IsEmpty() { return mKnobs.empty(); }
    void ResetAllToDefault();

    // Create knobs
    // Examples:
    //   CreateKnob<KnobCheckbox>("flag_name", bool defaultValue);
    //   CreateKnob<KnobSlider>("flag_name", int defaultValue, minValue, maxValue);
    //   CreateKnob<KnobDropdown>("flag_name", size_t defaultIndex, std::vector<std::string>::iterator choicesBegin, choicesEnd);
    template <typename T, typename... ArgsT>
    T* CreateKnob(const std::string& flagName, ArgsT... args)
    {
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists");

        T* knobPtr = new T(flagName, std::forward<ArgsT>(args)...);

        RegisterKnob(flagName, knobPtr);
        return knobPtr;
    }

    // ImGui
    void DrawAllKnobs(bool inExistingWindow = false);

    // Command-line flags
    std::string GetUsageMsg();
    void        UpdateFromFlags(const CliOptions& opts);

    // RegisterKnob will store newly created knob
    void RegisterKnob(const std::string& flagName, Knob* newKnob);
};

} // namespace ppx

#endif // ppx_knob_h
