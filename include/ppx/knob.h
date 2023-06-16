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
        : mFlagName(flagName), mDisplayName(flagName), mIndent(0) {}
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
    std::string mHelpParams;
    size_t      mIndent;
    bool        mUpdatedFlag;
};

// KnobCheckbox will be displayed as a checkbox in the UI
class KnobCheckbox final
    : public Knob
{
public:
    KnobCheckbox(const std::string& flagName, bool defaultValue)
        : Knob(flagName)
    {
        SetHelpParams(" <true|false>");
        SetDefaultAndValue(defaultValue);
    }

    void Draw() override
    {
        if (!ImGui::Checkbox(GetDisplayName().c_str(), &mValue)) {
            return;
        }
        RaiseUpdatedFlag();
    }

    void ResetToDefault() override { SetValue(mDefaultValue); }

    // Expected commandline flag format:
    // --flag_name <true|false>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(GetFlagName(), mValue));
    }

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void SetValue(bool newValue)
    {
        if (newValue == mValue) {
            return;
        }
        mValue = newValue;
        RaiseUpdatedFlag();
    }

private:
    void SetDefaultAndValue(bool newValue)
    {
        mDefaultValue = newValue;
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
        SetHelpParams(" <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">");
        SetDefaultAndValue(defaultValue);
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
        : Knob(flagName), mChoices(choicesBegin, choicesEnd)
    {
        PPX_ASSERT_MSG(defaultIndex < mChoices.size(), "defaultIndex is out of range");

        std::string choiceStr = "";
        for (const auto& choice : mChoices) {
            choiceStr += '\"' + choice + '\"' + "|";
        }
        if (!choiceStr.empty()) {
            choiceStr.pop_back();
        }
        choiceStr = " <" + choiceStr + ">";
        SetHelpParams(choiceStr);
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

// KnobConstant is intended for parameters that cannot be adjusted when the application is run
// They will not be displayed in the UI and cannot have their values changed by the application
template <typename T>
class KnobConstant final
    : public Knob
{
public:
    KnobConstant(const std::string& flagName, T defaultValue)
        : Knob(flagName)
    {
        static_assert(std::is_same_v<T, bool> || std::is_same_v<T, std::string> || std::is_same_v<T, int> || std::is_same_v<T, float>, "KnobConstant must be of type bool, std::string, int, or float");

        SetValue(defaultValue);
    }

    KnobConstant(const std::string& flagName, T defaultValue, T minValue, T maxValue)
        : Knob(flagName)
    {
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize KnobConstant");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");

        SetIsValidFunc([minValue, maxValue](T newValue) {
            if (newValue < minValue || newValue > maxValue) {
                return false;
            }
            return true;
        });

        std::string minValueText = std::to_string(minValue);
        std::string maxValueText = std::to_string(maxValue);
        if (minValue == INT_MIN) {
            minValueText = "INT_MIN";
        }
        if (maxValue == INT_MAX) {
            maxValueText = "INT_MAX";
        }
        SetHelpParams(" <" + minValueText + "~" + maxValueText + ">");
        SetValue(defaultValue);
    }

    void Draw() override {}

    void ResetToDefault() override {}

    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetValue(opts.GetExtraOptionValueOrDefault(GetFlagName(), mValue));
    }

    T GetValue() const { return mValue; }

    void SetIsValidFunc(std::function<bool(T)> isValidFunc)
    {
        mIsValidFunc = isValidFunc;
    }

private:
    bool IsValidValue(T val)
    {
        if (!mIsValidFunc) {
            return true;
        }
        return mIsValidFunc(val);
    }

    void SetValue(T newValue)
    {
        PPX_ASSERT_MSG(IsValidValue(newValue), "invalid default value");
        mValue = newValue;
    }

private:
    T                      mValue;
    std::function<bool(T)> mIsValidFunc;
};

// KnobConstantPair is intended for paired parameters that cannot be adjusted when the application is run
// They will not be displayed in the UI and cannot have their values changed by the application
template <typename T1, typename T2>
class KnobConstantPair final
    : public Knob
{
public:
    KnobConstantPair(const std::string& flagName, std::pair<T1, T2> defaultValue)
        : Knob(flagName)
    {
        SetValue(defaultValue);
    }

    void Draw() override {}

    void ResetToDefault() override {}

    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetValue(opts.GetIntPairValueOrDefault(GetFlagName(), mValue));
    }

    std::pair<T1, T2> GetValue() const { return mValue; }

    void SetIsValidFunc(std::function<bool(std::pair<T1, T2>)> isValidFunc)
    {
        mIsValidFunc = isValidFunc;
    }

private:
    bool IsValidValue(std::pair<T1, T2> value)
    {
        if (!mIsValidFunc) {
            return true;
        }
        return mIsValidFunc(value);
    }

    void SetValue(std::pair<T1, T2> newValue)
    {
        PPX_ASSERT_MSG(IsValidValue(newValue), "invalid default value");
        mValue = newValue;
    }

private:
    std::pair<T1, T2>                      mValue;
    std::function<bool(std::pair<T1, T2>)> mIsValidFunc;
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

    // Knobs which are not dislayed and that cannot have their values changed
    std::vector<std::shared_ptr<Knob>> mKnobConstants;

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
        PPX_ASSERT_MSG(flagName.substr(0, 3) != "no-", "knobs with prefix no- are not allowed");

        std::shared_ptr<T> knobPtr(new T(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr, false);
        return knobPtr;
    }

    // Example:
    //   CreateKnobConstant<bool>("flag_name", bool defaultValue);
    //   CreateKnobConstant<int>("flag_name", int defaultValue, minValue, maxValue);
    //   CreateKnobConstant<std::string>("flag_name", std::string defaultValue);
    template <typename T, typename... ArgsT>
    std::shared_ptr<KnobConstant<T>> CreateKnobConstant(const std::string& flagName, ArgsT... args)
    {
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists");
        PPX_ASSERT_MSG(flagName.substr(0, 3) != "no-", "knobs with prefix no- are not allowed");

        std::shared_ptr<KnobConstant<T>> knobPtr(new KnobConstant<T>(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr, true);
        return knobPtr;
    }

    // Example:
    //   CreateKnobConstantPair<int, int>("flag_name", int defaultValue);
    template <typename T1, typename T2, typename... ArgsT>
    std::shared_ptr<KnobConstantPair<T1, T2>> CreateKnobConstantPair(const std::string& flagName, ArgsT... args)
    {
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists");
        PPX_ASSERT_MSG(flagName.substr(0, 3) != "no-", "knobs with prefix no- are not allowed");

        std::shared_ptr<KnobConstantPair<T1, T2>> knobPtr(new KnobConstantPair<T1, T2>(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr, true);
        return knobPtr;
    }

    void        DrawAllKnobs(bool inExistingWindow = false);
    std::string GetUsageMsg();
    void        UpdateFromFlags(const CliOptions& opts);

private:
    void RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob, bool isConstant);
};

} // namespace ppx

#endif // ppx_knob_h
