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

// Knobs represent parameters that can be adjusted during the application runtime.
//
// Defining and registering a knob with the application's KnobManager will create a parameter
// whose starting value is determined by (from high priority -> low):
// - A specified command-line flag
// - The default value provided when the knob is created
//
// While the application is running:
// - Users can manually adjust the knob through the UI
// - The application can access the knob's values through the knob getters and setters

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

// Knob is an abstract class which contains common features for all knobs and knob hierarchy
class Knob
{
public:
    Knob(const std::string& flagName)
        : mFlagName(flagName), mDisplayName(flagName), mIndent(0), mUpdatedFlag(false) {}
    virtual ~Knob() = default;

    const std::string& GetDisplayName() const { return mDisplayName; }
    const std::string& GetFlagName() const { return mFlagName; }
    const std::string& GetFlagHelp() const { return mFlagDesc; }
    size_t             GetIndent() const { return mIndent; }
    const std::string& GetHelpParams() const { return mHelpParams; }

    void SetDisplayName(const std::string& displayName) { mDisplayName = displayName; }
    void SetFlagDesc(const std::string& flagDesc) { mFlagDesc = flagDesc; }
    void SetIndent(size_t indent) { mIndent = indent; }
    void SetHelpParams(const std::string& helpParams) { mHelpParams = helpParams; }

    virtual void Draw() = 0;

    // Updates knob value from commandline flag
    virtual void UpdateFromFlags(const CliOptions& opts) = 0;

    virtual void ResetToDefault() = 0;

    // Returns true if there has been an update to the knob value
    bool DigestUpdate();

protected:
    void RaiseUpdatedFlag() { mUpdatedFlag = true; }

private:
    std::string mDisplayName;
    std::string mFlagName;
    std::string mFlagDesc;
    size_t      mIndent;
    std::string mHelpParams;
    bool        mUpdatedFlag;
};

// KnobCheckbox will be displayed as a checkbox in the UI
class KnobCheckbox final
    : public Knob
{
public:
    KnobCheckbox(const std::string& flagName, bool defaultValue);

    void Draw() override;

    void ResetToDefault() override { SetValue(mDefaultValue); }

    // Expected commandline flag format:
    // --flag_name <true|false>
    void UpdateFromFlags(const CliOptions& opts) override;

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void SetValue(bool newValue);

private:
    void SetDefaultAndValue(bool newValue);

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
        : Knob(flagName), mValue(defaultValue), mDefaultValue(defaultValue), mMinValue(minValue), mMaxValue(maxValue)
    {
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize slider");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");
        SetHelpParams(" <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">");
        RaiseUpdatedFlag();
    }

    void Draw() override
    {
        ImGui::SliderInt(GetDisplayName().c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            RaiseUpdatedFlag();
        }
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    // Expected commandline flag format:
    // --flag_name <int>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(GetFlagName(), mValue));
    }

    T GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void SetValue(T newValue)
    {
        if (!IsValidValue(newValue)) {
            PPX_LOG_ERROR(GetFlagName() << " cannot be set to " << newValue << " because it's out of range " << mMinValue << "~" << mMaxValue);
            return;
        }
        if (newValue == mValue) {
            return;
        }
        mValue = newValue;
        RaiseUpdatedFlag();
    }

private:
    bool IsValidValue(T val)
    {
        return mMinValue <= val && val <= mMaxValue;
    }

    void SetDefaultAndValue(T newValue)
    {
        PPX_ASSERT_MSG(IsValidValue(newValue), "invalid default value");
        mDefaultValue = newValue;
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

    template <typename Iter>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        Iter               choicesBegin,
        Iter               choicesEnd)
        : Knob(flagName), mIndex(defaultIndex), mDefaultIndex(defaultIndex), mChoices(choicesBegin, choicesEnd)
    {
        PPX_ASSERT_MSG(defaultIndex < mChoices.size(), "defaultIndex is out of range");
        std::string choiceStr = "";
        for (const auto& choice : mChoices) {
            bool hasSpace = choice.find_first_of("\t ") != std::string::npos;
            if (hasSpace) {
                choiceStr += R"(")" + choice + R"("|)";
            }
            else {
                choiceStr += choice + "|";
            }
        }
        if (!choiceStr.empty()) {
            choiceStr.pop_back();
        }
        choiceStr = " <" + choiceStr + ">";
        SetHelpParams(choiceStr);
        RaiseUpdatedFlag();
    }

    template <typename Container>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        const Container&   container)
        : KnobDropdown(flagName, defaultIndex, std::begin(container), std::end(container)) {}

    void Draw() override
    {
        if (!ImGui::BeginCombo(GetDisplayName().c_str(), mChoices.at(mIndex).c_str())) {
            return;
        }
        for (size_t i = 0; i < mChoices.size(); ++i) {
            bool isSelected = (i == mIndex);
            if (ImGui::Selectable(mChoices.at(i).c_str(), isSelected)) {
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
    }

    void ResetToDefault() override { SetIndex(mDefaultIndex); }

    // Expected commandline flag format:
    // --flag_name <str>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndIndex(opts.GetExtraOptionValueOrDefault(GetFlagName(), GetValue()));
    }

    size_t   GetIndex() const { return mIndex; }
    const T& GetValue() const { return mChoices[mIndex]; }

    // Used for when mIndex needs to be updated outside of UI
    void SetIndex(size_t newIndex)
    {
        if (!IsValidIndex(newIndex)) {
            PPX_LOG_ERROR(GetFlagName() << " does not have this index in allowed choices: " << newIndex);
            return;
        }
        if (newIndex == mIndex) {
            return;
        }
        mIndex = newIndex;
        RaiseUpdatedFlag();
    }

    // Used for setting from flags
    void SetIndex(const std::string& newValue)
    {
        auto temp = std::find(mChoices.cbegin(), mChoices.cend(), newValue);
        if (temp == mChoices.cend()) {
            PPX_LOG_ERROR(GetFlagName() << " does not have this value in allowed range: " << newValue);
            return;
        }

        SetIndex(std::distance(mChoices.cbegin(), temp));
    }

private:
    bool IsValidIndex(size_t index)
    {
        return index < mChoices.size();
    }

    void SetDefaultAndIndex(size_t newIndex)
    {
        PPX_ASSERT_MSG(IsValidIndex(newIndex), "invalid default index");
        mDefaultIndex = newIndex;
        ResetToDefault();
    }

    void SetDefaultAndIndex(std::string newValue)
    {
        auto temp = std::find(mChoices.cbegin(), mChoices.cend(), newValue);
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
    //   CreateKnob<KnobDropdown<std::string>>("flag_name", size_t defaultIndex, std::vector<std::string> choices);
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
