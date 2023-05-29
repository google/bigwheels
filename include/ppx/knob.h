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
#include <memory>
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

    void SetDisplayName(const std::string& displayName) { mDisplayName = displayName; }
    void SetFlagDesc(const std::string& flagDesc) { mFlagDesc = flagDesc; }
    void SetIndent(size_t indent) { mIndent = indent; }

    virtual void Draw() = 0;

    // Returns the flag description used in usage message
    virtual std::string GetFlagHelpText() const = 0;

    // Updates knob value from commandline flag
    virtual void UpdateFromFlags(const CliOptions& opts) = 0;

    virtual void ResetToDefault() = 0;

private:
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
        ImGui::Checkbox(GetDisplayName().c_str(), &mValue);
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    std::string GetFlagHelpText() const override
    {
        std::string flagHelpText = "--" + GetFlagName() + " <true|false>";
        if (!GetFlagHelp().empty()) {
            flagHelpText += " : " + GetFlagHelp();
        }
        return flagHelpText + "\n";
    }

    // Expected commandline flag format:
    // --flag_name <true|false>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(GetFlagName(), mValue));
    }

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
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
    static_assert(std::is_same_v<T, int>, "KnobSlider must be created with type: int");

    KnobSlider(const std::string& flagName, T defaultValue, T minValue, T maxValue)
        : Knob(flagName)
    {
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize slider");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");

        mMinValue = minValue;
        mMaxValue = maxValue;
        SetDefaultAndValue(defaultValue);
    }

    void Draw() override
    {
        ImGui::SliderInt(GetDisplayName().c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    std::string GetFlagHelpText() const override
    {
        std::string flagHelpText = "--" + GetFlagName() + " <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">";
        if (!GetFlagHelp().empty()) {
            flagHelpText += " : " + GetFlagHelp();
        }
        return flagHelpText + "\n";
    }

    // Expected commandline flag format:
    // --flag_name <int>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(GetFlagName(), mValue));
    }

    T GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void SetValue(T newVal)
    {
        if (!IsValidValue(newVal)) {
            PPX_LOG_ERROR(GetFlagName() << " cannot be set to " << newVal << " because it's out of range " << mMinValue << "~" << mMaxValue);
            return;
        }

        mValue = newVal;
    }

private:
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
    static_assert(std::is_same_v<T, std::string>, "KnobDropdown must be created with type: std::string");

    template <std::forward_iterator Iter>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        Iter               choicesBegin,
        Iter               choicesEnd)
        : Knob(flagName), mChoices(choicesBegin, choicesEnd)
    {
        PPX_ASSERT_MSG(defaultIndex < mChoices.size(), "defaultIndex is out of range");

        SetDefaultAndIndex(defaultIndex);
    }

    template <typename Container>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        const Container&   container)
        : KnobDropdown(flagName, defaultIndex, std::begin(container), std::end(container)) {}

    void Draw() override
    {
        if (ImGui::BeginCombo(GetDisplayName().c_str(), mChoices.at(mIndex).c_str())) {
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

    std::string GetFlagHelpText() const override
    {
        std::string choiceStr = "";
        for (const auto& choice : mChoices) {
            choiceStr += '\"' + choice + '\"' + "|";
        }
        if (!choiceStr.empty())
            choiceStr.pop_back();

        std::string flagHelpText = "--" + GetFlagName() + " <" + choiceStr + ">";
        if (!GetFlagHelp().empty()) {
            flagHelpText += " : " + GetFlagHelp();
        }
        return flagHelpText + "\n";
    }

    // Expected commandline flag format:
    // --flag_name <str>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndIndex(opts.GetExtraOptionValueOrDefault(GetFlagName(), GetStr()));
    }

    size_t             GetIndex() const { return mIndex; }
    const std::string& GetStr() const { return mChoices[mIndex]; }

    // Used for when mIndex needs to be updated outside of UI
    void SetIndex(size_t newI)
    {
        if (!IsValidIndex(newI)) {
            PPX_LOG_ERROR(GetFlagName() << " does not have this index in allowed choices: " << newI);
            return;
        }

        mIndex = newI;
    }

    // Used for setting from flags
    void SetIndex(const std::string& newVal)
    {
        auto temp = std::find(mChoices.cbegin(), mChoices.cend(), newVal);
        if (temp == mChoices.cend()) {
            PPX_LOG_ERROR(GetFlagName() << " does not have this value in allowed range: " << newVal);
            return;
        }

        mIndex = std::distance(mChoices.cbegin(), temp);
    }

private:
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

// KnobManager holds the knobs in an application
class KnobManager
{
public:
    KnobManager()                   = default;
    KnobManager(const KnobManager&) = delete;
    KnobManager& operator=(const KnobManager&) = delete;

private:
    // Knobs are added on creation and never removed
    std::vector<std::shared_ptr<Knob>> mKnobs;

    // mFlagNames is kept to prevent multiple knobs having the same mFlagName
    std::unordered_set<std::string> mFlagNames;

public:
    bool IsEmpty() { return mKnobs.empty(); }
    void ResetAllToDefault(); // The knobs can be reset to default by a button in the UI

    // Examples of available knobs:
    //   CreateKnob<KnobCheckbox>("flag_name", bool defaultValue);
    //   CreateKnob<KnobSlider<int>>("flag_name", int defaultValue, minValue, maxValue);
    //   CreateKnob<KnobDropdown<std::string>>("flag_name", size_t defaultIndex, std::forward_iterator choicesBegin, choicesEnd);
    template <typename T, typename... ArgsT>
    std::shared_ptr<T> CreateKnob(const std::string& flagName, ArgsT... args)
    {
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists");

        std::shared_ptr<T> knobPtr(new T(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr);
        return knobPtr;
    }

    void        DrawAllKnobs(bool inExistingWindow = false);
    std::string GetUsageMsg();
    void        UpdateFromFlags(const CliOptions& opts);

private:
    void RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob);
};

} // namespace ppx

#endif // ppx_knob_h
