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
#include "ppx/string_util.h"

#include <iostream>
#include <memory>
#include <sstream>
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
    Knob(const std::string& flagName, bool visible = false)
        : mFlagName(flagName), mDisplayName(flagName), mIndent(0), mUpdatedFlag(false), mVisible(visible) {}
    virtual ~Knob() = default;

    // Customize flag usage message
    void SetFlagDescription(const std::string& flagDescription) { mFlagDescription = flagDescription; }
    void SetFlagParameters(const std::string& flagParameters) { mFlagParameters = flagParameters; }

    // Customize how knob is drawn in the UI
    void SetDisplayName(const std::string& displayName) { mDisplayName = displayName; }
    void SetIndent(size_t indent) { mIndent = indent; }
    void SetVisible(bool visible) { mVisible = visible; }

    // Returns true if there has been an update to the knob value
    bool DigestUpdate();

    virtual void ResetToDefault() = 0;

protected:
    void RaiseUpdatedFlag() { mUpdatedFlag = true; }

private:
    // Only called from KnobManager
    virtual void        Draw()        = 0;
    virtual std::string ValueString() = 0;

    // Updates knob value from commandline flag
    virtual void UpdateFromFlags(const CliOptions& opts) = 0;

protected:
    std::string mFlagName;
    std::string mDisplayName;

private:
    std::string mFlagParameters;
    std::string mFlagDescription;
    size_t      mIndent; // Indent for when knob is drawn in the UI
    bool        mUpdatedFlag;
    bool        mVisible;

private:
    friend class KnobManager;
};

// KnobCheckbox will be displayed as a checkbox in the UI
class KnobCheckbox final
    : public Knob
{
public:
    KnobCheckbox(const std::string& flagName, bool defaultValue);

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void ResetToDefault() override { SetValue(mDefaultValue); }
    void SetValue(bool newValue);

private:
    void        Draw() override;
    std::string ValueString() override;

    // Expected commandline flag format:
    // --flag_name <true|false>
    void UpdateFromFlags(const CliOptions& opts) override;

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
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>, "KnobSlider must be created with type: int, float");

    KnobSlider(const std::string& flagName, T defaultValue, T minValue, T maxValue)
        : Knob(flagName, true), mValue(defaultValue), mDefaultValue(defaultValue), mMinValue(minValue), mMaxValue(maxValue)
    {
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize slider");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");
        if constexpr (std::is_same_v<T, int>) {
            SetFlagParameters("<" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">");
        }
        else if constexpr (std::is_same_v<T, float>) {
            std::stringstream ss;
            ss.precision(1);
            ss << std::fixed << "<" << mMinValue << "~" << mMaxValue << ">";
            SetFlagParameters(ss.str());
        }

        RaiseUpdatedFlag();
    }

    T GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    void ResetToDefault() override { SetValue(mDefaultValue); }
    void SetValue(T newValue)
    {
        if (!IsValidValue(newValue)) {
            PPX_LOG_ERROR(mFlagName << " cannot be set to " << newValue << " because it's out of range " << mMinValue << "~" << mMaxValue);
            return;
        }
        if (newValue == mValue) {
            return;
        }
        mValue = newValue;
        RaiseUpdatedFlag();
    }

private:
    void Draw() override
    {
        if constexpr (std::is_same_v<T, int>) {
            ImGui::SliderInt(mDisplayName.c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);
        }
        else if constexpr (std::is_same_v<T, float>) {
            ImGui::SliderFloat(mDisplayName.c_str(), &mValue, mMinValue, mMaxValue, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            RaiseUpdatedFlag();
        }
    }

    std::string ValueString() override
    {
        return ppx::string_util::ToString(mValue);
    }

    // Expected commandline flag format:
    // --flag_name <int>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndValue(opts.GetOptionValueOrDefault(mFlagName, mValue));
    }

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

template <typename T, typename NameT = const char*>
struct DropdownEntry
{
    NameT name;
    T     value;
};

// KnobDropdown will be displayed as a dropdown in the UI
// The knob stores the index of a selected choice from a list of allowed options
template <typename T>
class KnobDropdown final
    : public Knob
{
private:
    using Entry = DropdownEntry<T, std::string>;
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
    Entry ConvertToEntry(const DropdownEntry<T, NameT>& entry)
    {
        return {entry.name, entry.value};
    }

public:
    template <typename Iter>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        Iter               choicesBegin,
        Iter               choicesEnd)
        : Knob(flagName, true), mIndex(defaultIndex), mDefaultIndex(defaultIndex)
    {
        for (auto iter = choicesBegin; iter != choicesEnd; ++iter) {
            mChoices.push_back(ConvertToEntry(*iter));
        }
        PPX_ASSERT_MSG(defaultIndex < mChoices.size(), "defaultIndex is out of range");
        std::string choiceStr = "";
        for (const auto& choice : mChoices) {
            bool hasSpace = choice.name.find_first_of("\t ") != std::string::npos;
            if (hasSpace) {
                choiceStr += "\"" + choice.name + "\"|";
            }
            else {
                choiceStr += choice.name + "|";
            }
        }
        if (!choiceStr.empty()) {
            choiceStr.pop_back();
        }
        choiceStr = "<" + choiceStr + ">";
        SetFlagParameters(choiceStr);
        RaiseUpdatedFlag();
    }

    template <typename Container>
    KnobDropdown(
        const std::string& flagName,
        size_t             defaultIndex,
        const Container&   container)
        : KnobDropdown(flagName, defaultIndex, std::begin(container), std::end(container)) {}

    size_t   GetIndex() const { return mIndex; }
    const T& GetValue() const { return mChoices[mIndex].value; }

    // Used for when mIndex needs to be updated outside of UI
    void ResetToDefault() override { SetIndex(mDefaultIndex); }
    void SetIndex(size_t newIndex)
    {
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

private:
    void Draw() override
    {
        if (!ImGui::BeginCombo(mDisplayName.c_str(), mChoices.at(mIndex).name.c_str())) {
            return;
        }
        for (size_t i = 0; i < mChoices.size(); ++i) {
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
    }

    std::string ValueString() override
    {
        return mChoices[mIndex].name;
    }

    // Expected commandline flag format:
    // --flag_name <str>
    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetDefaultAndIndex(opts.GetOptionValueOrDefault(mFlagName, ValueString()));
    }

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
        auto temp = std::find_if(
            mChoices.cbegin(),
            mChoices.cend(),
            [&newValue](const Entry& entry) {
                return entry.name == newValue;
            });
        PPX_ASSERT_MSG(temp != mChoices.cend(), "invalid default value");

        mDefaultIndex = std::distance(mChoices.cbegin(), temp);
        ResetToDefault();
    }

private:
    // mIndex indicates which of the mChoices is selected
    size_t             mIndex;
    size_t             mDefaultIndex;
    std::vector<Entry> mChoices;
};

// KnobFlag is intended for parameters that cannot be adjusted when the application is run
// They will be hidden in the UI by default
// Their values are the default unless otherwise set through commandline flags on application start up
template <typename T>
class KnobFlag final
    : public Knob
{
public:
    KnobFlag(const std::string& flagName, T defaultValue)
        : Knob(flagName, false)
    {
        SetValue(defaultValue);
    }

    KnobFlag(const std::string& flagName, T defaultValue, T minValue, T maxValue)
        : Knob(flagName, false)
    {
        static_assert(std::is_arithmetic_v<T>, "KnobFlag can only be defined with min/max when it's of arithmetic type");
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize KnobFlag");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");

        SetValidator([minValue, maxValue](T newValue) {
            if (newValue < minValue || newValue > maxValue) {
                return false;
            }
            return true;
        });

        SetValue(defaultValue);
    }

    T GetValue() const { return mValue; }

    void SetValidator(std::function<bool(T)> validatorFunc) { mValidatorFunc = validatorFunc; }

private:
    void Draw() override
    {
        std::string flagText = mFlagName + ": " + ValueString();
        ImGui::Text("%s", flagText.c_str());
    }

    std::string ValueString() override
    {
        return ppx::string_util::ToString(mValue);
    }

    void ResetToDefault() override {} // KnobFlag is always the "default" value

    void UpdateFromFlags(const CliOptions& opts) override
    {
        SetValue(opts.GetOptionValueOrDefault(mFlagName, mValue));
    }

    bool IsValidValue(T val)
    {
        if (!mValidatorFunc) {
            return true;
        }
        return mValidatorFunc(val);
    }

    void SetValue(T newValue)
    {
        PPX_ASSERT_MSG(IsValidValue(newValue), "invalid value for knob " + mFlagName);
        mValue = newValue;
    }

private:
    T                      mValue;
    std::function<bool(T)> mValidatorFunc;
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
    void ResetAllToDefault(); // The knobs can be reset to default by a button in the UI

    // Examples of available knobs:
    //   CreateKnob<KnobCheckbox>("flag_name", bool defaultValue);
    //   CreateKnob<KnobSlider<int>>("flag_name", int defaultValue, minValue, maxValue);
    //   CreateKnob<KnobDropdown<std::string>>("flag_name", size_t defaultIndex, std::vector<std::string> choices);
    template <typename T, typename... ArgsT>
    std::shared_ptr<T> CreateKnob(const std::string& flagName, ArgsT... args)
    {
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists: " + flagName);

        std::shared_ptr<T> knobPtr(new T(flagName, std::forward<ArgsT>(args)...));
        RegisterKnob(flagName, knobPtr);
        return knobPtr;
    }

    // Initialize knob in-place, equivalent to
    // *ppKnob = CreateKnob<std::remove_reference_t<decltype(*ppKnob)>>(name, ...)
    template <typename T, typename... ArgsT>
    void InitKnob(std::shared_ptr<T>* ppKnob, const std::string& flagName, ArgsT&&... args)
    {
        PPX_ASSERT_MSG(ppKnob != nullptr, "output parameter is nullptr");
        *ppKnob = CreateKnob<T>(flagName, std::forward<ArgsT>(args)...);
    }

    void        DrawAllKnobs(bool inExistingWindow = false);
    void        PrintFlags() const;
    void        UpdateFromFlags(const CliOptions& opts);

private:
    void RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob);
};

} // namespace ppx

#endif // ppx_knob_h
